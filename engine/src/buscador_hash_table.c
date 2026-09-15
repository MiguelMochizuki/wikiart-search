/**
 * buscador_hash_table.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Adapta a HashTable à interface Buscador.
 */
#include "buscador.h"
#include "hash_table.h"

/* ==============================
 * Constantes
 * ============================== */

/* Capacidade fixa da tabela. Para 80k obras, fator de carga ~0.3,
 * o que mantém as cadeias curtas e a busca por artista em O(1)
 * esperado. 1 << 18 = 262.144 buckets (~2 MB de ponteiros). */
#define CAPACIDADE_HASH (1 << 18)

/* ==============================
 * Adaptadores
 * ============================== */

/** Cria uma HashTable com a capacidade fixa */
static void* ht_criar(void) {
	return hash_table_criar(CAPACIDADE_HASH);
}

/** Libera a HashTable */
static void ht_liberar(void* p) {
	hash_table_liberar((HashTable*) p);
}

/** Insere uma obra na HashTable */
static void ht_inserir(void* p, const Obra* o) {
	hash_table_inserir((HashTable*) p, o);
}

/** Busca por artista na HashTable */
static Resultado* ht_buscar_artista(const void* p, const char* artista) {
	return hash_table_buscar_artista((const HashTable*) p, artista);
}

/** Busca por gênero na HashTable */
static Resultado* ht_buscar_genero(const void* p, const char* genero) {
	return hash_table_buscar_genero((const HashTable*) p, genero);
}

/** Busca por gênero + artista */
static Resultado* ht_buscar_genero_artista(const void* p, const char* genero,
					const char* artista) {
	return hash_table_buscar_genero_artista((const HashTable*) p, genero, artista);
}

/* ==============================
 * Instância pública
 * ============================== */

const Buscador BUSCADOR_HASH_TABLE = {
	.nome           = "hash_table",
	.criar          = ht_criar,
	.liberar        = ht_liberar,
	.inserir        = ht_inserir,
	.buscar_artista = ht_buscar_artista,
	.buscar_genero  = ht_buscar_genero,
	.buscar_genero_artista = ht_buscar_genero_artista
};
