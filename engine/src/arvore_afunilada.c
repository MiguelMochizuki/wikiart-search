/**
 * arvore_afunilada.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD ArvoreAfunilada (splay tree), com
 *            afunilamento de baixo para cima por ponteiros para o pai.
 *
 * Nenhum percurso aqui é recursivo. A árvore não se balanceia sozinha e
 * pode ficar tão funda quanto uma lista (inserir em ordem crescente faz
 * exatamente isso), então recursão arriscaria estourar a pilha. Os
 * ponteiros para o pai permitem andar em ordem e liberar tudo sem pilha
 * auxiliar.
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <time.h>
#include "arvore_afunilada.h"

/* ==============================
 * Implementação do TAD
 * ============================== */

typedef struct no_t {
	const void* item;
	struct no_t* esq;
	struct no_t* dir;
	struct no_t* pai;
} No;

struct arvore_afunilada_t {
	No* raiz;
	int n;
	Comparador cmp_itens;
};

/* ==============================
 * Helpers internos
 * ============================== */

/** Retorna o tempo atual em milissegundos (relógio monotônico)
 *
 * Retorna double: tempo em ms desde um ponto arbitrário
 */
static double agora_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/** Situa um nó em relação ao intervalo da chave
 *
 * Com cmp NULL não há chave, e todo nó está dentro.
 *
 * Parâmetros:
 * const No* u: nó a situar
 * Comparador cmp: comparador item x chave, ou NULL
 * const void* chave: chave do intervalo
 * long* comp: contador de comparações, ou NULL para não contar
 *
 * Retorna int: <0 antes do intervalo, 0 dentro, >0 depois
 */
static int situar(const No* u, Comparador cmp, const void* chave,
		  long* comp) {
	if (!cmp) return 0;
	if (comp) (*comp)++;
	return cmp(u->item, chave);
}

/** Desce a partir de u até o primeiro nó dentro do intervalo
 *
 * Um nó antes do intervalo deixa o intervalo inteiro à sua direita; um
 * depois, à esquerda. Então o primeiro nó de dentro que a descida
 * encontra é o mais alto do intervalo naquela subárvore, e todos os
 * outros nós do intervalo estão abaixo dele.
 *
 * Parâmetros:
 * No* u: raiz da subárvore onde descer (pode ser NULL)
 * Comparador cmp: comparador item x chave, ou NULL
 * const void* chave: chave do intervalo
 * long* comp: contador de comparações, ou NULL
 * No** ultimo_out: recebe o último nó visitado, ou NULL para ignorar
 *
 * Retorna No*: o nó encontrado, ou NULL se o intervalo não tem ninguém
 */
static No* descer(No* u, Comparador cmp, const void* chave, long* comp,
		  No** ultimo_out) {
	No* ultimo = NULL;
	while (u) {
		ultimo = u;
		int c = situar(u, cmp, chave, comp);
		if (c == 0) break;
		u = (c < 0) ? u->dir : u->esq;
	}
	if (ultimo_out) *ultimo_out = ultimo;
	return u;
}

/** Menor nó do intervalo dentro da subárvore de x
 *
 * Limite inferior numa árvore: ao achar um nó de dentro, guarda e
 * segue à esquerda atrás de um menor; ao achar um de antes, segue à
 * direita. Nada da subárvore esquerda de x vem depois do intervalo,
 * porque x está nele.
 *
 * Parâmetros:
 * No* x: nó dentro do intervalo
 * Comparador cmp: comparador item x chave, ou NULL
 * const void* chave: chave do intervalo
 * long* comp: contador de comparações, ou NULL
 *
 * Retorna No*: o menor nó do intervalo abaixo de x (ou o próprio x)
 */
static No* menor_do_intervalo(No* x, Comparador cmp, const void* chave,
			      long* comp) {
	No* menor = x;
	No* u = x->esq;
	while (u) {
		if (situar(u, cmp, chave, comp) < 0) {
			u = u->dir;
		} else {
			menor = u;
			u = u->esq;
		}
	}
	return menor;
}

/** Sucessor em ordem de um nó
 *
 * Com subárvore direita, é o mais à esquerda dela. Sem, é o primeiro
 * ancestral do qual u está na subárvore esquerda.
 *
 * Parâmetros:
 * No* u: nó de partida
 *
 * Retorna No*: o sucessor, ou NULL se u é o maior
 */
static No* sucessor(No* u) {
	if (u->dir) {
		u = u->dir;
		while (u->esq) u = u->esq;
		return u;
	}
	while (u->pai && u->pai->dir == u) u = u->pai;
	return u->pai;
}

/** Gira x sobre o pai, preservando a ordem em-ordem
 *
 * x sobe um nível e o pai desce para o lado oposto. A subárvore de x
 * que ficava entre os dois muda de dono e passa a ser filha do pai.
 *
 * Parâmetros:
 * ArvoreAfunilada* a: árvore (a raiz muda se o pai era a raiz)
 * No* x: nó que sobe; precisa ter pai
 */
