/**
 * obra.c
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD Obra.
 */
#include <stdlib.h>
#include <string.h>
#include "obra.h"

/* ==============================
 * Implementação do TAD
 * ============================== */

struct obra_t {
	int id;
	char* titulo;
	char* artista;
	char* genero;
	int ano;
	char* caminho;
};

/* ==============================
 * Helpers internos
 * ============================== */

/** Duplica uma string em heap.
 *
 * Parâmetros:
 * const char* s: string a ser duplicada
 *
 * Retorna char*: string duplicada; retorna NULL se entrada for NULL.
 */
static char* dup_str(const char* s) {
	if (!s) return NULL;
	size_t n = strlen(s) + 1;
	char* d = malloc(n);
	if (d) memcpy(d, s, n);
	return d;
}

/* ==============================
 * API pública
 * ============================== */

/** Cria uma obra a partir de campos já validados
 *
 * Parâmetros:
 * int id: o ID da obra
 * const char* titulo: título da obra
 * const char* artista: o artista
 * const char* genero: o gênero da obra
 * int ano: o ano da obra
 * const char* caminho: o caminho para o arquivo correspondente à obra
 *
 * Retorna Obra*: ponteiro para a instância alocada de Obra
 */
Obra* obra_criar(int id,
		const char* titulo,
		const char* artista,
		const char* genero,
		int ano,
		const char* caminho) {
	Obra* o = malloc(sizeof *o);
	if (!o) return NULL;

	o->id      = id;
	o->titulo  = dup_str(titulo);
	o->artista = dup_str(artista);
	o->genero  = dup_str(genero);
	o->ano     = ano;
	o->caminho = dup_str(caminho);

	if (!o->titulo || !o->artista || !o->genero || !o->caminho) {
		obra_liberar(o);
		return NULL;
	}
	return o;
}

/** Libera espaço alocado para uma obra
 *
 * Parâmetros:
 * Obra* o: ponteiro para obra a ser liberada
 */
void obra_liberar(Obra* o) {
	if (!o) return;
	free(o->titulo);
	free(o->artista);
	free(o->genero);
	free(o->caminho);
	free(o);
}

/* ==============================
 * Getters
 * ============================== */

int obra_id(const Obra* o) {
	return o->id;
}

const char* obra_titulo(const Obra* o) {
	return o->titulo;
}

const char* obra_artista(const Obra* o) {
	return o->artista;
}

const char* obra_genero(const Obra* o) {
	return o->genero;
}

int obra_ano(const Obra* o) {
	return o->ano;
}

const char* obra_caminho(const Obra* o) {
	return o->caminho;
}
