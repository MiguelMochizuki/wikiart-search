/**
 * test_catalogo.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários dos TADs Catalogo, Genero e Artista.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "munit.h"
#include "catalogo.h"

/* ==============================
 * Fixture
 * ============================== */

/* Três gêneros. Van Gogh aparece em dois deles. No Cubism, Braque e
 * Picasso empatam com uma obra cada. */
static const char* CSV_MISTO =
	"id;titulo;artista;genero;ano;caminho\n"
	"5;t5;monet;Impressionism;1870;p5\n"
	"2;t2;monet;Impressionism;1871;p2\n"
	"3;t3;renoir;Impressionism;1872;p3\n"
	"4;t4;van gogh;Post Impressionism;1888;p4\n"
	"1;t1;van gogh;Impressionism;1880;p1\n"
	"6;t6;cezanne;Post Impressionism;1890;p6\n"
	"7;t7;van gogh;Post Impressionism;1889;p7\n"
	"8;t8;picasso;Cubism;1910;p8\n"
	"9;t9;braque;Cubism;1911;p9\n";

/** Escreve um CSV temporário, carrega e apaga o arquivo
 *
 * Parâmetros:
 * const char* conteudo: texto do CSV
 *
 * Retorna Csv*: CSV carregado
 */
static Csv* carregar_texto(const char* conteudo) {
	char caminho[] = "/tmp/test_catalogo_XXXXXX";
	int fd = mkstemp(caminho);
	if (fd < 0) {
		perror("mkstemp");
		exit(1);
	}
	FILE* f = fdopen(fd, "w");
	fputs(conteudo, f);
	fclose(f);

	Csv* c = csv_carregar(caminho);
	unlink(caminho);
	return c;
}

typedef struct {
	Csv* csv;
	Catalogo* cat;
} Fixture;

static void* setup(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Fixture* f = malloc(sizeof *f);
	f->csv = carregar_texto(CSV_MISTO);
	f->cat = catalogo_montar(f->csv);
	return f;
}

static void teardown(void* fixture) {
	Fixture* f = fixture;
	catalogo_liberar(f->cat);
	csv_liberar(f->csv);
	free(f);
}

/* ==============================
 * Helpers
 * ============================== */

/** Procura um gênero pelo nome (a ordem de carga é embaralhada) */
static const Genero* achar_genero(const Catalogo* c, const char* nome) {
	for (int i = 0; i < catalogo_n_generos(c); i++) {
		const Genero* g = catalogo_genero(c, i);
		if (strcmp(genero_nome(g), nome) == 0) return g;
	}
	return NULL;
}

/** Procura um artista dentro de um gênero */
static const Artista* achar_artista(const Catalogo* c, const char* genero,
				    const char* nome) {
	for (int i = 0; i < catalogo_n_artistas(c); i++) {
		const Artista* a = catalogo_artista(c, i);
		if (strcmp(artista_genero(a), genero) == 0
		    && strcmp(artista_nome(a), nome) == 0) {
			return a;
		}
	}
	return NULL;
}

/* ==============================
 * Testes
 * ============================== */

static MunitResult test_conta_niveis(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	munit_assert_not_null(f->cat);
	munit_assert_int(catalogo_n_generos(f->cat), ==, 3);
	munit_assert_int(catalogo_n_artistas(f->cat), ==, 7);
	munit_assert_int(catalogo_n_obras(f->cat), ==, 9);
	return MUNIT_OK;
}

static MunitResult test_genero_conta_obras_e_artistas(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	const Genero* imp = achar_genero(f->cat, "Impressionism");
	munit_assert_not_null(imp);
	munit_assert_int(genero_obras(imp), ==, 4);
	munit_assert_int(genero_artistas(imp), ==, 3);

	const Genero* pos = achar_genero(f->cat, "Post Impressionism");
	munit_assert_not_null(pos);
	munit_assert_int(genero_obras(pos), ==, 3);
	munit_assert_int(genero_artistas(pos), ==, 2);
	return MUNIT_OK;
}

static MunitResult test_artista_repete_entre_generos(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* Uma entrada por gênero, cada uma com a contagem daquele gênero. */
	const Artista* no_imp = achar_artista(f->cat, "Impressionism", "van gogh");
	const Artista* no_pos = achar_artista(f->cat, "Post Impressionism", "van gogh");
	munit_assert_not_null(no_imp);
	munit_assert_not_null(no_pos);
	munit_assert_int(artista_obras(no_imp), ==, 1);
	munit_assert_int(artista_obras(no_pos), ==, 2);
	return MUNIT_OK;
}

static MunitResult test_capa_do_artista_e_a_primeira_obra(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* Monet tem os ids 5 e 2, nessa ordem no CSV. A capa segue a chave
	 * (menor id), não a ordem do arquivo. */
	const Artista* monet = achar_artista(f->cat, "Impressionism", "monet");
	munit_assert_int(obra_id(artista_capa(monet)), ==, 2);
	munit_assert_string_equal(obra_genero(artista_capa(monet)), "Impressionism");
	return MUNIT_OK;
}

