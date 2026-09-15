#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"
#include "benchmark.h"
#include "buscador.h"
#include "server.h"

static void listar_primeiras(const Csv* csv, int limite) {
	int n = csv_tamanho(csv);
	int fim = (limite < n) ? limite : n;

	for (int i = 0; i < fim; i++) {
		const Obra* o = csv_obra(csv, i);
		printf("  [%d] %s\n", obra_id(o), obra_titulo(o));
		printf("      artista: %s\n", obra_artista(o));
		printf("      genero:  %s\n", obra_genero(o));
		printf("      ano:     %d\n", obra_ano(o));
		printf("      caminho: %s\n\n", obra_caminho(o));
	}
}

static void uso(const char* prog) {
	fprintf(stderr,
		"Uso:\n"
		"  %s <csv> [limite]              lista as primeiras obras\n"
		"  %s <csv> --bench [n_buscas]    roda o benchmark\n"
		"  %s <csv> --server [porta]      inicia o servidor HTTP da API (padrao: 8080)\n"
		"  %s --help                      mostra esta mensagem\n",
		prog, prog, prog, prog);
}

int main(int argc, char** argv) {
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		uso(argv[0]);
		return 0;
	}

	const char* caminho = (argc > 1) ? argv[1] : "data/metadados_fake.csv";

	Csv* csv = csv_carregar(caminho);
	if (!csv) {
		fprintf(stderr, "Falha ao carregar %s\n", caminho);
		uso(argv[0]);
		return 1;
	}

	int modo_bench = 0;
	int modo_server = 0;
	int n_buscas = 1000;
	int porta = 8080;

	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "--bench") == 0) {
			modo_bench = 1;
			if (i + 1 < argc) {
				n_buscas = atoi(argv[i + 1]);
			}
		} else if (strcmp(argv[i], "--server") == 0) {
			modo_server = 1;
			if (i + 1 < argc && atoi(argv[i + 1]) > 0) {
				porta = atoi(argv[i + 1]);
			}
		}
	}

	if (modo_server) {
		server_iniciar(csv, porta);
	} else if (modo_bench) {
		const Buscador* buscadores[] = {
			&BUSCADOR_SKIP_LIST,
			&BUSCADOR_HASH_TABLE,
			&BUSCADOR_TABELA_ORD,
			NULL
		};
		benchmark_rodar(csv, buscadores, n_buscas);
	} else {
		int n = csv_tamanho(csv);
		int limite = (argc > 2) ? atoi(argv[2]) : 5;
		printf("Carregadas %d obras de %s\n\n", n, caminho);
		listar_primeiras(csv, limite);
	}

	csv_liberar(csv);
	return 0;
}