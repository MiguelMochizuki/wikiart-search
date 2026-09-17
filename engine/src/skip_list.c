/**
 * skip_list.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD SkipList. Lista encadeada
 *            probabilística, ordenada pela chave composta
 *            (gênero, artista).
 */
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "skip_list.h"

/* ==============================
 * Constantes
 * ============================== */

/* Altura máxima de uma torre. Com 80k elementos, log2(80000) é
 * aproximadamente 16.3. 20 dá folga sem desperdiçar muita memória. */
#define MAX_NIVEL 20

/* Probabilidade de promoção por nível. Metade dos nós sobem. */
#define PROB 2

/* ==============================
 * Implementação do TAD
 * ============================== */

typedef struct no_skip_t {
	const Obra* obra;
	struct no_skip_t** proximo;  /* array de ponteiros, um por nível */
} NoSkip;

struct skip_list_t {
	NoSkip* cabeca;              /* nó sentinela, sem obra */
	int nivel_atual;             /* maior nível em uso */
	int n;
	unsigned int seed;           /* estado do PRNG xorshift */
};

/* ==============================
 * Helpers internos
 * ============================== */

/** Gerador pseudoaleatório xorshift de 32 bits
 *
 * Parâmetros:
 * unsigned int* seed: estado do gerador, modificado a cada chamada
 *
 * Retorna unsigned int: próximo valor pseudoaleatório
 */
static unsigned int xorshift(unsigned int* seed) {
	unsigned int x = *seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*seed = x;
	return x;
}

/** Sorteia um nível para um novo nó
 *
 * Parâmetros:
 * unsigned int* seed: estado do PRNG
 *
 * Retorna int: nível sorteado, entre 0 e MAX_NIVEL - 1
 */
static int sorteia_nivel(unsigned int* seed) {
	int nivel = 0;
	while (nivel < MAX_NIVEL - 1 && (xorshift(seed) % PROB) == 0) {
		nivel++;
	}
	return nivel;
}

/** Cria um nó com o nível dado
 *
 * Parâmetros:
 * const Obra* o: obra a armazenar
 * int nivel: número de níveis da torre
 *
 * Retorna NoSkip*: ponteiro para o nó alocado, ou NULL em erro
 */
static NoSkip* no_criar(const Obra* o, int nivel) {
	NoSkip* no = malloc(sizeof *no);
	if (!no) return NULL;

	no->proximo = calloc((size_t)nivel + 1, sizeof *no->proximo);
	if (!no->proximo) {
		free(no);
		return NULL;
	}
	no->obra = o;
	return no;
}

/** Libera um nó e seu array de ponteiros
 *
 * Parâmetros:
 * NoSkip* no: ponteiro para o nó a ser liberado
 */
static void no_liberar(NoSkip* no) {
	if (!no) return;
	free(no->proximo);
	free(no);
}

/** Compara a chave de uma obra com a chave composta (gênero, artista)
 *
 * Compara primeiro o gênero; só desempata pelo artista quando os
 * gêneros são iguais. O artista é opcional: passar NULL faz a chave
 * procurada valer como "menor que qualquer artista daquele gênero",
 * o que posiciona a descida no início do bloco do gênero.
 *
 * Parâmetros:
 * const Obra* o: obra cuja chave será comparada
 * const char* genero: gênero procurado
 * const char* artista: artista procurado, ou NULL
 *
 * Retorna int: <0, 0 ou >0 conforme a obra seja menor, igual ou maior
 */
static int cmp_chave(const Obra* o, const char* genero, const char* artista) {
	int c = strcmp(obra_genero(o), genero);
	if (c != 0) return c;
	if (!artista) return 1;
	return strcmp(obra_artista(o), artista);
}

/** Retorna o tempo atual em milissegundos (relógio monotônico)
 *
 * Retorna double: tempo em ms desde um ponto arbitrário
 */
static double agora_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* ==============================
 * API pública
 * ============================== */

/** Cria uma Skip List vazia
 *
 * Retorna SkipList*: ponteiro para a lista alocada, ou NULL em erro
 */
SkipList* skip_list_criar(void) {
	SkipList* sl = malloc(sizeof *sl);
	if (!sl) return NULL;

	sl->cabeca = no_criar(NULL, MAX_NIVEL - 1);
	if (!sl->cabeca) {
		free(sl);
		return NULL;
	}

	sl->nivel_atual = 0;
	sl->n           = 0;
	sl->seed        = 42;  /* semente fixa para benchmarks reprodutíveis */
	return sl;
}

