/**
 * benchmark.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do módulo de benchmark. Mede tempo e comparações
 *            de buscas sobre a SkipList.
 */
#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "csv.h"

/** Roda o benchmark completo sobre um CSV carregado
 *
 * Parâmetros:
 * Csv* csv: metadados carregados
 * int n_buscas: número de consultas aleatórias por operação
 */
void benchmark_rodar(const Csv* csv, int n_buscas);

#endif
