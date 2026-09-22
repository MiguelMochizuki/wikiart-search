/**
 * server.c
 * Descrição: Servidor HTTP com POSIX sockets e roteamento da API.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server.h"
#include "buscador.h"
#include "json.h"

#define BUFFER_REQ 8192

typedef struct {
	const Csv* csv;
	void* sl_inst;
	void* to_inst;
} ServerContext;

/* Decodifica escape de URL (ex: 'van+gogh' ou '%20' -> 'van gogh') */
static void url_decode(char* dst, const char* src, size_t max_dst) {
	size_t di = 0;
	while (*src && di + 1 < max_dst) {
		if (*src == '+') {
			dst[di++] = ' ';
			src++;
		} else if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
			char hex[3] = { src[1], src[2], '\0' };
			dst[di++] = (char)strtol(hex, NULL, 16);
			src += 3;
		} else {
			dst[di++] = *src++;
		}
	}
	dst[di] = '\0';
}

/* Extrai o valor de um parâmetro da query string */
static int extrair_param(const char* query, const char* chave, char* destino, size_t max_dest) {
	if (!query || !chave) return 0;
	size_t chave_len = strlen(chave);
	const char* p = query;

	while (*p) {
		if ((p == query || *(p - 1) == '&') && strncmp(p, chave, chave_len) == 0 && p[chave_len] == '=') {
			const char* val_start = p + chave_len + 1;
			const char* val_end = strchr(val_start, '&');
			size_t len = val_end ? (size_t)(val_end - val_start) : strlen(val_start);

			char raw[512];
			if (len >= sizeof raw) len = sizeof(raw) - 1;
			memcpy(raw, val_start, len);
			raw[len] = '\0';

			url_decode(destino, raw, max_dest);
			return 1;
		}
		p++;
	}
	return 0;
}

static void enviar_resposta(int sock_cliente, int status, const char* status_msg,
                            const char* content_type, const char* corpo, size_t corpo_len) {
	char header[512];
	int hlen = snprintf(header, sizeof header,
	                    "HTTP/1.1 %d %s\r\n"
	                    "Content-Type: %s; charset=utf-8\r\n"
	                    "Content-Length: %zu\r\n"
	                    "Access-Control-Allow-Origin: *\r\n"
	                    "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
	                    "Access-Control-Allow-Headers: Content-Type\r\n"
	                    "Connection: close\r\n\r\n",
	                    status, status_msg, content_type, corpo_len);

	write(sock_cliente, header, (size_t)hlen);
	if (corpo && corpo_len > 0) {
		write(sock_cliente, corpo, corpo_len);
	}
}

static void handle_status(int sock, const ServerContext* ctx) {
	char buf[256];
	snprintf(buf, sizeof buf,
	         "{\"status\":\"online\",\"total_obras\":%d,"
	         "\"estruturas\":[\"skip_list\",\"tabela_ord\"]}",
	         csv_tamanho(ctx->csv));
	enviar_resposta(sock, 200, "OK", "application/json", buf, strlen(buf));
}

/** Envia um erro em JSON
 *
 * A mensagem é sempre um literal do próprio servidor, nunca entrada do
 * usuário, então não precisa de escape.
 *
 * Parâmetros:
 * int sock: socket do cliente
 * int status: código HTTP
 * const char* status_msg: frase do status
 * const char* mensagem: descrição do erro
 */
static void enviar_erro(int sock, int status, const char* status_msg,
			const char* mensagem) {
	char corpo[512];
	int n = snprintf(corpo, sizeof corpo, "{\"erro\":\"%s\"}", mensagem);
	if (n < 0) return;
	if ((size_t) n >= sizeof corpo) n = (int) sizeof corpo - 1;

	enviar_resposta(sock, status, status_msg, "application/json",
			corpo, (size_t) n);
}

/** Resolve o nome de uma ED para a vtable e a instância correspondente
 *
 * Parâmetros:
 * const char* nome: nome da ED vindo do parâmetro 'ed'
 * const ServerContext* ctx: contexto com as instâncias populadas
 * void** inst_out: saída para a instância escolhida
 *
 * Retorna const Buscador*: vtable da ED, ou NULL se o nome for inválido
 */
static const Buscador* buscador_por_nome(const char* nome,
					 const ServerContext* ctx,
					 void** inst_out) {
	if (strcmp(nome, "skip_list") == 0) {
		*inst_out = ctx->sl_inst;
		return &BUSCADOR_SKIP_LIST;
	}
	if (strcmp(nome, "tabela_ord") == 0) {
		*inst_out = ctx->to_inst;
		return &BUSCADOR_TABELA_ORD;
	}
	return NULL;
}

