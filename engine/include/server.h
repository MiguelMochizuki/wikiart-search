/**
 * server.h
 * Descrição: Servidor HTTP em C11 para expor os endpoints REST da engine.
 */
#ifndef SERVER_H
#define SERVER_H

#include "csv.h"

/**
 * Inicia o servidor HTTP escutando na porta especificada.
 * Bloqueia a execução atendendo requisições até ser interrompido (SIGINT).
 */
int server_iniciar(const Csv* csv, int porta);

#endif