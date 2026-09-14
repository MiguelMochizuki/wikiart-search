/**
 * test_csv.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD Csv.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "munit.h"
#include "csv.h"

/* Escreve um CSV temporário e devolve o caminho (estático). */
static const char* escrever_csv(const char* conteudo) {
	static char caminho[] = "/tmp/test_wikiart_XXXXXX";

	int fd = mkstemp(caminho);
	if (fd < 0) {
		perror("mkstemp");
		exit(1);
	}

	FILE* f = fdopen(fd, "w");
	if (!f) {
		perror("fdopen");
		close(fd);
		exit(1);
	}

	if (fputs(conteudo, f) == EOF) {
		perror("fputs");
		fclose(f);
		exit(1);
	}

	fclose(f);
	return caminho;
}

/* ==============================
 * Testes
 * ============================== */

static MunitResult test_carregar_vazio(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	const char* p = escrever_csv("id;titulo;artista;genero;ano;caminho\n");
	Csv* c = csv_carregar(p);
	munit_assert_not_null(c);
	munit_assert_int(csv_tamanho(c), ==, 0);
	csv_liberar(c);
	return MUNIT_OK;
}

static MunitResult test_carregar_uma(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	const char* p = escrever_csv(
		"id;titulo;artista;genero;ano;caminho\n"
		"1;starry;van gogh;post-impressionism;1889;path.jpg\n");

	Csv* c = csv_carregar(p);
	munit_assert_int(csv_tamanho(c), ==, 1);

	const Obra* o = csv_obra(c, 0);
	munit_assert_int(obra_id(o), ==, 1);
	munit_assert_string_equal(obra_titulo(o),  "starry");
	munit_assert_string_equal(obra_artista(o), "van gogh");
	munit_assert_string_equal(obra_genero(o),  "post-impressionism");
	munit_assert_int(obra_ano(o), ==, 1889);
	munit_assert_string_equal(obra_caminho(o), "path.jpg");

	csv_liberar(c);
	return MUNIT_OK;
}

static MunitResult test_carregar_varias(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	const char* p = escrever_csv(
		"id;titulo;artista;genero;ano;caminho\n"
		"1;a;autor-1;g;1900;p1\n"
		"2;b;autor-2;g;1901;p2\n"
		"3;c;autor-3;g;1902;p3\n");

	Csv* c = csv_carregar(p);
	munit_assert_int(csv_tamanho(c), ==, 3);
	munit_assert_string_equal(obra_titulo(csv_obra(c, 0)), "a");
	munit_assert_string_equal(obra_titulo(csv_obra(c, 2)), "c");
	csv_liberar(c);
	return MUNIT_OK;
}

static MunitResult test_ignora_malformada(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	const char* p = escrever_csv(
		"id;titulo;artista;genero;ano;caminho\n"
		"1;a;autor;g;1900;p\n"
		"2;b;so-tres-campos\n"
		"3;c;autor;g;1903;p\n");

	Csv* c = csv_carregar(p);
	munit_assert_int(csv_tamanho(c), ==, 2);
	csv_liberar(c);
	return MUNIT_OK;
}

static MunitResult test_caminho_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Csv* c = csv_carregar("/tmp/nao-existe-xyz.csv");
	munit_assert_null(c);
	return MUNIT_OK;
}

static MunitResult test_obra_fora_do_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	const char* p = escrever_csv(
		"id;titulo;artista;genero;ano;caminho\n"
		"1;a;autor;g;1900;p\n");

	Csv* c = csv_carregar(p);
	munit_assert_null(csv_obra(c, -1));
	munit_assert_null(csv_obra(c, 1));
	csv_liberar(c);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_csv = {
	"/csv",
	(MunitTest[]) {
		{ "/carregar-vazio",           test_carregar_vazio,          NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/carregar-uma",             test_carregar_uma,            NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/carregar-varias",          test_carregar_varias,         NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/ignora-malformada",        test_ignora_malformada,       NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/caminho-inexistente",      test_caminho_inexistente,     NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ "/obra-fora-do-intervalo",   test_obra_fora_do_intervalo,  NULL, 0, MUNIT_TEST_OPTION_NONE, NULL },
		{ NULL, NULL, NULL, 0, MUNIT_TEST_OPTION_NONE, NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_csv_get(void) {
	return &suite_csv;
}
