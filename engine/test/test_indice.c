/**
 * test_indice.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes do TAD Indice: as chaves de cada nível e as duas
 *            EDs respondendo igual à mesma navegação.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "munit.h"
#include "indice.h"

/* ==============================
 * Fixture
 * ============================== */

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
	"9;t9;braque;Cubism;1911;p9\n"
	"10;t10;monet;Impressionism;1872;p10\n";

/** Escreve um CSV temporário, carrega e apaga o arquivo */
static Csv* carregar_texto(const char* conteudo) {
	char caminho[] = "/tmp/test_indice_XXXXXX";
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
	Indice* tabela;
	Indice* arvore;
} Fixture;

static void* setup(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Fixture* f = malloc(sizeof *f);
	f->csv    = carregar_texto(CSV_MISTO);
	f->cat    = catalogo_montar(f->csv);
	f->tabela = indice_montar(&BUSCADOR_TABELA_ORD, f->cat);
	f->arvore = indice_montar(&BUSCADOR_ARVORE_AFUNILADA, f->cat);
	return f;
}

static void teardown(void* fixture) {
	Fixture* f = fixture;
	indice_liberar(f->tabela);
	indice_liberar(f->arvore);
	catalogo_liberar(f->cat);
	csv_liberar(f->csv);
	free(f);
}

/* ==============================
 * Testes: comparadores
 * ============================== */

static MunitResult test_cmp_obra_recorta_blocos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	Obra* o = obra_criar(7, "t", "monet", "Impressionism", 1870, "p");

	Chave todos      = { NULL, NULL, -1 };
	Chave do_genero  = { "Impressionism", NULL, -1 };
	Chave do_par     = { "Impressionism", "monet", -1 };
	Chave exata      = { "Impressionism", "monet", 7 };
	Chave id_maior   = { "Impressionism", "monet", 9 };
	Chave outro_par  = { "Impressionism", "renoir", -1 };
	Chave antes      = { "Baroque", NULL, -1 };

	munit_assert_int(indice_cmp_obra(o, &todos), ==, 0);
	munit_assert_int(indice_cmp_obra(o, &do_genero), ==, 0);
	munit_assert_int(indice_cmp_obra(o, &do_par), ==, 0);
	munit_assert_int(indice_cmp_obra(o, &exata), ==, 0);
	munit_assert_int(indice_cmp_obra(o, &id_maior), <, 0);
	munit_assert_int(indice_cmp_obra(o, &outro_par), <, 0);
	munit_assert_int(indice_cmp_obra(o, &antes), >, 0);

	obra_liberar(o);
	return MUNIT_OK;
}

static MunitResult test_niveis_ignoram_campos_finos(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* No nível 1 só o gênero conta; no 2, gênero e artista. Uma chave
	 * mais específica que o nível ainda casa com o item. */
	Chave fina = { "Cubism", "picasso", 8 };
	Resultado* r = indice_buscar(f->tabela, NIVEL_GENEROS, &fina);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	resultado_liberar(r);

	r = indice_buscar(f->tabela, NIVEL_ARTISTAS, &fina);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_string_equal(artista_nome(resultado_item(r, 0)), "picasso");
	resultado_liberar(r);
	return MUNIT_OK;
}

/* ==============================
 * Testes: busca por nível
 * ============================== */

/** Confere que as duas EDs devolvem os mesmos itens, na mesma ordem */
static void conferir_iguais(Fixture* f, Nivel nivel, const Chave* k,
			    int esperado) {
	Resultado* a = indice_buscar(f->tabela, nivel, k);
	Resultado* b = indice_buscar(f->arvore, nivel, k);

	munit_assert_int(resultado_tamanho(a), ==, esperado);
	munit_assert_int(resultado_tamanho(b), ==, esperado);
	for (int i = 0; i < esperado; i++) {
		munit_assert_ptr_equal(resultado_item(a, i), resultado_item(b, i));
	}

	resultado_liberar(a);
	resultado_liberar(b);
}

static MunitResult test_eds_respondem_igual(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	Chave imp        = { "Impressionism", NULL, -1 };
	Chave imp_monet  = { "Impressionism", "monet", -1 };
	Chave pos_gogh   = { "Post Impressionism", "van gogh", -1 };
	Chave uma_obra   = { "Impressionism", "monet", 5 };
	Chave inexistente = { "Surrealism", NULL, -1 };

	/* Duas passadas: na segunda a árvore já foi afunilada pela primeira,
	 * e as respostas não podem mudar com a forma. */
	for (int passada = 0; passada < 2; passada++) {
		conferir_iguais(f, NIVEL_GENEROS,  NULL,         3);
		conferir_iguais(f, NIVEL_GENEROS,  &imp,         1);
		conferir_iguais(f, NIVEL_ARTISTAS, &imp,         3);
		conferir_iguais(f, NIVEL_ARTISTAS, &inexistente, 0);
		conferir_iguais(f, NIVEL_OBRAS,    &imp,         5);
		conferir_iguais(f, NIVEL_OBRAS,    &imp_monet,   3);
		conferir_iguais(f, NIVEL_OBRAS,    &pos_gogh,    2);
		conferir_iguais(f, NIVEL_OBRAS,    &uma_obra,    1);
	}
	return MUNIT_OK;
}

