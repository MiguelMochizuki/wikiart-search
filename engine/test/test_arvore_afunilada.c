/**
 * test_arvore_afunilada.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Testes unitários do TAD ArvoreAfunilada (splay tree),
 *            com inteiros. As formas esperadas nos testes de rotação
 *            foram calculadas à mão a partir da ordem de inserção.
 */
#include <stdio.h>
#include <stdlib.h>
#include "munit.h"
#include "arvore_afunilada.h"

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

/** Valor do inteiro apontado por um item, ou -1 para NULL */
static int valor(const void* item) {
	return item ? *(const int*) item : -1;
}

/** Monta uma árvore com os valores dados, na ordem do array */
static ArvoreAfunilada* arvore_com(const int* valores, int n) {
	ArvoreAfunilada* a = arvore_afunilada_criar(cmp_int);
	for (int i = 0; i < n; i++) arvore_afunilada_inserir(a, &valores[i]);
	return a;
}

/** Busca um valor exato e devolve o resultado */
static Resultado* buscar_valor(ArvoreAfunilada* a, int v) {
	return arvore_afunilada_buscar(a, cmp_int, &v);
}

/** Confere que o percurso em ordem tem n itens estritamente crescentes */
static void conferir_ordem(ArvoreAfunilada* a, int n) {
	Resultado* r = arvore_afunilada_buscar(a, NULL, NULL);
	munit_assert_int(resultado_tamanho(r), ==, n);
	for (int i = 1; i < n; i++) {
		munit_assert_int(valor(resultado_item(r, i - 1)), <,
				 valor(resultado_item(r, i)));
	}
	resultado_liberar(r);
}

/* Árvore 10, 20, 30, 15. As três primeiras inserções viram uma lista
 * pela esquerda (30 -> 20 -> 10). O 15 entra como filho direito do 10
 * e sobe em zig-zag e depois zig, deixando:
 *
 *            15
 *          /    \
 *        10      30
 *               /
 *             20
 */
static const int FORMA_BASE[] = { 10, 20, 30, 15 };

/* ==============================
 * Testes: criação e inserção
 * ============================== */

static MunitResult test_criar_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	ArvoreAfunilada* a = arvore_afunilada_criar(cmp_int);
	munit_assert_not_null(a);
	munit_assert_int(arvore_afunilada_tamanho(a), ==, 0);
	munit_assert_null(arvore_afunilada_raiz(a));
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_criar_sem_comparador(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	munit_assert_null(arvore_afunilada_criar(NULL));
	return MUNIT_OK;
}

static MunitResult test_liberar_null(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	arvore_afunilada_liberar(NULL);
	return MUNIT_OK;
}

