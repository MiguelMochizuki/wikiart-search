/**
 * arvore_afunilada.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD ArvoreAfunilada (splay tree). Árvore
 *            binária de busca que, a cada acesso, leva o nó acessado
 *            até a raiz por rotações (afunilamento).
 *
 * Não há balanceamento explícito: a forma da árvore é consequência dos
 * acessos. Cada operação custa O(log n) amortizado, e o que foi usado
 * há pouco fica perto do topo, então acessos repetidos saem baratos.
 * Em troca, uma operação isolada pode custar O(n), e até as buscas
 * alteram a estrutura.
 *
 * Como a TabelaOrd, a árvore não conhece o tipo dos itens: recebe a
 * ordem entre itens na criação e um comparador de chave a cada busca.
 */
#ifndef ARVORE_AFUNILADA_H
#define ARVORE_AFUNILADA_H

#include "comparador.h"
#include "resultado.h"
#include "vista.h"

/* ==============================
 * TADs
 * ============================== */

typedef struct arvore_afunilada_t ArvoreAfunilada;

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
ArvoreAfunilada* arvore_afunilada_criar(Comparador cmp_itens);

/** Libera a árvore e todos os nós. Não libera os itens apontados.
 *
 * Parâmetros:
 * ArvoreAfunilada* a: ponteiro para a árvore a ser liberada
 */
void arvore_afunilada_liberar(ArvoreAfunilada* a);

/** Insere um item e afunila o nó novo até a raiz
 *
 * Desce como numa árvore binária de busca comum, pendura o nó como
 * folha e o leva ao topo. Itens iguais descem pela direita.
 *
 * Parâmetros:
 * ArvoreAfunilada* a: ponteiro para a árvore
 * const void* item: ponteiro para o item (não copiado, só referenciado)
 */
void arvore_afunilada_inserir(ArvoreAfunilada* a, const void* item);

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
int arvore_afunilada_tamanho(const ArvoreAfunilada* a);

/** Item guardado na raiz, ou NULL se a árvore está vazia
 *
 * Parâmetros:
 * const ArvoreAfunilada* a: ponteiro para a árvore
 *
 * Retorna const void*: item da raiz
 */
const void* arvore_afunilada_raiz(const ArvoreAfunilada* a);

/** Busca todos os itens do intervalo descrito pela chave, afunilando
 *
 * Desce da raiz até o primeiro nó dentro do intervalo, que é o mais
 * alto deles, e o afunila até a raiz. Todo o intervalo fica então
 * abaixo dela, e a coleta anda em ordem a partir do menor nó do
 * intervalo. Sem achado, afunila o último nó visitado, como manda a
 * regra da estrutura. Custo O(log n + k) amortizado.
 *
 * Com cmp NULL não há chave: devolve a árvore inteira em ordem, sem
 * comparar e sem mexer na forma.
 *
 * Parâmetros:
 * ArvoreAfunilada* a: ponteiro para a árvore
 * Comparador cmp: comparador item x chave, ou NULL para todos
 * const void* chave: chave procurada (ignorada se cmp for NULL)
 *
 * Retorna Resultado*: itens encontrados, em ordem, com comparações e
 *                     rotações da operação
 */
Resultado* arvore_afunilada_buscar(ArvoreAfunilada* a, Comparador cmp,
				   const void* chave);

/** Recorta os primeiros níveis da árvore restrita a um intervalo
 *
 * A vista mostra só os nós do intervalo, mantendo entre eles a
 * hierarquia real: o filho esquerdo de um nó na vista é o nó do
 * intervalo mais alto da sua subárvore esquerda (idem à direita). Nós
 * de fora do intervalo que estejam no caminho são pulados. Com cmp
 * NULL a vista é a própria árvore. Não altera a estrutura nem conta
 * comparações.
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
			   const void* chave, NoVista vista[], int niveis);

#endif
