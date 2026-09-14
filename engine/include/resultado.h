/**
 * resultado.h
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD Resultado. Encapsula o retorno de uma busca
 *            com métricas de tempo e comparações.
 */
#ifndef RESULTADO_H
#define RESULTADO_H

#include "obra.h"

/* ==============================
 * TADs
 * ============================== */

typedef struct resultado_t Resultado;

/* ==============================
 * API pública
 * ============================== */

/** Cria um resultado com capacidade inicial
 *
 * Parâmetros:
 * int capacidade_inicial: número estimado de itens (mínimo 1)
 *
 * Retorna Resultado*: ponteiro para o resultado alocado, ou NULL em erro
 */
Resultado* resultado_criar(int capacidade_inicial);

/** Libera o resultado e o array interno (não libera as Obras apontadas)
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado a ser liberado
 */
void resultado_liberar(Resultado* r);

/** Adiciona uma obra ao resultado
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * const Obra* o: ponteiro para obra (não é copiada, só referenciada)
 */
void resultado_adicionar(Resultado* r, const Obra* o);

/** Define as métricas do resultado
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * double tempo_ms: tempo gasto na busca, em milissegundos
 * long comparacoes: número de comparações realizadas
 */
void resultado_set_metricas(Resultado* r, double tempo_ms, long comparacoes);

/* ==============================
 * Getters
 * ============================== */

int resultado_tamanho(const Resultado* r);
const Obra* resultado_item(const Resultado* r, int indice);
double resultado_tempo_ms(const Resultado* r);
long resultado_comparacoes(const Resultado* r);

#endif
