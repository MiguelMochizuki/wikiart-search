/**
 * hash_table.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD HashTable. Tabela hash com encadeamento,
 *            indexada pela chave composta (gênero, artista) via djb2.
 *
 * O gênero é a chave primária e define o bucket; o artista é a chave
 * secundária e ordena a cadeia dentro do bucket. Isso concentra o bloco
 * de um gênero em um único bucket e permite parada antecipada ao
 * procurar um artista dentro dele. Em troca, a busca só por artista
 * não tem bucket a consultar e degenera em varredura completa.
 *
 * Como só existem 27 gêneros distintos no corpus, poucos buckets ficam
 * ocupados e cada cadeia é longa. É uma escolha de indexação, não de
 * desempenho de hash: o bucket resolve o gênero em O(1), mas o custo
 * real fica no percurso da cadeia.
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

/** Busca todas as obras de um dado artista, via varredura completa
 *
 * Artista é a chave secundária: não determina o bucket, então não há
 * como evitar percorrer a tabela toda. A cadeia ordenada permite
 * abandonar cada bucket cedo, mas o custo continua O(n).
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_artista(const HashTable* ht, const char* artista);

/** Busca todas as obras de um dado gênero, pelo bucket do gênero
 *
 * Gênero é a chave de hash: chega ao bucket em O(1) esperado e o
 * percorre inteiro. Custo O(k), com k igual ao tamanho da cadeia.
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_genero(const HashTable* ht, const char* genero);

/** Busca as obras de um artista dentro de um gênero
 *
 * Usa as duas chaves: o gênero leva ao bucket em O(1) esperado e a
 * ordenação da cadeia por artista permite parar assim que o artista
 * corrente passa do procurado. Custo O(k) no tamanho da cadeia, com
 * metade dela percorrida em média. Esta é a consulta que a navegação
 * usa no último nível.
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* genero: gênero procurado
 * const char* artista: artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_genero_artista(const HashTable* ht,
					    const char* genero,
					    const char* artista);

#endif
