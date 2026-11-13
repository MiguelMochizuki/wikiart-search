# WikiArt Search

Sistema de busca e navegação sobre os metadados do [WikiArt](https://www.wikiart.org/): uma engine em C11 que indexa 80.042 obras em duas estruturas de dados, um servidor HTTP e uma interface web que mostra a estrutura trabalhando.

Trabalho de Estruturas de Dados e Algoritmos II. Acervo de 80.042 obras, 1.119 artistas e 27 estilos, navegado em três níveis: **estilo → artista → obras**.

## Visão geral

```
Navegador (HTML + CSS + JS)  ->  Servidor HTTP em C  ->  Engine de busca em C
```

- **Duas estruturas de dados**, uma linear e uma hierárquica: uma lista indexada ordenada (`TabelaOrd`, busca binária) e uma árvore afunilada (`ArvoreAfunilada`, splay tree).
- **Uma interface comum** (`Buscador`, uma vtable) que liga as duas ao servidor e ao benchmark. Uma nova estrutura entra no sistema com uma linha de código.
- **Modificações nos algoritmos clássicos**, pensadas para a aplicação, descritas e medidas abaixo.
- **Benchmark** das duas estruturas sobre o acervo completo e **interface web** com uma tela para cada estrutura.

## Estruturas de dados

Cada estrutura mantém três instâncias, uma por nível da navegação. Ambas são genéricas: recebem na criação a ordem entre itens e, a cada busca, um comparador que recorta um intervalo contíguo dessa ordem.

| Nível | Itens | Chave de ordenação | Busca da navegação |
|---|---|---|---|
| 1. Gêneros | 27 estilos, com contagens e capa | nome | todos (listagem) ou um gênero |
| 2. Artistas | 2.082 pares (gênero, artista), com contagem e capa | (gênero, artista) | os artistas de um gênero |
| 3. Obras | 80.042 obras | (gênero, artista, id) | as obras de um artista num gênero |

No nível 2, quem pintou em mais de um gênero aparece uma vez em cada, com a contagem daquele gênero. Ordenar pelo gênero primeiro deixa os artistas de um gênero contíguos, e é isso que permite buscá-los pela chave parcial. No nível 3 o id desempata as obras de um mesmo par e deixa cada chave única, o que a árvore precisa para levar uma obra específica ao topo.

Os itens dos dois primeiros níveis saem do `Catalogo`, que a engine monta do próprio CSV na subida. Cada gênero e cada artista leva uma **obra de capa**: a do artista é a primeira obra dele no gênero, e a do gênero é a capa do seu artista mais prolífico (Pollock no Action Painting, Picasso no Cubismo Analítico).

| Estrutura | Busca de um intervalo | Página de `limite` itens | Inserção | Forma |
|---|---|---|---|---|
| TabelaOrd (lista indexada) | O(log n + k) | O(log n + limite) | O(n) | array contíguo |
| ArvoreAfunilada (splay tree) | O(log n + k) amortizado | O(log n + limite) amortizado | O(log n) amortizado | nós com filhos, pai e tamanho da subárvore |

`k` é o número de itens do intervalo encontrado.

### TabelaOrd

Array ordenado. A busca é uma busca binária de limite inferior até o começo do intervalo, seguida da coleta enquanto o comparador der zero. Inserir no meio obriga a deslocar a cauda.

### ArvoreAfunilada

Árvore binária de busca sem balanceamento explícito. Todo acesso leva o nó acessado até a raiz por rotações, dois níveis por passo: zig-zig quando nó, pai e avô estão alinhados, zig-zag quando fazem cotovelo, e zig quando só falta o pai. O que foi usado há pouco fica perto do topo, e o custo amortizado de cada operação é O(log n), mas uma operação isolada pode custar O(n), e até as buscas alteram a estrutura.

## Modificações nos algoritmos clássicos

Os algoritmos clássicos buscam uma chave exata. A aplicação precisa de blocos inteiros (todos os artistas de um estilo, todas as obras de um artista), em páginas, e sem que a árvore vire uma lista. As modificações:

| Modificação | Em | O que muda em relação ao clássico |
|---|---|---|
| Busca de intervalo por chave parcial | as duas | O comparador devolve <0, 0 ou >0 contra uma chave parcial, e um bloco inteiro sai numa busca. Na tabela, a busca binária vira limite inferior e não para no primeiro casamento. Na árvore, a descida para no primeiro nó de dentro (o mais alto do intervalo), que sobe à raiz; o intervalo fica abaixo dela e a coleta anda de sucessor em sucessor. |
| Tamanho da subárvore em cada nó | árvore | As rotações mantêm `tam = 1 + tam(esq) + tam(dir)` nos dois nós que mudam de lugar, e a inserção o incrementa no caminho. Com ele, o total do intervalo e a posição do primeiro item de uma página saem em O(altura), sem percorrer o intervalo. |
| Busca paginada `buscar_pagina(offset, limite)` | as duas | Devolve só uma fatia do intervalo. Na tabela, dois limites por busca binária dão o começo e o fim do intervalo. Na árvore, os tamanhos dão o nó de partida por seleção, e a coleta anda só `limite` sucessores, sem chamar o comparador por item. |
| Afunilamento ascendente sem recursão | árvore | Ponteiros para o pai permitem afunilar, percorrer em ordem e liberar sem pilha, porque a árvore pode ficar tão funda quanto uma lista. |
| Ordem de carga embaralhada | as duas | O CSV vem agrupado por artista e com ids crescentes. Carregado assim, cada bloco (gênero, artista) entraria em ordem crescente, e a árvore, que leva cada nó novo à raiz, viraria uma lista encadeada pela esquerda. A ordem é embaralhada com semente fixa, e as duas estruturas recebem a mesma. É uma troca, não um ganho de desempenho: ver [`results/RESULTADOS.md`](results/RESULTADOS.md#carga-embaralhada-contra-carga-na-ordem-das-chaves). |
| Vista restrita ao intervalo | árvore | Recorta os quatro primeiros níveis da árvore mostrando só os nós do intervalo, mantendo a hierarquia real entre eles, com o número de descendentes de cada nó calculado pelos tamanhos. |

## Resultados

Benchmark sobre 80.042 obras, 10.000 buscas por operação. Cada estrutura ganha num cenário diferente:

| Cenário | Quem vence | Números |
|---|---|---|
| Leitura aleatória (obras de um artista) | **Tabela**, 2,6x | 4,53 µs contra 11,81 µs: a tabela é um array contíguo, a árvore salta entre nós no heap |
| Leitura com localidade | **Tabela**, mas a árvore se adapta | rotações por busca caem a um terço (10,68 → 3,47) e a árvore fica 2,1x mais rápida, sem alcançar a tabela |
| Carga com ordem de chegada embaralhada | **Árvore**, 2,5x | 94 ms contra 231 ms: inserir no meio do array desloca a cauda |
| Carga na ordem das chaves | **Tabela** e árvore, ambas baratas | 17 ms e 3 ms |

A interface mostra esses cenários na aba **Resultados** (`#/resultados`), desenhada a partir do `bench_80k.csv`. A busca paginada (`buscar_pagina`) corta o tempo em cerca de 5,5x nas duas estruturas, e o servidor usa a tabela como padrão. Análise completa, gráficos e a comparação de cada modificação com o algoritmo clássico: [`results/RESULTADOS.md`](results/RESULTADOS.md). Dados brutos: [`results/bench_80k.csv`](results/bench_80k.csv).

## Interface web

A navegação é em três níveis: **estilo → artista → obras**. O controle de estrutura no topo da tela troca a estrutura e refaz a consulta da tela aberta, e toda tela mostra as métricas da busca (estrutura, algoritmo, tempo, comparações e rotações).

- **Tabela ordenada:** cada tela é uma grade, com estilos e artistas mostrados pela pintura de capa. As obras têm rolagem infinita, que pede à engine uma página de 30 por vez.
- **Árvore afunilada:** a tela desenha a vista dos quatro primeiros níveis da árvore (1 + 2 + 4 + 8 nós). A raiz ocupa a primeira fileira inteira, e cada nó divide a largura do pai com o irmão. Clicar num nó o afunila até a raiz, e a engine devolve a árvore já reorganizada. Clicar na raiz abre o próximo nível ou, nas obras, amplia a pintura. Nos níveis 2 e 3 a vista mostra só o intervalo da tela, e um selo `+N` na última fileira diz quantos itens ficaram abaixo.

As árvores guardam o estado entre requisições, e esse estado é do servidor, não do navegador: todos os clientes veem a mesma árvore, com o último acesso de qualquer um na raiz.

### Entendendo as métricas e a escolha da estrutura

Sob o painel de métricas de cada tela há o quadro **"O que significam estes números?"**, que explica em linguagem simples estrutura, tempo (sem rede nem imagens), comparações (o esforço que não depende da máquina) e rotações. Na tela de obras, o botão **"Comparar as duas estruturas nesta busca"** faz a mesma busca na tabela e na árvore, duas vezes seguidas, e mostra as comparações, as rotações e o tempo de cada uma, com frases que dizem o que aconteceu (por exemplo, se a segunda busca ficou mais barata na árvore porque o artista já estava no topo).

O que a escolha da estrutura muda para quem navega, sem exagero:

- **Velocidade percebida: nada.** As duas respondem em microssegundos, muito abaixo do tempo da rede e do carregamento das imagens.
- **A forma da tela:** lista em ordem (tabela) ou hierarquia viva que se reorganiza a cada clique (árvore).
- **O uso repetido:** a tabela custa o mesmo em toda busca, e a árvore leva o que foi aberto há pouco para perto do topo.
- **A escala e a manutenção:** cadastrar itens novos desloca a tabela inteira, enquanto a árvore só religa nós; num acervo maior ou que cresce, isso pesa, e é o que o benchmark de inserção mostra.

O tema é um claro-escuro de ateliê: fundo de noite, fios de ouro velho e numerais romanos marcando os três níveis. As fontes (Cinzel, Cormorant Garamond e Jost) vêm do Google Fonts, e sem rede a interface cai nas fontes do sistema. Quem usa `prefers-reduced-motion` não vê as animações.

### API

| Rota | Nível | Parâmetros |
|---|---|---|
| `GET /api/generos` | 1 | `ed`, `foco=<gênero>` |
| `GET /api/artistas` | 2 | `genero` (obrigatório), `ed`, `foco=<artista>` |
| `GET /api/busca` | 3 | `genero` (obrigatório), `artista`, `ed`, `foco=<id da obra>`, `offset` e `limite` |
| `GET /api/comparar` | 3 | `genero`, `artista`, `offset` e `limite`: a mesma busca nas duas estruturas |
| `GET /api/insercao` | | mede a carga dos três níveis em cada estrutura, com chegada embaralhada e ordenada, em índices descartáveis (bloqueia o servidor por cerca de 0,5 s no acervo completo) |
| `GET /api/status` | | |

`ed` é `tabela_ord` (padrão) ou `arvore_afunilada`. Com `limite`, a resposta traz só a página, e `total_encontrados` é o tamanho do intervalo inteiro. `foco` acessa um item e, na árvore, o leva à raiz.

### CORS

A API só atende navegadores vindos de origens autorizadas, definidas em `WIKIART_ORIGENS` (lista separada por vírgulas; padrão: a interface do compose, `http://localhost:3000` e `http://127.0.0.1:3000`). Um pedido com `Origin` fora da lista recebe `403` antes de qualquer trabalho, e não altera a árvore compartilhada. Pedidos de origem autorizada recebem `Access-Control-Allow-Origin` com a própria origem, e o navegador bloqueia a leitura para as demais. Pedidos sem `Origin` (curl, healthcheck do Docker) seguem normalmente. Se a interface rodar em outra porta ou host, inclua essa origem:

```bash
WIKIART_ORIGENS=http://localhost:4000 docker compose up -d
```

## Ferramentas

| Parte | Ferramentas |
|---|---|
| Engine e servidor | C11 (gcc, `-Wall -Wextra -Wpedantic`), Make, sockets POSIX |
| Testes | [µnit](https://nemequ.github.io/munit/) (incluído em `engine/test`), valgrind |
| Interface | HTML, CSS e JavaScript puros, sem build |
| Empacotamento | Docker e Docker Compose (engine em Debian slim, interface em nginx) |
| Dados e gráficos | Python 3.12, [uv](https://docs.astral.sh/uv/), pandas, matplotlib, ruff |

## Estrutura de pastas

```
.
├── engine/                     # engine e servidor em C
│   ├── data/                   # metadados (o CSV completo é gerado, ver abaixo)
│   ├── include/                # protótipos dos TADs
│   ├── src/                    # implementação
│   ├── test/                   # suíte µnit
│   ├── Dockerfile
│   └── Makefile
├── results/                    # benchmark: CSV, gráficos e RESULTADOS.md
├── docs/                       # relatório (LaTeX/PDF) e exemplos em HTML
├── scripts/                    # preparação dos dados e gráficos (Python)
├── web/                        # interface web estática
├── docker-compose.yml
├── pyproject.toml
└── README.md
```

## Como usar

### 1. Preparar os dados

Baixe o `classes.csv` do dataset [steubk/wikiart](https://www.kaggle.com/datasets/steubk/wikiart), coloque na raiz do projeto e rode:

```bash
uv sync
uv run python scripts/preparar_dados.py
```

Isso gera `engine/data/metadados.csv` no formato esperado pela engine.

### 2. Interface web

```bash
docker compose up -d --build            # sobe engine (8080) e interface (3000)
```

Abra <http://localhost:3000>. Por padrão a engine sobe com `engine/data/metadados_fake.csv`; para usar o acervo completo, defina `WIKIART_CSV=data/metadados.csv`. Para apontar a interface para uma engine em outra porta, use `http://localhost:3000/?api=http://localhost:9090`.

As imagens vêm do endpoint de arquivo único do Kaggle, aberto para este dataset (CC0) e sem token:

```
https://www.kaggle.com/api/v1/datasets/download/steubk/wikiart/<caminho>
```

### 3. Compilar e testar a engine

```bash
cd engine
make
make test              # 78 testes
make test-valgrind     # confere vazamentos
```

### 4. Benchmark

```bash
cd engine
./wikiart_server data/metadados.csv --bench 10000 > ../results/bench_80k.csv
cd ..
uv run python scripts/plotar_benchmark.py results/bench_80k.csv results/graficos
```

### 5. Modo de listagem

```bash
cd engine
./wikiart_server data/metadados_fake.csv        # lista 5 obras
./wikiart_server data/metadados_fake.csv 20     # lista 20 obras
```

## Processo de desenvolvimento

1. **Base de dados.** Selecionamos o WikiArt conforme a sugestão feita em sala: um acervo multimídia substancialmente grande, com 80.042 obras.
2. **Estruturas de dados.** Escolhemos a árvore afunilada e a tabela ordenada porque ambas permitem uma busca mais eficiente do que a linear. Com n = 80.042 itens:

   | Estrutura | Busca | Inserção |
   |---|---|---|
   | Busca linear (referência) | O(n), até 80.042 comparações | O(1) |
   | Tabela ordenada (busca binária) | O(log n), cerca de 16 comparações; O(log n + k) para um intervalo de k itens | O(n), pelo deslocamento da cauda |
   | Árvore afunilada (splay tree) | O(log n) amortizado; uma operação isolada pode custar O(n) | O(log n) amortizado |

3. **Engine.** Implementamos manualmente, em C, os cabeçalhos e o código-fonte das estruturas, que constituem a engine. Os testes foram implementados com o uso de IA generativa.
4. **Front-end e integração.** A integração entre o front-end e a engine pela API de sockets POSIX foi decidida para manter a integração em baixo nível.
5. **Infraestrutura.** A infraestrutura e o Docker foram escritos com o auxílio de IA generativa, assim como a configuração do projeto Python (`pyproject.toml` e uv).

**Principais dificuldades:** pensar nas modificações das estruturas de dados e avaliar, de forma realista, o impacto delas no sistema.

## Autores

- Miguel Mochizuki Silva
- Arthur Gomes
- Leudo Neto

## Licença

Código-fonte sob licença [MIT](LICENSE).

Os dados são provenientes do WikiArt e do dataset público [steubk/wikiart](https://www.kaggle.com/datasets/steubk/wikiart), destinados apenas a pesquisa não-comercial, conforme os termos do [WikiArt.org](https://www.wikiart.org/).
