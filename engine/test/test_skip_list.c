/**
 * test_skip_list.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD SkipList. Lista encadeada
 *            probabilística, ordenada pela chave composta
 *            (gênero, artista).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
 * const char* artista: artista (chave secundária de ordenação)
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
 * Chave composta (gênero, artista)
 * ============================== */

/** Monta uma lista com 5 obras cruzando 2 gêneros e 3 artistas
 *
 * van gogh aparece nos dois gêneros, o que exercita o espalhamento do
 * artista entre blocos. Dentro de post-impressionism há dois artistas,
 * o que exercita o desempate secundário.
 *
 * Parâmetros:
 * Obra** obras: array de 5 posições, preenchido com as obras criadas
 *
 * Retorna SkipList*: lista populada
 */
static SkipList* montar_lista_mista(Obra** obras) {
	obras[0] = obra_rapida(1, "t1", "van gogh", "realism");
	obras[1] = obra_rapida(2, "t2", "van gogh", "post-impressionism");
	obras[2] = obra_rapida(3, "t3", "van gogh", "post-impressionism");
	obras[3] = obra_rapida(4, "t4", "picasso",  "post-impressionism");
	obras[4] = obra_rapida(5, "t5", "klimt",    "realism");

	SkipList* sl = skip_list_criar();
	for (int i = 0; i < 5; i++) skip_list_inserir(sl, obras[i]);
	return sl;
}

/** Libera as 5 obras montadas por montar_lista_mista */
static void liberar_mistas(Obra** obras) {
	for (int i = 0; i < 5; i++) obra_liberar(obras[i]);
}

static MunitResult test_ordem_desempata_por_artista(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	SkipList* sl = montar_lista_mista(obras);

	/* Dentro do bloco de um gênero a ordem secundária é o artista,
	 * então o bloco sai com os artistas não decrescentes. */
	Resultado* r = skip_list_buscar_genero(sl, "post-impressionism");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	for (int i = 1; i < resultado_tamanho(r); i++) {
		const char* anterior = obra_artista(resultado_item(r, i - 1));
		const char* atual    = obra_artista(resultado_item(r, i));
		munit_assert_true(strcmp(anterior, atual) <= 0);
	}

	resultado_liberar(r);
	skip_list_liberar(sl);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_existente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	SkipList* sl = montar_lista_mista(obras);

	Resultado* r = skip_list_buscar_genero_artista(sl, "post-impressionism", "van gogh");

	/* Das 3 obras de van gogh, só as 2 pós-impressionistas entram. */
	munit_assert_int(resultado_tamanho(r), ==, 2);
	for (int i = 0; i < resultado_tamanho(r); i++) {
		const Obra* o = resultado_item(r, i);
		munit_assert_string_equal(obra_artista(o), "van gogh");
		munit_assert_string_equal(obra_genero(o), "post-impressionism");
	}

	resultado_liberar(r);
	skip_list_liberar(sl);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_par_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	SkipList* sl = montar_lista_mista(obras);

	/* Artista existe e gênero existe, mas o par não. */
	Resultado* r = skip_list_buscar_genero_artista(sl, "realism", "picasso");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	skip_list_liberar(sl);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_em_lista_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	SkipList* sl = skip_list_criar();

	Resultado* r = skip_list_buscar_genero_artista(sl, "realism", "van gogh");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	skip_list_liberar(sl);
	return MUNIT_OK;
}

static MunitResult test_genero_traz_artistas_diferentes(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	SkipList* sl = montar_lista_mista(obras);

	/* Sem artista, o bloco do gênero inteiro é devolvido. */
	Resultado* r = skip_list_buscar_genero(sl, "realism");
	munit_assert_int(resultado_tamanho(r), ==, 2);

	resultado_liberar(r);
	skip_list_liberar(sl);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_artista_atravessa_generos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	SkipList* sl = montar_lista_mista(obras);

	/* A varredura por artista alcança as obras dele em todos os gêneros. */
	Resultado* r = skip_list_buscar_artista(sl, "van gogh");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	/* Varredura do nível 0: uma comparação por elemento da lista. */
	munit_assert_long(resultado_comparacoes(r), ==, 5);

	resultado_liberar(r);
	skip_list_liberar(sl);
	liberar_mistas(obras);
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
		{ .name = "/ordem-desempata-por-artista",
		  .test = test_ordem_desempata_por_artista },
		{ .name = "/genero-artista-existente",
		  .test = test_genero_artista_existente },
		{ .name = "/genero-artista-par-inexistente",
		  .test = test_genero_artista_par_inexistente },
		{ .name = "/genero-artista-em-lista-vazia",
		  .test = test_genero_artista_em_lista_vazia },
		{ .name = "/genero-traz-artistas-diferentes",
		  .test = test_genero_traz_artistas_diferentes },
		{ .name = "/artista-atravessa-generos",
		  .test = test_artista_atravessa_generos },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_skip_list_get(void) {
	return &suite_skip_list;
}
