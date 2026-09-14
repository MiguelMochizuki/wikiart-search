/**
 * buscador.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Interface comum para estruturas de busca plugáveis
 *            no benchmark. Cada ED expõe uma instância de Buscador.
 */
#ifndef BUSCADOR_H
#define BUSCADOR_H

#include "obra.h"
#include "resultado.h"

/* ==============================
 * TADs
 * ============================== */

/** Vtable que descreve uma ED de busca
 *
 * Campos:
 * nome: identificador curto (usado no CSV de saída)
 * criar: aloca e retorna a estrutura vazia
 * liberar: destrói a estrutura e seus nós internos
 * inserir: adiciona uma obra à estrutura
 * buscar_artista: busca por artista, retorna Resultado
 * buscar_genero: busca por gênero, retorna Resultado
 */
typedef struct buscador_t {
	const char* nome;
	void*       (*criar)(void);
	void        (*liberar)(void*);
	void        (*inserir)(void*, const Obra*);
	Resultado*  (*buscar_artista)(const void*, const char*);
	Resultado*  (*buscar_genero)(const void*, const char*);
} Buscador;

/* ==============================
 * Instâncias
 * ============================== */

extern const Buscador BUSCADOR_SKIP_LIST;

#endif
