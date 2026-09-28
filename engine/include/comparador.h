/**
 * comparador.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Tipo das funções que as EDs usam para ordenar e buscar
 *            itens sem conhecer o tipo deles.
 */
#ifndef COMPARADOR_H
#define COMPARADOR_H

/** Compara um item guardado numa ED com uma chave
 *
 * A chave descreve um intervalo contíguo na ordem dos itens: o
 * comparador devolve <0 se o item vem antes do intervalo, 0 se está
 * dentro dele e >0 se vem depois. Uma chave parcial (só o gênero, por
 * exemplo) casa com um bloco inteiro; uma chave completa, com um item
 * só. Na inserção a ED passa outro item no lugar da chave, e aí o
 * comparador vira a ordem total entre itens.
 *
 * Parâmetros:
 * const void* item: item guardado na estrutura
 * const void* chave: chave procurada (ou outro item, na inserção)
 *
 * Retorna int: <0, 0 ou >0 conforme o item venha antes, dentro ou depois
 */
typedef int (*Comparador)(const void* item, const void* chave);

#endif
