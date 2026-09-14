/**
 * hash_table.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD HashTable. Tabela hash com encadeamento,
 *            indexada por artista (chave de hash) via djb2.
 */
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "obra.h"
#include "resultado.h"

/* ==============================
 * TADs
 * ============================== */

typedef struct hash_table_t HashTable;

/* ==============================
 * API pública
 * ============================== */

/** Cria uma tabela hash com a capacidade dada
 *
 * Parâmetros:
 * int capacidade: número de buckets (mínimo 1)
 *
 * Retorna HashTable*: ponteiro para a tabela alocada, ou NULL em erro
 */
HashTable* hash_table_criar(int capacidade);

/** Libera a tabela e todos os nós internos. Não libera as Obras.
 *
 * Parâmetros:
 * HashTable* ht: ponteiro para a tabela a ser liberada
 */
void hash_table_liberar(HashTable* ht);

/** Insere uma obra na tabela, no bucket determinado pelo hash do artista
 *
 * Parâmetros:
 * HashTable* ht: ponteiro para a tabela
 * const Obra* o: ponteiro para a obra (não copiada, só referenciada)
 */
void hash_table_inserir(HashTable* ht, const Obra* o);

/* ==============================
 * Consultas
 * ============================== */

/** Número de obras na tabela
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 *
 * Retorna int: quantidade de elementos
 */
int hash_table_tamanho(const HashTable* ht);

/** Busca todas as obras de um dado artista
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_artista(const HashTable* ht, const char* artista);

/** Busca todas as obras de um dado gênero
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_genero(const HashTable* ht, const char* genero);

#endif
