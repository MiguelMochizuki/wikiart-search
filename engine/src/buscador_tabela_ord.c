/**
 * buscador_tabela_ord.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Adapta a TabelaOrd à interface Buscador.
 */
#include "buscador.h"
#include "tabela_ord.h"

/* ==============================
 * Adaptadores
 * ============================== */

/** Cria uma TabelaOrd vazia */
static void* to_criar(void) {
	return tabela_ord_criar();
}

/** Libera a TabelaOrd */
static void to_liberar(void* p) {
	tabela_ord_liberar((TabelaOrd*) p);
}

/** Insere uma obra na TabelaOrd */
static void to_inserir(void* p, const Obra* o) {
	tabela_ord_inserir((TabelaOrd*) p, o);
}

/** Busca por artista na TabelaOrd */
static Resultado* to_buscar_artista(const void* p, const char* artista) {
	return tabela_ord_buscar_artista((const TabelaOrd*) p, artista);
}

/** Busca por gênero na TabelaOrd */
static Resultado* to_buscar_genero(const void* p, const char* genero) {
	return tabela_ord_buscar_genero((const TabelaOrd*) p, genero);
}

/** Busca por gênero + artista */
static Resultado* to_buscar_genero_artista(const void* p, const char* genero,
					const char* artista) {
	return tabela_ord_buscar_genero_artista((const TabelaOrd*) p, genero, artista);
}

/* ==============================
 * Instância pública
 * ============================== */

const Buscador BUSCADOR_TABELA_ORD = {
	.nome           = "tabela_ord",
	.criar          = to_criar,
	.liberar        = to_liberar,
	.inserir        = to_inserir,
	.buscar_artista = to_buscar_artista,
	.buscar_genero  = to_buscar_genero,
	.buscar_genero_artista = to_buscar_genero_artista
};
