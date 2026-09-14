/**
 * tabela_ord.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD TabelaOrd. Lista indexada ordenada
 *            por artista, com busca binária na chave.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tabela_ord.h"

/* ==============================
 * Constantes
 * ============================== */

#define CAP_INICIAL 1024

/* ==============================
 * Implementação do TAD
 * ============================== */

struct tabela_ord_t {
	const Obra** itens;
	int n;
	int cap;
};

/* ==============================
 * Helpers internos
 * ============================== */

/** Retorna o tempo atual em milissegundos (relógio monotônico)
 *
 * Retorna double: tempo em ms desde um ponto arbitrário
 */
static double agora_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/** Encontra o índice da primeira obra com artista >= chave
 *
 * Implementa a busca binária clássica de limite inferior: devolve o
 * menor índice tal que o artista da obra é maior ou igual à chave.
 * Se todas forem menores, devolve n.
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * const char* chave: artista procurado
 * long* comp_out: contador de comparações, incrementado
 *
 * Retorna int: índice encontrado (0 a n)
 */
static int limite_inferior(const TabelaOrd* t, const char* chave, long* comp_out) {
	int inf = 0;
	int sup = t->n;

	while (inf < sup) {
		int meio = inf + (sup - inf) / 2;
		(*comp_out)++;
		if (strcmp(obra_artista(t->itens[meio]), chave) < 0) {
			inf = meio + 1;
		} else {
			sup = meio;
		}
	}
	return inf;
}

/** Garante que o array tem espaço para mais um elemento
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela
 *
 * Retorna int: 1 em sucesso, 0 em falha de alocação
 */
static int garantir_capacidade(TabelaOrd* t) {
	if (t->n < t->cap) return 1;

	int nova_cap = t->cap * 2;
	const Obra** tmp = realloc((void*) t->itens, nova_cap * sizeof *tmp);
	if (!tmp) return 0;

	t->itens = tmp;
	t->cap   = nova_cap;
	return 1;
}

/* ==============================
 * API pública
 * ============================== */

/** Cria uma tabela ordenada vazia
 *
 * Retorna TabelaOrd*: ponteiro para a tabela alocada, ou NULL em erro
 */
TabelaOrd* tabela_ord_criar(void) {
	TabelaOrd* t = malloc(sizeof *t);
	if (!t) return NULL;

	t->itens = malloc(CAP_INICIAL * sizeof *t->itens);
	if (!t->itens) {
		free(t);
		return NULL;
	}

	t->n   = 0;
	t->cap = CAP_INICIAL;
	return t;
}

/** Libera a tabela e o array interno. Não libera as Obras apontadas.
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela a ser liberada
 */
void tabela_ord_liberar(TabelaOrd* t) {
	if (!t) return;
	free((void*) t->itens);
	free(t);
}

/** Insere uma obra na posição correta, mantendo a ordenação por artista
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela
 * const Obra* o: ponteiro para a obra (não copiada, só referenciada)
 */
void tabela_ord_inserir(TabelaOrd* t, const Obra* o) {
	if (!garantir_capacidade(t)) return;

	long comp_ignorado = 0;
	int pos = limite_inferior(t, obra_artista(o), &comp_ignorado);

	/* Abre espaço: move do fim até pos uma posição à frente. */
	memmove((void*) &t->itens[pos + 1],
		&t->itens[pos],
		(size_t)(t->n - pos) * sizeof *t->itens);

	t->itens[pos] = o;
	t->n++;
}

/** Número de obras na tabela
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 *
 * Retorna int: quantidade de elementos
 */
int tabela_ord_tamanho(const TabelaOrd* t) {
	return t->n;
}

/** Retorna a Obra no índice dado, ou NULL se inválido
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * int indice: posição desejada (0 <= indice < tamanho)
 *
 * Retorna const Obra*: ponteiro para a obra na posição, ou NULL
 */
const Obra* tabela_ord_item(const TabelaOrd* t, int indice) {
	if (indice < 0 || indice >= t->n) return NULL;
	return t->itens[indice];
}

/** Busca todas as obras de um dado artista, via busca binária
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* tabela_ord_buscar_artista(const TabelaOrd* t, const char* artista) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Acha a primeira posição com artista >= chave. */
	int pos = limite_inferior(t, artista, &comp);

	/* Coleta todas as ocorrências consecutivas com artista == chave. */
	while (pos < t->n) {
		comp++;
		if (strcmp(obra_artista(t->itens[pos]), artista) != 0) break;
		resultado_adicionar(r, t->itens[pos]);
		pos++;
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}

/** Busca todas as obras de um dado gênero, via varredura linear
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* tabela_ord_buscar_genero(const TabelaOrd* t, const char* genero) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Gênero não é chave de ordenação. Varredura completa. */
	for (int i = 0; i < t->n; i++) {
		comp++;
		if (strcmp(obra_genero(t->itens[i]), genero) == 0) {
			resultado_adicionar(r, t->itens[i]);
		}
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}
