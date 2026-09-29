/**
 * json.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
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

/** Cria um buffer JSON vazio
 *
 * Parâmetros:
 * size_t capacidade_inicial: bytes reservados (mínimo 64)
 *
 * Retorna JsonBuffer*: ponteiro para o buffer, ou NULL em erro
 */
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

/** Libera o buffer e o texto acumulado
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer a liberar (pode ser NULL)
 */
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

/** Garante espaço para mais `extra` bytes e o terminador
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer
 * size_t extra: bytes que serão acrescentados
 */
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

/** Acrescenta um texto ao buffer, sem escapar
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const char* str: texto a acrescentar
 */
static void jb_adicionar_raw(JsonBuffer* jb, const char* str) {
	size_t l = strlen(str);
	jb_garantir_espaco(jb, l);
	memcpy(jb->dados + jb->len, str, l);
	jb->len += l;
	jb->dados[jb->len] = '\0';
}

/** Acrescenta uma string JSON entre aspas, com escapes
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const char* str: texto a escrever (NULL vira "")
 */
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

/** Serializa uma obra como objeto JSON
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Obra* o: obra a serializar
 */
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

/** Escreve uma string escapada, ou o literal null se for NULL
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const char* str: string a escrever, ou NULL
 */
static void jb_adicionar_str_ou_null(JsonBuffer* jb, const char* str) {
	if (!str) {
		jb_adicionar_raw(jb, "null");
		return;
	}
	jb_adicionar_escapado(jb, str);
}

/** Serializa um gênero, com contagens e a obra de capa
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Genero* g: gênero a serializar
 */
void json_serializar_genero(JsonBuffer* jb, const Genero* g) {
	char buf[96];
	jb_adicionar_raw(jb, "{\"nome\":");
	jb_adicionar_escapado(jb, genero_nome(g));
	snprintf(buf, sizeof buf, ",\"obras\":%d,\"artistas\":%d,\"capa\":",
	         genero_obras(g), genero_artistas(g));
	jb_adicionar_raw(jb, buf);
	json_serializar_obra(jb, genero_capa(g));
	jb_adicionar_raw(jb, "}");
}

/** Serializa um par (gênero, artista), com contagem e obra de capa
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Artista* a: artista a serializar
 */
void json_serializar_artista(JsonBuffer* jb, const Artista* a) {
	char buf[64];
	jb_adicionar_raw(jb, "{\"genero\":");
	jb_adicionar_escapado(jb, artista_genero(a));
	jb_adicionar_raw(jb, ",\"nome\":");
	jb_adicionar_escapado(jb, artista_nome(a));
	snprintf(buf, sizeof buf, ",\"obras\":%d,\"capa\":", artista_obras(a));
	jb_adicionar_raw(jb, buf);
	json_serializar_obra(jb, artista_capa(a));
	jb_adicionar_raw(jb, "}");
}

/** Serializa um item conforme o tipo do nível
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * Nivel nivel: nível de onde o item veio
 * const void* item: Genero*, Artista* ou Obra*
 */
static void serializar_item(JsonBuffer* jb, Nivel nivel, const void* item) {
	switch (nivel) {
		case NIVEL_GENEROS:  json_serializar_genero(jb, item);  break;
		case NIVEL_ARTISTAS: json_serializar_artista(jb, item); break;
		default:             json_serializar_obra(jb, item);    break;
	}
}

/** Nome do nível na API */
static const char* nome_nivel(Nivel nivel) {
	switch (nivel) {
		case NIVEL_GENEROS:  return "generos";
		case NIVEL_ARTISTAS: return "artistas";
		default:             return "obras";
	}
}

/** Escreve os campos comuns às respostas de busca, terminando em vírgula
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Resultado* r: resultado de onde saem as métricas
 * const JsonConsulta* c: consulta que o originou
 */
static void serializar_cabecalho(JsonBuffer* jb, const Resultado* r,
                                 const JsonConsulta* c) {
	char buf[192];

	jb_adicionar_raw(jb, "\"nivel\":");
	jb_adicionar_escapado(jb, nome_nivel(c->nivel));
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"genero\":");
	jb_adicionar_str_ou_null(jb, c->genero);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"artista\":");
	jb_adicionar_str_ou_null(jb, c->artista);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"estrutura\":");
	jb_adicionar_escapado(jb, c->estrutura);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"algoritmo\":");
	jb_adicionar_escapado(jb, c->algoritmo);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"metricas\":{");
	snprintf(buf, sizeof buf,
	         "\"tempo_ms\":%.6f,\"comparacoes\":%ld,\"rotacoes\":%ld,"
	         "\"total_encontrados\":%d",
	         resultado_tempo_ms(r), resultado_comparacoes(r),
	         resultado_rotacoes(r), resultado_total(r));
	jb_adicionar_raw(jb, buf);
	jb_adicionar_raw(jb, "},");
}