/** Descreve como a ED resolveu a consulta, para a interface exibir
 *
 * As duas EDs indexam pela chave composta (gênero, artista), então
 * gênero e o par saem por busca indexada e artista sozinho cai em
 * varredura nas duas. O que muda é como cada uma alcança o bloco:
 * busca binária ou descida de níveis.
 *
 * Parâmetros:
 * const char* ed: nome da ED
 * const char* consulta: "genero", "artista" ou "genero+artista"
 *
 * Retorna const char*: rótulo do algoritmo usado
 */
static const char* algoritmo_de(const char* ed, const char* consulta) {
	int so_artista = (strcmp(consulta, "artista") == 0);

	/* Cada rótulo nomeia o algoritmo, não a chave: as duas indexam
	 * pela mesma chave composta, o que muda é como alcançam o bloco.
	 * Só a TabelaOrd faz busca binária de fato, porque só ela tem
	 * acesso aleatório. A SkipList anda por níveis, O(log n)
	 * esperado e não garantido. */
	if (strcmp(ed, "tabela_ord") == 0) {
		return so_artista ? "varredura linear" : "busca binaria";
	}
	if (so_artista) return "varredura linear";
	return "descida por niveis";
}

/** Lê um parâmetro e informa se veio preenchido
 *
 * Parâmetros:
 * const char* query: query string da requisição
 * const char* chave: nome do parâmetro
 * char* destino: buffer de saída
 * size_t max: tamanho do buffer
 *
 * Retorna int: 1 se o parâmetro veio e não está vazio, 0 caso contrário
 */
static int param_preenchido(const char* query, const char* chave,
			    char* destino, size_t max) {
	return extrair_param(query, chave, destino, max) && destino[0] != '\0';
}

static void handle_busca(int sock, const ServerContext* ctx, const char* query) {
	char genero[256]  = {0};
	char artista[256] = {0};
	char ed[32]       = "tabela_ord";

	int tem_genero  = param_preenchido(query, "genero",  genero,  sizeof genero);
	int tem_artista = param_preenchido(query, "artista", artista, sizeof artista);

	extrair_param(query, "ed", ed, sizeof ed);

	void* inst = NULL;
	const Buscador* b = buscador_por_nome(ed, ctx, &inst);
	if (!b) {
		enviar_erro(sock, 400, "Bad Request",
			    "Parametro 'ed' invalido. Use skip_list "
			    "ou tabela_ord.");
		return;
	}

	/* Despacho: a combinação dos parâmetros escolhe a busca. */
	Resultado* r = NULL;
	const char* consulta = NULL;

	if (tem_genero && tem_artista) {
		consulta = "genero+artista";
		r = b->buscar_genero_artista(inst, genero, artista);
	} else if (tem_genero) {
		consulta = "genero";
		r = b->buscar_genero(inst, genero);
	} else if (tem_artista) {
		consulta = "artista";
		r = b->buscar_artista(inst, artista);
	} else {
		enviar_erro(sock, 400, "Bad Request",
			    "Informe 'genero', 'artista' ou os dois.");
		return;
	}

	if (!r) {
		enviar_erro(sock, 500, "Internal Server Error",
			    "Falha ao alocar o resultado.");
		return;
	}

	JsonConsulta jc = {
		.genero    = tem_genero  ? genero  : NULL,
		.artista   = tem_artista ? artista : NULL,
		.estrutura = b->nome,
		.consulta  = consulta,
		.algoritmo = algoritmo_de(b->nome, consulta)
	};

	JsonBuffer* jb = json_buffer_criar(4096);
	if (!jb) {
		resultado_liberar(r);
		enviar_erro(sock, 500, "Internal Server Error",
			    "Falha ao alocar o buffer de resposta.");
		return;
	}

	json_serializar_resultado(jb, r, &jc);
	enviar_resposta(sock, 200, "OK", "application/json",
			json_buffer_obter_texto(jb), json_buffer_tamanho(jb));

	json_buffer_liberar(jb);
	resultado_liberar(r);
}

