/**
 * test_resultado.c
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD Resultado.
 */
#include "munit.h"
#include "resultado.h"
#include "obra.h"

static Obra* o1 = NULL;
static Obra* o2 = NULL;

static void* setup(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;
	o1 = obra_criar(1, "a", "artista-a", "g", 1900, "p1");
	o2 = obra_criar(2, "b", "artista-b", "g", 1901, "p2");
	return NULL;
}

static void teardown(void* fixture) {
	(void) fixture;
	obra_liberar(o1);
	obra_liberar(o2);
	o1 = NULL;
	o2 = NULL;
}

/* ==============================
 * Testes
 * ============================== */

static MunitResult test_criar_vazio(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Resultado* r = resultado_criar(4);
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);
	resultado_liberar(r);
	return MUNIT_OK;
}

static MunitResult test_adicionar(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Resultado* r = resultado_criar(2);
	resultado_adicionar(r, o1);
	resultado_adicionar(r, o2);

	munit_assert_int(resultado_tamanho(r), ==, 2);
	munit_assert_ptr_equal(resultado_item(r, 0), o1);
	munit_assert_ptr_equal(resultado_item(r, 1), o2);

	resultado_liberar(r);
	return MUNIT_OK;
}

static MunitResult test_adicionar_cresce(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Capacidade inicial 1, adiciona 5. Força realloc. */
	Resultado* r = resultado_criar(1);
	for (int i = 0; i < 5; i++) resultado_adicionar(r, o1);

	munit_assert_int(resultado_tamanho(r), ==, 5);
	resultado_liberar(r);
	return MUNIT_OK;
}

static MunitResult test_item_fora_do_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Resultado* r = resultado_criar(2);
	resultado_adicionar(r, o1);

	munit_assert_null(resultado_item(r, -1));
	munit_assert_null(resultado_item(r, 1));
	munit_assert_null(resultado_item(r, 999));

	resultado_liberar(r);
	return MUNIT_OK;
}

static MunitResult test_metricas(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Resultado* r = resultado_criar(1);
	resultado_set_metricas(r, 12.5, 1000);

	munit_assert_double(resultado_tempo_ms(r), ==, 12.5);
	munit_assert_long(resultado_comparacoes(r), ==, 1000);

	resultado_liberar(r);
	return MUNIT_OK;
}

static MunitResult test_rotacoes(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Começa em 0: só as EDs que giram nós definem o valor. */
	Resultado* r = resultado_criar(1);
	munit_assert_long(resultado_rotacoes(r), ==, 0);

	resultado_set_rotacoes(r, 42);
	munit_assert_long(resultado_rotacoes(r), ==, 42);

	resultado_liberar(r);
	return MUNIT_OK;
}

static MunitResult test_capacidade_invalida(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Capacidade 0 vira 1 (não crasha). */
	Resultado* r = resultado_criar(0);
	munit_assert_not_null(r);
	resultado_adicionar(r, o1);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	resultado_liberar(r);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_resultado = {
	"/resultado",
	(MunitTest[]) {
		{ "/criar-vazio",             test_criar_vazio,             setup, teardown, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/adicionar",               test_adicionar,               setup, teardown, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/adicionar-cresce",        test_adicionar_cresce,        setup, teardown, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/item-fora-do-intervalo",  test_item_fora_do_intervalo,  setup, teardown, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/metricas",                test_metricas,                setup, teardown, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/rotacoes",                test_rotacoes,                setup, teardown, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/capacidade-invalida",     test_capacidade_invalida,     setup, teardown, MUNIT_TEST_OPTION_NONE, NULL },
		{ NULL, NULL, NULL, 0, MUNIT_TEST_OPTION_NONE, NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_resultado_get(void) {
	return &suite_resultado;
}
