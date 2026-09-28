/**
 * buscador_arvore_afunilada.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Adapta a ArvoreAfunilada à interface Buscador.
 */
#include "buscador.h"
#include "arvore_afunilada.h"

/* ==============================
 * Adaptadores
 * ============================== */

/** Cria uma ArvoreAfunilada vazia */
static void* aa_criar(Comparador cmp_itens) {
	return arvore_afunilada_criar(cmp_itens);
}

/** Libera a ArvoreAfunilada */
static void aa_liberar(void* p) {
	arvore_afunilada_liberar((ArvoreAfunilada*) p);
}

/** Insere um item na ArvoreAfunilada */
static void aa_inserir(void* p, const void* item) {
	arvore_afunilada_inserir((ArvoreAfunilada*) p, item);
}

/** Busca o intervalo da chave, afunilando o achado até a raiz */
static Resultado* aa_buscar(void* p, Comparador cmp, const void* chave) {
	return arvore_afunilada_buscar((ArvoreAfunilada*) p, cmp, chave);
}

/** Recorta os primeiros níveis da árvore restrita ao intervalo */
static int aa_vista(const void* p, Comparador cmp, const void* chave,
		    NoVista vista[], int niveis) {
	return arvore_afunilada_vista((const ArvoreAfunilada*) p, cmp, chave,
				      vista, niveis);
}

/* ==============================
 * Instância pública
 * ============================== */

const Buscador BUSCADOR_ARVORE_AFUNILADA = {
	.nome               = "arvore_afunilada",
	.algoritmo_busca    = "afunilamento (splay)",
	.algoritmo_percurso = "percurso em ordem",
	.criar              = aa_criar,
	.liberar            = aa_liberar,
	.inserir            = aa_inserir,
	.buscar             = aa_buscar,
	.vista              = aa_vista
};
