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
	void* ht_inst;
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
	         "\"estruturas\":[\"hash_table\",\"skip_list\",\"tabela_ord\"]}",
	         csv_tamanho(ctx->csv));
	enviar_resposta(sock, 200, "OK", "application/json", buf, strlen(buf));
}

static void handle_busca(int sock, const ServerContext* ctx, const char* query) {
	char termo[256] = {0};
	char tipo[32] = "artista";
	char ed[32] = "hash_table";

	extrair_param(query, "q", termo, sizeof termo);
	extrair_param(query, "tipo", tipo, sizeof tipo);
	extrair_param(query, "ed", ed, sizeof ed);

	const Buscador* b = &BUSCADOR_HASH_TABLE;
	void* inst = ctx->ht_inst;

	if (strcmp(ed, "skip_list") == 0) {
		b = &BUSCADOR_SKIP_LIST;
		inst = ctx->sl_inst;
	} else if (strcmp(ed, "tabela_ord") == 0) {
		b = &BUSCADOR_TABELA_ORD;
		inst = ctx->to_inst;
	} else {
		strcpy(ed, "hash_table");
	}

	Resultado* r = NULL;
	if (strcmp(tipo, "genero") == 0) {
		r = b->buscar_genero(inst, termo);
	} else {
		strcpy(tipo, "artista");
		r = b->buscar_artista(inst, termo);
	}

	JsonBuffer* jb = json_buffer_criar(4096);
	json_serializar_resultado(jb, r, termo, tipo, ed);

	enviar_resposta(sock, 200, "OK", "application/json",
	                json_buffer_obter_texto(jb), json_buffer_tamanho(jb));

	json_buffer_liberar(jb);
	resultado_liberar(r);
}

static void handle_comparar(int sock, const ServerContext* ctx, const char* query) {
	char termo[256] = {0};
	char tipo[32] = "artista";

	extrair_param(query, "q", termo, sizeof termo);
	extrair_param(query, "tipo", tipo, sizeof tipo);

	int is_genero = (strcmp(tipo, "genero") == 0);
	const char* tipo_str = is_genero ? "genero" : "artista";

	Resultado* r_ht = is_genero ? BUSCADOR_HASH_TABLE.buscar_genero(ctx->ht_inst, termo)
	                            : BUSCADOR_HASH_TABLE.buscar_artista(ctx->ht_inst, termo);

	Resultado* r_sl = is_genero ? BUSCADOR_SKIP_LIST.buscar_genero(ctx->sl_inst, termo)
	                            : BUSCADOR_SKIP_LIST.buscar_artista(ctx->sl_inst, termo);

	Resultado* r_to = is_genero ? BUSCADOR_TABELA_ORD.buscar_genero(ctx->to_inst, termo)
	                            : BUSCADOR_TABELA_ORD.buscar_artista(ctx->to_inst, termo);

	char json[1024];
	snprintf(json, sizeof json,
	         "{\"termo\":\"%s\",\"tipo\":\"%s\",\"total_encontrados\":%d,"
	         "\"comparativo\":["
	         "{\"estrutura\":\"hash_table\",\"tempo_ms\":%.6f,\"comparacoes\":%ld},"
	         "{\"estrutura\":\"skip_list\",\"tempo_ms\":%.6f,\"comparacoes\":%ld},"
	         "{\"estrutura\":\"tabela_ord\",\"tempo_ms\":%.6f,\"comparacoes\":%ld}"
	         "]}",
	         termo, tipo_str, resultado_tamanho(r_ht),
	         resultado_tempo_ms(r_ht), resultado_comparacoes(r_ht),
	         resultado_tempo_ms(r_sl), resultado_comparacoes(r_sl),
	         resultado_tempo_ms(r_to), resultado_comparacoes(r_to));

	enviar_resposta(sock, 200, "OK", "application/json", json, strlen(json));

	resultado_liberar(r_ht);
	resultado_liberar(r_sl);
	resultado_liberar(r_to);
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
	ctx.ht_inst = BUSCADOR_HASH_TABLE.criar();
	ctx.sl_inst = BUSCADOR_SKIP_LIST.criar();
	ctx.to_inst = BUSCADOR_TABELA_ORD.criar();

	int total = csv_tamanho(csv);
	for (int i = 0; i < total; i++) {
		const Obra* o = csv_obra(csv, i);
		BUSCADOR_HASH_TABLE.inserir(ctx.ht_inst, o);
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
	printf("  - GET /api/busca?q=<termo>&tipo=<artista|genero>&ed=<hash_table|skip_list|tabela_ord>\n");
	printf("  - GET /api/comparar?q=<termo>&tipo=<artista|genero>\n");
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
	BUSCADOR_HASH_TABLE.liberar(ctx.ht_inst);
	BUSCADOR_SKIP_LIST.liberar(ctx.sl_inst);
	BUSCADOR_TABELA_ORD.liberar(ctx.to_inst);
	return 0;
}