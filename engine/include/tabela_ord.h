/**
 * tabela_ord.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD TabelaOrd. Lista indexada ordenada por
 *            artista, com busca binária na chave.
 */
#ifndef TABELA_ORD_H
#define TABELA_ORD_H

#include "obra.h"
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
 * Retorna TabelaOrd*: ponteiro para a tabela alocada, ou NULL em erro
 */
TabelaOrd* tabela_ord_criar(void);

/** Libera a tabela e o array interno. Não libera as Obras apontadas.
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela a ser liberada
 */
void tabela_ord_liberar(TabelaOrd* t);

/** Insere uma obra na posição correta, mantendo a ordenação por artista
 *
 * Parâmetros:
 * TabelaOrd* t: ponteiro para a tabela
 * const Obra* o: ponteiro para a obra (não copiada, só referenciada)
 */
void tabela_ord_inserir(TabelaOrd* t, const Obra* o);

/* ==============================
 * Consultas
 * ============================== */

/** Número de obras na tabela
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 *
 * Retorna int: quantidade de elementos
 */
int tabela_ord_tamanho(const TabelaOrd* t);

/** Retorna a Obra no índice dado, ou NULL se inválido
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * int indice: posição desejada (0 <= indice < tamanho)
 *
 * Retorna const Obra*: ponteiro para a obra na posição, ou NULL
 */
const Obra* tabela_ord_item(const TabelaOrd* t, int indice);

/** Busca todas as obras de um dado artista, via busca binária
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* tabela_ord_buscar_artista(const TabelaOrd* t, const char* artista);

/** Busca todas as obras de um dado gênero, via varredura linear
 *
 * Parâmetros:
 * const TabelaOrd* t: ponteiro para a tabela
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* tabela_ord_buscar_genero(const TabelaOrd* t, const char* genero);

#endif