static MunitResult test_inserir_leva_a_raiz(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 50, 30, 80, 40, 10 };
	ArvoreAfunilada* a = arvore_afunilada_criar(cmp_int);

	for (int i = 0; i < 5; i++) {
		arvore_afunilada_inserir(a, &valores[i]);
		munit_assert_ptr_equal(arvore_afunilada_raiz(a), &valores[i]);
		munit_assert_int(arvore_afunilada_tamanho(a), ==, i + 1);
	}
	conferir_ordem(a, 5);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_percurso_nao_mexe_na_forma(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[50];
	for (int i = 0; i < 50; i++) valores[i] = (i * 17) % 50;
	ArvoreAfunilada* a = arvore_com(valores, 50);
	const void* raiz = arvore_afunilada_raiz(a);

	Resultado* r = arvore_afunilada_buscar(a, NULL, NULL);
	munit_assert_int(resultado_tamanho(r), ==, 50);
	for (int i = 0; i < 50; i++) {
		munit_assert_int(valor(resultado_item(r, i)), ==, i);
	}
	munit_assert_long(resultado_comparacoes(r), ==, 0);
	munit_assert_long(resultado_rotacoes(r), ==, 0);
	munit_assert_ptr_equal(arvore_afunilada_raiz(a), raiz);

	resultado_liberar(r);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

/* ==============================
 * Testes: afunilamento
 * ============================== */

static MunitResult test_busca_leva_achado_a_raiz(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[20];
	for (int i = 0; i < 20; i++) valores[i] = (i * 7) % 20;
	ArvoreAfunilada* a = arvore_com(valores, 20);

	for (int alvo = 0; alvo < 20; alvo += 3) {
		Resultado* r = buscar_valor(a, alvo);
		munit_assert_int(resultado_tamanho(r), ==, 1);
		munit_assert_int(valor(resultado_item(r, 0)), ==, alvo);
		munit_assert_int(valor(arvore_afunilada_raiz(a)), ==, alvo);
		resultado_liberar(r);
	}
	conferir_ordem(a, 20);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_zig_zig(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* 1, 2, 3 em ordem: lista pela esquerda, 3 -> 2 -> 1. O 1 está
	 * alinhado com pai e avô: zig-zig, e a lista inverte de lado. */
	int valores[] = { 1, 2, 3 };
	ArvoreAfunilada* a = arvore_com(valores, 3);

	Resultado* r = buscar_valor(a, 1);
	munit_assert_long(resultado_rotacoes(r), ==, 2);
	/* 3 na descida (3, 2, 1) e 1 na coleta (o sucessor, 2, encerra). */
	munit_assert_long(resultado_comparacoes(r), ==, 4);
	resultado_liberar(r);

	NoVista v[7];
	munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v, 3), ==, 3);
	munit_assert_int(valor(v[0].item), ==, 1);
	munit_assert_null(v[1].item);
	munit_assert_int(valor(v[2].item), ==, 2);
	munit_assert_int(valor(v[6].item), ==, 3);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_zig_zag(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Na FORMA_BASE o 20 é filho esquerdo de um filho direito: cotovelo,
	 * zig-zag. Ele sobe com o 15 à esquerda e o 30 à direita. */
	ArvoreAfunilada* a = arvore_com(FORMA_BASE, 4);

	Resultado* r = buscar_valor(a, 20);
	munit_assert_int(resultado_tamanho(r), ==, 1);
	munit_assert_long(resultado_rotacoes(r), ==, 2);
	/* Descida 15, 30, 20; coleta olha o 15 à esquerda e o sucessor 30. */
	munit_assert_long(resultado_comparacoes(r), ==, 5);
	resultado_liberar(r);

	NoVista v[7];
	arvore_afunilada_vista(a, NULL, NULL, v, 3);
	munit_assert_int(valor(v[0].item), ==, 20);
	munit_assert_int(valor(v[1].item), ==, 15);
	munit_assert_int(valor(v[2].item), ==, 30);
	munit_assert_int(valor(v[3].item), ==, 10);
	munit_assert_null(v[4].item);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_raiz_nao_gira(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	ArvoreAfunilada* a = arvore_com(FORMA_BASE, 4);

	Resultado* r = buscar_valor(a, 15);
	munit_assert_long(resultado_rotacoes(r), ==, 0);
	munit_assert_int(valor(arvore_afunilada_raiz(a)), ==, 15);

	resultado_liberar(r);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_busca_sem_achado_afunila_ultimo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Lista 30 -> 20 -> 10. Buscar 5 desce até o 10 e não acha: o 10,
	 * último do caminho, sobe em zig-zig. */
	int valores[] = { 10, 20, 30 };
	ArvoreAfunilada* a = arvore_com(valores, 3);

	Resultado* r = buscar_valor(a, 5);
	munit_assert_int(resultado_tamanho(r), ==, 0);
	munit_assert_long(resultado_rotacoes(r), ==, 2);
	munit_assert_int(valor(arvore_afunilada_raiz(a)), ==, 10);
	resultado_liberar(r);

	conferir_ordem(a, 3);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_busca_em_arvore_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	ArvoreAfunilada* a = arvore_afunilada_criar(cmp_int);
	Resultado* r = buscar_valor(a, 1);

	munit_assert_not_null(r);
	munit_assert_int(resultado_tamanho(r), ==, 0);
	munit_assert_long(resultado_rotacoes(r), ==, 0);

	resultado_liberar(r);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_rotacoes_igualam_profundidade(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Zig-zig e zig-zag sobem dois níveis com duas rotações, zig sobe
	 * um com uma: afunilar um nó de profundidade d custa d rotações.
	 * A vista da árvore inteira dá a profundidade de cada posição. */
	int valores[100];
	for (int i = 0; i < 100; i++) valores[i] = (i * 37) % 100;

	for (int pos = 1; pos < 15; pos++) {
		ArvoreAfunilada* a = arvore_com(valores, 100);
		NoVista v[15];
		arvore_afunilada_vista(a, NULL, NULL, v, 4);
		if (!v[pos].item) {
			arvore_afunilada_liberar(a);
			continue;
		}

		int profundidade = 0;
		for (int p = pos; p > 0; p = (p - 1) / 2) profundidade++;

		Resultado* r = buscar_valor(a, valor(v[pos].item));
		munit_assert_long(resultado_rotacoes(r), ==, profundidade);
		munit_assert_ptr_equal(arvore_afunilada_raiz(a), v[pos].item);

		resultado_liberar(r);
		arvore_afunilada_liberar(a);
	}
	return MUNIT_OK;
}

/* ==============================
 * Testes: intervalos
 * ============================== */

static MunitResult test_buscar_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[30];
	for (int i = 0; i < 30; i++) valores[i] = (i * 11) % 30;
	ArvoreAfunilada* a = arvore_com(valores, 30);

	Faixa f = { 10, 19 };
	Resultado* r = arvore_afunilada_buscar(a, cmp_faixa, &f);

	munit_assert_int(resultado_tamanho(r), ==, 10);
	for (int i = 0; i < 10; i++) {
		munit_assert_int(valor(resultado_item(r, i)), ==, 10 + i);
	}
	/* Quem sobe é o nó mais alto do intervalo, então a raiz é dele. */
	int raiz = valor(arvore_afunilada_raiz(a));
	munit_assert_int(raiz, >=, 10);
	munit_assert_int(raiz, <=, 19);

	resultado_liberar(r);
	conferir_ordem(a, 30);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_buscar_intervalo_vazio(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 1, 5, 9 };
	ArvoreAfunilada* a = arvore_com(valores, 3);

	Faixa f = { 6, 8 };
	Resultado* r = arvore_afunilada_buscar(a, cmp_faixa, &f);
	munit_assert_int(resultado_tamanho(r), ==, 0);

	resultado_liberar(r);
	conferir_ordem(a, 3);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_buscar_repetidos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	int valores[] = { 5, 2, 5, 9, 5, 1 };
	ArvoreAfunilada* a = arvore_com(valores, 6);

	Resultado* r = buscar_valor(a, 5);
	munit_assert_int(resultado_tamanho(r), ==, 3);
	munit_assert_int(valor(arvore_afunilada_raiz(a)), ==, 5);

	resultado_liberar(r);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

/* ==============================
 * Testes: vista
 * ============================== */

static MunitResult test_vista_da_arvore_inteira(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	ArvoreAfunilada* a = arvore_com(FORMA_BASE, 4);

	NoVista v[7];
	munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v, 3), ==, 4);

	munit_assert_int(valor(v[0].item), ==, 15);
	munit_assert_int(valor(v[1].item), ==, 10);
	munit_assert_int(valor(v[2].item), ==, 30);
	munit_assert_null(v[3].item);
	munit_assert_null(v[4].item);
	munit_assert_int(valor(v[5].item), ==, 20);
	munit_assert_null(v[6].item);

	munit_assert_int(v[0].descendentes, ==, 3);
	munit_assert_int(v[1].descendentes, ==, 0);
	munit_assert_int(v[2].descendentes, ==, 1);
	munit_assert_int(v[5].descendentes, ==, 0);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_vista_restrita_ao_intervalo(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	ArvoreAfunilada* a = arvore_com(FORMA_BASE, 4);

	/* Só 20 e 30 estão em [16, 40]. A raiz 15 fica de fora e é pulada:
	 * a vista começa no 30, com o 20 de filho esquerdo. */
	Faixa f = { 16, 40 };
	NoVista v[7];
	munit_assert_int(arvore_afunilada_vista(a, cmp_faixa, &f, v, 3), ==, 2);

	munit_assert_int(valor(v[0].item), ==, 30);
	munit_assert_int(valor(v[1].item), ==, 20);
	munit_assert_null(v[2].item);
	munit_assert_int(v[0].descendentes, ==, 1);

	/* A vista não afunila nada. */
	munit_assert_int(valor(arvore_afunilada_raiz(a)), ==, 15);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_vista_conta_ocultos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Lista de 10 nós pela esquerda: a vista de 2 níveis mostra só dois,
	 * e o de baixo carrega os 8 que ficaram de fora. */
	int valores[10];
	for (int i = 0; i < 10; i++) valores[i] = i;
	ArvoreAfunilada* a = arvore_com(valores, 10);

	NoVista v[3];
	munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v, 2), ==, 10);
	munit_assert_int(valor(v[0].item), ==, 9);
	munit_assert_int(valor(v[1].item), ==, 8);
	munit_assert_int(v[0].descendentes, ==, 9);
	munit_assert_int(v[1].descendentes, ==, 8);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_vista_niveis_invalidos(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	ArvoreAfunilada* a = arvore_com(FORMA_BASE, 4);
	NoVista v[(1 << VISTA_MAX_NIVEIS) - 1];

	munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v, 0), ==, -1);
	munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v,
						VISTA_MAX_NIVEIS + 1), ==, -1);
	munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v,
						VISTA_MAX_NIVEIS), ==, 4);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_vista_de_arvore_vazia(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	ArvoreAfunilada* a = arvore_afunilada_criar(cmp_int);
	NoVista v[7];

	munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v, 3), ==, 0);
	for (int i = 0; i < 7; i++) munit_assert_null(v[i].item);

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

