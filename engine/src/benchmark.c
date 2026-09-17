/**
 * benchmark.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do módulo de benchmark. Mede tempo e
 *            comparações de buscas sobre qualquer Buscador.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "benchmark.h"

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

/* ==============================
 * Medições sobre um Buscador
 * ============================== */

/** Mede o tempo de inserção de todas as obras do CSV
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Buscador* b: estrutura a popular
 *
 * Retorna double: tempo gasto na inserção, em milissegundos
 */
static double medir_insercao(const Csv* csv, const Buscador* b, void* instancia) {
	int n = csv_tamanho(csv);

	double t0 = agora_ms();
	for (int i = 0; i < n; i++) {
		b->inserir(instancia, csv_obra(csv, i));
	}
	return agora_ms() - t0;
}

/** Mede o custo médio de N buscas por artista
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Buscador* b: estrutura populada
 * void* instancia: instância da estrutura
 * int n_buscas: número de consultas
 * double* media_ms_out: saída para tempo médio
 * double* media_comp_out: saída para comparações médias
 */
static void medir_buscar_artista(const Csv* csv, const Buscador* b,
				 void* instancia, int n_buscas,
				 double* media_ms_out,
				 double* media_comp_out) {
	int n = csv_tamanho(csv);
	long comp_total = 0;
	double t_total = 0.0;

	for (int i = 0; i < n_buscas; i++) {
		const Obra* o = csv_obra(csv, rand() % n);
		Resultado* r = b->buscar_artista(instancia, obra_artista(o));

		t_total    += resultado_tempo_ms(r);
		comp_total += resultado_comparacoes(r);

		resultado_liberar(r);
	}

	*media_ms_out   = t_total / n_buscas;
	*media_comp_out = (double) comp_total / n_buscas;
}

/** Mede o custo médio de N buscas por gênero
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Buscador* b: estrutura populada
 * void* instancia: instância da estrutura
 * int n_buscas: número de consultas
 * double* media_ms_out: saída para tempo médio
 * double* media_comp_out: saída para comparações médias
 */
static void medir_buscar_genero(const Csv* csv, const Buscador* b,
				void* instancia, int n_buscas,
				double* media_ms_out,
				double* media_comp_out) {
	int n = csv_tamanho(csv);
	long comp_total = 0;
	double t_total = 0.0;

	for (int i = 0; i < n_buscas; i++) {
		const Obra* o = csv_obra(csv, rand() % n);
		Resultado* r = b->buscar_genero(instancia, obra_genero(o));

		t_total    += resultado_tempo_ms(r);
		comp_total += resultado_comparacoes(r);

		resultado_liberar(r);
	}

	*media_ms_out   = t_total / n_buscas;
	*media_comp_out = (double) comp_total / n_buscas;
}

/** Mede o custo médio de N buscas por gênero e artista
 *
 * Sorteia uma obra real e usa o par (gênero, artista) dela, o que
 * garante resultado não vazio e reproduz a consulta do último nível
 * da navegação.
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Buscador* b: estrutura populada
 * void* instancia: instância da estrutura
 * int n_buscas: número de consultas
 * double* media_ms_out: saída para tempo médio
 * double* media_comp_out: saída para comparações médias
 */
static void medir_buscar_genero_artista(const Csv* csv, const Buscador* b,
					void* instancia, int n_buscas,
					double* media_ms_out,
					double* media_comp_out) {
	int n = csv_tamanho(csv);
	long comp_total = 0;
	double t_total = 0.0;

	for (int i = 0; i < n_buscas; i++) {
		const Obra* o = csv_obra(csv, rand() % n);
		Resultado* r = b->buscar_genero_artista(instancia,
						        obra_genero(o),
						        obra_artista(o));

		t_total    += resultado_tempo_ms(r);
		comp_total += resultado_comparacoes(r);

		resultado_liberar(r);
	}

	*media_ms_out   = t_total / n_buscas;
	*media_comp_out = (double) comp_total / n_buscas;
}

/* ==============================
 * Benchmark de um Buscador
 * ============================== */

/** Executa o benchmark completo para uma estrutura
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Buscador* b: estrutura a medir
 * int n_buscas: número de consultas por operação
 */
static void benchmark_um(const Csv* csv, const Buscador* b, int n_buscas) {
	int n = csv_tamanho(csv);

	void* instancia = b->criar();
	if (!instancia) {
		fprintf(stderr, "Falha ao criar %s.\n", b->nome);
		return;
	}

	double insercao_ms = medir_insercao(csv, b, instancia);

	double ms_artista, comp_artista;
	double ms_genero,  comp_genero;
	double ms_gen_art, comp_gen_art;

	medir_buscar_artista(csv, b, instancia, n_buscas,
			     &ms_artista, &comp_artista);
	medir_buscar_genero (csv, b, instancia, n_buscas,
			     &ms_genero, &comp_genero);
	medir_buscar_genero_artista(csv, b, instancia, n_buscas,
				    &ms_gen_art, &comp_gen_art);

	b->liberar(instancia);

	printf("%s,buscar_artista,%d,%d,%.4f,%.6f,%.2f\n",
	       b->nome, n_buscas, n, insercao_ms, ms_artista, comp_artista);

	printf("%s,buscar_genero,%d,%d,%.4f,%.6f,%.2f\n",
	       b->nome, n_buscas, n, insercao_ms, ms_genero, comp_genero);

	printf("%s,buscar_genero_artista,%d,%d,%.4f,%.6f,%.2f\n",
	       b->nome, n_buscas, n, insercao_ms, ms_gen_art, comp_gen_art);
}

/* ==============================
 * API pública
 * ============================== */

/** Roda o benchmark para cada Buscador do array sobre o CSV carregado
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

	srand(42);  /* semente fixa para reprodutibilidade */

	printf("estrutura,operacao,consultas,total_elementos,"
	       "insercao_ms,media_ms,media_comparacoes\n");

	for (int i = 0; buscadores[i] != NULL; i++) {
		benchmark_um(csv, buscadores[i], n_buscas);
	}
}
