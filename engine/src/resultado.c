/**
 * resultado.c
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD Resultado.
 */
#include <stdlib.h>
#include "resultado.h"

/* ==============================
 * Implementação do TAD
 * ============================== */

struct resultado_t {
	const void** itens;
	int n;
	int cap;
	double tempo_ms;
	long comparacoes;
	long rotacoes;
};

/* ==============================
 * API pública
 * ============================== */

/** Cria um resultado com capacidade inicial
 *
 * Parâmetros:
 * int capacidade_inicial: número estimado de itens (mínimo 1)
 *
 * Retorna Resultado*: ponteiro para o resultado alocado, ou NULL em erro
 */
Resultado* resultado_criar(int capacidade_inicial) {
	if (capacidade_inicial < 1) capacidade_inicial = 1;

	Resultado* r = malloc(sizeof *r);
	if (!r) return NULL;

	r->itens = malloc(capacidade_inicial * sizeof *r->itens);
	if (!r->itens) {
		free(r);
		return NULL;
	}

	r->n           = 0;
	r->cap         = capacidade_inicial;
	r->tempo_ms    = 0.0;
	r->comparacoes = 0;
	r->rotacoes    = 0;
	return r;
}

/** Libera o resultado e o array interno (não libera os itens apontados)
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado a ser liberado
 */
void resultado_liberar(Resultado* r) {
	if (!r) return;
	free((void*) r->itens);
	free(r);
}

/** Adiciona um item ao resultado
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * const void* item: ponteiro para o item (não é copiado, só referenciado)
 */
void resultado_adicionar(Resultado* r, const void* item) {
	if (r->n == r->cap) {
		int nova_cap = r->cap * 2;
		const void** tmp = realloc((void*) r->itens,
					   nova_cap * sizeof *tmp);
		if (!tmp) return;
		r->itens = tmp;
		r->cap   = nova_cap;
	}
	r->itens[r->n++] = item;
}

/** Define as métricas do resultado
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * double tempo_ms: tempo gasto na busca, em milissegundos
 * long comparacoes: número de comparações realizadas
 */
void resultado_set_metricas(Resultado* r, double tempo_ms, long comparacoes) {
	r->tempo_ms    = tempo_ms;
	r->comparacoes = comparacoes;
}

/** Define quantas rotações a busca fez na estrutura
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * long rotacoes: número de rotações realizadas
 */
void resultado_set_rotacoes(Resultado* r, long rotacoes) {
	r->rotacoes = rotacoes;
}

/* ==============================
 * Getters
 * ============================== */

int resultado_tamanho(const Resultado* r) {
	return r->n;
}

const void* resultado_item(const Resultado* r, int indice) {
	if (indice < 0 || indice >= r->n) return NULL;
	return r->itens[indice];
}

double resultado_tempo_ms(const Resultado* r) {
	return r->tempo_ms;
}

long resultado_comparacoes(const Resultado* r) {
	return r->comparacoes;
}

long resultado_rotacoes(const Resultado* r) {
	return r->rotacoes;
}
