/**
 * main_test.c
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Runner de testes. Agrega todas as suítes e executa.
 */
#include "munit.h"

const MunitSuite* suite_obra_get(void);
const MunitSuite* suite_resultado_get(void);
const MunitSuite* suite_csv_get(void);
const MunitSuite* suite_skip_list_get(void);
const MunitSuite* suite_tabela_ord_get(void);

int main(int argc, char* argv[]) {
	MunitSuite suites[] = {
		*suite_obra_get(),
		*suite_resultado_get(),
		*suite_csv_get(),
		*suite_skip_list_get(),
		*suite_tabela_ord_get(),
		{ NULL, NULL, NULL, 1, MUNIT_SUITE_OPTION_NONE }
	};

	MunitSuite raiz = {
		"",
		NULL,
		suites,
		1,
		MUNIT_SUITE_OPTION_NONE
	};

	return munit_suite_main(&raiz, NULL, argc, argv);
}