/** Libera a Skip List e todos os nós internos. Não libera as Obras.
 *
 * Parâmetros:
 * SkipList* sl: ponteiro para a lista a ser liberada
 */
void skip_list_liberar(SkipList* sl) {
	if (!sl) return;

	NoSkip* no = sl->cabeca;
	while (no) {
		NoSkip* prox = no->proximo[0];
		no_liberar(no);
		no = prox;
	}
	free(sl);
}

/** Insere uma obra mantendo a ordenação por (gênero, artista)
 *
 * Parâmetros:
 * SkipList* sl: ponteiro para a lista
 * const Obra* o: ponteiro para a obra (não copiada, só referenciada)
 */
void skip_list_inserir(SkipList* sl, const Obra* o) {
	/* atual[i] guarda o último nó visitado no nível i,
	 * que será o antecessor do novo nó naquele nível. */
	NoSkip* atual[MAX_NIVEL];
	NoSkip* no = sl->cabeca;

	for (int i = sl->nivel_atual; i >= 0; i--) {
		while (no->proximo[i] &&
		       cmp_chave(no->proximo[i]->obra,
		                 obra_genero(o), obra_artista(o)) < 0) {
			no = no->proximo[i];
		}
		atual[i] = no;
	}

	int novo_nivel = sorteia_nivel(&sl->seed);

	if (novo_nivel > sl->nivel_atual) {
		for (int i = sl->nivel_atual + 1; i <= novo_nivel; i++) {
			atual[i] = sl->cabeca;
		}
		sl->nivel_atual = novo_nivel;
	}

	NoSkip* novo = no_criar(o, novo_nivel);
	if (!novo) return;

	for (int i = 0; i <= novo_nivel; i++) {
		novo->proximo[i]  = atual[i]->proximo[i];
		atual[i]->proximo[i] = novo;
	}

	sl->n++;
}

/** Número de obras na lista
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 *
 * Retorna int: quantidade de elementos
 */
int skip_list_tamanho(const SkipList* sl) {
	return sl->n;
}

/** Busca todas as obras de um dado artista
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_artista(const SkipList* sl, const char* artista) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Artista é a chave secundária: as obras de um mesmo artista estão
	 * espalhadas entre os blocos de gênero. Os níveis superiores
	 * particionam por gênero e não ajudam aqui, então varremos o
	 * nível 0 inteiro. */
	for (NoSkip* no = sl->cabeca->proximo[0]; no; no = no->proximo[0]) {
		comp++;
		if (strcmp(obra_artista(no->obra), artista) == 0) {
			resultado_adicionar(r, no->obra);
		}
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}

/** Busca todas as obras de um dado gênero
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_genero(const SkipList* sl, const char* genero) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Gênero é a chave primária: o bloco é contíguo no nível 0. Desce
	 * com artista NULL, que posiciona no início do bloco. */
	NoSkip* no = sl->cabeca;
	for (int i = sl->nivel_atual; i >= 0; i--) {
		while (no->proximo[i]) {
			comp++;
			if (cmp_chave(no->proximo[i]->obra, genero, NULL) < 0) {
				no = no->proximo[i];
			} else {
				break;
			}
		}
	}

	/* Agora `no` é o maior nó anterior ao bloco. Avança no nível 0
	 * coletando enquanto o gênero casar. */
	no = no->proximo[0];
	while (no && strcmp(obra_genero(no->obra), genero) == 0) {
		comp++;
		resultado_adicionar(r, no->obra);
		no = no->proximo[0];
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}

/** Busca as obras de um artista filtrando por gênero
 *
 * Parâmetros:
 * const SkipList* sl: ponteiro para a lista
 * const char* genero: gênero procurado
 * const char* artista: artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* skip_list_buscar_genero_artista(const SkipList* sl,
					   const char* genero,
					   const char* artista) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Chave composta completa: o bloco do par é contíguo no nível 0.
	 * Desce até o maior nó com chave menor que a procurada. */
	NoSkip* no = sl->cabeca;
	for (int i = sl->nivel_atual; i >= 0; i--) {
		while (no->proximo[i]) {
			comp++;
			if (cmp_chave(no->proximo[i]->obra, genero, artista) < 0) {
				no = no->proximo[i];
			} else {
				break;
			}
		}
	}

	/* Coleta as ocorrências consecutivas com a mesma chave composta. */
	no = no->proximo[0];
	while (no && cmp_chave(no->obra, genero, artista) == 0) {
		comp++;
		resultado_adicionar(r, no->obra);
		no = no->proximo[0];
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}
