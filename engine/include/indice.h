/**
 * indice.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD Indice. Monta, para uma ED, as três
 *            estruturas da navegação estilo -> artista -> obras e
 *            responde às buscas de cada nível.
 *
 * Nível 1, gêneros: ordenados pelo nome.
 * Nível 2, artistas: ordenados por (gênero, artista). Os artistas de um
 *          gênero ficam contíguos, o que permite buscá-los pelo gênero.
 * Nível 3, obras: ordenadas por (gênero, artista, id). O id desempata
 *          as obras de um mesmo par e deixa cada chave única, o que a
 *          árvore afunilada precisa para levar uma obra específica ao
 *          topo.
 */
#ifndef INDICE_H
#define INDICE_H

#include "buscador.h"
#include "catalogo.h"

/* ==============================
 * Tipos
 * ============================== */

/** Os três níveis da navegação */
typedef enum {
	NIVEL_GENEROS,
	NIVEL_ARTISTAS,
	NIVEL_OBRAS,
	N_NIVEIS
} Nivel;

/** Chave de busca comum aos três níveis
 *
 * Campo em aberto casa com qualquer valor, então uma chave parcial
 * recorta um bloco: {g, NULL, -1} no nível 2 são todos os artistas de
 * g; {g, a, -1} no nível 3, todas as obras de a em g. Cada nível
 * ignora os campos mais finos que ele: no nível 1 só vale o gênero, no
 * 2, gênero e artista.
 *
 * Campos:
 * genero: gênero procurado, ou NULL para qualquer um
 * artista: artista procurado, ou NULL para qualquer um
 * id: id da obra procurada, ou negativo para qualquer uma
 */
typedef struct {
	const char* genero;
	const char* artista;
	int id;
} Chave;

typedef struct indice_t Indice;

/* ==============================
 * Comparadores item x Chave
 * ============================== */

int indice_cmp_genero(const void* item, const void* chave);
int indice_cmp_artista(const void* item, const void* chave);
int indice_cmp_obra(const void* item, const void* chave);

/* ==============================
 * API pública
 * ============================== */

/** Cria as três estruturas de uma ED e carrega o catálogo nelas
 *
 * Parâmetros:
 * const Buscador* ed: ED a usar nos três níveis
 * const Catalogo* c: itens a carregar, na ordem de carga do catálogo
 *
 * Retorna Indice*: índice montado, ou NULL em erro
 */
Indice* indice_montar(const Buscador* ed, const Catalogo* c);

/** Libera as estruturas do índice. Não libera os itens.
 *
 * Parâmetros:
 * Indice* ix: ponteiro para o índice
 */
void indice_liberar(Indice* ix);

/** ED usada pelo índice
 *
 * Parâmetros:
 * const Indice* ix: ponteiro para o índice
 *
 * Retorna const Buscador*: vtable da ED
 */
const Buscador* indice_ed(const Indice* ix);

/** Busca o intervalo da chave num nível
 *
 * Parâmetros:
 * Indice* ix: ponteiro para o índice (a busca pode reorganizar a ED)
 * Nivel nivel: nível consultado
 * const Chave* chave: chave procurada, ou NULL para listar o nível todo
 *
 * Retorna Resultado*: itens do nível (Genero*, Artista* ou Obra*)
 */
Resultado* indice_buscar(Indice* ix, Nivel nivel, const Chave* chave);

/** Busca uma página do intervalo da chave num nível
 *
 * Parâmetros:
 * Indice* ix: ponteiro para o índice (a busca pode reorganizar a ED)
 * Nivel nivel: nível consultado
 * const Chave* chave: chave procurada, ou NULL para o nível todo
 * int offset: itens do intervalo a pular
 * int limite: máximo de itens a devolver
 *
 * Retorna Resultado*: a página; resultado_total dá o intervalo inteiro
 */
Resultado* indice_buscar_pagina(Indice* ix, Nivel nivel, const Chave* chave,
				int offset, int limite);

/** Recorta a forma da ED num nível, restrita ao intervalo da chave
 *
 * Parâmetros:
 * const Indice* ix: ponteiro para o índice
 * Nivel nivel: nível consultado
 * const Chave* chave: intervalo a mostrar, ou NULL para o nível todo
 * NoVista vista[]: saída com 2^niveis - 1 posições
 * int niveis: quantos níveis recortar
 *
 * Retorna int: total de itens no intervalo, ou -1 se a ED não tem
 *              forma hierárquica
 */
int indice_vista(const Indice* ix, Nivel nivel, const Chave* chave,
		 NoVista vista[], int niveis);

#endif