/* ==============================
 * Testes: robustez
 * ============================== */

static MunitResult test_arvore_degenerada_sem_recursao(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	/* Inserir em ordem crescente deixa uma lista de N nós. Percorrer,
	 * afunilar o fundo e liberar precisam funcionar sem recursão. */
	enum { N = 100000 };
	int* valores = malloc(N * sizeof *valores);
	for (int i = 0; i < N; i++) valores[i] = i;
	ArvoreAfunilada* a = arvore_com(valores, N);

	conferir_ordem(a, N);

	Resultado* r = buscar_valor(a, 0);
	munit_assert_long(resultado_rotacoes(r), ==, N - 1);
	munit_assert_int(valor(arvore_afunilada_raiz(a)), ==, 0);
	resultado_liberar(r);

	arvore_afunilada_liberar(a);
	free(valores);
	return MUNIT_OK;
}

static MunitResult test_operacoes_aleatorias_preservam_ordem(const MunitParameter params[], void* data) {
	(void) params;
	(void) data;

	enum { N = 500, OPERACOES = 300 };
	static int valores[N];
	for (int i = 0; i < N; i++) valores[i] = (i * 263) % N * 2;  /* só pares */
	ArvoreAfunilada* a = arvore_com(valores, N);

	srand(7);
	for (int op = 0; op < OPERACOES; op++) {
		int alvo = rand() % (2 * N);
		Resultado* r;

		if (op % 3 == 0) {
			Faixa f = { alvo, alvo + rand() % 40 };
			r = arvore_afunilada_buscar(a, cmp_faixa, &f);
		} else {
			/* Ímpares não existem: metade das buscas exatas falha. */
			r = buscar_valor(a, alvo);
			int achou = (alvo % 2 == 0);
			munit_assert_int(resultado_tamanho(r), ==, achou);
			if (achou) {
				munit_assert_int(valor(arvore_afunilada_raiz(a)), ==, alvo);
			}
		}
		resultado_liberar(r);

		/* Invariantes: ordem em-ordem e contas da vista fechando. */
		conferir_ordem(a, N);

		NoVista v[15];
		munit_assert_int(arvore_afunilada_vista(a, NULL, NULL, v, 4), ==, N);
		munit_assert_int(v[0].descendentes, ==, N - 1);
		for (int i = 0; i < 7; i++) {
			if (!v[i].item) continue;
			int abaixo = 0;
			if (v[2 * i + 1].item) abaixo += v[2 * i + 1].descendentes + 1;
			if (v[2 * i + 2].item) abaixo += v[2 * i + 2].descendentes + 1;
			munit_assert_int(v[i].descendentes, ==, abaixo);
		}
	}

	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}


