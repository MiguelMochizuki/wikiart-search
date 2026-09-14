/**
 * main.c
 * Autor: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Programa de teste. Carrega um CSV de metadados e imprime
 *            as primeiras obras. Serve para validar o TAD Csv.
 */
#include <stdio.h>
#include "csv.h"

int main(int argc, char** argv) {
	const char* caminho = (argc > 1) ? argv[1] : "data/metadados_fake.csv";

	Csv* c = csv_carregar(caminho);
	if (!c) {
		fprintf(stderr, "Falha ao carregar %s\n", caminho);
		return 1;
	}

	int n = csv_tamanho(c);
	printf("Carregadas %d obras de %s\n\n", n, caminho);

	int limite = (n < 5) ? n : 5;
	for (int i = 0; i < limite; i++) {
		const Obra* o = csv_obra(c, i);
		printf("  [%d] %s\n", obra_id(o), obra_titulo(o));
		printf("      artista: %s\n", obra_artista(o));
		printf("      genero:  %s\n", obra_genero(o));
		printf("      ano:     %d\n", obra_ano(o));
		printf("      caminho: %s\n\n", obra_caminho(o));
	}

	csv_liberar(c);
	return 0;
}
