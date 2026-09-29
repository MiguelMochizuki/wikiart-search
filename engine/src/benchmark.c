/**
 * benchmark.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do módulo de benchmark. Mede tempo,
 *            comparações e rotações das buscas de cada nível da
 *            navegação sobre qualquer Buscador.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "benchmark.h"
#include "catalogo.h"
#include "indice.h"

/* ==============================
 * Constantes
 * ============================== */

/* Semente das consultas. Reiniciada a cada ED, para que todas respondam
 * exatamente à mesma sequência. */
#define SEMENTE_CONSULTAS 42
#define PAGINA_OBRAS 30  /* itens de uma página de obras */

/* Consultas com localidade: uma "sessão de navegação" que se demora em
 * poucas obras. QUENTES obras ficam em foco, 90% das consultas caem nelas
 * e as demais são sorteadas em todo o acervo; o foco muda a cada
 * TROCA_FOCO consultas. */
#define QUENTES 8
#define PCT_QUENTE 90
#define TROCA_FOCO 250

/* ==============================
 * Tipos internos
 * ============================== */

/** Como as consultas escolhem a obra de que tiram a chave
 *
 * ALEATORIAS: cada consulta sorteia uma obra em todo o acervo.
 * LOCAIS: a maior parte das consultas volta a um pequeno foco.
 */
typedef enum { ALEATORIAS, LOCAIS } Consultas;

/** Qual parte das obras de um artista a busca pede
 *
 * TODAS: o intervalo inteiro. PRIMEIRA: a primeira página.
 * QUALQUER: uma página em posição sorteada dentro do intervalo.
 */
typedef enum { TODAS, PRIMEIRA, QUALQUER } Pagina;

/** Médias de uma operação medida
 *
 * Campos:
 * ms: tempo médio por busca, em milissegundos
 * comparacoes: comparações médias por busca
 * rotacoes: rotações médias por busca
 */
typedef struct {
	double ms;
	double comparacoes;
	double rotacoes;
} Medicao;

/* ==============================
 * Helpers internos
 * ============================== */

/** Retorna o tempo atual em milissegundos (relógio monotônico)
 *
 * Retorna double: tempo em ms desde um ponto arbitrário
 */
static double agora_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/** Sorteia a obra de que sai a chave da próxima consulta
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * Consultas modo: aleatórias ou com localidade
 * int quentes[]: foco atual (QUENTES posições), usado no modo LOCAIS
 * int i: número da consulta, para trocar o foco a cada TROCA_FOCO
 *
 * Retorna const Obra*: obra sorteada
 */
static const Obra* sortear_obra(const Csv* csv, Consultas modo,
				int quentes[], int i) {
	int n = csv_tamanho(csv);

	if (modo == LOCAIS) {
		if (i % TROCA_FOCO == 0) {
			for (int q = 0; q < QUENTES; q++) quentes[q] = rand() % n;
		}
		if (rand() % 100 < PCT_QUENTE) {
			return csv_obra(csv, quentes[rand() % QUENTES]);
		}
	}
	return csv_obra(csv, rand() % n);
}

/** Quantas obras um artista tem num gênero
 *
 * Parâmetros:
 * const Catalogo* cat: catálogo agregado do CSV
 * const Obra* o: obra que dá o gênero e o artista
 *
 * Retorna int: obras do par (gênero, artista)
 */
static int obras_do_par(const Catalogo* cat, const Obra* o) {
	for (int i = 0; i < catalogo_n_artistas(cat); i++) {
		const Artista* a = catalogo_artista(cat, i);
		if (strcmp(artista_genero(a), obra_genero(o)) == 0 &&
		    strcmp(artista_nome(a), obra_artista(o)) == 0) {
			return artista_obras(a);
		}
	}
	return 0;
}

/** Mede o custo médio de N buscas num nível
 *
 * Cada busca escolhe uma obra real (ver Consultas) e usa a chave dela
 * até o nível medido: o gênero no nível 1 (o próprio gênero) e no 2 (os
 * artistas dele), gênero e artista no 3 (as obras do par). Sortear pela
 * obra dá a cada gênero e artista um peso proporcional ao acervo, como
 * numa navegação real, e garante resultado não vazio.
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Catalogo* cat: catálogo, para o tamanho dos blocos de obras
 * Indice* ix: índice montado
 * Nivel nivel: nível medido
 * int n_buscas: número de consultas
 * Consultas modo: aleatórias ou com localidade
 * Pagina pagina: parte das obras pedida (só vale no nível de obras)
 *
 * Retorna Medicao: médias da operação
 */
static Medicao medir(const Csv* csv, const Catalogo* cat, Indice* ix,
		     Nivel nivel, int n_buscas, Consultas modo, Pagina pagina) {
	double t_total = 0.0;
	long comp_total = 0;
	long rot_total = 0;
	int quentes[QUENTES] = { 0 };

	for (int i = 0; i < n_buscas; i++) {
		const Obra* o = sortear_obra(csv, modo, quentes, i);
		Chave k = {
			obra_genero(o),
			nivel == NIVEL_OBRAS ? obra_artista(o) : NULL,
			-1
		};

		Resultado* r;
		if (nivel != NIVEL_OBRAS || pagina == TODAS) {
			r = indice_buscar(ix, nivel, &k);
		} else {
			int offset = 0;
			if (pagina == QUALQUER) {
				int bloco = obras_do_par(cat, o);
				offset = bloco > 0 ? rand() % bloco : 0;
			}
			r = indice_buscar_pagina(ix, nivel, &k, offset, PAGINA_OBRAS);
		}

		t_total    += resultado_tempo_ms(r);
		comp_total += resultado_comparacoes(r);
		rot_total  += resultado_rotacoes(r);

		resultado_liberar(r);
	}

	Medicao m = {
		t_total / n_buscas,
		(double) comp_total / n_buscas,
		(double) rot_total / n_buscas
	};
	return m;
}

