# WikiArt Search

Sistema de busca sobre metadados do [WikiArt](https://www.wikiart.org/) com engine em C, benchmark comparativo de três estruturas de dados e interface web.

Desenvolvido para a disciplina de Estruturas de Dados. Corpus de aproximadamente 80.000 obras, 1.119 artistas e 27 estilos.

## Arquitetura

```
Navegador (HTML + CSS + JS)  ->  Servidor HTTP em C  ->  Engine de busca em C
```

A engine em C11 puro implementa os TADs `Obra`, `Resultado`, `Csv` e `Buscador`, além das três estruturas de busca comparadas.

## Estruturas de dados

| Estrutura | Busca por artista | Busca por gênero | Inserção |
|---|---|---|---|
| HashTable (encadeamento) | O(1) esperado | O(n) | O(1) esperado |
| SkipList (probabilística) | O(log n) esperado | O(n) | O(log n) esperado |
| TabelaOrd (lista indexada) | O(log n) | O(n) | O(n) |

A chave de ordenação e de hash é o artista. Gênero não é chave, então a busca por gênero é sempre varredura completa. Cada estrutura implementa a interface `Buscador`, uma vtable que permite plugar novas EDs no benchmark com uma linha de código.

## Resultados

Benchmark sobre 80.042 obras, com 10.000 buscas por operação.

### Busca por artista, número médio de comparações

![Busca por artista, comparações](assets/graficos/bench_80k_buscar_artista_comparacoes.png)

### Busca por artista, tempo médio

![Busca por artista, tempo](assets/graficos/bench_80k_buscar_artista_tempo.png)

### Busca por gênero, número médio de comparações

![Busca por gênero, comparações](assets/graficos/bench_80k_buscar_genero_comparacoes.png)

### Busca por gênero, tempo médio

![Busca por gênero, tempo](assets/graficos/bench_80k_buscar_genero_tempo.png)

A HashTable vence a busca por artista (O(1) esperado), enquanto SkipList e TabelaOrd ficam parelhas em O(log n). A busca por gênero degenera para varredura linear em todas as estruturas, porque gênero não é chave de ordenação.

## Estrutura de pastas

```
wikiart_search/
├── assets/                     # CSVs e graficos do benchmark
├── engine/                     # Engine em C
│   ├── data/
│   ├── include/
│   ├── src/
│   ├── test/
│   └── Makefile
├── scripts/                    # Scripts Python auxiliares
├── web/                        # Interface web
└── README.md
```

## Como usar

### 1. Preparar os dados

Baixe o `classes.csv` do dataset [steubk/wikiart](https://www.kaggle.com/datasets/steubk/wikiart) e coloque na raiz do projeto. Depois:

```bash
uv sync
uv run python scripts/preparar_dados.py
```

Isso gera `engine/data/metadados.csv` no formato esperado pela engine.

### 2. Compilar e testar

```bash
cd engine
make
make test
```

A suíte tem 61 testes. Para checar vazamentos:

```bash
make test-valgrind
```

### 3. Rodar o benchmark

```bash
./wikiart_server data/metadados.csv --bench 10000 > ../assets/bench_80k.csv
cd ..

uv run python scripts/plotar_benchmark.py assets/bench_80k.csv
```

Gera os quatro gráficos em `assets/graficos/`.

### 4. Modo de listagem

```bash
cd engine
./wikiart_server data/metadados_fake.csv        # lista 5 obras
./wikiart_server data/metadados_fake.csv 20     # lista 20 obras
```

## Autores

- Miguel Mochizuki Silva
- Arthur Gomes
- Leudo Neto

## Licença

Código-fonte sob licença [MIT](LICENSE).

Os dados são provenientes do WikiArt e do dataset público [steubk/wikiart](https://www.kaggle.com/datasets/steubk/wikiart), destinados apenas a pesquisa não-comercial, conforme os termos do [WikiArt.org](https://www.wikiart.org/).
