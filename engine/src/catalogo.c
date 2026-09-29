/**
 * catalogo.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação dos TADs Genero, Artista e Catalogo.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "catalogo.h"

/* ==============================
 * Constantes
 * ============================== */

/* Semente do embaralhamento da ordem de carga. Fixa para que servidor,
 * benchmark e testes montem sempre as mesmas estruturas. */
#define SEMENTE_CARGA 42u

/* ==============================
 * Implementação dos TADs
 * ============================== */

struct genero_t {
	const char* nome;
	int obras;
	int artistas;
	const Obra* capa;
};

struct artista_t {
	const char* genero;
	const char* nome;
	int obras;
	const Obra* capa;
};

struct catalogo_t {
	Genero* generos;
	Artista* artistas;
	int n_generos;
	int n_artistas;
	int n_obras;

	/* Ordem de carga: ponteiros embaralhados para os itens acima e
	 * para as obras do Csv. */
	const void** ordem_generos;
	const void** ordem_artistas;
	const void** ordem_obras;
};

/* ==============================
 * Helpers internos
 * ============================== */

/** Ordem (gênero, artista, id) entre obras, no formato do qsort
 *
 * Parâmetros:
 * const void* a: ponteiro para um elemento do array (const void*)
 * const void* b: idem
 *
 * Retorna int: <0, 0 ou >0
 */
static int cmp_obras(const void* a, const void* b) {
	const Obra* x = *(const void* const*) a;
	const Obra* y = *(const void* const*) b;

	int c = strcmp(obra_genero(x), obra_genero(y));
	if (c != 0) return c;
	c = strcmp(obra_artista(x), obra_artista(y));
	if (c != 0) return c;
	return (obra_id(x) > obra_id(y)) - (obra_id(x) < obra_id(y));
}

/** Diz se a obra i abre um bloco novo de gênero e de artista
 *
 * Parâmetros:
 * const void** obras: obras ordenadas por (gênero, artista, id)
 * int i: posição da obra
 * int* novo_genero: saída, 1 se a obra abre um gênero
 * int* novo_artista: saída, 1 se a obra abre um artista no gênero
 */
static void fronteiras(const void** obras, int i,
		       int* novo_genero, int* novo_artista) {
	if (i == 0) {
		*novo_genero = 1;
		*novo_artista = 1;
		return;
	}
	const Obra* ant = obras[i - 1];
	const Obra* cur = obras[i];
	*novo_genero  = strcmp(obra_genero(ant), obra_genero(cur)) != 0;
	*novo_artista = *novo_genero
		|| strcmp(obra_artista(ant), obra_artista(cur)) != 0;
}

/** Sorteia o próximo número do gerador (xorshift32)
 *
 * Gerador próprio, e não rand(): o benchmark reinicia o rand() com a
 * semente dele, e a ordem de carga não pode depender disso.
 *
 * Parâmetros:
 * uint32_t* estado: estado do gerador, nunca zero
 *
 * Retorna uint32_t: número sorteado
 */
static uint32_t sortear(uint32_t* estado) {
	uint32_t x = *estado;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*estado = x;
	return x;
}

/** Embaralha um array de ponteiros (Fisher-Yates)
 *
 * Parâmetros:
 * const void** v: array a embaralhar
 * int n: tamanho do array
 * uint32_t* estado: estado do gerador
 */
static void embaralhar(const void** v, int n, uint32_t* estado) {
	for (int i = n - 1; i > 0; i--) {
		int j = (int) (sortear(estado) % (uint32_t) (i + 1));
		const void* tmp = v[i];
		v[i] = v[j];
		v[j] = tmp;
	}
}

/* ==============================
 * API pública
 * ============================== */

/** Agrega as obras do CSV em gêneros e artistas por gênero
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * int embaralhar_ordem: 1 para a ordem de carga embaralhada, 0 para a
 *                       ordem das chaves
 *
 * Retorna Catalogo*: catálogo alocado, ou NULL em erro
 */
