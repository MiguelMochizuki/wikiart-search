/**
 * test_tabela_ord.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD TabelaOrd. Lista indexada
 *            ordenada por (gênero, artista), com busca binária.
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

	/* Insere fora de ordem. A chave primária é o gênero, então a ordem
	 * final segue cubism < post-impressionism < symbolism, e não a
	 * ordem alfabética dos artistas (klimt < picasso < van gogh). */
	Obra* picasso  = obra_rapida(1, "g", "picasso",  "cubism");
	Obra* van_gogh = obra_rapida(2, "s", "van gogh", "post-impressionism");
	Obra* klimt    = obra_rapida(3, "k", "klimt",    "symbolism");

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, van_gogh);
	tabela_ord_inserir(t, klimt);
	tabela_ord_inserir(t, picasso);

	munit_assert_int(tabela_ord_tamanho(t), ==, 3);
	munit_assert_ptr_equal(tabela_ord_item(t, 0), picasso);
	munit_assert_ptr_equal(tabela_ord_item(t, 1), van_gogh);
	munit_assert_ptr_equal(tabela_ord_item(t, 2), klimt);

	tabela_ord_liberar(t);
	obra_liberar(picasso);
	obra_liberar(van_gogh);
	obra_liberar(klimt);
	return MUNIT_OK;
}

static MunitResult test_ordem_desempata_por_artista(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Dentro do mesmo gênero, o desempate é pelo artista. */
	Obra* c = obra_rapida(1, "t", "ccc", "mesmo-genero");
	Obra* a = obra_rapida(2, "t", "aaa", "mesmo-genero");
	Obra* b = obra_rapida(3, "t", "bbb", "mesmo-genero");

	TabelaOrd* t = tabela_ord_criar();
	tabela_ord_inserir(t, c);
	tabela_ord_inserir(t, a);
	tabela_ord_inserir(t, b);

	munit_assert_ptr_equal(tabela_ord_item(t, 0), a);
	munit_assert_ptr_equal(tabela_ord_item(t, 1), b);
	munit_assert_ptr_equal(tabela_ord_item(t, 2), c);

	tabela_ord_liberar(t);
	obra_liberar(a);
	obra_liberar(b);
	obra_liberar(c);
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

	/* Verifica a ordenação composta: gênero primeiro, artista depois */
	for (int i = 1; i < 200; i++) {
		const Obra* ant = tabela_ord_item(t, i - 1);
		const Obra* cur = tabela_ord_item(t, i);
		int cg = strcmp(obra_genero(ant), obra_genero(cur));
		munit_assert_true(cg < 0 ||
			(cg == 0 && strcmp(obra_artista(ant), obra_artista(cur)) <= 0));
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
 * Testes: busca por gênero + artista
 * ============================== */

/** Monta uma tabela com o mesmo artista em dois gêneros
 *
 * Parâmetros:
 * Obra** obras: array de 5 posições preenchido com as obras criadas
 *
 * Retorna TabelaOrd*: tabela populada
 */
static TabelaOrd* montar_tabela_mista(Obra** obras) {
	obras[0] = obra_rapida(1, "t1", "van gogh", "realism");
	obras[1] = obra_rapida(2, "t2", "van gogh", "post-impressionism");
	obras[2] = obra_rapida(3, "t3", "van gogh", "post-impressionism");
	obras[3] = obra_rapida(4, "t4", "picasso",  "post-impressionism");
	obras[4] = obra_rapida(5, "t5", "klimt",    "realism");

	TabelaOrd* t = tabela_ord_criar();
	for (int i = 0; i < 5; i++) tabela_ord_inserir(t, obras[i]);
	return t;
}

/** Libera as 5 obras montadas por montar_tabela_mista */
static void liberar_mistas(Obra** obras) {
	for (int i = 0; i < 5; i++) obra_liberar(obras[i]);
}

static MunitResult test_genero_artista_existente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	TabelaOrd* t = montar_tabela_mista(obras);

	Resultado* r = tabela_ord_buscar_genero_artista(t, "post-impressionism", "van gogh");

	/* Das 3 obras de van gogh, só as 2 pós-impressionistas entram. */
	munit_assert_int(resultado_tamanho(r), ==, 2);
	for (int i = 0; i < resultado_tamanho(r); i++) {
		const Obra* o = resultado_item(r, i);
		munit_assert_string_equal(obra_artista(o), "van gogh");
		munit_assert_string_equal(obra_genero(o), "post-impressionism");
	}

	resultado_liberar(r);
	tabela_ord_liberar(t);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_par_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	TabelaOrd* t = montar_tabela_mista(obras);

	/* Artista existe e gênero existe, mas o par não. */
	Resultado* r = tabela_ord_buscar_genero_artista(t, "realism", "picasso");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_em_tabela_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	TabelaOrd* t = tabela_ord_criar();
	Resultado* r = tabela_ord_buscar_genero_artista(t, "cubism", "picasso");

	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_null_equivale_a_genero(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	TabelaOrd* t = montar_tabela_mista(obras);

	Resultado* com_null = tabela_ord_buscar_genero_artista(t, "realism", NULL);
	Resultado* so_genero = tabela_ord_buscar_genero(t, "realism");

	munit_assert_int(resultado_tamanho(com_null), ==, resultado_tamanho(so_genero));
	munit_assert_int(resultado_tamanho(com_null), ==, 2);

	resultado_liberar(com_null);
	resultado_liberar(so_genero);
	tabela_ord_liberar(t);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_traz_artistas_diferentes(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	TabelaOrd* t = montar_tabela_mista(obras);

	/* post-impressionism tem van gogh (2) e picasso (1). */
	Resultado* r = tabela_ord_buscar_genero(t, "post-impressionism");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	int tem_van_gogh = 0, tem_picasso = 0;
	for (int i = 0; i < resultado_tamanho(r); i++) {
		const char* a = obra_artista(resultado_item(r, i));
		if (strcmp(a, "van gogh") == 0) tem_van_gogh = 1;
		if (strcmp(a, "picasso")  == 0) tem_picasso  = 1;
	}
	munit_assert_true(tem_van_gogh);
	munit_assert_true(tem_picasso);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_artista_atravessa_generos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	TabelaOrd* t = montar_tabela_mista(obras);

	/* A varredura por artista alcança as obras dele em todos os gêneros. */
	Resultado* r = tabela_ord_buscar_artista(t, "van gogh");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	/* Varredura completa: uma comparação por elemento da tabela. */
	munit_assert_long(resultado_comparacoes(r), ==, 5);

	resultado_liberar(r);
	tabela_ord_liberar(t);
	liberar_mistas(obras);
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
		{ .name = "/ordem-desempata-por-artista",
		  .test = test_ordem_desempata_por_artista },
		{ .name = "/genero-artista-existente",
		  .test = test_genero_artista_existente },
		{ .name = "/genero-artista-par-inexistente",
		  .test = test_genero_artista_par_inexistente },
		{ .name = "/genero-artista-em-tabela-vazia",
		  .test = test_genero_artista_em_tabela_vazia },
		{ .name = "/genero-artista-null-equivale-a-genero",
		  .test = test_genero_artista_null_equivale_a_genero },
		{ .name = "/genero-traz-artistas-diferentes",
		  .test = test_genero_traz_artistas_diferentes },
		{ .name = "/artista-atravessa-generos",
		  .test = test_artista_atravessa_generos },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_tabela_ord_get(void) {
	return &suite_tabela_ord;
}