static MunitResult test_niveis_em_ordem_de_chave(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* Gêneros por nome; obras de um par por id. */
	Resultado* r = indice_buscar(f->tabela, NIVEL_GENEROS, NULL);
	munit_assert_string_equal(genero_nome(resultado_item(r, 0)), "Cubism");
	munit_assert_string_equal(genero_nome(resultado_item(r, 1)), "Impressionism");
	munit_assert_string_equal(genero_nome(resultado_item(r, 2)), "Post Impressionism");
	resultado_liberar(r);

	Chave imp_monet = { "Impressionism", "monet", -1 };
	r = indice_buscar(f->tabela, NIVEL_OBRAS, &imp_monet);
	munit_assert_int(obra_id(resultado_item(r, 0)), ==, 2);
	munit_assert_int(obra_id(resultado_item(r, 1)), ==, 5);
	munit_assert_int(obra_id(resultado_item(r, 2)), ==, 10);
	resultado_liberar(r);
	return MUNIT_OK;
}

static MunitResult test_nivel_invalido(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	NoVista v[7];
	munit_assert_null(indice_buscar(f->tabela, N_NIVEIS, NULL));
	munit_assert_int(indice_vista(f->arvore, N_NIVEIS, NULL, v, 3), ==, -1);
	return MUNIT_OK;
}

/* ==============================
 * Testes: vista
 * ============================== */

static MunitResult test_so_a_arvore_tem_vista(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	NoVista v[15];
	munit_assert_int(indice_vista(f->tabela, NIVEL_GENEROS, NULL, v, 4), ==, -1);
	munit_assert_int(indice_vista(f->arvore, NIVEL_GENEROS, NULL, v, 4), ==, 3);
	return MUNIT_OK;
}

static MunitResult test_foco_leva_a_raiz_da_vista(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	/* Acessar um artista o põe na raiz; a vista do gênero dele continua
	 * mostrando só os artistas do gênero, agora com ele no topo. */
	const char* nomes[] = { "van gogh", "monet", "renoir" };
	Chave imp = { "Impressionism", NULL, -1 };

	for (int i = 0; i < 3; i++) {
		Chave foco = { "Impressionism", nomes[i], -1 };
		Resultado* r = indice_buscar(f->arvore, NIVEL_ARTISTAS, &foco);
		munit_assert_int(resultado_tamanho(r), ==, 1);
		resultado_liberar(r);

		NoVista v[15];
		munit_assert_int(indice_vista(f->arvore, NIVEL_ARTISTAS, &imp, v, 4), ==, 3);
		munit_assert_string_equal(artista_nome(v[0].item), nomes[i]);
		munit_assert_int(v[0].descendentes, ==, 2);

		for (int p = 0; p < 15; p++) {
			if (v[p].item) {
				munit_assert_string_equal(artista_genero(v[p].item),
							  "Impressionism");
			}
		}
	}
	return MUNIT_OK;
}

static MunitResult test_foco_em_obra(const MunitParameter params[], void* data) {
	(void) params;
	Fixture* f = data;

	Chave par  = { "Impressionism", "monet", -1 };
	Chave foco = { "Impressionism", "monet", 10 };

	Resultado* r = indice_buscar(f->arvore, NIVEL_OBRAS, &foco);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_int(obra_id(resultado_item(r, 0)), ==, 10);
	resultado_liberar(r);

	NoVista v[15];
	munit_assert_int(indice_vista(f->arvore, NIVEL_OBRAS, &par, v, 4), ==, 3);
	munit_assert_int(obra_id(v[0].item), ==, 10);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_indice = {
	"/indice",
	(MunitTest[]) {
		{ .name = "/cmp-obra-recorta-blocos",
		  .test = test_cmp_obra_recorta_blocos },
		{ .name = "/niveis-ignoram-campos-finos",
		  .test = test_niveis_ignoram_campos_finos,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/eds-respondem-igual",
		  .test = test_eds_respondem_igual,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/niveis-em-ordem-de-chave",
		  .test = test_niveis_em_ordem_de_chave,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/nivel-invalido",
		  .test = test_nivel_invalido,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/so-a-arvore-tem-vista",
		  .test = test_so_a_arvore_tem_vista,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/foco-leva-a-raiz-da-vista",
		  .test = test_foco_leva_a_raiz_da_vista,
		  .setup = setup, .tear_down = teardown },
		{ .name = "/foco-em-obra",
		  .test = test_foco_em_obra,
		  .setup = setup, .tear_down = teardown },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_indice_get(void) {
	return &suite_indice;
}
