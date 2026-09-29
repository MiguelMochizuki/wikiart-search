/**
 * buscador_tabela_ord.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Adapta a TabelaOrd à interface Buscador.
 */
#include <stddef.h>
#include "buscador.h"
#include "tabela_ord.h"

/* ==============================
 * Adaptadores
 * ============================== */

/** Cria uma TabelaOrd vazia */
static void* to_criar(Comparador cmp_itens) {
	return tabela_ord_criar(cmp_itens);
}

/** Libera a TabelaOrd */
static void to_liberar(void* p) {
	tabela_ord_liberar((TabelaOrd*) p);
}

/** Insere um item na TabelaOrd */
static void to_inserir(void* p, const void* item) {
	tabela_ord_inserir((TabelaOrd*) p, item);
}

/** Busca o intervalo da chave na TabelaOrd */
static Resultado* to_buscar(void* p, Comparador cmp, const void* chave) {
	return tabela_ord_buscar((const TabelaOrd*) p, cmp, chave);
}

/** Busca uma página do intervalo da chave */
static Resultado* to_pagina(void* p, Comparador cmp, const void* chave,
			       int offset, int limite) {
	return tabela_ord_buscar_pagina((const TabelaOrd*) p, cmp, chave,
					 offset, limite);
}

/* ==============================
 * Instância pública
 * ============================== */

/* Sem vista: um array não tem hierarquia a mostrar. */
const Buscador BUSCADOR_TABELA_ORD = {
	.nome               = "tabela_ord",
	.algoritmo_busca    = "busca binaria",
	.algoritmo_percurso = "percurso sequencial",
	.criar              = to_criar,
	.liberar            = to_liberar,
	.inserir            = to_inserir,
	.buscar             = to_buscar,
	.buscar_pagina      = to_pagina,
	.vista              = NULL
};