/** Escreve uma linha do CSV de saída */
static void imprimir(const char* ed, const char* operacao, int n_buscas,
		     int n, double insercao_ms, Medicao m) {
	printf("%s,%s,%d,%d,%.4f,%.6f,%.2f,%.2f\n",
	       ed, operacao, n_buscas, n, insercao_ms,
	       m.ms, m.comparacoes, m.rotacoes);
}

/* ==============================
 * Benchmark de um Buscador
 * ============================== */

/** Executa o benchmark de uma estrutura carregada com um catálogo
 *
 * A inserção medida é a carga dos três níveis (gêneros, artistas por
 * gênero e obras), na ordem de carga do catálogo. Cada operação
 * recomeça a sequência de consultas da mesma semente, então todas as
 * estruturas respondem exatamente às mesmas consultas.
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Catalogo* cat: catálogo com a ordem de carga a testar
 * const Buscador* b: estrutura a medir
 * const char* rotulo: nome da estrutura na saída
 * int n_buscas: número de consultas por operação
 * int completo: 1 para medir também localidade e páginas em posição
 *               sorteada; 0 só para as quatro operações básicas
 */
static void benchmark_um(const Csv* csv, const Catalogo* cat,
			 const Buscador* b, const char* rotulo,
			 int n_buscas, int completo) {
	int n = csv_tamanho(csv);

	double t0 = agora_ms();
	Indice* ix = indice_montar(b, cat);
	double insercao_ms = agora_ms() - t0;

	if (!ix) {
		fprintf(stderr, "Falha ao criar %s.\n", rotulo);
		return;
	}

	static const struct {
		const char* sufixo;
		Consultas modo;
	} CARGAS[] = { { "", ALEATORIAS }, { "_local", LOCAIS } };

	for (int c = 0; c < (completo ? 2 : 1); c++) {
		char nome[64];
		const struct { const char* op; Nivel nivel; Pagina pagina; } OPS[] = {
			{ "buscar_genero",          NIVEL_GENEROS,  TODAS },
			{ "buscar_artistas_genero", NIVEL_ARTISTAS, TODAS },
			{ "buscar_obras_artista",   NIVEL_OBRAS,    TODAS },
			{ "buscar_obras_pagina",    NIVEL_OBRAS,    PRIMEIRA },
			{ "buscar_obras_pagina_qualquer", NIVEL_OBRAS, QUALQUER }
		};
		int n_ops = (completo && c == 0) ? 5 : (completo ? 4 : 4);

		for (int o = 0; o < n_ops; o++) {
			srand(SEMENTE_CONSULTAS);
			Medicao m = medir(csv, cat, ix, OPS[o].nivel, n_buscas,
					  CARGAS[c].modo, OPS[o].pagina);
			snprintf(nome, sizeof nome, "%s%s", OPS[o].op,
				 CARGAS[c].sufixo);
			imprimir(rotulo, nome, n_buscas, n, insercao_ms, m);
		}
	}

	indice_liberar(ix);
}

/* ==============================
 * API pública
 * ============================== */

/** Roda o benchmark para cada Buscador do array sobre o CSV carregado
 *
 * Cada estrutura é medida com a carga embaralhada (a do sistema) e com a
 * carga clássica, na ordem das chaves, que sai com o sufixo
 * _carga_ordenada.
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Buscador* buscadores[]: array terminado em NULL
 * int n_buscas: número de consultas aleatórias por operação
 */
void benchmark_rodar(const Csv* csv,
		     const Buscador* buscadores[],
		     int n_buscas) {
	if (csv_tamanho(csv) == 0) {
		fprintf(stderr, "CSV vazio, nada a fazer.\n");
		return;
	}
	if (n_buscas < 1) {
		fprintf(stderr, "Numero de buscas invalido.\n");
		return;
	}

	Catalogo* cat = catalogo_montar(csv);
	Catalogo* cat_ord = catalogo_montar_ordenado(csv);
	if (!cat || !cat_ord) {
		fprintf(stderr, "Falha ao montar o catalogo.\n");
		catalogo_liberar(cat);
		catalogo_liberar(cat_ord);
		return;
	}

	printf("estrutura,operacao,consultas,total_elementos,"
	       "insercao_ms,media_ms,media_comparacoes,media_rotacoes\n");

	for (int i = 0; buscadores[i] != NULL; i++) {
		char rotulo[64];
		benchmark_um(csv, cat, buscadores[i], buscadores[i]->nome,
			     n_buscas, 1);
		snprintf(rotulo, sizeof rotulo, "%s_carga_ordenada",
			 buscadores[i]->nome);
		benchmark_um(csv, cat_ord, buscadores[i], rotulo, n_buscas, 0);
	}

	catalogo_liberar(cat);
	catalogo_liberar(cat_ord);
}