static void rotacionar(ArvoreAfunilada* a, No* x) {
	No* p = x->pai;
	No* g = p->pai;

	if (p->esq == x) {
		p->esq = x->dir;
		if (x->dir) x->dir->pai = p;
		x->dir = p;
	} else {
		p->dir = x->esq;
		if (x->esq) x->esq->pai = p;
		x->esq = p;
	}
	p->pai = x;
	x->pai = g;

	if (!g) {
		a->raiz = x;
	} else if (g->esq == p) {
		g->esq = x;
	} else {
		g->dir = x;
	}
}

/** Afunila x até a raiz
 *
 * Sobe dois níveis por passo, olhando pai e avô. Alinhados (zig-zig),
 * gira primeiro o pai e depois x: é essa ordem que encurta o caminho
 * pela metade e dá o custo amortizado O(log n). Em cotovelo (zig-zag),
 * gira x duas vezes. Se só falta o pai (zig), uma rotação basta.
 *
 * Parâmetros:
 * ArvoreAfunilada* a: árvore
 * No* x: nó a levar ao topo
 *
 * Retorna long: número de rotações feitas
 */
static long afunilar(ArvoreAfunilada* a, No* x) {
	long rotacoes = 0;

	while (x->pai) {
		No* p = x->pai;
		No* g = p->pai;

		if (!g) {
			rotacionar(a, x);                     /* zig */
			rotacoes += 1;
		} else if ((g->esq == p) == (p->esq == x)) {
			rotacionar(a, p);                     /* zig-zig */
			rotacionar(a, x);
			rotacoes += 2;
		} else {
			rotacionar(a, x);                     /* zig-zag */
			rotacionar(a, x);
			rotacoes += 2;
		}
	}
	return rotacoes;
}

/* ==============================
 * API pública
 * ============================== */

/** Cria uma árvore afunilada vazia
 *
 * Parâmetros:
 * Comparador cmp_itens: ordem total entre itens, usada na inserção
 *
 * Retorna ArvoreAfunilada*: ponteiro para a árvore, ou NULL em erro
 */
ArvoreAfunilada* arvore_afunilada_criar(Comparador cmp_itens) {
	if (!cmp_itens) return NULL;

	ArvoreAfunilada* a = malloc(sizeof *a);
	if (!a) return NULL;

	a->raiz      = NULL;
	a->n         = 0;
	a->cmp_itens = cmp_itens;
	return a;
}

/** Libera a árvore e todos os nós. Não libera os itens apontados.
 *
 * Pós-ordem sem pilha: desce até uma folha, libera e volta pelo pai,
 * que perde aquele filho e pode virar folha também.
 *
 * Parâmetros:
 * ArvoreAfunilada* a: ponteiro para a árvore a ser liberada
 */
void arvore_afunilada_liberar(ArvoreAfunilada* a) {
	if (!a) return;

	No* u = a->raiz;
	while (u) {
		if (u->esq) {
			u = u->esq;
		} else if (u->dir) {
			u = u->dir;
		} else {
			No* p = u->pai;
			if (p) {
				if (p->esq == u) p->esq = NULL;
				else             p->dir = NULL;
			}
			free(u);
			u = p;
		}
	}
	free(a);
}

/** Insere um item e afunila o nó novo até a raiz
 *
 * Parâmetros:
 * ArvoreAfunilada* a: ponteiro para a árvore
 * const void* item: ponteiro para o item (não copiado, só referenciado)
 */
void arvore_afunilada_inserir(ArvoreAfunilada* a, const void* item) {
	No* novo = malloc(sizeof *novo);
	if (!novo) return;

	novo->item = item;
	novo->esq  = NULL;
	novo->dir  = NULL;

	No* pai = NULL;
	No* u = a->raiz;
	int pela_esq = 0;
	while (u) {
		pai = u;
		pela_esq = a->cmp_itens(u->item, item) > 0;
		u = pela_esq ? u->esq : u->dir;
	}

	novo->pai = pai;
	if (!pai) {
		a->raiz = novo;
	} else if (pela_esq) {
		pai->esq = novo;
	} else {
		pai->dir = novo;
	}
	a->n++;

	afunilar(a, novo);
}

/* ==============================
 * Consultas
 * ============================== */

/** Número de itens na árvore
 *
 * Parâmetros:
 * const ArvoreAfunilada* a: ponteiro para a árvore
 *
 * Retorna int: quantidade de elementos
 */
int arvore_afunilada_tamanho(const ArvoreAfunilada* a) {
	return a->n;
}

/** Item guardado na raiz, ou NULL se a árvore está vazia
 *
 * Parâmetros:
 * const ArvoreAfunilada* a: ponteiro para a árvore
 *
 * Retorna const void*: item da raiz
 */
const void* arvore_afunilada_raiz(const ArvoreAfunilada* a) {
	return a->raiz ? a->raiz->item : NULL;
}

