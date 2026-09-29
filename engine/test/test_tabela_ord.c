/**
 * test_tabela_ord.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD TabelaOrd. Lista indexada genérica,
 *            testada com inteiros para isolar a estrutura da ordem dos
 *            níveis (que o test_indice cobre).
 */
#include <stdio.h>
#include <stdlib.h>
#include "munit.h"
#include "tabela_ord.h"

/* ==============================
 * Helpers
 * ============================== */

/** Ordem natural entre inteiros (item e chave apontam para int) */
static int cmp_int(const void* item, const void* chave) {
	int a = *(const int*) item;
	int b = *(const int*) chave;
	return (a > b) - (a < b);
}

/** Intervalo fechado de inteiros usado como chave de busca */
typedef struct {
	int min;
	int max;
} Faixa;

/** Situa um inteiro em relação a uma Faixa */
static int cmp_faixa(const void* item, const void* chave) {
	int v = *(const int*) item;
	const Faixa* f = chave;
	if (v < f->min) return -1;
	if (v > f->max) return 1;
	return 0;
}

/** Valor do inteiro apontado por um item */
static int valor(const void* item) {
	return *(const int*) item;
}

/** Monta uma tabela com os valores dados, na ordem do array */
static TabelaOrd* tabela_com(const int* valores, int n) {
	TabelaOrd* t = tabela_ord_criar(cmp_int);
	for (int i = 0; i < n; i++) tabela_ord_inserir(t, &valores[i]);
	return t;
}

/* ==============================
 * Testes: criação e tamanho
 * ============================== */

