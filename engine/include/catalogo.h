/**
 * catalogo.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo dos TADs Genero, Artista e Catalogo. O catálogo
 *            agrega as obras do CSV nos itens dos dois primeiros níveis
 *            da navegação e fixa a ordem em que as EDs são carregadas.
 *
 * Genero: um estilo, com quantas obras e quantos artistas tem.
 * Artista: um artista dentro de um gênero. Quem pintou em mais de um
 *          gênero aparece uma vez em cada, com a contagem daquele
 *          gênero.
 *
 * Os dois levam uma obra de capa (preview), para a interface não ter só
 * nome e número nas duas primeiras telas. As strings dos itens apontam
 * para as da obra de capa, então o Csv precisa viver mais que o
 * catálogo.
 */
#ifndef CATALOGO_H
#define CATALOGO_H

#include "csv.h"
#include "obra.h"

/* ==============================
 * TADs
 * ============================== */

typedef struct genero_t Genero;
typedef struct artista_t Artista;
typedef struct catalogo_t Catalogo;

/* ==============================
 * API pública
 * ============================== */

/** Agrega as obras do CSV em gêneros e artistas por gênero
 *
 * A capa de um artista é a primeira obra dele no gênero, na ordem da
 * chave (menor id). A de um gênero é a capa do seu artista mais
 * prolífico, que costuma ser o nome que define o estilo.
 *
 * Os três níveis saem em ordem embaralhada, com semente fixa. O CSV vem
 * agrupado por artista e com ids crescentes; carregado assim, cada
 * bloco (gênero, artista) entraria em ordem crescente, e a árvore
 * afunilada, que leva cada nó novo à raiz, viraria uma lista encadeada
 * pela esquerda em cada bloco. Todas as EDs recebem a mesma ordem.
 *
 * Parâmetros:
 * const Csv* csv: metadados carregados
 *
 * Retorna Catalogo*: catálogo alocado, ou NULL em erro
 */
Catalogo* catalogo_montar(const Csv* csv);

/** Libera o catálogo e seus itens. Não libera as obras do Csv.
 *
 * Parâmetros:
 * Catalogo* c: ponteiro para o catálogo
 */
void catalogo_liberar(Catalogo* c);

/* ==============================
 * Acesso, na ordem de carga
 * ============================== */

int catalogo_n_generos(const Catalogo* c);
const Genero* catalogo_genero(const Catalogo* c, int indice);

int catalogo_n_artistas(const Catalogo* c);
const Artista* catalogo_artista(const Catalogo* c, int indice);

int catalogo_n_obras(const Catalogo* c);
const Obra* catalogo_obra(const Catalogo* c, int indice);

/* ==============================
 * Getters
 * ============================== */

const char* genero_nome(const Genero* g);
int genero_obras(const Genero* g);
int genero_artistas(const Genero* g);
const Obra* genero_capa(const Genero* g);

const char* artista_genero(const Artista* a);
const char* artista_nome(const Artista* a);
int artista_obras(const Artista* a);
const Obra* artista_capa(const Artista* a);

#endif
