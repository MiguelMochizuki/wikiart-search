/**
 * hash_table.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Implementação do TAD HashTable. Tabela hash com
 *            encadeamento, indexada por gênero (chave primária, define
 *            o bucket) e com a cadeia de cada bucket mantida ordenada
 *            por artista (chave secundária).
 */
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_table.h"

/* ==============================
 * Implementação do TAD
 * ============================== */

typedef struct no_hash_t {
	const Obra* obra;
	struct no_hash_t* prox;
} NoHash;

struct hash_table_t {
	NoHash** buckets;
	int capacidade;
	int n;
};

/* ==============================
 * Helpers internos
 * ============================== */

/** Hash djb2 sobre uma string
 *
 * Parâmetros:
 * const char* s: string de entrada
 *
 * Retorna unsigned long: valor de hash
 */
static unsigned long hash_djb2(const char* s) {
	unsigned long h = 5381;
	int c;
	while ((c = *s++)) {
		h = ((h << 5) + h) + (unsigned char) c;
	}
	return h;
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

/** Cria uma tabela hash com a capacidade dada
 *
 * Parâmetros:
 * int capacidade: número de buckets (mínimo 1)
 *
 * Retorna HashTable*: ponteiro para a tabela alocada, ou NULL em erro
 */
HashTable* hash_table_criar(int capacidade) {
	if (capacidade < 1) capacidade = 1;

	HashTable* ht = malloc(sizeof *ht);
	if (!ht) return NULL;

	ht->buckets = calloc((size_t) capacidade, sizeof *ht->buckets);
	if (!ht->buckets) {
		free(ht);
		return NULL;
	}

	ht->capacidade = capacidade;
	ht->n          = 0;
	return ht;
}

/** Libera a tabela e todos os nós internos. Não libera as Obras.
 *
 * Parâmetros:
 * HashTable* ht: ponteiro para a tabela a ser liberada
 */
void hash_table_liberar(HashTable* ht) {
	if (!ht) return;

	for (int i = 0; i < ht->capacidade; i++) {
		NoHash* no = ht->buckets[i];
		while (no) {
			NoHash* prox = no->prox;
			free(no);
			no = prox;
		}
	}
	free(ht->buckets);
	free(ht);
}

/** Insere uma obra no bucket do seu gênero, ordenada por artista
 *
 * O bucket sai do hash do gênero. Dentro dele a cadeia é mantida
 * ordenada por artista, o que custa uma varredura até o ponto de
 * inserção: O(k) no tamanho da cadeia, não O(1).
 *
 * Parâmetros:
 * HashTable* ht: ponteiro para a tabela
 * const Obra* o: ponteiro para a obra (não copiada, só referenciada)
 */
void hash_table_inserir(HashTable* ht, const Obra* o) {
	unsigned long h = hash_djb2(obra_genero(o)) % (unsigned long) ht->capacidade;

	NoHash* novo = malloc(sizeof *novo);
	if (!novo) return;

	novo->obra = o;

	/* Avança enquanto o artista da cadeia for menor que o da obra nova.
	 * `ligacao` aponta para o campo que receberá o nó, o que trata o
	 * início da cadeia sem caso especial. */
	NoHash** ligacao = &ht->buckets[h];
	while (*ligacao &&
	       strcmp(obra_artista((*ligacao)->obra), obra_artista(o)) < 0) {
		ligacao = &(*ligacao)->prox;
	}

	novo->prox = *ligacao;
	*ligacao   = novo;
	ht->n++;
}

/** Número de obras na tabela
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 *
 * Retorna int: quantidade de elementos
 */
int hash_table_tamanho(const HashTable* ht) {
	return ht->n;
}

/** Busca todas as obras de um dado artista
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* artista: nome do artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_artista(const HashTable* ht, const char* artista) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Artista é a chave secundária: vive dentro da cadeia, não decide
	 * o bucket. Sem bucket para consultar, varremos a tabela inteira.
	 * A ordenação da cadeia ainda ajuda: assim que o artista corrente
	 * passa do procurado, o resto daquela cadeia é maior e pode ser
	 * abandonado. Continua O(n), com constante menor. */
	for (int i = 0; i < ht->capacidade; i++) {
		for (NoHash* no = ht->buckets[i]; no; no = no->prox) {
			comp++;
			int c = strcmp(obra_artista(no->obra), artista);
			if (c < 0) continue;
			if (c > 0) break;
			resultado_adicionar(r, no->obra);
		}
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}

/** Busca todas as obras de um dado gênero
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* genero: gênero procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_genero(const HashTable* ht, const char* genero) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Gênero é a chave de hash: um único bucket concentra o bloco.
	 * A cadeia está ordenada por artista, não por gênero, então uma
	 * colisão entre dois gêneros aparece intercalada e obriga a
	 * comparar o gênero de cada nó em vez de parar cedo. */
	unsigned long h = hash_djb2(genero) % (unsigned long) ht->capacidade;

	for (NoHash* no = ht->buckets[h]; no; no = no->prox) {
		comp++;
		if (strcmp(obra_genero(no->obra), genero) == 0) {
			resultado_adicionar(r, no->obra);
		}
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}

/** Busca as obras de um artista filtrando por gênero
 *
 * Parâmetros:
 * const HashTable* ht: ponteiro para a tabela
 * const char* genero: gênero procurado
 * const char* artista: artista procurado
 *
 * Retorna Resultado*: resultado com os ponteiros encontrados
 */
Resultado* hash_table_buscar_genero_artista(const HashTable* ht,
					    const char* genero,
					    const char* artista) {
	Resultado* r = resultado_criar(16);
	if (!r) return NULL;

	long comp = 0;
	double t0 = agora_ms();

	/* Usa as duas chaves: o gênero leva ao bucket em O(1) e a cadeia
	 * ordenada por artista permite parar assim que o artista corrente
	 * passa do procurado. Ainda assim percorre metade da cadeia em
	 * média, porque lista encadeada não admite busca binária. */
	unsigned long h = hash_djb2(genero) % (unsigned long) ht->capacidade;

	for (NoHash* no = ht->buckets[h]; no; no = no->prox) {
		comp++;
		int c = strcmp(obra_artista(no->obra), artista);
		if (c < 0) continue;
		if (c > 0) break;

		comp++;
		if (strcmp(obra_genero(no->obra), genero) == 0) {
			resultado_adicionar(r, no->obra);
		}
	}

	resultado_set_metricas(r, agora_ms() - t0, comp);
	return r;
}
