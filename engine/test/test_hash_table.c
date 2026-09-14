/**
 * test_hash_table.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD HashTable. Tabela hash com
 *            encadeamento, indexada por artista.
 */
#include <stdio.h>
#include <stdlib.h>
#include "munit.h"
#include "hash_table.h"
#include "obra.h"

/* ==============================
 * Helpers
 * ============================== */

/** Cria uma obra com os campos essenciais para a tabela
 *
 * Parâmetros:
 * int id: ID da obra
 * const char* titulo: título
 * const char* artista: artista (chave de hash e busca)
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

	HashTable* ht = hash_table_criar(16);
	munit_assert_not_null(ht);
	munit_assert_int(hash_table_tamanho(ht), ==, 0);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_liberar_null(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	hash_table_liberar(NULL);
	return MUNIT_OK;
}

static MunitResult test_criar_capacidade_zero(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Capacidade 0 deve virar 1 (não crasha nem divide por zero) */
	HashTable* ht = hash_table_criar(0);
	munit_assert_not_null(ht);
	munit_assert_int(hash_table_tamanho(ht), ==, 0);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

/* ==============================
 * Testes: inserção
 * ============================== */

static MunitResult test_inserir_um(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);

	munit_assert_int(hash_table_tamanho(ht), ==, 1);

	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_inserir_varios(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);
	hash_table_inserir(ht, f->picasso);
	hash_table_inserir(ht, f->klimt);

	munit_assert_int(hash_table_tamanho(ht), ==, 3);

	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_inserir_duplicado(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);
	hash_table_inserir(ht, f->van_gogh);

	munit_assert_int(hash_table_tamanho(ht), ==, 2);

	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_inserir_forca_colisao(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Capacidade 1 força todos os elementos no mesmo bucket.
	 * Testa o encadeamento puro. */
	HashTable* ht = hash_table_criar(1);

	Obra* a = obra_rapida(1, "a", "artista-a", "g");
	Obra* b = obra_rapida(2, "b", "artista-b", "g");
	Obra* c = obra_rapida(3, "c", "artista-c", "g");

	hash_table_inserir(ht, a);
	hash_table_inserir(ht, b);
	hash_table_inserir(ht, c);

	munit_assert_int(hash_table_tamanho(ht), ==, 3);

	hash_table_liberar(ht);
	obra_liberar(a);
	obra_liberar(b);
	obra_liberar(c);
	return MUNIT_OK;
}

static MunitResult test_inserir_muitos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	HashTable* ht = hash_table_criar(32);

	Obra* obras[200];
	char nomes[200][16];
	for (int i = 0; i < 200; i++) {
		snprintf(nomes[i], sizeof nomes[i], "a%03d", i);
		obras[i] = obra_rapida(i, "titulo", nomes[i], "genero");
		hash_table_inserir(ht, obras[i]);
	}

	munit_assert_int(hash_table_tamanho(ht), ==, 200);

	hash_table_liberar(ht);
	for (int i = 0; i < 200; i++) obra_liberar(obras[i]);
	return MUNIT_OK;
}

/* ==============================
 * Testes: busca por artista
 * ============================== */

static MunitResult test_buscar_artista_existente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);
	hash_table_inserir(ht, f->picasso);
	hash_table_inserir(ht, f->klimt);

	Resultado* r = hash_table_buscar_artista(ht, "picasso");
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), f->picasso);

	resultado_liberar(r);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);
	hash_table_inserir(ht, f->picasso);

	Resultado* r = hash_table_buscar_artista(ht, "monet");
	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_multiplos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* a1 = obra_rapida(1, "obra-1", "picasso", "cubism");
	Obra* a2 = obra_rapida(2, "obra-2", "picasso", "cubism");
	Obra* a3 = obra_rapida(3, "obra-3", "monet",   "impressionism");

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, a1);
	hash_table_inserir(ht, a2);
	hash_table_inserir(ht, a3);

	Resultado* r = hash_table_buscar_artista(ht, "picasso");
	munit_assert_int(resultado_tamanho(r), ==, 2);

	resultado_liberar(r);
	hash_table_liberar(ht);
	obra_liberar(a1);
	obra_liberar(a2);
	obra_liberar(a3);
	return MUNIT_OK;
}

static MunitResult test_buscar_artista_em_colisao(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Capacidade 1: todos caem no mesmo bucket. A busca tem que
	 * percorrer a cadeia e achar o certo. */
	HashTable* ht = hash_table_criar(1);

	Obra* a = obra_rapida(1, "a", "artista-a", "g");
	Obra* b = obra_rapida(2, "b", "artista-b", "g");
	Obra* c = obra_rapida(3, "c", "artista-c", "g");

	hash_table_inserir(ht, a);
	hash_table_inserir(ht, b);
	hash_table_inserir(ht, c);

	Resultado* r = hash_table_buscar_artista(ht, "artista-b");
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), b);

	resultado_liberar(r);
	hash_table_liberar(ht);
	obra_liberar(a);
	obra_liberar(b);
	obra_liberar(c);
	return MUNIT_OK;
}

/* ==============================
 * Testes: busca por gênero
 * ============================== */

static MunitResult test_buscar_genero_existente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);
	hash_table_inserir(ht, f->picasso);
	hash_table_inserir(ht, f->klimt);

	Resultado* r = hash_table_buscar_genero(ht, "cubism");
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_ptr_equal(resultado_item(r, 0), f->picasso);

	resultado_liberar(r);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_buscar_genero_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);

	Resultado* r = hash_table_buscar_genero(ht, "barroco");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_buscar_genero_multiplos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* a1 = obra_rapida(1, "obra-1", "a", "cubism");
	Obra* a2 = obra_rapida(2, "obra-2", "b", "cubism");
	Obra* a3 = obra_rapida(3, "obra-3", "c", "cubism");

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, a1);
	hash_table_inserir(ht, a2);
	hash_table_inserir(ht, a3);

	Resultado* r = hash_table_buscar_genero(ht, "cubism");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	resultado_liberar(r);
	hash_table_liberar(ht);
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

	HashTable* ht = hash_table_criar(16);
	hash_table_inserir(ht, f->van_gogh);
	hash_table_inserir(ht, f->picasso);
	hash_table_inserir(ht, f->klimt);

	Resultado* r = hash_table_buscar_artista(ht, "picasso");
	munit_assert_long(resultado_comparacoes(r), >, 0);
	munit_assert_double(resultado_tempo_ms(r), >=, 0.0);

	resultado_liberar(r);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_hash_table = {
	"/hash-table",
	(MunitTest[]) {
		{ .name = "/criar-vazia",
		  .test = test_criar_vazia },
		{ .name = "/liberar-null",
		  .test = test_liberar_null },
		{ .name = "/criar-capacidade-zero",
		  .test = test_criar_capacidade_zero },
		{ .name = "/inserir-um",
		  .test = test_inserir_um,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/inserir-varios",
		  .test = test_inserir_varios,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/inserir-duplicado",
		  .test = test_inserir_duplicado,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/inserir-forca-colisao",
		  .test = test_inserir_forca_colisao },
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
		{ .name = "/buscar-artista-em-colisao",
		  .test = test_buscar_artista_em_colisao },
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

const MunitSuite* suite_hash_table_get(void) {
	return &suite_hash_table;
}
