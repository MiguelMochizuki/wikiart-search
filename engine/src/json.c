/**
 * json.c
 * Descrição: Implementação da serialização JSON.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"

struct json_buffer_t {
	char* dados;
	size_t len;
	size_t cap;
};

JsonBuffer* json_buffer_criar(size_t capacidade_inicial) {
	if (capacidade_inicial < 64) capacidade_inicial = 64;
	JsonBuffer* jb = malloc(sizeof *jb);
	if (!jb) return NULL;

	jb->dados = malloc(capacidade_inicial);
	if (!jb->dados) {
		free(jb);
		return NULL;
	}
	jb->dados[0] = '\0';
	jb->len = 0;
	jb->cap = capacidade_inicial;
	return jb;
}

void json_buffer_liberar(JsonBuffer* jb) {
	if (!jb) return;
	free(jb->dados);
	free(jb);
}

const char* json_buffer_obter_texto(const JsonBuffer* jb) {
	return jb ? jb->dados : "";
}

size_t json_buffer_tamanho(const JsonBuffer* jb) {
	return jb ? jb->len : 0;
}

static void jb_garantir_espaco(JsonBuffer* jb, size_t extra) {
	if (jb->len + extra + 1 > jb->cap) {
		size_t nova_cap = (jb->cap * 2 > jb->len + extra + 1)
		                  ? jb->cap * 2
		                  : jb->len + extra + 64;
		char* tmp = realloc(jb->dados, nova_cap);
		if (tmp) {
			jb->dados = tmp;
			jb->cap = nova_cap;
		}
	}
}

static void jb_adicionar_raw(JsonBuffer* jb, const char* str) {
	size_t l = strlen(str);
	jb_garantir_espaco(jb, l);
	memcpy(jb->dados + jb->len, str, l);
	jb->len += l;
	jb->dados[jb->len] = '\0';
}

static void jb_adicionar_escapado(JsonBuffer* jb, const char* str) {
	if (!str) {
		jb_adicionar_raw(jb, "\"\"");
		return;
	}
	jb_adicionar_raw(jb, "\"");
	for (const char* p = str; *p; p++) {
		switch (*p) {
			case '\"': jb_adicionar_raw(jb, "\\\""); break;
			case '\\': jb_adicionar_raw(jb, "\\\\"); break;
			case '\b': jb_adicionar_raw(jb, "\\b"); break;
			case '\f': jb_adicionar_raw(jb, "\\f"); break;
			case '\n': jb_adicionar_raw(jb, "\\n"); break;
			case '\r': jb_adicionar_raw(jb, "\\r"); break;
			case '\t': jb_adicionar_raw(jb, "\\t"); break;
			default: {
				char c[2] = { *p, '\0' };
				jb_adicionar_raw(jb, c);
				break;
			}
		}
	}
	jb_adicionar_raw(jb, "\"");
}

void json_serializar_obra(JsonBuffer* jb, const Obra* o) {
	char buf[64];
	jb_adicionar_raw(jb, "{");

	jb_adicionar_raw(jb, "\"id\":");
	snprintf(buf, sizeof buf, "%d,", obra_id(o));
	jb_adicionar_raw(jb, buf);

	jb_adicionar_raw(jb, "\"titulo\":");
	jb_adicionar_escapado(jb, obra_titulo(o));
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"artista\":");
	jb_adicionar_escapado(jb, obra_artista(o));
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"genero\":");
	jb_adicionar_escapado(jb, obra_genero(o));
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"ano\":");
	snprintf(buf, sizeof buf, "%d,", obra_ano(o));
	jb_adicionar_raw(jb, buf);

	jb_adicionar_raw(jb, "\"caminho\":");
	jb_adicionar_escapado(jb, obra_caminho(o));

	jb_adicionar_raw(jb, "}");
}

void json_serializar_resultado(JsonBuffer* jb, const Resultado* r,
                               const char* termo, const char* tipo,
                               const char* estrutura) {
	char buf[128];
	jb_adicionar_raw(jb, "{");

	jb_adicionar_raw(jb, "\"termo\":");
	jb_adicionar_escapado(jb, termo);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"tipo\":");
	jb_adicionar_escapado(jb, tipo);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"estrutura\":");
	jb_adicionar_escapado(jb, estrutura);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"metricas\":{");
	snprintf(buf, sizeof buf, "\"tempo_ms\":%.6f,\"comparacoes\":%ld,\"total_encontrados\":%d",
	         resultado_tempo_ms(r), resultado_comparacoes(r), resultado_tamanho(r));
	jb_adicionar_raw(jb, buf);
	jb_adicionar_raw(jb, "},");

	jb_adicionar_raw(jb, "\"resultados\":[");
	int tot = resultado_tamanho(r);
	for (int i = 0; i < tot; i++) {
		if (i > 0) jb_adicionar_raw(jb, ",");
		json_serializar_obra(jb, resultado_item(r, i));
	}
	jb_adicionar_raw(jb, "]}");
}