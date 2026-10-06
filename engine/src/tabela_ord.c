/**
 * tabela_ord.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD TabelaOrd. Lista indexada ordenada
 *            por um Comparador, com busca binária.
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
	const void** itens;
	int n;
	int cap;
	Comparador cmp_itens;
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

/** Encontra o índice do primeiro item que não vem antes da chave
 *
 * Busca binária de limite inferior sobre o intervalo semiaberto
 * [inf, sup). Não sai antecipadamente ao encontrar um item do
 * intervalo, porque pode haver outro mais à esquerda. Se todos os
 * itens vierem antes, devolve n.
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * Comparador cmp: comparador item x chave
 * const void* chave: chave procurada
 * long* comp_out: contador de comparações, incrementado
 *
 * Retorna int: índice encontrado (0 a n)
 */
// MODIFICAÇÃO: busca binária de LIMITE INFERIOR, para intervalo e repetidos.
// Clássico: pára ao achar uma chave igual e devolve aquele índice (qualquer
// um, se há repetidos). Nosso: não pára; devolve o primeiro índice que não vem
// antes da chave, que é o começo do intervalo.
static int limite_inferior(const TabelaOrd* t, Comparador cmp,
			   const void* chave, long* comp_out) {
	int inf = 0;
	int sup = t->n;

	while (inf < sup) {
		int meio = inf + (sup - inf) / 2;
		(*comp_out)++;
		if (cmp(t->itens[meio], chave) < 0) {
			inf = meio + 1;
		} else {
			sup = meio;
		}
	}
	return inf;
}

/** Encontra o índice do primeiro item que vem depois do intervalo
 *
 * Mesma busca de limite_inferior, mas o item dentro do intervalo
 * também fica à esquerda. Se nenhum vier depois, devolve n.
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * Comparador cmp: comparador item x chave
 * const void* chave: chave procurada
 * long* comp_out: contador de comparações, incrementado
 *
 * Retorna int: índice encontrado (0 a n)
 */
// MODIFICAÇÃO (variante nova): limite superior, o fim do intervalo.
// Clássico: não há. Nosso: mesma busca binária, com o item dentro do intervalo
// também à esquerda; com os dois limites o tamanho do intervalo sai em O(log n).
static int limite_superior(const TabelaOrd* t, Comparador cmp,
			   const void* chave, long* comp_out) {
	int inf = 0;
	int sup = t->n;

	while (inf < sup) {
		int meio = inf + (sup - inf) / 2;
		(*comp_out)++;
		if (cmp(t->itens[meio], chave) <= 0) {
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
	const void** tmp = realloc((void*) t->itens, nova_cap * sizeof *tmp);
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
 * Parâmetros:
 * Comparador cmp_itens: ordem total entre itens, usada na inserção
 *
 * Retorna TabelaOrd*: ponteiro para a tabela alocada, ou NULL em erro
 */
TabelaOrd* tabela_ord_criar(Comparador cmp_itens) {
	if (!cmp_itens) return NULL;

	TabelaOrd* t = malloc(sizeof *t);
	if (!t) return NULL;

	t->itens = malloc(CAP_INICIAL * sizeof *t->itens);
	if (!t->itens) {
		free(t);
		return NULL;
	}

	t->n         = 0;
	t->cap       = CAP_INICIAL;
	t->cmp_itens = cmp_itens;
	return t;
}

/** Libera a tabela e o array interno. Não libera os itens apontados.
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela a ser liberada
 */
void tabela_ord_liberar(TabelaOrd* t) {
	if (!t) return;
	free((void*) t->itens);
	free(t);
}

/** Insere um item na posição correta, mantendo a ordenação
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela
 * const void* item: ponteiro para o item (não copiado, só referenciado)
 */
void tabela_ord_inserir(TabelaOrd* t, const void* item) {
	if (!garantir_capacidade(t)) return;

	/* O próprio item faz o papel de chave: o limite inferior é a
	 * primeira posição que não vem antes dele. */
	long comp_ignorado = 0;
	int pos = limite_inferior(t, t->cmp_itens, item, &comp_ignorado);

	/* Abre espaço: move do fim até pos uma posição à frente. */
	memmove((void*) &t->itens[pos + 1],
		(const void*) &t->itens[pos],
		(size_t)(t->n - pos) * sizeof *t->itens);

	t->itens[pos] = item;
	t->n++;
}

/** Número de itens na tabela
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 *
 * Retorna int: quantidade de elementos
 */
int tabela_ord_tamanho(const TabelaOrd* t) {
	return t->n;
}

/** Retorna o item no índice dado, ou NULL se inválido
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * int indice: posição desejada (0 <= indice < tamanho)
 *
 * Retorna const void*: ponteiro para o item na posição, ou NULL
 */
const void* tabela_ord_item(const TabelaOrd* t, int indice) {
	if (indice < 0 || indice >= t->n) return NULL;
	return t->itens[indice];
}

/* ==============================
 * Busca
 * ============================== */

/** Busca todos os itens do intervalo descrito pela chave
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * Comparador cmp: comparador item x chave, ou NULL para todos
 * const void* chave: chave procurada (ignorada se cmp for NULL)
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
// MODIFICAÇÃO: busca de intervalo contíguo.
// Clássico: a busca binária devolve um único item. Nosso: acha o começo do
// intervalo e coleta enquanto o comparador der zero, O(log n + k).
Resultado* tabela_ord_buscar(const TabelaOrd* t, Comparador cmp,
			     const void* chave) {
	Resultado* r = resultado_criar(cmp ? 16 : t->n);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	if (!cmp) {
		/* Sem chave, a busca vira percurso: a ordem do array já é
		 * a resposta, e nada precisa ser comparado. */
		for (int i = 0; i < t->n; i++) {
			resultado_adicionar(r, t->itens[i]);
		}
	} else {
		/* O intervalo é contíguo na ordem da tabela: basta achar o
		 * início e coletar até o comparador sair do zero. */
		int pos = limite_inferior(t, cmp, chave, &comp);
		while (pos < t->n) {
			comp++;
			if (cmp(t->itens[pos], chave) != 0) break;
			resultado_adicionar(r, t->itens[pos]);
			pos++;
		}
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}

/** Busca uma página do intervalo descrito pela chave
 *
 * Dois limites por busca binária dão o início e o tamanho do intervalo,
 * e a página é uma fatia do array. Custo O(log n + limite).
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * Comparador cmp: comparador item x chave, ou NULL para todos
 * const void* chave: chave procurada (ignorada se cmp for NULL)
 * int offset: quantos itens do intervalo pular
 * int limite: máximo de itens a devolver
 *
 * Retorna Resultado*: a página, com o total do intervalo
 */
// MODIFICAÇÃO: busca paginada (offset, limite).
// Clássico: coletar o intervalo inteiro e cortar depois. Nosso: dois limites
// por busca binária e uma fatia do array, O(log n + limite).
Resultado* tabela_ord_buscar_pagina(const TabelaOrd* t, Comparador cmp,
				    const void* chave, int offset,
				    int limite) {
	if (offset < 0) offset = 0;
	if (limite < 0) limite = 0;

	Resultado* r = resultado_criar(limite < 1024 ? limite : 1024);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	int ini = 0;
	int fim = t->n;
	if (cmp) {
		ini = limite_inferior(t, cmp, chave, &comp);
		fim = limite_superior(t, cmp, chave, &comp);
	}

	int total = fim - ini;
	for (int i = ini + offset; i < fim && i < ini + offset + limite; i++) {
		resultado_adicionar(r, t->itens[i]);
	}

	resultado_set_total(r, total);
	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}
