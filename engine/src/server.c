/**
 * server.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Servidor HTTP com POSIX sockets e roteamento da API.
 *
 * Cada ED tem o seu Indice, com as três estruturas da navegação. Uma
 * rota por nível: /api/generos, /api/artistas e /api/busca (obras). Na
 * tabela ordenada a resposta é a lista do nível; na árvore afunilada é a
 * vista dos primeiros níveis da árvore, porque ali a forma é o que
 * interessa mostrar.
 *
 * O parâmetro opcional 'foco' pede um acesso a um item específico do
 * nível: na árvore ele é afunilado até a raiz. As árvores guardam o
 * estado entre requisições, e esse estado é compartilhado por todos os
 * clientes: é o servidor, e não o navegador, que lembra o último acesso.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server.h"
#include "buscador.h"
#include "catalogo.h"
#include "indice.h"
#include "json.h"

#define BUFFER_REQ 8192

/* Níveis da árvore que a vista manda para a interface: 1 + 2 + 4 + 8 nós. */
#define NIVEIS_VISTA 4

/* EDs servidas. A primeira é a padrão quando 'ed' não vem na URL. */
static const Buscador* const EDS[] = {
	&BUSCADOR_TABELA_ORD,
	&BUSCADOR_ARVORE_AFUNILADA
};
#define N_EDS ((int) (sizeof EDS / sizeof *EDS))

