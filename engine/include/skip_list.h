/**
 * skip_list.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD SkipList. Lista encadeada probabilística,
 *            mantida ordenada por artista, com busca esperada O(log n).
 */
#ifndef SKIP_LIST_H
#define SKIP_LIST_H

#include "obra.h"
#include "resultado.h"

/* ==============================
 * TADs
 * ============================== */

typedef struct skip_list_t SkipList;

/* ==============================
 * API pública
 * ============================== */

/** Cria uma Skip List vazia
 *
 * Retorna SkipList*: ponteiro para a lista alocada, ou NULL em erro
 */
SkipList* skip_list_criar(void);

/** Libera a Skip List e todos os nós internos. Não libera as Obras.
 *
 * Parâmetros:
 * SkipList* sl: ponteiro para a lista a ser liberada
 */
void skip_list_liberar(SkipList* sl);

/** Insere uma obra mantendo a ordenação por artista
 *
 * Parâmetros:
 * SkipList* sl: ponteiro para a lista
 * const Obra* o: ponteiro para a obra (não copiada, só referenciada)
 */
void skip_list_inserir(SkipList* sl, const Obra* o);

/* ==============================
 * Consultas
 * ============================== */

/** Número de obras na lista
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 *
 * Retorna int: quantidade de elementos
 */
int skip_list_tamanho(const SkipList* sl);

/** Busca todas as obras de um dado artista
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_artista(const SkipList* sl, const char* artista);

/** Busca todas as obras de um dado gênero
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_genero(const SkipList* sl, const char* genero);

#endif
