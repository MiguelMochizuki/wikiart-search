/**
 * obra.h
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD Obra
 */
#ifndef OBRA_H
#define OBRA_H

/* ==============================
 * TADs
 * ============================== */

typedef struct obra_t Obra;

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
		const char* caminho);

/** Libera espaço alocado para uma obra
 *
 * Parâmetros:
 * Obra* o: ponteiro para obra a ser liberada
 */
void obra_liberar(Obra* o);

/* ==============================
 * Getters
 * ============================== */

int obra_id(const Obra* o);
const char* obra_titulo(const Obra* o);
const char* obra_artista(const Obra* o);
const char* obra_genero(const Obra* o);
int obra_ano(const Obra* o);
const char* obra_caminho(const Obra* o);

#endif
