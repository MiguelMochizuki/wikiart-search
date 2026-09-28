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

/* ==============================
 * Tipos internos
 * ============================== */

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

/** Mede o custo médio de N buscas num nível
 *
 * Cada busca sorteia uma obra real e usa a chave dela até o nível
 * medido: o gênero no nível 1 (o próprio gênero) e no 2 (os artistas
 * dele), gênero e artista no 3 (as obras do par). Sortear pela obra dá
 * a cada gênero e artista um peso proporcional ao acervo, como numa
 * navegação real, e garante resultado não vazio.
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * Indice* ix: índice montado
 * Nivel nivel: nível medido
 * int n_buscas: número de consultas
 *
 * Retorna Medicao: médias da operação
 */
static Medicao medir(const Csv* csv, Indice* ix, Nivel nivel, int n_buscas) {
	int n = csv_tamanho(csv);
	double t_total = 0.0;
	long comp_total = 0;
	long rot_total = 0;

	for (int i = 0; i < n_buscas; i++) {
		const Obra* o = csv_obra(csv, rand() % n);
		Chave k = {
			obra_genero(o),
			nivel == NIVEL_OBRAS ? obra_artista(o) : NULL,
			-1
		};
		Resultado* r = indice_buscar(ix, nivel, &k);

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

/** Executa o benchmark completo para uma estrutura
 *
 * A inserção medida é a carga dos três níveis (gêneros, artistas por
 * gênero e obras), na ordem de carga do catálogo.
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Catalogo* cat: catálogo agregado do CSV
 * const Buscador* b: estrutura a medir
 * int n_buscas: número de consultas por operação
 */
static void benchmark_um(const Csv* csv, const Catalogo* cat,
			 const Buscador* b, int n_buscas) {
	int n = csv_tamanho(csv);

	double t0 = agora_ms();
	Indice* ix = indice_montar(b, cat);
	double insercao_ms = agora_ms() - t0;

	if (!ix) {
		fprintf(stderr, "Falha ao criar %s.\n", b->nome);
		return;
	}

	srand(SEMENTE_CONSULTAS);
	Medicao genero   = medir(csv, ix, NIVEL_GENEROS,  n_buscas);
	Medicao artistas = medir(csv, ix, NIVEL_ARTISTAS, n_buscas);
	Medicao obras    = medir(csv, ix, NIVEL_OBRAS,    n_buscas);

	indice_liberar(ix);

	imprimir(b->nome, "buscar_genero",           n_buscas, n, insercao_ms, genero);
	imprimir(b->nome, "buscar_artistas_genero",  n_buscas, n, insercao_ms, artistas);
	imprimir(b->nome, "buscar_obras_artista",    n_buscas, n, insercao_ms, obras);
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
	if (n_buscas < 1) {
		fprintf(stderr, "Numero de buscas invalido.\n");
		return;
	}

	Catalogo* cat = catalogo_montar(csv);
	if (!cat) {
		fprintf(stderr, "Falha ao montar o catalogo.\n");
		return;
	}

	printf("estrutura,operacao,consultas,total_elementos,"
	       "insercao_ms,media_ms,media_comparacoes,media_rotacoes\n");

	for (int i = 0; buscadores[i] != NULL; i++) {
		benchmark_um(csv, cat, buscadores[i], n_buscas);
	}

	catalogo_liberar(cat);
}
