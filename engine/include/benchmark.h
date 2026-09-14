/**
 * benchmark.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do módulo de benchmark. Mede tempo e comparações
 *            de buscas sobre estruturas plugáveis via Buscador.
 */
#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "csv.h"
#include "buscador.h"

/** Roda o benchmark para cada Buscador do array sobre o CSV carregado
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * const Buscador* buscadores[]: array terminado em NULL
 * int n_buscas: número de consultas aleatórias por operação
 */
void benchmark_rodar(const Csv* csv,
		     const Buscador* buscadores[],
		     int n_buscas);

#endif