static Catalogo* montar(const Csv* csv, int embaralhar_ordem) {
	Catalogo* c = calloc(1, sizeof *c);
	if (!c) return NULL;

	int n = csv_tamanho(csv);
	c->n_obras = n;

	/* +1 nas alocações: com n = 0, malloc(0) pode devolver NULL, e
	 * aqui NULL quer dizer falha. */
	c->ordem_obras = malloc(((size_t) n + 1) * sizeof *c->ordem_obras);
	if (!c->ordem_obras) {
		catalogo_liberar(c);
		return NULL;
	}
	for (int i = 0; i < n; i++) c->ordem_obras[i] = csv_obra(csv, i);

	/* Com as obras em (gênero, artista, id), cada gênero e cada par
	 * viram blocos contíguos, e duas passadas bastam: uma conta, a
	 * outra preenche. */
	qsort((void*) c->ordem_obras, (size_t) n, sizeof *c->ordem_obras,
	      cmp_obras);

	for (int i = 0; i < n; i++) {
		int novo_genero, novo_artista;
		fronteiras(c->ordem_obras, i, &novo_genero, &novo_artista);
		c->n_generos  += novo_genero;
		c->n_artistas += novo_artista;
	}

	c->generos  = malloc(((size_t) c->n_generos + 1) * sizeof *c->generos);
	c->artistas = malloc(((size_t) c->n_artistas + 1) * sizeof *c->artistas);
	c->ordem_generos  = malloc(((size_t) c->n_generos + 1)
				   * sizeof *c->ordem_generos);
	c->ordem_artistas = malloc(((size_t) c->n_artistas + 1)
				   * sizeof *c->ordem_artistas);
	if (!c->generos || !c->artistas
	    || !c->ordem_generos || !c->ordem_artistas) {
		catalogo_liberar(c);
		return NULL;
	}

	Genero* g = NULL;
	Artista* a = NULL;
	int ig = 0;
	int ia = 0;
	int maior_bloco = 0;  /* obras do artista mais prolífico do gênero */

	for (int i = 0; i < n; i++) {
		const Obra* o = c->ordem_obras[i];
		int novo_genero, novo_artista;
		fronteiras(c->ordem_obras, i, &novo_genero, &novo_artista);

		if (novo_genero) {
			g = &c->generos[ig++];
			g->nome     = obra_genero(o);
			g->obras    = 0;
			g->artistas = 0;
			g->capa     = o;
			maior_bloco = 0;
		}
		if (novo_artista) {
			a = &c->artistas[ia++];
			a->genero = obra_genero(o);
			a->nome   = obra_artista(o);
			a->obras  = 0;
			a->capa   = o;
			g->artistas++;
		}

		a->obras++;
		g->obras++;

		/* Comparação estrita: no empate fica o artista que veio
		 * antes, o primeiro em ordem alfabética. */
		if (a->obras > maior_bloco) {
			maior_bloco = a->obras;
			g->capa = a->capa;
		}
	}

	for (int i = 0; i < c->n_generos; i++) {
		c->ordem_generos[i] = &c->generos[i];
	}
	for (int i = 0; i < c->n_artistas; i++) {
		c->ordem_artistas[i] = &c->artistas[i];
	}

	if (embaralhar_ordem) {
		uint32_t estado = SEMENTE_CARGA;
		embaralhar(c->ordem_generos,  c->n_generos,  &estado);
		embaralhar(c->ordem_artistas, c->n_artistas, &estado);
		embaralhar(c->ordem_obras,    c->n_obras,    &estado);
	}
	return c;
}

/** Monta o catálogo com a ordem de carga embaralhada
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 *
 * Retorna Catalogo*: catálogo alocado, ou NULL em erro
 */
Catalogo* catalogo_montar(const Csv* csv) {
	return montar(csv, 1);
}

/** Monta o catálogo com os níveis na ordem das chaves
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 *
 * Retorna Catalogo*: catálogo alocado, ou NULL em erro
 */
Catalogo* catalogo_montar_ordenado(const Csv* csv) {
	return montar(csv, 0);
}

/** Libera o catálogo e seus itens. Não libera as obras do Csv.
 *
 * Parâmetros:
 * Catalogo* c: ponteiro para o catálogo
 */
void catalogo_liberar(Catalogo* c) {
	if (!c) return;
	free(c->generos);
	free(c->artistas);
	free((void*) c->ordem_generos);
	free((void*) c->ordem_artistas);
	free((void*) c->ordem_obras);
	free(c);
}

/* ==============================
 * Acesso, na ordem de carga
 * ============================== */

int catalogo_n_generos(const Catalogo* c) {
	return c->n_generos;
}

const Genero* catalogo_genero(const Catalogo* c, int indice) {
	if (indice < 0 || indice >= c->n_generos) return NULL;
	return c->ordem_generos[indice];
}

int catalogo_n_artistas(const Catalogo* c) {
	return c->n_artistas;
}

const Artista* catalogo_artista(const Catalogo* c, int indice) {
	if (indice < 0 || indice >= c->n_artistas) return NULL;
	return c->ordem_artistas[indice];
}

int catalogo_n_obras(const Catalogo* c) {
	return c->n_obras;
}

const Obra* catalogo_obra(const Catalogo* c, int indice) {
	if (indice < 0 || indice >= c->n_obras) return NULL;
	return c->ordem_obras[indice];
}

/* ==============================
 * Getters
 * ============================== */

const char* genero_nome(const Genero* g) {
	return g->nome;
}

int genero_obras(const Genero* g) {
	return g->obras;
}

int genero_artistas(const Genero* g) {
	return g->artistas;
}

const Obra* genero_capa(const Genero* g) {
	return g->capa;
}

const char* artista_genero(const Artista* a) {
	return a->genero;
}

const char* artista_nome(const Artista* a) {
	return a->nome;
}

int artista_obras(const Artista* a) {
	return a->obras;
}

const Obra* artista_capa(const Artista* a) {
	return a->capa;
}
