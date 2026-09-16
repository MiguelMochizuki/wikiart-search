/**
 * buscador_skip_list.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Adapta a SkipList à interface Buscador.
 */
#include "buscador.h"
#include "skip_list.h"

/* ==============================
 * Adaptadores
 * ============================== */

/** Cria uma SkipList vazia */
static void* sl_criar(void) {
	return skip_list_criar();
}

/** Libera a SkipList */
static void sl_liberar(void* p) {
	skip_list_liberar((SkipList*) p);
}

/** Insere uma obra na SkipList */
static void sl_inserir(void* p, const Obra* o) {
	skip_list_inserir((SkipList*) p, o);
}

/** Busca por artista na SkipList */
static Resultado* sl_buscar_artista(const void* p, const char* artista) {
	return skip_list_buscar_artista((const SkipList*) p, artista);
}

/** Busca por gênero na SkipList */
static Resultado* sl_buscar_genero(const void* p, const char* genero) {
	return skip_list_buscar_genero((const SkipList*) p, genero);
}

/** Busca por gênero + artista */
static Resultado* sl_buscar_genero_artista(const void* p, const char* genero,
					const char* artista) {
	return skip_list_buscar_genero_artista((const SkipList*) p, genero, artista);
}

/* ==============================
 * Instância pública
 * ============================== */

const Buscador BUSCADOR_SKIP_LIST = {
	.nome           = "skip_list",
	.criar          = sl_criar,
	.liberar        = sl_liberar,
	.inserir        = sl_inserir,
	.buscar_artista = sl_buscar_artista,
	.buscar_genero  = sl_buscar_genero,
	.buscar_genero_artista = sl_buscar_genero_artista
};
