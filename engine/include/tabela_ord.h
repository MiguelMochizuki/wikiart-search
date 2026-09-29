/**
 * tabela_ord.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD TabelaOrd. Lista indexada: array mantido
 *            ordenado por um Comparador, com busca binária.
 *
 * A tabela não conhece o tipo dos itens. Quem a cria entrega a ordem
 * entre itens, e cada busca entrega um comparador de chave que recorta
 * um intervalo contíguo dessa ordem. Assim a mesma estrutura serve aos
 * três níveis da navegação: gêneros por nome, artistas por (gênero,
 * artista) e obras por (gênero, artista, id).
 */
#ifndef TABELA_ORD_H
#define TABELA_ORD_H

#include "comparador.h"
#include "resultado.h"

/* ==============================
 * TADs
 * ============================== */

typedef struct tabela_ord_t TabelaOrd;

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
TabelaOrd* tabela_ord_criar(Comparador cmp_itens);

/** Libera a tabela e o array interno. Não libera os itens apontados.
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela a ser liberada
 */
void tabela_ord_liberar(TabelaOrd* t);

/** Insere um item na posição correta, mantendo a ordenação
 *
 * Localiza a posição por busca binária e desloca a cauda do array uma
 * casa para a direita. Custo O(n) no pior caso, pelo deslocamento.
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela
 * const void* item: ponteiro para o item (não copiado, só referenciado)
 */
void tabela_ord_inserir(TabelaOrd* t, const void* item);

/* ==============================
 * Consultas
 * ============================== */

/** Número de itens na tabela
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 *
 * Retorna int: quantidade de elementos
 */
int tabela_ord_tamanho(const TabelaOrd* t);

/** Retorna o item no índice dado, ou NULL se inválido
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * int indice: posição desejada (0 <= indice < tamanho)
 *
 * Retorna const void*: ponteiro para o item na posição, ou NULL
 */
const void* tabela_ord_item(const TabelaOrd* t, int indice);

/** Busca todos os itens do intervalo descrito pela chave
 *
 * Busca binária de limite inferior até o início do intervalo e coleta
 * os itens seguintes enquanto o comparador devolver 0. Custo
 * O(log n + k), com k igual ao número de itens encontrados. Com cmp
 * NULL não há chave: devolve a tabela inteira, em ordem, sem comparar.
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * Comparador cmp: comparador item x chave, ou NULL para todos
 * const void* chave: chave procurada (ignorada se cmp for NULL)
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* tabela_ord_buscar(const TabelaOrd* t, Comparador cmp,
			     const void* chave);

/** Busca uma página do intervalo descrito pela chave
 *
 * Dois limites por busca binária dão o início e o tamanho do intervalo,
 * e a página é uma fatia do array. Custo O(log n + limite),
 * independente do tamanho do intervalo.
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * Comparador cmp: comparador item x chave, ou NULL para todos
 * const void* chave: chave procurada (ignorada se cmp for NULL)
 * int offset: quantos itens do intervalo pular
 * int limite: máximo de itens a devolver
 *
 * Retorna Resultado*: a página, com o total do intervalo em
 *                     resultado_total
 */
Resultado* tabela_ord_buscar_pagina(const TabelaOrd* t, Comparador cmp,
				    const void* chave, int offset,
				    int limite);

#endif
