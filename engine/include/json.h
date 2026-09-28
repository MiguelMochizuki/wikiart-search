/**
 * json.h
 * Descrição: Utilitários para serialização de TADs em JSON com buffer dinâmico.
 */
#ifndef JSON_H
#define JSON_H

#include "catalogo.h"
#include "indice.h"
#include "obra.h"
#include "resultado.h"
#include "vista.h"

typedef struct json_buffer_t JsonBuffer;

/** Descreve a consulta que originou um Resultado
 *
 * Campos:
 * nivel: nível consultado, que também diz o tipo dos itens
 * genero: gênero procurado, ou NULL se não informado
 * artista: artista procurado, ou NULL se não informado
 * estrutura: nome da ED usada (ex: "tabela_ord")
 * algoritmo: como a ED resolveu a consulta (ex: "busca binaria")
 */
typedef struct {
	Nivel nivel;
	const char* genero;
	const char* artista;
	const char* estrutura;
	const char* algoritmo;
} JsonConsulta;

JsonBuffer* json_buffer_criar(size_t capacidade_inicial);
void        json_buffer_liberar(JsonBuffer* jb);
const char* json_buffer_obter_texto(const JsonBuffer* jb);
size_t      json_buffer_tamanho(const JsonBuffer* jb);

void json_serializar_obra(JsonBuffer* jb, const Obra* o);
void json_serializar_genero(JsonBuffer* jb, const Genero* g);
void json_serializar_artista(JsonBuffer* jb, const Artista* a);

/** Serializa um resultado como lista, com as métricas da busca
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Resultado* r: resultado da busca
 * const JsonConsulta* c: consulta que o originou
 */
void json_serializar_resultado(JsonBuffer* jb, const Resultado* r,
                               const JsonConsulta* c);

/** Serializa as métricas de uma busca junto com a vista da árvore
 *
 * No lugar da lista de itens vai a vista, em layout de heap: a posição
 * i tem os filhos em 2i+1 e 2i+2, e posição vazia sai como null.
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const Resultado* r: resultado da busca, de onde saem as métricas
 * const JsonConsulta* c: consulta que o originou
 * const NoVista* vista: posições da vista
 * int niveis: níveis da vista (2^niveis - 1 posições)
 * int total: itens do intervalo mostrado
 */
void json_serializar_vista(JsonBuffer* jb, const Resultado* r,
                           const JsonConsulta* c,
                           const NoVista* vista, int niveis, int total);

/** Uma linha do comparativo entre EDs
 *
 * Campos:
 * estrutura: nome da ED
 * algoritmo: como ela resolveu a consulta
 * resultado: resultado da busca, de onde saem as métricas
 */
typedef struct {
	const char* estrutura;
	const char* algoritmo;
	const Resultado* resultado;
} JsonComparacao;

/** Serializa o comparativo de uma mesma consulta entre várias EDs
 *
 * Parâmetros:
 * JsonBuffer* jb: buffer de destino
 * const char* genero: gênero procurado, ou NULL
 * const char* artista: artista procurado, ou NULL
 * const char* consulta: chave usada na consulta
 * const JsonComparacao* itens: array de linhas do comparativo
 * int n: número de linhas
 */
void json_serializar_comparativo(JsonBuffer* jb,
                                 const char* genero, const char* artista,
                                 const char* consulta,
                                 const JsonComparacao* itens, int n);

#endif