static MunitResult test_criar_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	TabelaOrd* t = tabela_ord_criar(cmp_int);
	munit_assert_not_null(t);
	munit_assert_int(tabela_ord_tamanho(t), ==, 0);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_criar_sem_comparador(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	munit_assert_null(tabela_ord_criar(NULL));
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
	(void) data;

	int v = 7;
	TabelaOrd* t = tabela_ord_criar(cmp_int);
	tabela_ord_inserir(t, &v);

	munit_assert_int(tabela_ord_tamanho(t), ==, 1);
	munit_assert_ptr_equal(tabela_ord_item(t, 0), &v);

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_ordem_mantida_apos_insercoes(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 5, 1, 4, 2, 3 };
	TabelaOrd* t = tabela_com(valores, 5);

	munit_assert_int(tabela_ord_tamanho(t), ==, 5);
	for (int i = 0; i < 5; i++) {
		munit_assert_int(valor(tabela_ord_item(t, i)), ==, i + 1);
	}

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_insercao_no_meio(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Insere 1, 3, 2. O 2 entra entre os dois e desloca o 3. */
	int valores[] = { 1, 3, 2 };
	TabelaOrd* t = tabela_com(valores, 3);

	munit_assert_ptr_equal(tabela_ord_item(t, 0), &valores[0]);
	munit_assert_ptr_equal(tabela_ord_item(t, 1), &valores[2]);
	munit_assert_ptr_equal(tabela_ord_item(t, 2), &valores[1]);

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_inserir_duplicado(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int v = 9;
	TabelaOrd* t = tabela_ord_criar(cmp_int);
	tabela_ord_inserir(t, &v);
	tabela_ord_inserir(t, &v);

	munit_assert_int(tabela_ord_tamanho(t), ==, 2);
	munit_assert_ptr_equal(tabela_ord_item(t, 0), &v);
	munit_assert_ptr_equal(tabela_ord_item(t, 1), &v);

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_inserir_muitos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Passa da capacidade inicial (1024) e força o realloc. Os valores
	 * saem embaralhados de uma progressão modular. */
	enum { N = 3000 };
	static int valores[N];
	for (int i = 0; i < N; i++) valores[i] = (i * 7919) % N;

	TabelaOrd* t = tabela_com(valores, N);
	munit_assert_int(tabela_ord_tamanho(t), ==, N);
	for (int i = 0; i < N; i++) {
		munit_assert_int(valor(tabela_ord_item(t, i)), ==, i);
	}

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_item_fora_do_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int v = 1;
	TabelaOrd* t = tabela_com(&v, 1);

	munit_assert_null(tabela_ord_item(t, -1));
	munit_assert_null(tabela_ord_item(t, 1));
	munit_assert_null(tabela_ord_item(t, 999));

	tabela_ord_liberar(t);
	return MUNIT_OK;
}

/* ==============================
 * Testes: busca
 * ============================== */

static MunitResult test_buscar_exato(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 4, 8, 15, 16, 23, 42 };
	TabelaOrd* t = tabela_com(valores, 6);

	int chave = 16;
	Resultado* r = tabela_ord_buscar(t, cmp_int, &chave);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), &valores[3]);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 4, 8, 15 };
	TabelaOrd* t = tabela_com(valores, 3);

	int chave = 10;
	Resultado* r = tabela_ord_buscar(t, cmp_int, &chave);
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_em_tabela_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	TabelaOrd* t = tabela_ord_criar(cmp_int);
	int chave = 1;
	Resultado* r = tabela_ord_buscar(t, cmp_int, &chave);

	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 9, 2, 7, 4, 1, 6, 3, 8, 5, 10 };
	TabelaOrd* t = tabela_com(valores, 10);

	Faixa f = { 3, 6 };
	Resultado* r = tabela_ord_buscar(t, cmp_faixa, &f);

	/* O intervalo sai inteiro e em ordem: 3, 4, 5, 6. */
	munit_assert_int(resultado_tamanho(r), ==, 4);
	for (int i = 0; i < 4; i++) {
		munit_assert_int(valor(resultado_item(r, i)), ==, 3 + i);
	}

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_intervalo_com_repetidos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 5, 2, 5, 9, 5, 1 };
	TabelaOrd* t = tabela_com(valores, 6);

	int chave = 5;
	Resultado* r = tabela_ord_buscar(t, cmp_int, &chave);
	munit_assert_int(resultado_tamanho(r), ==, 3);
	for (int i = 0; i < 3; i++) {
		munit_assert_int(valor(resultado_item(r, i)), ==, 5);
	}

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_buscar_sem_chave_lista_tudo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 3, 1, 2 };
	TabelaOrd* t = tabela_com(valores, 3);

	/* Sem comparador não há o que buscar: vem tudo, em ordem, e sem
	 * nenhuma comparação. */
	Resultado* r = tabela_ord_buscar(t, NULL, NULL);
	munit_assert_int(resultado_tamanho(r), ==, 3);
	for (int i = 0; i < 3; i++) {
		munit_assert_int(valor(resultado_item(r, i)), ==, i + 1);
	}
	munit_assert_long(resultado_comparacoes(r), ==, 0);
	munit_assert_long(resultado_rotacoes(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_busca_binaria_conta_comparacoes(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	enum { N = 1024 };
	static int valores[N];
	for (int i = 0; i < N; i++) valores[i] = i;
	TabelaOrd* t = tabela_com(valores, N);

	/* Busca binária em 1024 itens: até 11 passos até o limite
	 * inferior, mais 2 da coleta (o achado e o que encerra). */
	int chave = 700;
	Resultado* r = tabela_ord_buscar(t, cmp_int, &chave);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_long(resultado_comparacoes(r), >, 0);
	munit_assert_long(resultado_comparacoes(r), <=, 13);
	munit_assert_double(resultado_tempo_ms(r), >=, 0.0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_pagina_igual_a_fatia_da_busca(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	enum { N = 300 };
	static int v[N];
	for (int i = 0; i < N; i++) v[i] = (i * 7919) % 100;  /* com repetidos */

	TabelaOrd* t = tabela_com(v, N);
	Faixa f[] = { {0, 99}, {40, 60}, {50, 50}, {95, 200}, {-5, -1} };
	int offs[] = { 0, 1, 37, 159, 400 };
	int lims[] = { 0, 1, 10, 500 };

	for (unsigned i = 0; i < sizeof f / sizeof *f; i++) {
		Resultado* c = tabela_ord_buscar(t, cmp_faixa, &f[i]);
		int k = resultado_tamanho(c);
		for (unsigned j = 0; j < sizeof offs / sizeof *offs; j++) {
			for (unsigned l = 0; l < sizeof lims / sizeof *lims; l++) {
				Resultado* p = tabela_ord_buscar_pagina(
					t, cmp_faixa, &f[i], offs[j], lims[l]);
				int esperado = offs[j] >= k ? 0 : k - offs[j];
				if (esperado > lims[l]) esperado = lims[l];
				munit_assert_int(resultado_total(p), ==, k);
				munit_assert_int(resultado_tamanho(p), ==, esperado);
				for (int m = 0; m < esperado; m++) {
					munit_assert_ptr_equal(
						resultado_item(p, m),
						resultado_item(c, offs[j] + m));
				}
				resultado_liberar(p);
			}
		}
		resultado_liberar(c);
	}
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
		{ .name = "/criar-sem-comparador",
		  .test = test_criar_sem_comparador },
		{ .name = "/liberar-null",
		  .test = test_liberar_null },
		{ .name = "/inserir-um",
		  .test = test_inserir_um },
		{ .name = "/ordem-mantida-apos-insercoes",
		  .test = test_ordem_mantida_apos_insercoes },
		{ .name = "/insercao-no-meio",
		  .test = test_insercao_no_meio },
		{ .name = "/inserir-duplicado",
		  .test = test_inserir_duplicado },
		{ .name = "/inserir-muitos",
		  .test = test_inserir_muitos },
		{ .name = "/item-fora-do-intervalo",
		  .test = test_item_fora_do_intervalo },
		{ .name = "/buscar-exato",
		  .test = test_buscar_exato },
		{ .name = "/buscar-inexistente",
		  .test = test_buscar_inexistente },
		{ .name = "/buscar-em-tabela-vazia",
		  .test = test_buscar_em_tabela_vazia },
		{ .name = "/buscar-intervalo",
		  .test = test_buscar_intervalo },
		{ .name = "/buscar-intervalo-com-repetidos",
		  .test = test_buscar_intervalo_com_repetidos },
		{ .name = "/buscar-sem-chave-lista-tudo",
		  .test = test_buscar_sem_chave_lista_tudo },
		{ .name = "/busca-binaria-conta-comparacoes",
		  .test = test_busca_binaria_conta_comparacoes },
		{ .name = "/pagina-igual-a-fatia-da-busca",
		  .test = test_pagina_igual_a_fatia_da_busca },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_tabela_ord_get(void) {
	return &suite_tabela_ord;
}
