# Makefile
# Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
# Descrição: Makefile da engine C do projeto

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -g
INCLUDE := -Iinclude

# Núcleo compartilhado pelo servidor e pelos testes
CORE    := src/csv.c src/obra.c src/resultado.c \
           src/tabela_ord.c src/arvore_afunilada.c \
           src/buscador_tabela_ord.c src/buscador_arvore_afunilada.c \
           src/catalogo.c src/indice.c

SRC     := src/main.c $(CORE) \
           src/benchmark.c src/json.c src/server.c
OBJ     := $(SRC:.c=.o)
BIN     := wikiart_server

TEST_INC    := -Itest -Iinclude
TEST_SRC    := test/munit.c \
               test/main_test.c \
               test/test_obra.c \
               test/test_resultado.c \
               test/test_csv.c \
               test/test_tabela_ord.c \
               test/test_arvore_afunilada.c \
               test/test_catalogo.c \
               test/test_indice.c \
               $(CORE)
TEST_BIN    := test_runner

.PHONY: all clean run bench valgrind test test-valgrind

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

run: $(BIN)
	./$(BIN) data/metadados_fake.csv

bench: $(BIN)
	./$(BIN) data/metadados_fake.csv --bench 1000

valgrind: $(BIN)
	valgrind --leak-check=full --error-exitcode=1 ./$(BIN) data/metadados_fake.csv

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_INC) -o $@ $^

test-valgrind: $(TEST_BIN)
	valgrind --leak-check=full --error-exitcode=1 ./$(TEST_BIN)

clean:
	rm -f $(OBJ) $(BIN) $(TEST_BIN) test/*.o
