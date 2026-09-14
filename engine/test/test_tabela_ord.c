/**
 * test_tabela_ord.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD TabelaOrd. Lista indexada
 *            ordenada por artista, com busca binária na chave.
 */
#include <stdio.h>
#include <stdlib.h>
#include "munit.h"
#include "tabela_ord.h"
#include "obra.h"

/* ==============================
 * Helpers
 * ============================== */

/** Cria uma obra com os campos essenciais para a tabela
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

	TabelaOrd* t = tabela_ord_criar();
	munit_assert_not_null(t);
	munit_assert_int(tabela_ord_tamanho(t), ==, 0);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_liberar_null(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	tabela_ord_liberar(NULL);
	return MUNIT_OK;
}

/* ==============================
 * Testes: inserção e ordenação
 * ============================== */

static MunitResult test_inserir_um(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);

	munit_assert_int(tabela_ord_tamanho(t), ==, 1);
	munit_assert_ptr_equal(tabela_ord_item(t, 0), f->van_gogh);

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_ordem_mantida_apos_insercoes(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Insere fora de ordem: picasso, van gogh, klimt.
	 * A ordem lexicográfica é: klimt, picasso, van gogh. */
	Obra* picasso  = obra_rapida(1, "g", "picasso",  "cubism");
	Obra* van_gogh = obra_rapida(2, "s", "van gogh", "post-impressionism");
	Obra* klimt    = obra_rapida(3, "k", "klimt",    "symbolism");

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, picasso);
	tabela_ord_inserir(t, van_gogh);
	tabela_ord_inserir(t, klimt);

	munit_assert_int(tabela_ord_tamanho(t), ==, 3);
	munit_assert_ptr_equal(tabela_ord_item(t, 0), klimt);
	munit_assert_ptr_equal(tabela_ord_item(t, 1), picasso);
	munit_assert_ptr_equal(tabela_ord_item(t, 2), van_gogh);

	tabela_ord_liberar(t);
	obra_liberar(picasso);
	obra_liberar(van_gogh);
	obra_liberar(klimt);
	return MUNIT_OK;
}

static MunitResult test_insercao_no_meio(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Insere A, C, B. A ordem final deve ser A, B, C. */
	Obra* a = obra_rapida(1, "a", "aaa", "g");
	Obra* b = obra_rapida(2, "b", "bbb", "g");
	Obra* c = obra_rapida(3, "c", "ccc", "g");

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, a);
	tabela_ord_inserir(t, c);
	tabela_ord_inserir(t, b);   /* entra no meio */

	munit_assert_int(tabela_ord_tamanho(t), ==, 3);
	munit_assert_ptr_equal(tabela_ord_item(t, 0), a);
	munit_assert_ptr_equal(tabela_ord_item(t, 1), b);
	munit_assert_ptr_equal(tabela_ord_item(t, 2), c);

	tabela_ord_liberar(t);
	obra_liberar(a);
	obra_liberar(b);
	obra_liberar(c);
	return MUNIT_OK;
}

static MunitResult test_inserir_duplicado(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);
	tabela_ord_inserir(t, f->van_gogh);

	munit_assert_int(tabela_ord_tamanho(t), ==, 2);
	munit_assert_ptr_equal(tabela_ord_item(t, 0), f->van_gogh);
	munit_assert_ptr_equal(tabela_ord_item(t, 1), f->van_gogh);

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_inserir_muitos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	TabelaOrd* t = tabela_ord_criar();

	Obra* obras[200];
	char nomes[200][16];
	for (int i = 0; i < 200; i++) {
		snprintf(nomes[i], sizeof nomes[i], "a%03d", i);
		obras[i] = obra_rapida(i, "titulo", nomes[i], "genero");
		tabela_ord_inserir(t, obras[i]);
	}

	munit_assert_int(tabela_ord_tamanho(t), ==, 200);

	/* Verifica ordenação completa */
	for (int i = 1; i < 200; i++) {
		const char* a = obra_artista(tabela_ord_item(t, i - 1));
		const char* b = obra_artista(tabela_ord_item(t, i));
		munit_assert_true(strcmp(a, b) <= 0);
	}

	tabela_ord_liberar(t);
	for (int i = 0; i < 200; i++) obra_liberar(obras[i]);
	return MUNIT_OK;
}