/** Serializa um resultado como lista, com as métricas da busca
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Resultado* r: resultado da busca
 * const JsonConsulta* c: consulta que o originou
 */
void json_serializar_resultado(JsonBuffer* jb, const Resultado* r,
                               const JsonConsulta* c) {
	jb_adicionar_raw(jb, "{");
	serializar_cabecalho(jb, r, c);

	jb_adicionar_raw(jb, "\"resultados\":[");
	int tot = resultado_tamanho(r);
	for (int i = 0; i < tot; i++) {
		if (i > 0) jb_adicionar_raw(jb, ",");
		serializar_item(jb, c->nivel, resultado_item(r, i));
	}
	jb_adicionar_raw(jb, "]}");
}

/** Serializa as métricas de uma busca junto com a vista da árvore
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Resultado* r: resultado da busca, de onde saem as métricas
 * const JsonConsulta* c: consulta que o originou
 * const NoVista* vista: posições da vista, em layout de heap
 * int niveis: níveis da vista (2^niveis - 1 posições)
 * int total: itens do intervalo mostrado
 */
void json_serializar_vista(JsonBuffer* jb, const Resultado* r,
                           const JsonConsulta* c,
                           const NoVista* vista, int niveis, int total) {
	char buf[96];
	jb_adicionar_raw(jb, "{");
	serializar_cabecalho(jb, r, c);

	snprintf(buf, sizeof buf, "\"total\":%d,\"niveis\":%d,\"vista\":[",
	         total, niveis);
	jb_adicionar_raw(jb, buf);

	int n_pos = (1 << niveis) - 1;
	for (int i = 0; i < n_pos; i++) {
		if (i > 0) jb_adicionar_raw(jb, ",");
		if (!vista[i].item) {
			jb_adicionar_raw(jb, "null");
			continue;
		}
		snprintf(buf, sizeof buf, "{\"descendentes\":%d,\"item\":",
		         vista[i].descendentes);
		jb_adicionar_raw(jb, buf);
		serializar_item(jb, c->nivel, vista[i].item);
		jb_adicionar_raw(jb, "}");
	}
	jb_adicionar_raw(jb, "]}");
}

/** Serializa a mesma busca respondida por várias EDs
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const char* genero: gênero consultado
 * const char* artista: artista consultado, ou NULL
 * const char* consulta: rótulo da consulta
 * const JsonComparacao* itens: uma linha por ED
 * int n: número de linhas
 */
void json_serializar_comparativo(JsonBuffer* jb,
                                 const char* genero, const char* artista,
                                 const char* consulta,
                                 const JsonComparacao* itens, int n) {
	char buf[192];
	jb_adicionar_raw(jb, "{");

	jb_adicionar_raw(jb, "\"genero\":");
	jb_adicionar_str_ou_null(jb, genero);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"artista\":");
	jb_adicionar_str_ou_null(jb, artista);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"consulta\":");
	jb_adicionar_escapado(jb, consulta);
	jb_adicionar_raw(jb, ",");

	jb_adicionar_raw(jb, "\"total_encontrados\":");
	snprintf(buf, sizeof buf, "%d,",
	         n > 0 ? resultado_tamanho(itens[0].resultado) : 0);
	jb_adicionar_raw(jb, buf);

	jb_adicionar_raw(jb, "\"comparativo\":[");
	for (int i = 0; i < n; i++) {
		if (i > 0) jb_adicionar_raw(jb, ",");

		jb_adicionar_raw(jb, "{\"estrutura\":");
		jb_adicionar_escapado(jb, itens[i].estrutura);
		jb_adicionar_raw(jb, ",\"algoritmo\":");
		jb_adicionar_escapado(jb, itens[i].algoritmo);

		snprintf(buf, sizeof buf,
		         ",\"tempo_ms\":%.6f,\"comparacoes\":%ld,\"rotacoes\":%ld,"
		         "\"encontrados\":%d}",
		         resultado_tempo_ms(itens[i].resultado),
		         resultado_comparacoes(itens[i].resultado),
		         resultado_rotacoes(itens[i].resultado),
		         resultado_tamanho(itens[i].resultado));
		jb_adicionar_raw(jb, buf);
	}
	jb_adicionar_raw(jb, "]}");
}
