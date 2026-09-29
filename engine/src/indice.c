/**
 * indice.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD Indice.
 */
#include <stdlib.h>
#include <string.h>
#include "indice.h"

/* ==============================
 * Implementação do TAD
 * ============================== */

struct indice_t {
	const Buscador* ed;
	void* niveis[N_NIVEIS];
};

/* ==============================
 * Comparação por chave
 * ============================== */

/** Compara os campos de um item com uma chave, do mais grosso ao fino
 *
 * Para no primeiro campo em aberto da chave: dali em diante, qualquer
 * valor está dentro do intervalo.
 *
 * Parâmetros:
 * const char* genero: gênero do item
 * const char* artista: artista do item (não lido se a chave parar antes)
 * int id: id do item (idem)
 * const Chave* k: chave comparada
 *
 * Retorna int: <0 antes do intervalo, 0 dentro, >0 depois
 */
static int cmp_campos(const char* genero, const char* artista, int id,
		      const Chave* k) {
	if (!k->genero) return 0;
	int c = strcmp(genero, k->genero);
	if (c != 0 || !k->artista) return c;
	c = strcmp(artista, k->artista);
	if (c != 0 || k->id < 0) return c;
	return (id > k->id) - (id < k->id);
}

/** Compara um Genero com uma Chave (só o gênero conta) */
int indice_cmp_genero(const void* item, const void* chave) {
	const Chave* k = chave;
	Chave so_genero = { k->genero, NULL, -1 };
	return cmp_campos(genero_nome(item), NULL, -1, &so_genero);
}

/** Compara um Artista com uma Chave (gênero e artista contam) */
int indice_cmp_artista(const void* item, const void* chave) {
	const Chave* k = chave;
	Chave sem_id = { k->genero, k->artista, -1 };
	return cmp_campos(artista_genero(item), artista_nome(item), -1,
			  &sem_id);
}

/** Compara uma Obra com uma Chave (os três campos contam) */
int indice_cmp_obra(const void* item, const void* chave) {
	return cmp_campos(obra_genero(item), obra_artista(item),
			  obra_id(item), chave);
}

/* ==============================
 * Ordem entre itens
 * ============================== */

/* Na inserção, a ED compara item com item. Basta tirar a chave completa
 * do segundo e reaproveitar o comparador de chave do nível. */

static int cmp_itens_genero(const void* a, const void* b) {
	Chave k = { genero_nome(b), NULL, -1 };
	return indice_cmp_genero(a, &k);
}

static int cmp_itens_artista(const void* a, const void* b) {
	Chave k = { artista_genero(b), artista_nome(b), -1 };
	return indice_cmp_artista(a, &k);
}

static int cmp_itens_obra(const void* a, const void* b) {
	Chave k = { obra_genero(b), obra_artista(b), obra_id(b) };
	return indice_cmp_obra(a, &k);
}

static const Comparador CMP_CHAVE[N_NIVEIS] = {
	indice_cmp_genero,
	indice_cmp_artista,
	indice_cmp_obra
};

static const Comparador CMP_ITENS[N_NIVEIS] = {
	cmp_itens_genero,
	cmp_itens_artista,
	cmp_itens_obra
};

/* ==============================
 * API pública
 * ============================== */

/** Cria as três estruturas de uma ED e carrega o catálogo nelas
 *
 * Parâmetros:
 * const Buscador* ed: ED a usar nos três níveis
 * const Catalogo* c: itens a carregar, na ordem de carga do catálogo
 *
 * Retorna Indice*: índice montado, ou NULL em erro
 */
Indice* indice_montar(const Buscador* ed, const Catalogo* c) {
	Indice* ix = calloc(1, sizeof *ix);
	if (!ix) return NULL;

	ix->ed = ed;
	for (int n = 0; n < N_NIVEIS; n++) {
		ix->niveis[n] = ed->criar(CMP_ITENS[n]);
		if (!ix->niveis[n]) {
			indice_liberar(ix);
			return NULL;
		}
	}

	for (int i = 0; i < catalogo_n_generos(c); i++) {
		ed->inserir(ix->niveis[NIVEL_GENEROS], catalogo_genero(c, i));
	}
	for (int i = 0; i < catalogo_n_artistas(c); i++) {
		ed->inserir(ix->niveis[NIVEL_ARTISTAS], catalogo_artista(c, i));
	}
	for (int i = 0; i < catalogo_n_obras(c); i++) {
		ed->inserir(ix->niveis[NIVEL_OBRAS], catalogo_obra(c, i));
	}
	return ix;
}

/** Libera as estruturas do índice. Não libera os itens.
 *
 * Parâmetros:
 * Indice* ix: ponteiro para o índice
 */
void indice_liberar(Indice* ix) {
	if (!ix) return;
	for (int n = 0; n < N_NIVEIS; n++) {
		if (ix->niveis[n]) ix->ed->liberar(ix->niveis[n]);
	}
	free(ix);
}

/** ED usada pelo índice */
const Buscador* indice_ed(const Indice* ix) {
	return ix->ed;
}

/** Busca o intervalo da chave num nível
 *
 * Parâmetros:
 * Indice* ix: ponteiro para o índice
 * Nivel nivel: nível consultado
 * const Chave* chave: chave procurada, ou NULL para listar o nível todo
 *
 * Retorna Resultado*: itens do nível, ou NULL se o nível é inválido
 */
Resultado* indice_buscar(Indice* ix, Nivel nivel, const Chave* chave) {
	if ((unsigned) nivel >= N_NIVEIS) return NULL;
	return ix->ed->buscar(ix->niveis[nivel],
			      chave ? CMP_CHAVE[nivel] : NULL, chave);
}

/** Busca uma página do intervalo da chave num nível
 *
 * Parâmetros:
 * Indice* ix: ponteiro para o índice
 * Nivel nivel: nível consultado
 * const Chave* chave: chave procurada, ou NULL para o nível todo
 * int offset: itens do intervalo a pular
 * int limite: máximo de itens a devolver
 *
 * Retorna Resultado*: a página, com o total em resultado_total, ou NULL
 *                     se o nível é inválido
 */
Resultado* indice_buscar_pagina(Indice* ix, Nivel nivel, const Chave* chave,
				int offset, int limite) {
	if ((unsigned) nivel >= N_NIVEIS) return NULL;
	return ix->ed->buscar_pagina(ix->niveis[nivel],
				     chave ? CMP_CHAVE[nivel] : NULL, chave,
				     offset, limite);
}

/** Recorta a forma da ED num nível, restrita ao intervalo da chave
 *
 * Parâmetros:
 * const Indice* ix: ponteiro para o índice
 * Nivel nivel: nível consultado
 * const Chave* chave: intervalo a mostrar, ou NULL para o nível todo
 * NoVista vista[]: saída com 2^niveis - 1 posições
 * int niveis: quantos níveis recortar
 *
 * Retorna int: total de itens no intervalo, ou -1 sem forma hierárquica
 */
int indice_vista(const Indice* ix, Nivel nivel, const Chave* chave,
		 NoVista vista[], int niveis) {
	if ((unsigned) nivel >= N_NIVEIS || !ix->ed->vista) return -1;
	return ix->ed->vista(ix->niveis[nivel],
			     chave ? CMP_CHAVE[nivel] : NULL, chave,
			     vista, niveis);
}
