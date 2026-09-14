# Makefile
# Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
# Descrição: Makefile da engine C do projeto

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -g
INCLUDE := -Iinclude

SRC     := src/main.c src/csv.c src/obra.c src/resultado.c
OBJ     := $(SRC:.c=.o)
BIN     := wikiart_server

TEST_INC    := -Itest -Iinclude
TEST_SRC    := test/munit.c \
               test/main_test.c \
               test/test_obra.c \
               test/test_resultado.c \
               test/test_csv.c \
               test/test_skip_list.c \
               src/obra.c \
               src/resultado.c \
               src/csv.c \
               src/skip_list.c
TEST_BIN    := test_runner

.PHONY: all clean run valgrind test test-valgrind

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

run: $(BIN)
	./$(BIN) data/metadados_fake.csv

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