/** A página é a fatia [offset, offset+limite) do intervalo completo */
static void conferir_pagina(Resultado* completo, Resultado* pag, int offset,
			    int limite) {
	int k = resultado_tamanho(completo);
	int esperado = offset >= k ? 0 : k - offset;
	if (esperado > limite) esperado = limite;

	munit_assert_int(resultado_total(pag), ==, k);
	munit_assert_int(resultado_tamanho(pag), ==, esperado);
	for (int i = 0; i < esperado; i++) {
		munit_assert_ptr_equal(resultado_item(pag, i),
				       resultado_item(completo, offset + i));
	}
}

static MunitResult test_pagina_igual_a_fatia_da_busca(const MunitParameter params[], void* data) {
	(void) params; (void) data;

	enum { N = 300 };
	static int v[N];
	for (int i = 0; i < N; i++) v[i] = (i * 7919) % N;  /* permutação */

	ArvoreAfunilada* a = arvore_com(v, N);
	Faixa f[] = { {0, N - 1}, {40, 199}, {250, 250}, {295, 400}, {-5, -1} };
	int offs[] = { 0, 1, 37, 159, 400 };
	int lims[] = { 0, 1, 10, 500 };

	for (unsigned i = 0; i < sizeof f / sizeof *f; i++) {
		for (unsigned j = 0; j < sizeof offs / sizeof *offs; j++) {
			for (unsigned l = 0; l < sizeof lims / sizeof *lims; l++) {
				Resultado* c = arvore_afunilada_buscar(a, cmp_faixa, &f[i]);
				Resultado* p = arvore_afunilada_buscar_pagina(
					a, cmp_faixa, &f[i], offs[j], lims[l]);
				conferir_pagina(c, p, offs[j], lims[l]);
				resultado_liberar(c);
				resultado_liberar(p);
			}
		}
	}
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_pagina_sem_chave(const MunitParameter params[], void* data) {
	(void) params; (void) data;

	int v[] = { 5, 2, 8, 1, 9, 3 };
	ArvoreAfunilada* a = arvore_com(v, 6);

	Resultado* p = arvore_afunilada_buscar_pagina(a, NULL, NULL, 2, 3);
	munit_assert_int(resultado_total(p), ==, 6);
	munit_assert_int(resultado_tamanho(p), ==, 3);
	munit_assert_int(valor(resultado_item(p, 0)), ==, 3);
	munit_assert_int(valor(resultado_item(p, 2)), ==, 8);
	resultado_liberar(p);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

static MunitResult test_pagina_nao_percorre_o_intervalo(const MunitParameter params[], void* data) {
	(void) params; (void) data;

	enum { N = 2000 };
	static int v[N];
	for (int i = 0; i < N; i++) v[i] = (i * 1237) % N;

	ArvoreAfunilada* a = arvore_com(v, N);
	Faixa tudo = { 0, N - 1 };
	Resultado* c = arvore_afunilada_buscar(a, cmp_faixa, &tudo);
	Resultado* p = arvore_afunilada_buscar_pagina(a, cmp_faixa, &tudo, 500, 20);

	/* A busca completa compara cada item coletado; a página, não. */
	munit_assert_long(resultado_comparacoes(c), >, N);
	munit_assert_long(resultado_comparacoes(p), <, 200);
	resultado_liberar(c);
	resultado_liberar(p);
	arvore_afunilada_liberar(a);
	return MUNIT_OK;
}

/* ==============================
 * Suíte
 * ============================== */

static const MunitSuite suite_arvore_afunilada = {
	"/arvore-afunilada",
	(MunitTest[]) {
		{ .name = "/criar-vazia",
		  .test = test_criar_vazia },
		{ .name = "/criar-sem-comparador",
		  .test = test_criar_sem_comparador },
		{ .name = "/liberar-null",
		  .test = test_liberar_null },
		{ .name = "/inserir-leva-a-raiz",
		  .test = test_inserir_leva_a_raiz },
		{ .name = "/percurso-nao-mexe-na-forma",
		  .test = test_percurso_nao_mexe_na_forma },
		{ .name = "/busca-leva-achado-a-raiz",
		  .test = test_busca_leva_achado_a_raiz },
		{ .name = "/zig-zig",
		  .test = test_zig_zig },
		{ .name = "/zig-zag",
		  .test = test_zig_zag },
		{ .name = "/raiz-nao-gira",
		  .test = test_raiz_nao_gira },
		{ .name = "/busca-sem-achado-afunila-ultimo",
		  .test = test_busca_sem_achado_afunila_ultimo },
		{ .name = "/busca-em-arvore-vazia",
		  .test = test_busca_em_arvore_vazia },
		{ .name = "/rotacoes-igualam-profundidade",
		  .test = test_rotacoes_igualam_profundidade },
		{ .name = "/buscar-intervalo",
		  .test = test_buscar_intervalo },
		{ .name = "/buscar-intervalo-vazio",
		  .test = test_buscar_intervalo_vazio },
		{ .name = "/buscar-repetidos",
		  .test = test_buscar_repetidos },
		{ .name = "/vista-da-arvore-inteira",
		  .test = test_vista_da_arvore_inteira },
		{ .name = "/vista-restrita-ao-intervalo",
		  .test = test_vista_restrita_ao_intervalo },
		{ .name = "/vista-conta-ocultos",
		  .test = test_vista_conta_ocultos },
		{ .name = "/vista-niveis-invalidos",
		  .test = test_vista_niveis_invalidos },
		{ .name = "/vista-de-arvore-vazia",
		  .test = test_vista_de_arvore_vazia },
		{ .name = "/arvore-degenerada-sem-recursao",
		  .test = test_arvore_degenerada_sem_recursao },
		{ .name = "/operacoes-aleatorias-preservam-ordem",
		  .test = test_operacoes_aleatorias_preservam_ordem },
		{ .name = "/pagina-igual-a-fatia-da-busca",
		  .test = test_pagina_igual_a_fatia_da_busca },
		{ .name = "/pagina-sem-chave",
		  .test = test_pagina_sem_chave },
		{ .name = "/pagina-nao-percorre-o-intervalo",
		  .test = test_pagina_nao_percorre_o_intervalo },
		{ .name = NULL }
	},
	NULL, 1, MUNIT_SUITE_OPTION_NONE
};

const MunitSuite* suite_arvore_afunilada_get(void) {
	return &suite_arvore_afunilada;
}
