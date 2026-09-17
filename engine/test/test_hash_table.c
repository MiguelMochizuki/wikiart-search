/**
 * test_hash_table.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD HashTable. Tabela hash com
 *            encadeamento, indexada por gênero (bucket) e ordenada
 *            por artista dentro de cada cadeia.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
 * const char* artista: artista (chave secundária, ordena a cadeia)
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
 * Chave composta (gênero, artista)
 * ============================== */

/** Monta uma tabela com 5 obras cruzando 2 gêneros e 3 artistas
 *
 * van gogh aparece nos dois gêneros, o que exercita o espalhamento do
 * artista entre buckets. Dentro de post-impressionism há dois artistas,
 * o que exercita a ordenação da cadeia.
 *
 * Parâmetros:
 * Obra** obras: array de 5 posições, preenchido com as obras criadas
 *
 * Retorna HashTable*: tabela populada
 */
static HashTable* montar_tabela_mista(Obra** obras) {
	obras[0] = obra_rapida(1, "t1", "van gogh", "realism");
	obras[1] = obra_rapida(2, "t2", "van gogh", "post-impressionism");
	obras[2] = obra_rapida(3, "t3", "van gogh", "post-impressionism");
	obras[3] = obra_rapida(4, "t4", "picasso",  "post-impressionism");
	obras[4] = obra_rapida(5, "t5", "klimt",    "realism");

	HashTable* ht = hash_table_criar(16);
	for (int i = 0; i < 5; i++) hash_table_inserir(ht, obras[i]);
	return ht;
}

/** Libera as 5 obras montadas por montar_tabela_mista */
static void liberar_mistas(Obra** obras) {
	for (int i = 0; i < 5; i++) obra_liberar(obras[i]);
}

static MunitResult test_cadeia_ordenada_por_artista(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	HashTable* ht = montar_tabela_mista(obras);

	/* A cadeia do bucket é mantida ordenada por artista, e a busca
	 * percorre nessa ordem, então o bloco sai não decrescente. */
	Resultado* r = hash_table_buscar_genero(ht, "post-impressionism");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	for (int i = 1; i < resultado_tamanho(r); i++) {
		const char* anterior = obra_artista(resultado_item(r, i - 1));
		const char* atual    = obra_artista(resultado_item(r, i));
		munit_assert_true(strcmp(anterior, atual) <= 0);
	}

	resultado_liberar(r);
	hash_table_liberar(ht);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_cadeia_ordenada_em_colisao(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Capacidade 1: os dois gêneros caem no mesmo bucket e as obras
	 * ficam intercaladas por artista. A busca por gênero tem que
	 * filtrar em vez de parar cedo. */
	Obra* obras[5];
	obras[0] = obra_rapida(1, "t1", "van gogh", "realism");
	obras[1] = obra_rapida(2, "t2", "van gogh", "post-impressionism");
	obras[2] = obra_rapida(3, "t3", "van gogh", "post-impressionism");
	obras[3] = obra_rapida(4, "t4", "picasso",  "post-impressionism");
	obras[4] = obra_rapida(5, "t5", "klimt",    "realism");

	HashTable* ht = hash_table_criar(1);
	for (int i = 0; i < 5; i++) hash_table_inserir(ht, obras[i]);

	Resultado* r = hash_table_buscar_genero(ht, "realism");
	munit_assert_int(resultado_tamanho(r), ==, 2);
	for (int i = 0; i < resultado_tamanho(r); i++) {
		munit_assert_string_equal(obra_genero(resultado_item(r, i)), "realism");
	}

	resultado_liberar(r);
	hash_table_liberar(ht);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_existente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	HashTable* ht = montar_tabela_mista(obras);

	Resultado* r = hash_table_buscar_genero_artista(ht, "post-impressionism", "van gogh");

	/* Das 3 obras de van gogh, só as 2 pós-impressionistas entram. */
	munit_assert_int(resultado_tamanho(r), ==, 2);
	for (int i = 0; i < resultado_tamanho(r); i++) {
		const Obra* o = resultado_item(r, i);
		munit_assert_string_equal(obra_artista(o), "van gogh");
		munit_assert_string_equal(obra_genero(o), "post-impressionism");
	}

	resultado_liberar(r);
	hash_table_liberar(ht);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_par_inexistente(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	HashTable* ht = montar_tabela_mista(obras);

	/* Artista existe e gênero existe, mas o par não. */
	Resultado* r = hash_table_buscar_genero_artista(ht, "realism", "picasso");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	hash_table_liberar(ht);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_genero_artista_em_tabela_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	HashTable* ht = hash_table_criar(16);

	Resultado* r = hash_table_buscar_genero_artista(ht, "realism", "van gogh");
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	hash_table_liberar(ht);
	return MUNIT_OK;
}

static MunitResult test_genero_traz_artistas_diferentes(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	HashTable* ht = montar_tabela_mista(obras);

	/* Sem artista, o bucket do gênero inteiro é devolvido. */
	Resultado* r = hash_table_buscar_genero(ht, "realism");
	munit_assert_int(resultado_tamanho(r), ==, 2);

	resultado_liberar(r);
	hash_table_liberar(ht);
	liberar_mistas(obras);
	return MUNIT_OK;
}

static MunitResult test_artista_atravessa_generos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* obras[5];
	HashTable* ht = montar_tabela_mista(obras);

	/* A varredura por artista alcança as obras dele em todos os buckets. */
	Resultado* r = hash_table_buscar_artista(ht, "van gogh");
	munit_assert_int(resultado_tamanho(r), ==, 3);

	/* van gogh é o maior artista das duas cadeias, então nenhuma
	 * parada antecipada dispara: 5 comparações, uma por elemento. */
	munit_assert_long(resultado_comparacoes(r), ==, 5);

	resultado_liberar(r);
	hash_table_liberar(ht);
	liberar_mistas(obras);
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
		{ .name = "/cadeia-ordenada-por-artista",
		  .test = test_cadeia_ordenada_por_artista },
		{ .name = "/cadeia-ordenada-em-colisao",
		  .test = test_cadeia_ordenada_em_colisao },
		{ .name = "/genero-artista-existente",
		  .test = test_genero_artista_existente },
		{ .name = "/genero-artista-par-inexistente",
		  .test = test_genero_artista_par_inexistente },
		{ .name = "/genero-artista-em-tabela-vazia",
		  .test = test_genero_artista_em_tabela_vazia },
		{ .name = "/genero-traz-artistas-diferentes",
		  .test = test_genero_traz_artistas_diferentes },
		{ .name = "/artista-atravessa-generos",
		  .test = test_artista_atravessa_generos },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_hash_table_get(void) {
	return &suite_hash_table;
}
