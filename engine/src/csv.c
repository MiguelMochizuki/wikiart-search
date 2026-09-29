/**
 * csv.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD Csv. Parser simples de CSV com
 *            separador ';'.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"

#define MAX_LINHA 4096

/* ==============================
 * Implementação do TAD
 * ============================== */

struct csv_t {
	Obra** obras;
	int n;
	int cap;
};

/* ==============================
 * Helpers internos
 * ============================== */

/** Divide uma linha em campos por ';'. Modifica a linha no lugar.
 *
 * Parâmetros:
 * char* linha: buffer da linha (modificado)
 * char* campos[]: array de saída com os ponteiros para cada campo
 * int max: número máximo de campos
 *
 * Retorna int: número de campos encontrados
 */
static int split(char* linha, char* campos[], int max) {
	int n = 0;
	char* p = linha;
	campos[n++] = p;
	while (*p && n < max) {
		if (*p == ';') {
			*p = '\0';
			campos[n++] = p + 1;
		}
		p++;
	}
	char* ult = campos[n - 1];
	size_t len = strlen(ult);
	while (len && (ult[len - 1] == '\n' || ult[len - 1] == '\r')) {
		ult[--len] = '\0';
	}
	return n;
}

/* ==============================
 * API pública
 * ============================== */

/** Carrega o CSV inteiro em memória
 *
 * Parâmetros:
 * const char* caminho: caminho para o arquivo CSV
 *
 * Retorna Csv*: ponteiro para o Csv alocado, ou NULL em erro
 */
Csv* csv_carregar(const char* caminho) {
	FILE* f = fopen(caminho, "r");
	if (!f) {
		perror("fopen");
		return NULL;
	}

	char linha[MAX_LINHA];
	if (!fgets(linha, sizeof linha, f)) {  /* cabeçalho */
		fclose(f);
		return NULL;
	}

	Csv* c = malloc(sizeof *c);
	if (!c) {
		fclose(f);
		return NULL;
	}

	c->cap   = 1024;
	c->n     = 0;
	c->obras = malloc(c->cap * sizeof *c->obras);
	if (!c->obras) {
		free(c);
		fclose(f);
		return NULL;
	}

	while (fgets(linha, sizeof linha, f)) {
		char* campos[8];
		int nc = split(linha, campos, 8);
		if (nc < 6) continue;

		if (c->n == c->cap) {
			int nova_cap = c->cap * 2;
			Obra** tmp = realloc(c->obras, nova_cap * sizeof *tmp);
			if (!tmp) break;
			c->obras = tmp;
			c->cap   = nova_cap;
		}

		Obra* o = obra_criar(atoi(campos[0]),
				     campos[1],
				     campos[2],
				     campos[3],
				     atoi(campos[4]),
				     campos[5]);
		if (!o) break;

		c->obras[c->n++] = o;
	}

	fclose(f);
	return c;
}

/** Libera o CSV e todas as Obras contidas nele
 *
 * Parâmetros:
 * Csv* c: ponteiro para o Csv a ser liberado
 */
void csv_liberar(Csv* c) {
	if (!c) return;
	for (int i = 0; i < c->n; i++) {
		obra_liberar(c->obras[i]);
	}
	free(c->obras);
	free(c);
}

/* ==============================
 * Acesso
 * ============================== */

int csv_tamanho(const Csv* c) {
	return c->n;
}

const Obra* csv_obra(const Csv* c, int indice) {
	if (indice < 0 || indice >= c->n) return NULL;
	return c->obras[indice];
}