static void handle_comparar(int sock, const ServerContext* ctx, const char* query) {
	char genero[256]  = {0};
	char artista[256] = {0};

	int tem_genero  = param_preenchido(query, "genero",  genero,  sizeof genero);
	int tem_artista = param_preenchido(query, "artista", artista, sizeof artista);

	if (!tem_genero && !tem_artista) {
		enviar_erro(sock, 400, "Bad Request",
			    "Informe 'genero', 'artista' ou os dois.");
		return;
	}

	const char* consulta = (tem_genero && tem_artista) ? "genero+artista"
			     : (tem_genero ? "genero" : "artista");

	const Buscador* eds[] = {
		&BUSCADOR_SKIP_LIST,
		&BUSCADOR_TABELA_ORD
	};
	void* insts[] = { ctx->sl_inst, ctx->to_inst };
	const int n_eds = (int) (sizeof eds / sizeof *eds);

	/* Roda a mesma consulta nas duas EDs e coleta as métricas. */
	JsonComparacao linhas[2];
	int n = 0;

	for (int i = 0; i < n_eds; i++) {
		Resultado* r;
		if (tem_genero && tem_artista) {
			r = eds[i]->buscar_genero_artista(insts[i], genero, artista);
		} else if (tem_genero) {
			r = eds[i]->buscar_genero(insts[i], genero);
		} else {
			r = eds[i]->buscar_artista(insts[i], artista);
		}
		if (!r) continue;

		linhas[n].estrutura = eds[i]->nome;
		linhas[n].algoritmo = algoritmo_de(eds[i]->nome, consulta);
		linhas[n].resultado = r;
		n++;
	}

	JsonBuffer* jb = json_buffer_criar(1024);
	if (jb) {
		json_serializar_comparativo(jb,
					    tem_genero  ? genero  : NULL,
					    tem_artista ? artista : NULL,
					    consulta, linhas, n);
		enviar_resposta(sock, 200, "OK", "application/json",
				json_buffer_obter_texto(jb),
				json_buffer_tamanho(jb));
		json_buffer_liberar(jb);
	} else {
		enviar_erro(sock, 500, "Internal Server Error",
			    "Falha ao alocar o buffer de resposta.");
	}

	for (int i = 0; i < n; i++) {
		resultado_liberar((Resultado*) linhas[i].resultado);
	}
}

static void processar_requisicao(int sock, const ServerContext* ctx) {
	char req_buf[BUFFER_REQ];
	ssize_t lidos = read(sock, req_buf, sizeof(req_buf) - 1);
	if (lidos <= 0) return;
	req_buf[lidos] = '\0';

	char metodo[16], uri[1024];
	if (sscanf(req_buf, "%15s %1023s", metodo, uri) < 2) return;

	if (strcmp(metodo, "OPTIONS") == 0) {
		enviar_resposta(sock, 204, "No Content", "text/plain", "", 0);
		return;
	}

	char* query = strchr(uri, '?');
	if (query) {
		*query = '\0';
		query++;
	} else {
		query = "";
	}

	if (strcmp(uri, "/api/status") == 0) {
		handle_status(sock, ctx);
	} else if (strcmp(uri, "/api/busca") == 0) {
		handle_busca(sock, ctx, query);
	} else if (strcmp(uri, "/api/comparar") == 0) {
		handle_comparar(sock, ctx, query);
	} else {
		const char* not_found = "{\"error\":\"Rota nao encontrada\"}";
		enviar_resposta(sock, 404, "Not Found", "application/json", not_found, strlen(not_found));
	}
}

int server_iniciar(const Csv* csv, int porta) {
	ServerContext ctx;
	ctx.csv = csv;

	printf("Populando estruturas na memoria para o servidor...\n");
	ctx.sl_inst = BUSCADOR_SKIP_LIST.criar();
	ctx.to_inst = BUSCADOR_TABELA_ORD.criar();

	int total = csv_tamanho(csv);
	for (int i = 0; i < total; i++) {
		const Obra* o = csv_obra(csv, i);
		BUSCADOR_SKIP_LIST.inserir(ctx.sl_inst, o);
		BUSCADOR_TABELA_ORD.inserir(ctx.to_inst, o);
	}
	printf("Estruturas carregadas com sucesso (%d obras).\n", total);

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
		return 1;
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons((uint16_t)porta);

	if (bind(server_fd, (struct sockaddr*)&addr, sizeof addr) < 0) {
		perror("bind");
		close(server_fd);
		return 1;
	}

	if (listen(server_fd, 16) < 0) {
		perror("listen");
		close(server_fd);
		return 1;
	}

	printf("\n======================================================\n");
	printf("Servidor WikiArt ativo em: http://localhost:%d\n", porta);
	printf("Endpoints disponiveis:\n");
	printf("  - GET /api/status\n");
	printf("  - GET /api/busca?genero=<g>&artista=<a>&ed=<skip_list|tabela_ord>\n");
	printf("      informe genero, artista ou os dois. \"ed\" e opcional (padrao: tabela_ord)\n");
	printf("  - GET /api/comparar?genero=<g>&artista=<a>\n");
	printf("======================================================\n\n");

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof client_addr;
		int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
		if (client_fd < 0) continue;

		processar_requisicao(client_fd, &ctx);
		close(client_fd);
	}

	close(server_fd);
	BUSCADOR_SKIP_LIST.liberar(ctx.sl_inst);
	BUSCADOR_TABELA_ORD.liberar(ctx.to_inst);
	return 0;
}