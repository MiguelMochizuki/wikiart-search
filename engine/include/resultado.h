/**
 * resultado.h
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD Resultado. Encapsula o retorno de uma busca
 *            com métricas de tempo, comparações e rotações.
 *
 * Os itens são ponteiros opacos: o mesmo Resultado serve às buscas de
 * gêneros, de artistas e de obras. Quem lê sabe o tipo pelo nível que
 * consultou.
 */
#ifndef RESULTADO_H
#define RESULTADO_H

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

/** Libera o resultado e o array interno (não libera os itens apontados)
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado a ser liberado
 */
void resultado_liberar(Resultado* r);

/** Adiciona um item ao resultado
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * const void* item: ponteiro para o item (não é copiado, só referenciado)
 */
void resultado_adicionar(Resultado* r, const void* item);

/** Define as métricas do resultado
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * double tempo_ms: tempo gasto na busca, em milissegundos
 * long comparacoes: número de comparações realizadas
 */
void resultado_set_metricas(Resultado* r, double tempo_ms, long comparacoes);

/** Define quantas rotações a busca fez na estrutura
 *
 * Só as EDs que se reorganizam ao serem consultadas giram nós; nas
 * demais o valor fica em 0.
 *
 * Parâmetros:
 * Resultado* r: ponteiro para resultado
 * long rotacoes: número de rotações realizadas
 */
void resultado_set_rotacoes(Resultado* r, long rotacoes);

/* ==============================
 * Getters
 * ============================== */

int resultado_tamanho(const Resultado* r);
const void* resultado_item(const Resultado* r, int indice);
double resultado_tempo_ms(const Resultado* r);
long resultado_comparacoes(const Resultado* r);
long resultado_rotacoes(const Resultado* r);

#endif
