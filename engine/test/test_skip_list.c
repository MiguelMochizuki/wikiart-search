/**
 * test_skip_list.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD SkipList. Lista encadeada
 *            probabilística, ordenada por artista.
 */
#include <stdio.h>
#include <stdlib.h>
#include "munit.h"
#include "skip_list.h"
#include "obra.h"

/* ==============================
 * Helpers
 * ============================== */

/** Cria uma obra com os campos essenciais para a lista
 *
 * Parâmetros:
 * int id: ID da obra
 * const char* titulo: título
 * const char* artista: artista (chave de ordenação e busca)
 * const char* genero: gênero
 *
 * Retorna Obra*: ponteiro para a obra criada
 */
static Obra* obra_rapida(int id, const char* titulo,
			 const char* artista, const char* genero) {
	return obra_criar(id, titulo, artista, genero, 1900, "path");
}

/* ==============================
 * Fixture
 * ============================== */

typedef struct {
	Obra* van_gogh;
	Obra* picasso;
	Obra* klimt;
} Fixture;

static void* setup(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Fixture* f = malloc(sizeof *f);
	f->van_gogh = obra_rapida(1, "starry-night", "van gogh", "post-impressionism");
	f->picasso  = obra_rapida(2, "guernica",     "picasso",  "cubism");
	f->klimt    = obra_rapida(3, "the-kiss",     "klimt",    "symbolism");
	return f;
}

static void teardown(void* fixture) {
	Fixture* f = fixture;
	obra_liberar(f->van_gogh);
	obra_liberar(f->picasso);
	obra_liberar(f->klimt);
	free(f);
}

/* ==============================
 * Testes: criação e tamanho
 * ============================== */

static MunitResult test_criar_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	SkipList* sl = skip_list_criar();
	munit_assert_not_null(sl);
	munit_assert_int(skip_list_tamanho(sl), ==, 0);
	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_liberar_null(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	skip_list_liberar(NULL);
	return MUNIT_OK;
}

/* ==============================
 * Testes: inserção
 * ============================== */

static MunitResult test_inserir_um(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);

	munit_assert_int(skip_list_tamanho(sl), ==, 1);

	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_inserir_varios(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);
	skip_list_inserir(sl, f->picasso);
	skip_list_inserir(sl, f->klimt);

	munit_assert_int(skip_list_tamanho(sl), ==, 3);

	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_inserir_duplicado(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);
	skip_list_inserir(sl, f->van_gogh);

	munit_assert_int(skip_list_tamanho(sl), ==, 2);

	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_inserir_muitos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* 200 obras, artistas gerados, força vários níveis da skip list */
	SkipList* sl = skip_list_criar();

	Obra* obras[200];
	char nomes[200][16];
	for (int i = 0; i < 200; i++) {
		/* nomes tipo "a000", "a001", ..., "a199" para ordenar bem */
		snprintf(nomes[i], sizeof nomes[i], "a%03d", i);
		obras[i] = obra_rapida(i, "titulo", nomes[i], "genero");
		skip_list_inserir(sl, obras[i]);
	}

	munit_assert_int(skip_list_tamanho(sl), ==, 200);

	skip_list_liberar(sl);
	for (int i = 0; i < 200; i++) obra_liberar(obras[i]);
	return MUNIT_OK;
}

/* ==============================
 * Testes: busca por artista
 * ============================== */

static MunitResult test_buscar_artista_existente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);
	skip_list_inserir(sl, f->picasso);
	skip_list_inserir(sl, f->klimt);

	Resultado* r = skip_list_buscar_artista(sl, "picasso");
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), f->picasso);

	resultado_liberar(r);
	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);
	skip_list_inserir(sl, f->picasso);

	Resultado* r = skip_list_buscar_artista(sl, "monet");
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_multiplos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* a1 = obra_rapida(1, "obra-1", "picasso", "cubism");
	Obra* a2 = obra_rapida(2, "obra-2", "picasso", "cubism");
	Obra* a3 = obra_rapida(3, "obra-3", "monet",   "impressionism");

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, a1);
	skip_list_inserir(sl, a2);
	skip_list_inserir(sl, a3);

	Resultado* r = skip_list_buscar_artista(sl, "picasso");
	munit_assert_int(resultado_tamanho(r), ==, 2);

	resultado_liberar(r);
	skip_list_liberar(sl);
	obra_liberar(a1);
	obra_liberar(a2);
	obra_liberar(a3);
	return MUNIT_OK;
}

/* ==============================
 * Testes: busca por gênero
 * ============================== */

static MunitResult test_buscar_genero_existente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);
	skip_list_inserir(sl, f->picasso);
	skip_list_inserir(sl, f->klimt);

	Resultado* r = skip_list_buscar_genero(sl, "cubism");
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), f->picasso);

	resultado_liberar(r);
	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_buscar_genero_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);

	Resultado* r = skip_list_buscar_genero(sl, "barroco");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_buscar_genero_multiplos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* a1 = obra_rapida(1, "obra-1", "a", "cubism");
	Obra* a2 = obra_rapida(2, "obra-2", "b", "cubism");
	Obra* a3 = obra_rapida(3, "obra-3", "c", "cubism");

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, a1);
	skip_list_inserir(sl, a2);
	skip_list_inserir(sl, a3);

	Resultado* r = skip_list_buscar_genero(sl, "cubism");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	resultado_liberar(r);
	skip_list_liberar(sl);
	obra_liberar(a1);
	obra_liberar(a2);
	obra_liberar(a3);
	return MUNIT_OK;
}

/* ==============================
 * Testes: métricas
 * ============================== */

static MunitResult test_busca_conta_comparacoes(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	SkipList* sl = skip_list_criar();
	skip_list_inserir(sl, f->van_gogh);
	skip_list_inserir(sl, f->picasso);
	skip_list_inserir(sl, f->klimt);

	Resultado* r = skip_list_buscar_artista(sl, "picasso");
	munit_assert_long(resultado_comparacoes(r), >, 0);
	munit_assert_double(resultado_tempo_ms(r), >=, 0.0);

	resultado_liberar(r);
	skip_list_liberar(sl);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_skip_list = {
	"/skip-list",
	(MunitTest[]) {
		{ .name = "/criar-vazia",
		  .test = test_criar_vazia },
		{ .name = "/liberar-null",
		  .test = test_liberar_null },
		{ .name = "/inserir-um",
		  .test = test_inserir_um,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/inserir-varios",
		  .test = test_inserir_varios,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/inserir-duplicado",
		  .test = test_inserir_duplicado,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/inserir-muitos",
		  .test = test_inserir_muitos },
		{ .name = "/buscar-artista-existente",
		  .test = test_buscar_artista_existente,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/buscar-artista-inexistente",
		  .test = test_buscar_artista_inexistente,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/buscar-artista-multiplos",
		  .test = test_buscar_artista_multiplos },
		{ .name = "/buscar-genero-existente",
		  .test = test_buscar_genero_existente,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/buscar-genero-inexistente",
		  .test = test_buscar_genero_inexistente,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/buscar-genero-multiplos",
		  .test = test_buscar_genero_multiplos },
		{ .name = "/busca-conta-comparacoes",
		  .test = test_busca_conta_comparacoes,
		  .setup = setup, .tear_down = teardown },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_skip_list_get(void) {
	return &suite_skip_list;
}