typedef struct {
	const Csv* csv;
	Catalogo* catalogo;
	Indice* indices[N_EDS];
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

/** Envia uma resposta HTTP completa e fecha o envio
 *
 * Parâmetros:
 * int sock_cliente: socket do cliente
 * int status: código HTTP
 * const char* status_msg: frase do status
 * const char* content_type: tipo do corpo (charset utf-8 é acrescentado)
 * const char* corpo: corpo da resposta, ou NULL
 * size_t corpo_len: tamanho do corpo em bytes
 */
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

/** GET /api/status: estado da engine e tamanho do acervo
 *
 * Parâmetros:
 * int sock: socket do cliente
 * const ServerContext* ctx: contexto com o CSV e o catálogo
 */
static void handle_status(int sock, const ServerContext* ctx) {
	char buf[256];
	snprintf(buf, sizeof buf,
	         "{\"status\":\"online\",\"total_obras\":%d,"
	         "\"generos\":%d,\"artistas\":%d,"
	         "\"estruturas\":[\"tabela_ord\",\"arvore_afunilada\"]}",
	         csv_tamanho(ctx->csv),
	         catalogo_n_generos(ctx->catalogo),
	         catalogo_n_artistas(ctx->catalogo));
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

/** Escolhe o índice da ED pedida no parâmetro 'ed'
 *
 * Sem o parâmetro, vale a primeira ED de EDS. Nome desconhecido já
 * responde 400 ao cliente.
 *
 * Parâmetros:
 * int sock: socket do cliente, para o erro
 * const ServerContext* ctx: contexto com os índices montados
 * const char* query: query string da requisição
 *
 * Retorna Indice*: índice escolhido, ou NULL se o nome for inválido
 */
static Indice* indice_da_query(int sock, const ServerContext* ctx,
			       const char* query) {
	char ed[32];
	if (!param_preenchido(query, "ed", ed, sizeof ed)) {
		return ctx->indices[0];
	}
	for (int i = 0; i < N_EDS; i++) {
		if (strcmp(ed, EDS[i]->nome) == 0) return ctx->indices[i];
	}
	enviar_erro(sock, 400, "Bad Request",
		    "Parametro 'ed' invalido. Use tabela_ord ou "
		    "arvore_afunilada.");
	return NULL;
}

/** Busca um nível e responde em lista ou, se a ED tiver forma, em vista
 *
 * A busca feita é a do foco, quando há um, ou a do intervalo do nível.
 * A vista é sempre do intervalo: depois de afunilar um artista, a tela
 * continua mostrando só os artistas daquele gênero, agora com ele na
 * raiz.
 *
 * Parâmetros:
 * int sock: socket do cliente
 * Indice* ix: índice da ED escolhida
 * Nivel nivel: nível consultado
 * const Chave* intervalo: intervalo do nível, ou NULL para o nível todo
 * const Chave* foco: item acessado, ou NULL
 * const char* genero: gênero a ecoar na resposta, ou NULL
 * const char* artista: artista a ecoar na resposta, ou NULL
 * int offset: itens do intervalo a pular (só com limite)
 * int limite: tamanho da página, ou negativo para devolver o intervalo todo
 */
static void responder_nivel(int sock, Indice* ix, Nivel nivel,
			    const Chave* intervalo, const Chave* foco,
			    const char* genero, const char* artista,
			    int offset, int limite) {
	const Buscador* ed = indice_ed(ix);
	const Chave* chave = foco ? foco : intervalo;

	Resultado* r = limite >= 0
		? indice_buscar_pagina(ix, nivel, chave, offset, limite)
		: indice_buscar(ix, nivel, chave);
	if (!r) {
		enviar_erro(sock, 500, "Internal Server Error",
			    "Falha ao alocar o resultado.");
		return;
	}

	JsonBuffer* jb = json_buffer_criar(4096);
	if (!jb) {
		resultado_liberar(r);
		enviar_erro(sock, 500, "Internal Server Error",
			    "Falha ao alocar o buffer de resposta.");
		return;
	}

	JsonConsulta jc = {
		.nivel     = nivel,
		.genero    = genero,
		.artista   = artista,
		.estrutura = ed->nome,
		.algoritmo = chave ? ed->algoritmo_busca : ed->algoritmo_percurso
	};

	NoVista vista[(1 << NIVEIS_VISTA) - 1];
	int total = indice_vista(ix, nivel, intervalo, vista, NIVEIS_VISTA);

	if (total >= 0) {
		json_serializar_vista(jb, r, &jc, vista, NIVEIS_VISTA, total);
	} else {
		json_serializar_resultado(jb, r, &jc);
	}
	enviar_resposta(sock, 200, "OK", "application/json",
			json_buffer_obter_texto(jb), json_buffer_tamanho(jb));

	json_buffer_liberar(jb);
	resultado_liberar(r);
}

/** Nível 1: GET /api/generos?ed=&foco=<genero> */
static void handle_generos(int sock, const ServerContext* ctx, const char* query) {
	Indice* ix = indice_da_query(sock, ctx, query);
	if (!ix) return;

	char foco[256] = {0};
	int tem_foco = param_preenchido(query, "foco", foco, sizeof foco);
	Chave k_foco = { foco, NULL, -1 };

	responder_nivel(sock, ix, NIVEL_GENEROS, NULL,
			tem_foco ? &k_foco : NULL, NULL, NULL, 0, -1);
}

/** Nível 2: GET /api/artistas?genero=<g>&ed=&foco=<artista> */
static void handle_artistas(int sock, const ServerContext* ctx, const char* query) {
	char genero[256] = {0};
	char foco[256]   = {0};

	if (!param_preenchido(query, "genero", genero, sizeof genero)) {
		enviar_erro(sock, 400, "Bad Request", "Informe 'genero'.");
		return;
	}
	int tem_foco = param_preenchido(query, "foco", foco, sizeof foco);

	Indice* ix = indice_da_query(sock, ctx, query);
	if (!ix) return;

	Chave intervalo = { genero, NULL, -1 };
	Chave k_foco    = { genero, foco, -1 };

	responder_nivel(sock, ix, NIVEL_ARTISTAS, &intervalo,
			tem_foco ? &k_foco : NULL, genero, NULL, 0, -1);
}

/** Converte o texto do foco das obras num id
 *
 * Parâmetros:
 * const char* s: texto vindo da URL
 * int* id_out: saída com o id
 *
 * Retorna int: 1 se for um inteiro não negativo válido, 0 caso contrário
 */
static int ler_id(const char* s, int* id_out) {
	char* fim = NULL;
	errno = 0;
	long v = strtol(s, &fim, 10);
	if (errno != 0 || fim == s || *fim != '\0' || v < 0 || v > INT_MAX) {
		return 0;
	}
	*id_out = (int) v;
	return 1;
}

/** Lê um inteiro não negativo opcional da query
 *
 * Parâmetros:
 * const char* query: query string da requisição
 * const char* nome: nome do parâmetro
 * int padrao: valor quando o parâmetro não veio
 * int* saida: recebe o valor
 *
 * Retorna int: 1 se veio ausente ou válido, 0 se veio malformado
 */
static int int_opcional(const char* query, const char* nome, int padrao,
			int* saida) {
	char txt[16];
	if (!param_preenchido(query, nome, txt, sizeof txt)) {
		*saida = padrao;
		return 1;
	}
	return ler_id(txt, saida);
}

/** Nível 3: GET /api/busca?genero=<g>&artista=<a>&ed=&foco=<id>&offset=&limite= */
static void handle_busca(int sock, const ServerContext* ctx, const char* query) {
	char genero[256]  = {0};
	char artista[256] = {0};
	char foco[32]     = {0};

	int tem_genero  = param_preenchido(query, "genero",  genero,  sizeof genero);
	int tem_artista = param_preenchido(query, "artista", artista, sizeof artista);
	int tem_foco    = param_preenchido(query, "foco",    foco,    sizeof foco);

	/* Gênero é a chave primária dos três níveis: sem ele não há bloco
	 * por onde entrar na estrutura. */
	if (!tem_genero) {
		enviar_erro(sock, 400, "Bad Request",
			    "Informe 'genero' (e, opcionalmente, 'artista').");
		return;
	}

	int id = -1;
	if (tem_foco && (!tem_artista || !ler_id(foco, &id))) {
		enviar_erro(sock, 400, "Bad Request",
			    "'foco' e o id de uma obra e exige 'artista'.");
		return;
	}

	int offset, limite;
	if (!int_opcional(query, "offset", 0, &offset) ||
	    !int_opcional(query, "limite", -1, &limite)) {
		enviar_erro(sock, 400, "Bad Request",
			    "'offset' e 'limite' sao inteiros nao negativos.");
		return;
	}

	Indice* ix = indice_da_query(sock, ctx, query);
	if (!ix) return;

	const char* a = tem_artista ? artista : NULL;
	Chave intervalo = { genero, a, -1 };
	Chave k_foco    = { genero, a, id };

	responder_nivel(sock, ix, NIVEL_OBRAS, &intervalo,
			tem_foco ? &k_foco : NULL, genero, a, offset, limite);
}

/** GET /api/comparar?genero=<g>&artista=<a>: a mesma busca de obras em todas as EDs */
static void handle_comparar(int sock, const ServerContext* ctx, const char* query) {
	char genero[256]  = {0};
	char artista[256] = {0};

	int tem_genero  = param_preenchido(query, "genero",  genero,  sizeof genero);
	int tem_artista = param_preenchido(query, "artista", artista, sizeof artista);

	if (!tem_genero) {
		enviar_erro(sock, 400, "Bad Request",
			    "Informe 'genero' (e, opcionalmente, 'artista').");
		return;
	}

	const char* consulta = tem_artista ? "genero+artista" : "genero";
	Chave k = { genero, tem_artista ? artista : NULL, -1 };

	/* Roda a mesma consulta em todas as EDs e coleta as métricas. */
	JsonComparacao linhas[N_EDS];
	int n = 0;

	for (int i = 0; i < N_EDS; i++) {
		Resultado* r = indice_buscar(ctx->indices[i], NIVEL_OBRAS, &k);
		if (!r) continue;

		linhas[n].estrutura = EDS[i]->nome;
		linhas[n].algoritmo = EDS[i]->algoritmo_busca;
		linhas[n].resultado = r;
		n++;
	}

	JsonBuffer* jb = json_buffer_criar(1024);
	if (jb) {
		json_serializar_comparativo(jb, genero,
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

/** Registra no log a URL pedida, como veio, com a query string
 *
 * Os parâmetros ficam codificados como na URL (%20, +), então nada do que
 * o cliente manda vira quebra de linha. Só um byte de controle cru (um
 * ESC, por exemplo) mexeria no terminal de quem lê o log, e esse sai
 * escapado como \xNN. O flush põe a linha no log na hora, mesmo com a
 * saída redirecionada para arquivo.
 *
 * Parâmetros:
 * const char* uri: URI da requisição
 */
static void registrar_url(const char* uri) {
	for (const unsigned char* p = (const unsigned char*) uri; *p; p++) {
		if (*p < 0x20 || *p == 0x7f) printf("\\x%02x", *p);
		else                         putchar(*p);
	}
	putchar('\n');
	fflush(stdout);
}

/** Lê uma requisição, registra a URL e a encaminha à rota certa
 *
 * Parâmetros:
 * int sock: socket do cliente
 * const ServerContext* ctx: contexto com os índices montados
 */
static void processar_requisicao(int sock, const ServerContext* ctx) {
	char req_buf[BUFFER_REQ];
	ssize_t lidos = read(sock, req_buf, sizeof(req_buf) - 1);
	if (lidos <= 0) return;
	req_buf[lidos] = '\0';

	char metodo[16], uri[1024];
	if (sscanf(req_buf, "%15s %1023s", metodo, uri) < 2) return;

	registrar_url(uri);

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
	} else if (strcmp(uri, "/api/generos") == 0) {
		handle_generos(sock, ctx, query);
	} else if (strcmp(uri, "/api/artistas") == 0) {
		handle_artistas(sock, ctx, query);
	} else if (strcmp(uri, "/api/busca") == 0) {
		handle_busca(sock, ctx, query);
	} else if (strcmp(uri, "/api/comparar") == 0) {
		handle_comparar(sock, ctx, query);
	} else {
		const char* not_found = "{\"error\":\"Rota nao encontrada\"}";
		enviar_resposta(sock, 404, "Not Found", "application/json", not_found, strlen(not_found));
	}
}

/** Libera o catálogo e os índices já montados do contexto */
static void liberar_contexto(ServerContext* ctx) {
	for (int i = 0; i < N_EDS; i++) indice_liberar(ctx->indices[i]);
	catalogo_liberar(ctx->catalogo);
}

/** Monta as EDs, abre o socket e atende requisições até ser interrompido
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * int porta: porta TCP onde escutar
 *
 * Retorna int: 1 se a montagem ou o socket falhar
 */
int server_iniciar(const Csv* csv, int porta) {
	ServerContext ctx = { .csv = csv };

	printf("Populando estruturas na memoria para o servidor...\n");
	Catalogo* catalogo = catalogo_montar(csv);
	if (!catalogo) {
		fprintf(stderr, "Falha ao montar o catalogo.\n");
		return 1;
	}
	ctx.catalogo = catalogo;

	for (int i = 0; i < N_EDS; i++) {
		ctx.indices[i] = indice_montar(EDS[i], catalogo);
		if (!ctx.indices[i]) {
			fprintf(stderr, "Falha ao montar %s.\n", EDS[i]->nome);
			liberar_contexto(&ctx);
			return 1;
		}
	}
	printf("Estruturas carregadas com sucesso (%d generos, %d artistas, "
	       "%d obras).\n",
	       catalogo_n_generos(catalogo), catalogo_n_artistas(catalogo),
	       csv_tamanho(csv));

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
		liberar_contexto(&ctx);
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
		liberar_contexto(&ctx);
		return 1;
	}

	if (listen(server_fd, 16) < 0) {
		perror("listen");
		close(server_fd);
		liberar_contexto(&ctx);
		return 1;
	}

	printf("\n======================================================\n");
	printf("Servidor WikiArt ativo em: http://localhost:%d\n", porta);
	printf("Endpoints disponiveis (\"ed\": tabela_ord | arvore_afunilada, padrao tabela_ord):\n");
	printf("  - GET /api/status\n");
	printf("  - GET /api/generos?ed=<ed>&foco=<genero>\n");
	printf("  - GET /api/artistas?genero=<g>&ed=<ed>&foco=<artista>\n");
	printf("  - GET /api/busca?genero=<g>&artista=<a>&ed=<ed>&foco=<id>\n");
	printf("      \"offset\" e \"limite\" (opcionais) paginam as obras; total_encontrados e o intervalo todo\n");
	printf("      \"foco\" e opcional: acessa um item e, na arvore, o leva a raiz\n");
	printf("  - GET /api/comparar?genero=<g>&artista=<a>\n");
	printf("Log de acesso: a URL de cada requisicao, uma por linha\n");
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
	liberar_contexto(&ctx);
	return 0;
}
