/**
 * server.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do servidor HTTP que expõe a API REST da engine.
 */
#ifndef SERVER_H
#define SERVER_H

#include "csv.h"

/** Inicia o servidor HTTP
 *
 * Monta o catálogo e um Indice por ED, escuta na porta e atende as
 * requisições uma a uma. Bloqueia até ser interrompido (SIGINT).
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 * int porta: porta TCP onde escutar
 *
 * Retorna int: 1 se a montagem ou o socket falhar (não retorna em
 *              funcionamento normal)
 */
int server_iniciar(const Csv* csv, int porta);

#endif
