/**
 * benchmark.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do módulo de benchmark. Mede tempo e
 *            comparações de buscas sobre a SkipList.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "benchmark.h"
#include "skip_list.h"

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

/** Mede o tempo total de inserção de todas as obras numa SkipList
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * SkipList* sl: lista vazia onde inserir
 *
 * Retorna double: tempo gasto na inserção, em milissegundos
 */
static double medir_insercao(const Csv* csv, SkipList* sl) {
	int n = csv_tamanho(csv);

	double t0 = agora_ms();
	for (int i = 0; i < n; i++) {
		skip_list_inserir(sl, csv_obra(csv, i));
	}
	return agora_ms() - t0;
}

/* ==============================
 * Benchmark de busca por artista
 * ============================== */

/** Mede N buscas por artista, sorteando obras do CSV
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const SkipList* sl: lista populada
 * int n_buscas: número de consultas
 * double* media_ms_out: ponteiro de saída para tempo médio
 * double* media_comp_out: ponteiro de saída para comparações médias
 */
static void medir_buscar_artista(const Csv* csv, const SkipList* sl,
				 int n_buscas,
				 double* media_ms_out,
				 double* media_comp_out) {
	int n = csv_tamanho(csv);
	long comp_total = 0;
	double t_total = 0.0;

	for (int i = 0; i < n_buscas; i++) {
		const Obra* o = csv_obra(csv, rand() % n);
		Resultado* r = skip_list_buscar_artista(sl, obra_artista(o));

		t_total    += resultado_tempo_ms(r);
		comp_total += resultado_comparacoes(r);

		resultado_liberar(r);
	}

	*media_ms_out   = t_total / n_buscas;
	*media_comp_out = (double) comp_total / n_buscas;
}

/** Mede N buscas por gênero, sorteando obras do CSV
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const SkipList* sl: lista populada
 * int n_buscas: número de consultas
 * double* media_ms_out: ponteiro de saída para tempo médio
 * double* media_comp_out: ponteiro de saída para comparações médias
 */
static void medir_buscar_genero(const Csv* csv, const SkipList* sl,
				int n_buscas,
				double* media_ms_out,
				double* media_comp_out) {
	int n = csv_tamanho(csv);
	long comp_total = 0;
	double t_total = 0.0;

	for (int i = 0; i < n_buscas; i++) {
		const Obra* o = csv_obra(csv, rand() % n);
		Resultado* r = skip_list_buscar_genero(sl, obra_genero(o));

		t_total    += resultado_tempo_ms(r);
		comp_total += resultado_comparacoes(r);

		resultado_liberar(r);
	}

	*media_ms_out   = t_total / n_buscas;
	*media_comp_out = (double) comp_total / n_buscas;
}

/* ==============================
 * API pública
 * ============================== */

/** Roda o benchmark completo sobre um CSV carregado
 *
 * Parâmetros:
 * Csv* csv: metadados carregados
 * int n_buscas: número de consultas aleatórias por operação
 */
void benchmark_rodar(const Csv* csv, int n_buscas) {
	int n = csv_tamanho(csv);
	if (n == 0) {
		fprintf(stderr, "CSV vazio, nada a fazer.\n");
		return;
	}

	srand(42);  /* semente fixa para reprodutibilidade */

	SkipList* sl = skip_list_criar();
	if (!sl) {
		fprintf(stderr, "Falha ao criar SkipList.\n");
		return;
	}

	double insercao_ms = medir_insercao(csv, sl);

	double ms_artista, comp_artista;
	double ms_genero,  comp_genero;

	medir_buscar_artista(csv, sl, n_buscas, &ms_artista, &comp_artista);
	medir_buscar_genero (csv, sl, n_buscas, &ms_genero,  &comp_genero);

	skip_list_liberar(sl);

	/* Cabeçalho */
	printf("estrutura,operacao,consultas,total_elementos,"
	       "insercao_ms,media_ms,media_comparacoes\n");

	printf("skip_list,buscar_artista,%d,%d,%.4f,%.6f,%.2f\n",
	       n_buscas, n, insercao_ms, ms_artista, comp_artista);

	printf("skip_list,buscar_genero,%d,%d,%.4f,%.6f,%.2f\n",
	       n_buscas, n, insercao_ms, ms_genero, comp_genero);
}