static MunitResult test_capa_do_genero_e_do_mais_prolifico(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* Impressionism: Monet (2 obras) ganha de Renoir e Van Gogh (1). */
	const Genero* imp = achar_genero(f->cat, "Impressionism");
	munit_assert_int(obra_id(genero_capa(imp)), ==, 2);

	/* Post Impressionism: Van Gogh (2) ganha de Cézanne (1). */
	const Genero* pos = achar_genero(f->cat, "Post Impressionism");
	munit_assert_int(obra_id(genero_capa(pos)), ==, 4);
	return MUNIT_OK;
}

static MunitResult test_empate_fica_o_primeiro_alfabetico(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* Braque e Picasso empatam: vale Braque, que vem antes. */
	const Genero* cub = achar_genero(f->cat, "Cubism");
	munit_assert_string_equal(obra_artista(genero_capa(cub)), "braque");
	return MUNIT_OK;
}

static MunitResult test_ordem_de_carga_e_permutacao(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* Cada obra do CSV aparece exatamente uma vez na ordem de carga. */
	int vistas[10] = {0};
	for (int i = 0; i < catalogo_n_obras(f->cat); i++) {
		vistas[obra_id(catalogo_obra(f->cat, i))]++;
	}
	for (int id = 1; id <= 9; id++) munit_assert_int(vistas[id], ==, 1);

	/* E cada gênero, uma vez. */
	for (int i = 0; i < catalogo_n_generos(f->cat); i++) {
		for (int j = i + 1; j < catalogo_n_generos(f->cat); j++) {
			munit_assert_ptr_not_equal(catalogo_genero(f->cat, i),
						   catalogo_genero(f->cat, j));
		}
	}
	return MUNIT_OK;
}

static MunitResult test_indices_fora_do_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	munit_assert_null(catalogo_genero(f->cat, -1));
	munit_assert_null(catalogo_genero(f->cat, 3));
	munit_assert_null(catalogo_artista(f->cat, 7));
	munit_assert_null(catalogo_obra(f->cat, 9));
	return MUNIT_OK;
}

static MunitResult test_csv_vazio(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Csv* csv = carregar_texto("id;titulo;artista;genero;ano;caminho\n");
	Catalogo* cat = catalogo_montar(csv);

	munit_assert_not_null(cat);
	munit_assert_int(catalogo_n_generos(cat), ==, 0);
	munit_assert_int(catalogo_n_artistas(cat), ==, 0);
	munit_assert_int(catalogo_n_obras(cat), ==, 0);

	catalogo_liberar(cat);
	csv_liberar(csv);
	return MUNIT_OK;
}

static MunitResult test_liberar_null(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	catalogo_liberar(NULL);
	return MUNIT_OK;
}

static MunitResult test_montar_ordenado_segue_a_ordem_das_chaves(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Csv* csv = carregar_texto(CSV_MISTO);
	Catalogo* c = catalogo_montar_ordenado(csv);

	/* Gêneros por nome, artistas por (gênero, artista), obras por
	 * (gênero, artista, id): a carga clássica, sem embaralhar. */
	munit_assert_int(catalogo_n_generos(c), ==, 3);
	for (int i = 1; i < catalogo_n_generos(c); i++) {
		munit_assert_int(strcmp(genero_nome(catalogo_genero(c, i - 1)),
					genero_nome(catalogo_genero(c, i))), <, 0);
	}
	for (int i = 1; i < catalogo_n_obras(c); i++) {
		const Obra* a = catalogo_obra(c, i - 1);
		const Obra* b = catalogo_obra(c, i);
		int cmp = strcmp(obra_genero(a), obra_genero(b));
		if (cmp == 0) cmp = strcmp(obra_artista(a), obra_artista(b));
		if (cmp == 0) cmp = obra_id(a) - obra_id(b);
		munit_assert_int(cmp, <, 0);
	}

	catalogo_liberar(c);
	csv_liberar(csv);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_catalogo = {
	"/catalogo",
	(MunitTest[]) {
		{ .name = "/conta-niveis",
		  .test = test_conta_niveis,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/genero-conta-obras-e-artistas",
		  .test = test_genero_conta_obras_e_artistas,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/artista-repete-entre-generos",
		  .test = test_artista_repete_entre_generos,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/capa-do-artista-e-a-primeira-obra",
		  .test = test_capa_do_artista_e_a_primeira_obra,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/capa-do-genero-e-do-mais-prolifico",
		  .test = test_capa_do_genero_e_do_mais_prolifico,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/empate-fica-o-primeiro-alfabetico",
		  .test = test_empate_fica_o_primeiro_alfabetico,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/ordem-de-carga-e-permutacao",
		  .test = test_ordem_de_carga_e_permutacao,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/montar-ordenado-segue-a-ordem-das-chaves",
		  .test = test_montar_ordenado_segue_a_ordem_das_chaves },
		{ .name = "/indices-fora-do-intervalo",
		  .test = test_indices_fora_do_intervalo,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/csv-vazio",
		  .test = test_csv_vazio },
		{ .name = "/liberar-null",
		  .test = test_liberar_null },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_catalogo_get(void) {
	return &suite_catalogo;
}
