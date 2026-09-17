/**
 * skip_list.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD SkipList. Lista encadeada probabilística,
 *            mantida ordenada pela chave composta (gênero, artista),
 *            com busca esperada O(log n).
 *
 * A ordenação primária é o gênero e a secundária o artista. Isso torna
 * contíguos no nível 0 tanto o bloco de um gênero quanto, dentro dele,
 * o bloco de um artista, permitindo descida pelos níveis nas duas
 * consultas. Em troca, as obras de um mesmo artista ficam espalhadas
 * entre os blocos de gênero, então a busca só por artista degenera em
 * varredura do nível 0.
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

/** Busca todas as obras de um dado artista, via varredura do nível 0
 *
 * Artista é a chave secundária: suas obras ficam distribuídas entre os
 * blocos de gênero, e os níveis superiores particionam por gênero, sem
 * nada a descartar aqui. Custo O(n).
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_artista(const SkipList* sl, const char* artista);

/** Busca todas as obras de um dado gênero, descendo pelos níveis
 *
 * Gênero é a chave primária, então o bloco é contíguo. Custo
 * O(log n + k) esperado, com k igual ao número de obras encontradas.
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_genero(const SkipList* sl, const char* genero);

/** Busca as obras de um artista dentro de um gênero
 *
 * Usa a chave composta completa, então o bloco é contíguo. Custo
 * O(log n + k) esperado. Esta é a consulta que a navegação usa no
 * último nível.
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* genero: gênero procurado
 * const char* artista: artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_genero_artista(const SkipList* sl,
					   const char* genero,
					   const char* artista);

#endif
