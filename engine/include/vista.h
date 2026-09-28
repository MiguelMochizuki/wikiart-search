/**
 * vista.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Recorte dos primeiros níveis de uma ED hierárquica, para a
 *            interface desenhar a árvore como ela está.
 */
#ifndef VISTA_H
#define VISTA_H

/** Maior número de níveis que uma vista pode pedir */
#define VISTA_MAX_NIVEIS 6

/** Uma posição da vista, em layout de heap
 *
 * A posição i tem o filho esquerdo em 2i+1 e o direito em 2i+2, então
 * uma vista de k níveis ocupa 2^k - 1 posições e a fileira d começa na
 * posição 2^d - 1.
 *
 * Campos:
 * item: item do nó, ou NULL se a posição está vazia
 * descendentes: quantos itens do intervalo estão abaixo deste nó,
 *               inclusive os que ficaram fora da vista
 */
typedef struct {
	const void* item;
	int descendentes;
} NoVista;

#endif
