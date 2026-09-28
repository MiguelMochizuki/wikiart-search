# WikiArt Search

Sistema de busca sobre metadados do [WikiArt](https://www.wikiart.org/) com engine em C, benchmark comparativo de duas estruturas de dados e interface web.

Desenvolvido para a disciplina de Estruturas de Dados. Corpus de aproximadamente 80.000 obras, 1.119 artistas e 27 estilos.

## Arquitetura

```
Navegador (HTML + CSS + JS)  ->  Servidor HTTP em C  ->  Engine de busca em C
```

A engine em C11 puro implementa os TADs `Obra`, `Resultado`, `Csv`,
`Catalogo` e `Indice`, as duas estruturas de busca comparadas e a interface
`Buscador`, uma vtable que permite plugar novas EDs no servidor e no benchmark
com uma linha de código.

## Estruturas de dados

A navegação tem três níveis, **estilo → artista → obras**, e cada ED mantém
três estruturas, uma por nível. As duas EDs são genéricas: recebem na criação a
ordem entre itens e, a cada busca, um comparador que recorta um intervalo
contíguo dessa ordem.

| Nível | Itens | Chave de ordenação | Busca da navegação |
|---|---|---|---|
| 1. Gêneros | 27 estilos, com contagens e capa | nome | todos (listagem) ou um gênero |
| 2. Artistas | 2.082 pares (gênero, artista), com contagem e capa | (gênero, artista) | os artistas de um gênero |
| 3. Obras | 80.042 obras | (gênero, artista, id) | as obras de um artista num gênero |

No nível 2, quem pintou em mais de um gênero aparece uma vez em cada, com a
contagem daquele gênero. Ordenar pelo gênero primeiro deixa os artistas de um
gênero contíguos, e é isso que permite buscá-los pela chave parcial. No nível 3
o id desempata as obras de um mesmo par e deixa cada chave única, que é o que a
árvore precisa para levar uma obra específica ao topo.

Os itens dos dois primeiros níveis saem do `Catalogo`, que a engine monta do
próprio CSV na subida. Cada gênero e cada artista leva uma **obra de capa**:
a do artista é a primeira obra dele no gênero, e a do gênero é a capa do seu
artista mais prolífico (Pollock no Action Painting, Picasso no Cubismo
Analítico).

| Estrutura | Busca por chave | Inserção | Forma |
|---|---|---|---|
| TabelaOrd (lista indexada) | O(log n + k) | O(n) | array contíguo |
| ArvoreAfunilada (splay tree) | O(log n + k) amortizado | O(log n) amortizado | nós com três ponteiros (filhos e pai) |

`k` é o número de itens do intervalo encontrado.

A **TabelaOrd** guarda tudo num array ordenado: busca binária de limite
inferior até o começo do intervalo e coleta enquanto o comparador der zero.
Inserir no meio obriga a deslocar a cauda.

A **ArvoreAfunilada** é uma árvore binária de busca sem balanceamento
explícito. Todo acesso leva o nó acessado até a raiz por rotações, dois níveis
por passo: zig-zig quando nó, pai e avô estão alinhados, zig-zag quando fazem
cotovelo, e zig quando só falta o pai. Afunilar um nó de profundidade `d` custa
exatamente `d` rotações. O que foi usado há pouco fica perto do topo, e o custo
amortizado de cada operação é O(log n), mas uma operação isolada pode custar
O(n), e até as buscas alteram a estrutura. Na busca de um intervalo, a descida
para no primeiro nó de dentro, que é o mais alto do intervalo, e é ele que sobe
à raiz. Depois disso o intervalo inteiro fica abaixo da raiz, e a coleta anda
de sucessor em sucessor. Busca sem achado também afunila, com o último nó do
caminho. Nenhum percurso é recursivo: a árvore pode virar uma lista, e os
ponteiros para o pai permitem percorrer e liberar tudo sem pilha.

A ordem de carga é embaralhada com semente fixa, e as duas EDs recebem a mesma.
O CSV vem agrupado por artista e com ids crescentes. Carregado assim, cada
bloco (gênero, artista) entraria em ordem crescente, e a árvore, que leva cada
nó novo à raiz, viraria uma lista encadeada pela esquerda em cada bloco.

## Resultados

Benchmark sobre 80.042 obras, com 10.000 buscas por operação. Cada busca
sorteia uma obra e usa a chave dela até o nível medido, então gêneros e
artistas pesam na proporção do acervo, como numa navegação real. As duas EDs
respondem à mesma sequência de consultas. Dados brutos em
`assets/bench_80k.csv`.

| Estrutura | Operação | Tempo médio (µs) | Comparações médias | Rotações médias |
|---|---|---|---|---|
| tabela_ord | buscar_genero | **0,12** | **6,88** | 0 |
| tabela_ord | buscar_artistas_genero | **1,71** | **147,55** | 0 |
| tabela_ord | buscar_obras_artista | **5,36** | **268,91** | 0 |
| arvore_afunilada | buscar_genero | 0,16 | 7,77 | 3,24 |
| arvore_afunilada | buscar_artistas_genero | 2,25 | 149,16 | 3,25 |
| arvore_afunilada | buscar_obras_artista | 13,45 | 276,37 | 10,71 |

Custo de carga dos três níveis (27 + 2.082 + 80.042 itens):

| Estrutura | Inserção (ms) |
|---|---|
| tabela_ord | 316,25 |
| arvore_afunilada | **95,47** |

### Busca de um gênero, número médio de comparações

![Busca de um gênero, comparações](assets/graficos/bench_80k_buscar_genero_comparacoes.png)

### Busca de um gênero, tempo médio

![Busca de um gênero, tempo](assets/graficos/bench_80k_buscar_genero_tempo.png)

### Busca dos artistas de um gênero, número médio de comparações

![Busca dos artistas de um gênero, comparações](assets/graficos/bench_80k_buscar_artistas_genero_comparacoes.png)

### Busca dos artistas de um gênero, tempo médio

![Busca dos artistas de um gênero, tempo](assets/graficos/bench_80k_buscar_artistas_genero_tempo.png)

### Busca das obras de um artista, número médio de comparações

![Busca das obras de um artista, comparações](assets/graficos/bench_80k_buscar_obras_artista_comparacoes.png)

### Busca das obras de um artista, tempo médio

![Busca das obras de um artista, tempo](assets/graficos/bench_80k_buscar_obras_artista_tempo.png)

**Busca de um gênero.** A tabela faz 6,88 comparações: busca binária sobre 27
itens (log₂ 27 ≈ 4,75) mais as duas da coleta. A árvore faz 7,77, e as 3,24
rotações dizem a profundidade média em que o gênero estava. Uma árvore
perfeitamente balanceada de 27 nós tem profundidade média 3,04, e a afunilada
chega perto disso sem balancear nada, porque as consultas são enviesadas:
38% delas caem em Impressionism, Realism e Romanticism, e a entropia da
distribuição é 4,07 bits, contra os 4,75 de uma uniforme. O que é muito
pedido fica perto do topo. A diferença que resta vem da coleta genérica de
intervalo: para confirmar que não há outro item do intervalo à esquerda da
raiz, a árvore desce a espinha direita da subárvore esquerda.

**Busca dos artistas de um gênero.** 147,55 contra 149,16 comparações, quase
tudo coleta: o gênero sorteado tem em média 135 artistas, e as duas pagam uma
comparação por artista coletado. Chegar ao bloco custa cerca de 11 comparações
na tabela e 13 na árvore.

**Busca das obras de um artista.** As comparações quase empatam (268,91 contra
276,37), porque o bloco médio tem 251 obras e a coleta domina nas duas. O
relógio não empata: 5,36 µs da tabela contra 13,45 µs da árvore, 2,5x. É
localidade de memória. A tabela percorre um array contíguo, e a árvore salta
entre 80 mil nós espalhados pelo heap, andando de sucessor em sucessor pelos
ponteiros (às vezes subindo pelos pais), e ainda faz 10,71 rotações por busca,
cada uma reescrevendo até seis ponteiros.

**Inserção.** Aqui a ordem se inverte. A árvore carrega os três níveis em 95 ms
contra 316 ms da tabela, 3,3x mais rápida, e é a diferença entre O(log n)
amortizado e O(n): com a carga embaralhada, cada inserção no meio do array
desloca em média metade da cauda, enquanto a árvore só desce e religa
ponteiros.

**O balanço.** A tabela responde mais rápido nos três níveis, e a árvore
constrói o índice 3,3x mais rápido e se adapta ao uso, o que a tabela não faz.
Como o índice é montado uma vez na subida do servidor e consultado a cada
navegação, a tabela é o padrão da interface. A árvore entra como a vista que
mostra a estrutura trabalhando.

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

A suíte tem 73 testes. Para checar vazamentos:

```bash
make test-valgrind
```

### 3. Rodar o benchmark

```bash
./wikiart_server data/metadados.csv --bench 10000 > ../assets/bench_80k.csv
cd ..

uv run python scripts/plotar_benchmark.py assets/bench_80k.csv assets/graficos
```

Gera os seis gráficos em `assets/graficos/`, dois por nível da navegação.

### 4. Modo de listagem

```bash
cd engine
./wikiart_server data/metadados_fake.csv        # lista 5 obras
./wikiart_server data/metadados_fake.csv 20     # lista 20 obras
```

### 5. Interface web

A navegação é em três níveis: **estilo → artista → obras**.

```bash
docker compose up -d --build            # sobe engine (8080) e interface (3000)
```

Abra <http://localhost:3000>.

O tema é um claro-escuro de ateliê: fundo de noite, fios de ouro velho e
numerais romanos marcando os três níveis. As miniaturas entram rebaixadas e só
acendem sob o cursor. As fontes (Cinzel, Cormorant Garamond e Jost) vêm do
Google Fonts; sem rede a interface cai nas serifadas e sans do sistema, sem
quebrar o layout. Quem usa `prefers-reduced-motion` não vê as animações nem as
pétalas de fundo.

Os três níveis vêm da engine, cada um da sua estrutura:

| Rota | Nível | Parâmetros |
|---|---|---|
| `GET /api/generos` | 1 | `ed`, `foco=<gênero>` |
| `GET /api/artistas` | 2 | `genero` (obrigatório), `ed`, `foco=<artista>` |
| `GET /api/busca` | 3 | `genero` (obrigatório), `artista`, `ed`, `foco=<id da obra>` |
| `GET /api/comparar` | 3 | `genero`, `artista`: a mesma busca nas duas EDs |
| `GET /api/status` | | |

`ed` é `tabela_ord` (padrão) ou `arvore_afunilada`. Toda resposta traz as
métricas da busca (estrutura, algoritmo, tempo, comparações e rotações), e a
interface as mostra no topo das três telas. O seletor no topo troca a estrutura
e refaz a consulta da tela aberta.

Com a **tabela ordenada**, a resposta é a lista do nível, em ordem de chave, e
cada tela é uma grade: estilos e artistas com a pintura de capa, obras com
rolagem infinita.

Com a **árvore afunilada**, a resposta é a vista dos quatro primeiros níveis da
árvore (1 + 2 + 4 + 8 nós, em layout de heap), e a tela desenha essa
hierarquia: a raiz ocupa a primeira fileira inteira, e cada nó divide a largura
do pai com o irmão. Clicar num nó manda o `foco` e o afunila até a raiz, e a
engine devolve a árvore já reorganizada, com o ramo dele à vista. Clicar na
raiz abre o próximo nível (os artistas do estilo, as obras do artista) ou, nas
obras, amplia a pintura. Nos níveis 2 e 3 a vista mostra só o intervalo da
tela, os artistas daquele estilo ou as obras daquele artista, mantendo entre
eles a hierarquia real: nós de fora do intervalo que estejam no caminho são
pulados. Um selo `+N` na última fileira diz quantos itens ficaram abaixo.

As árvores guardam o estado entre requisições, e esse estado é do servidor, não
do navegador: todos os clientes veem a mesma árvore, com o último acesso de
qualquer um na raiz.

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
