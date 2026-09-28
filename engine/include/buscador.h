/**
 * buscador.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Interface comum para estruturas de busca plugáveis no
 *            servidor e no benchmark. Cada ED expõe uma instância de
 *            Buscador.
 *
 * A interface é genérica sobre o tipo dos itens: a mesma ED guarda os
 * gêneros, os artistas por gênero e as obras, cada nível numa instância
 * própria criada com a ordem daquele nível.
 */
#ifndef BUSCADOR_H
#define BUSCADOR_H

#include "comparador.h"
#include "resultado.h"
#include "vista.h"

/* ==============================
 * TADs
 * ============================== */

/** Vtable que descreve uma ED de busca
 *
 * Campos:
 * nome: identificador curto (usado na API e no CSV do benchmark)
 * algoritmo_busca: rótulo da busca por chave, para a interface exibir
 * algoritmo_percurso: rótulo da listagem completa, sem chave
 * criar: aloca a estrutura vazia com a ordem entre itens dada
 * liberar: destrói a estrutura e seus nós internos
 * inserir: adiciona um item à estrutura
 * buscar: devolve o intervalo da chave (cmp NULL: todos os itens)
 * vista: recorta os primeiros níveis da forma da ED, restritos ao
 *        intervalo da chave; NULL nas EDs sem forma hierárquica
 */
typedef struct buscador_t {
	const char* nome;
	const char* algoritmo_busca;
	const char* algoritmo_percurso;
	void*       (*criar)(Comparador cmp_itens);
	void        (*liberar)(void*);
	void        (*inserir)(void*, const void* item);
	Resultado*  (*buscar)(void*, Comparador cmp, const void* chave);
	int         (*vista)(const void*, Comparador cmp, const void* chave,
			     NoVista vista[], int niveis);
} Buscador;

/* ==============================
 * Instâncias
 * ============================== */

extern const Buscador BUSCADOR_TABELA_ORD;
extern const Buscador BUSCADOR_ARVORE_AFUNILADA;

#endif
