/**
 * test_obra.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD Obra.
 */
#include <string.h>
#include "munit.h"
#include "obra.h"

/* ==============================
 * Fixtures
 * ============================== */

static Obra* obra_exemplo(void) {
	return obra_criar(42,
			"the-starry-night",
			"vincent van gogh",
			"Post-Impressionism",
			1889,
			"Post_Impressionism/vincent-van-gogh_the-starry-night-1889.jpg");
}

/* ==============================
 * Testes
 * ============================== */

static MunitResult test_criar_campos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* o = obra_exemplo();
	munit_assert_not_null(o);

	munit_assert_int(obra_id(o), ==, 42);
	munit_assert_string_equal(obra_titulo(o),  "the-starry-night");
	munit_assert_string_equal(obra_artista(o), "vincent van gogh");
	munit_assert_string_equal(obra_genero(o),  "Post-Impressionism");
	munit_assert_int(obra_ano(o), ==, 1889);
	munit_assert_string_equal(obra_caminho(o),
		"Post_Impressionism/vincent-van-gogh_the-starry-night-1889.jpg");

	obra_liberar(o);
	return MUNIT_OK;
}

static MunitResult test_criar_copia_strings(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Se a Obra não copiar as strings, mexer no buffer original
	 * corrompe a Obra. Este teste garante a cópia. */
	char titulo[64] = "titulo-original";
	Obra* o = obra_criar(1, titulo, "artista", "genero", 2000, "path");

	strcpy(titulo, "MODIFICADO");

	munit_assert_string_equal(obra_titulo(o), "titulo-original");
	obra_liberar(o);
	return MUNIT_OK;
}

static MunitResult test_criar_ano_zero(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Muitas obras antigas têm ano desconhecido (0). */
	Obra* o = obra_criar(1, "obra", "autor", "genero", 0, "path");
	munit_assert_int(obra_ano(o), ==, 0);
	obra_liberar(o);
	return MUNIT_OK;
}

static MunitResult test_liberar_null(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Não deve crashar */
	obra_liberar(NULL);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_obra = {
	"/obra",
	(MunitTest[]) {
		{ "/criar-campos",         test_criar_campos,         NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/criar-copia-strings",  test_criar_copia_strings,  NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/criar-ano-zero",       test_criar_ano_zero,       NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/liberar-null",         test_liberar_null,         NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ NULL, NULL, NULL, 0, MUNIT_TEST_OPTION_NONE, NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_obra_get(void) {
	return &suite_obra;
}