/* ==============================
 * Testes: busca por artista
 * ============================== */

static MunitResult test_buscar_artista_existente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);
	tabela_ord_inserir(t, f->picasso);
	tabela_ord_inserir(t, f->klimt);

	Resultado* r = tabela_ord_buscar_artista(t, "picasso");
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), f->picasso);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);
	tabela_ord_inserir(t, f->picasso);

	Resultado* r = tabela_ord_buscar_artista(t, "monet");
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_em_array_vazio(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	TabelaOrd* t = tabela_ord_criar();

	Resultado* r = tabela_ord_buscar_artista(t, "qualquer");
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_multiplos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* a1 = obra_rapida(1, "obra-1", "picasso", "cubism");
	Obra* a2 = obra_rapida(2, "obra-2", "picasso", "cubism");
	Obra* a3 = obra_rapida(3, "obra-3", "monet",   "impressionism");

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, a1);
	tabela_ord_inserir(t, a2);
	tabela_ord_inserir(t, a3);

	Resultado* r = tabela_ord_buscar_artista(t, "picasso");
	munit_assert_int(resultado_tamanho(r), ==, 2);

	resultado_liberar(r);
	tabela_ord_liberar(t);
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

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);
	tabela_ord_inserir(t, f->picasso);
	tabela_ord_inserir(t, f->klimt);

	Resultado* r = tabela_ord_buscar_genero(t, "cubism");
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), f->picasso);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_genero_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);

	Resultado* r = tabela_ord_buscar_genero(t, "barroco");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_genero_multiplos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* a1 = obra_rapida(1, "obra-1", "a", "cubism");
	Obra* a2 = obra_rapida(2, "obra-2", "b", "cubism");
	Obra* a3 = obra_rapida(3, "obra-3", "c", "cubism");

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, a1);
	tabela_ord_inserir(t, a2);
	tabela_ord_inserir(t, a3);

	Resultado* r = tabela_ord_buscar_genero(t, "cubism");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	resultado_liberar(r);
	tabela_ord_liberar(t);
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

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);
	tabela_ord_inserir(t, f->picasso);
	tabela_ord_inserir(t, f->klimt);

	Resultado* r = tabela_ord_buscar_artista(t, "picasso");
	munit_assert_long(resultado_comparacoes(r), >, 0);
	munit_assert_double(resultado_tempo_ms(r), >=, 0.0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_item_fora_do_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, f->van_gogh);

	munit_assert_null(tabela_ord_item(t, -1));
	munit_assert_null(tabela_ord_item(t, 1));
	munit_assert_null(tabela_ord_item(t, 999));

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_tabela_ord = {
	"/tabela-ord",
	(MunitTest[]) {
		{ .name = "/criar-vazia",
		  .test = test_criar_vazia },
		{ .name = "/liberar-null",
		  .test = test_liberar_null },
		{ .name = "/inserir-um",
		  .test = test_inserir_um,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/ordem-mantida-apos-insercoes",
		  .test = test_ordem_mantida_apos_insercoes },
		{ .name = "/insercao-no-meio",
		  .test = test_insercao_no_meio },
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
		{ .name = "/buscar-artista-em-array-vazio",
		  .test = test_buscar_artista_em_array_vazio },
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
		{ .name = "/item-fora-do-intervalo",
		  .test = test_item_fora_do_intervalo,
		  .setup = setup, .tear_down = teardown },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_tabela_ord_get(void) {
	return &suite_tabela_ord;
}
