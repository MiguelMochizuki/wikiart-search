# WikiArt Search

Sistema de busca sobre metadados do [WikiArt](https://www.wikiart.org/) com engine em C, benchmark comparativo de três estruturas de dados e interface web.

Desenvolvido para a disciplina de Estruturas de Dados. Corpus de aproximadamente 80.000 obras, 1.119 artistas e 27 estilos.

## Arquitetura

```
Navegador (HTML + CSS + JS)  ->  Servidor HTTP em C  ->  Engine de busca em C
```

A engine em C11 puro implementa os TADs `Obra`, `Resultado`, `Csv` e `Buscador`, além das três estruturas de busca comparadas.

## Estruturas de dados

As três estruturas indexam pela mesma chave composta **(gênero, artista)**:
gênero primário, artista secundário. `k` é o número de obras do bloco alcançado.

| Estrutura | Como aplica a chave | Busca por artista | Busca por gênero | Gênero + artista | Inserção |
|---|---|---|---|---|---|
| HashTable (encadeamento) | gênero no bucket, artista ordena a cadeia | O(n) | O(k) | O(k) | O(k) |
| SkipList (probabilística) | nível 0 ordenado pela chave composta | O(n) | O(log n + k) esperado | O(log n + k) esperado | O(log n) esperado |
| TabelaOrd (lista indexada) | array ordenado pela chave composta | O(n) | O(log n + k) | O(log n + k) | O(n) |

Cada uma traduz a chave do jeito que a sua natureza permite. A **TabelaOrd**
ordena o array e faz busca binária. A **SkipList** ordena o nível 0 e desce
pelos níveis. A **HashTable** não tem ordem nenhuma, então parte a chave em
duas: o gênero escolhe o bucket, o artista ordena a cadeia dentro dele.

O efeito é comum às três: gênero e o par gênero+artista ficam rápidos, e a
busca só por artista degenera em varredura completa, porque as obras de um
mesmo artista ficam espalhadas entre os blocos de gênero.

A HashTable paga essa tradução duas vezes. Só existem 27 gêneros, então a
tabela tem 27 buckets ocupados com cadeias de milhares de nós — por isso a
capacidade foi reduzida de 2^18 para 2^10 buckets, já que o que a dimensiona
agora é o número de gêneros, não o de obras. E manter cada cadeia ordenada
custa uma varredura por inserção, não O(1).

Cada estrutura implementa a interface `Buscador`, uma vtable que permite plugar novas EDs no benchmark com uma linha de código.

## Resultados

Benchmark sobre 80.042 obras, com 10.000 buscas por operação. Dados brutos em
`assets/bench_80k.csv`.

| Estrutura | Operação | Tempo médio (ms) | Comparações médias |
|---|---|---|---|
| skip_list | buscar_artista | 2.3558 | 80042.00 |
| skip_list | buscar_genero | 0.1558 | 6420.70 |
| skip_list | buscar_genero_artista | **0.0094** | **284.12** |
| hash_table | buscar_artista | 0.8876 | 40367.03 |
| hash_table | buscar_genero | 0.1481 | 6332.78 |
| hash_table | buscar_genero_artista | 0.0626 | 3526.26 |
| tabela_ord | buscar_artista | 1.6900 | 80042.00 |
| tabela_ord | buscar_genero | 0.1506 | 6312.01 |
| tabela_ord | buscar_genero_artista | **0.0091** | **267.74** |

Custo de carga das 80.042 obras:

| Estrutura | Inserção (ms) |
|---|---|
| hash_table | 2075.75 |
| tabela_ord | 138.29 |
| skip_list | 55.05 |

### Busca por artista, número médio de comparações

![Busca por artista, comparações](assets/graficos/bench_80k_buscar_artista_comparacoes.png)

### Busca por artista, tempo médio

![Busca por artista, tempo](assets/graficos/bench_80k_buscar_artista_tempo.png)

### Busca por gênero, número médio de comparações

![Busca por gênero, comparações](assets/graficos/bench_80k_buscar_genero_comparacoes.png)

### Busca por gênero, tempo médio

![Busca por gênero, tempo](assets/graficos/bench_80k_buscar_genero_tempo.png)

### Busca por gênero e artista, número médio de comparações

![Busca por gênero e artista, comparações](assets/graficos/bench_80k_buscar_genero_artista_comparacoes.png)

### Busca por gênero e artista, tempo médio

![Busca por gênero e artista, tempo](assets/graficos/bench_80k_buscar_genero_artista_tempo.png)

**Busca por gênero.** As três empatam em torno de 6.300 comparações, e não é
coincidência: esse é o tamanho médio do bloco de um gênero, ponderado pela
chance de cada gênero ser sorteado. Chegar ao bloco é barato nas três, seja por
busca binária, descida de níveis ou bucket. O que sobra é copiar o resultado, e
isso ninguém evita.

**Busca por gênero e artista.** Aqui aparece a diferença real. SkipList (284) e
TabelaOrd (268) resolvem em poucas centenas de comparações, porque ambas
localizam o bloco do par exato e só percorrem ele. A HashTable precisa de 3.526,
uma ordem de grandeza a mais: o bucket resolve o gênero em O(1), mas o artista
está dentro de uma cadeia encadeada, e lista encadeada não admite busca
binária. Sobra percorrer metade da cadeia — cerca de 3.100 nós.

**Busca por artista.** Degenera nas três, como esperado de uma chave
secundária. SkipList e TabelaOrd pagam as 80.042 comparações cheias. A
HashTable pagou 40.367, quase exatamente metade: como cada cadeia está ordenada
por artista, a busca abandona o bucket assim que passa do nome procurado, e em
média isso acontece no meio. É uma constante menor sobre o mesmo O(n).

**Inserção.** O contraste mais forte da tabela. A HashTable saltou para 2.076 ms
contra 55 ms da SkipList, 38x mais lenta. Manter ordenada uma cadeia de milhares
de nós custa uma varredura a cada inserção, e são 80.042 delas. É o preço
direto de usar ordenação dentro de uma estrutura que não foi feita para
ordenar.

**O balanço.** A chave composta acertou o alvo: `buscar_genero_artista`, a
consulta que a interface realmente faz na navegação estilo → artista → obras,
é a operação mais rápida das três estruturas. O custo foi empurrado para a
busca só por artista, que nenhuma tela usa. Entre as três, SkipList e TabelaOrd
absorveram bem a mudança; a HashTable ficou sendo a pior nas duas pontas que
importam, busca composta e inserção, porque hash e ordenação resolvem problemas
diferentes e forçar uma a imitar a outra cobra caro.

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

A suíte tem 81 testes. Para checar vazamentos:

```bash
make test-valgrind
```

### 3. Rodar o benchmark

```bash
./wikiart_server data/metadados.csv --bench 10000 > ../assets/bench_80k.csv
cd ..

uv run python scripts/plotar_benchmark.py assets/bench_80k.csv
```

Gera os seis gráficos em `assets/graficos/`, dois por operação medida.

### 4. Modo de listagem

```bash
cd engine
./wikiart_server data/metadados_fake.csv        # lista 5 obras
./wikiart_server data/metadados_fake.csv 20     # lista 20 obras
```

### 5. Interface web

A navegação é em três níveis: **estilo → artista → obras**.

```bash
uv run python scripts/gerar_indice.py   # gera web/indice.json a partir do metadados.csv
docker compose up -d                    # sobe engine (8080) e interface (3000)
```

Abra <http://localhost:3000>.

As listas de estilos e de artistas por estilo saem do `web/indice.json`, um
arquivo estático de ~86 KB gerado do próprio `metadados.csv`. A engine só
responde obras, então montar essas listas pela API custaria baixar o gênero
inteiro (13 mil obras no Impressionismo) apenas para extrair nomes. Regenere o
índice sempre que o `metadados.csv` mudar.

A busca do terceiro nível vai em `/api/busca?genero=&artista=&ed=` e exibe as
métricas que a engine devolve — estrutura, algoritmo, tempo e número de
comparações. O seletor no topo troca a estrutura e refaz a mesma consulta, o
que permite comparar as três lado a lado pela interface.

As imagens vêm direto do endpoint de arquivo único do Kaggle, que é aberto para
este dataset (CC0) e não exige token:

```
https://www.kaggle.com/api/v1/datasets/download/steubk/wikiart/<caminho>
```

Para apontar a interface para uma engine em outra porta, use
`http://localhost:3000/?api=http://localhost:9090` — a escolha fica salva no
navegador.

## Autores

- Miguel Mochizuki Silva
- Arthur Gomes
- Leudo Neto

## Licença

Código-fonte sob licença [MIT](LICENSE).

Os dados são provenientes do WikiArt e do dataset público [steubk/wikiart](https://www.kaggle.com/datasets/steubk/wikiart), destinados apenas a pesquisa não-comercial, conforme os termos do [WikiArt.org](https://www.wikiart.org/).
