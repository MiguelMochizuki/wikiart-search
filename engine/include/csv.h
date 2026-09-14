/**
 * csv.h
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD Csv. Carrega e itera sobre os metadados
 *            do WikiArt.
 *
 * Formato esperado (separador ';', com cabeçalho):
 *     id;titulo;artista;genero;ano;caminho
 */
#ifndef CSV_H
#define CSV_H

#include "obra.h"

/* ==============================
 * TADs
 * ============================== */

typedef struct csv_t Csv;

/* ==============================
 * API pública
 * ============================== */

/** Carrega o CSV inteiro em memória
 *
 * Parâmetros:
 * const char* caminho: caminho para o arquivo CSV
 *
 * Retorna Csv*: ponteiro para o Csv alocado, ou NULL em erro
 */
Csv* csv_carregar(const char* caminho);

/** Libera o CSV e todas as Obras contidas nele
 *
 * Parâmetros:
 * Csv* c: ponteiro para o Csv a ser liberado
 */
void csv_liberar(Csv* c);

/* ==============================
 * Acesso
 * ============================== */

int csv_tamanho(const Csv* c);
const Obra* csv_obra(const Csv* c, int indice);

#endif