/** Busca todos os itens do intervalo descrito pela chave, afunilando
 *
 * Parâmetros:
 * ArvoreAfunilada* a: ponteiro para a árvore
 * Comparador cmp: comparador item x chave, ou NULL para todos
 * const void* chave: chave procurada (ignorada se cmp for NULL)
 *
 * Retorna Resultado*: itens encontrados, em ordem, com as métricas
 */
Resultado* arvore_afunilada_buscar(ArvoreAfunilada* a, Comparador cmp,
				   const void* chave) {
	Resultado* r = resultado_criar(cmp ? 16 : a->n);
	if (!r) return NULL;

	long comp = 0;
	long rotacoes = 0;
	double t0 = agora_ms();

	No* ultimo = NULL;
	No* x = descer(a->raiz, cmp, chave, &comp, &ultimo);

	if (x) {
		/* O acessado vai ao topo. Como x era o nó mais alto do
		 * intervalo, as rotações não tiram ninguém do intervalo de
		 * baixo dele: depois do afunilamento, o intervalo inteiro
		 * está sob a raiz. */
		rotacoes = afunilar(a, x);

		No* u = menor_do_intervalo(x, cmp, chave, &comp);
		resultado_adicionar(r, u->item);
		for (u = sucessor(u); u; u = sucessor(u)) {
			if (situar(u, cmp, chave, &comp) != 0) break;
			resultado_adicionar(r, u->item);
		}
	} else if (ultimo) {
		/* Busca sem achado também afunila: o último nó do caminho
		 * sobe, e a próxima busca por perto sai mais barata. */
		rotacoes = afunilar(a, ultimo);
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	resultado_set_rotacoes(r, rotacoes);
	return r;
}

/** Recorta os primeiros níveis da árvore restrita a um intervalo
 *
 * Parâmetros:
 * const ArvoreAfunilada* a: ponteiro para a árvore
 * Comparador cmp: comparador item x chave, ou NULL para todos
 * const void* chave: chave do intervalo (ignorada se cmp for NULL)
 * NoVista vista[]: saída com 2^niveis - 1 posições, em layout de heap
 * int niveis: quantos níveis recortar (1 a VISTA_MAX_NIVEIS)
 *
 * Retorna int: total de itens no intervalo, ou -1 se niveis é inválido
 */
int arvore_afunilada_vista(const ArvoreAfunilada* a, Comparador cmp,
			   const void* chave, NoVista vista[], int niveis) {
	enum { MAX_POS = (1 << VISTA_MAX_NIVEIS) - 1 };

	if (niveis < 1 || niveis > VISTA_MAX_NIVEIS) return -1;

	const int n_pos = (1 << niveis) - 1;
	const int primeira_folha = n_pos / 2;  /* posições da última fileira */

	No* nos[MAX_POS];
	int posto[MAX_POS];  /* posição em ordem do nó dentro do intervalo */
	int ini[MAX_POS];    /* trecho de postos que a subárvore cobre */
	int fim[MAX_POS];

	for (int i = 0; i < n_pos; i++) {
		nos[i] = NULL;
		posto[i] = -1;
		vista[i].item = NULL;
		vista[i].descendentes = 0;
	}

	/* A hierarquia da vista: cada filho é o nó do intervalo mais alto
	 * da subárvore correspondente, achado pela mesma descida da busca. */
	nos[0] = descer(a->raiz, cmp, chave, NULL, NULL);
	if (!nos[0]) return 0;

	for (int i = 0; i < primeira_folha; i++) {
		if (!nos[i]) continue;
		nos[2 * i + 1] = descer(nos[i]->esq, cmp, chave, NULL, NULL);
		nos[2 * i + 2] = descer(nos[i]->dir, cmp, chave, NULL, NULL);
	}

	/* Um percurso em ordem pelo intervalo dá o total e o posto de cada
	 * nó da vista. Sai da subárvore de nos[0] só para um ancestral
	 * dele, que está fora do intervalo e encerra o laço. */
	int total = 0;
	for (No* u = menor_do_intervalo(nos[0], cmp, chave, NULL);
	     u && situar(u, cmp, chave, NULL) == 0;
	     u = sucessor(u)) {
		for (int i = 0; i < n_pos; i++) {
			if (nos[i] == u) {
				posto[i] = total;
				break;
			}
		}
		total++;
	}

	/* Cada subárvore cobre um trecho contíguo de postos: a raiz, todos;
	 * o filho esquerdo, os postos antes do pai dentro do trecho dele; o
	 * direito, os depois. O tamanho do trecho, menos o próprio nó, é o
	 * número de descendentes. */
	ini[0] = 0;
	fim[0] = total - 1;
	for (int i = 0; i < n_pos; i++) {
		if (!nos[i]) continue;

		vista[i].item = nos[i]->item;
		vista[i].descendentes = fim[i] - ini[i];

		if (i < primeira_folha) {
			ini[2 * i + 1] = ini[i];
			fim[2 * i + 1] = posto[i] - 1;
			ini[2 * i + 2] = posto[i] + 1;
			fim[2 * i + 2] = fim[i];
		}
	}
	return total;
}
