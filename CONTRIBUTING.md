# Contribuindo

## Convenções de documentação

Código, comentários e documentação em português (PT-BR), com acentuação completa. Todo arquivo de código abre com um cabeçalho com nome, autores e descrição; toda função declarada num `.h` (C) ou definida em Python leva um comentário curto. Arquivos de terceiros (`engine/test/munit.*`) ficam como estão.

### C

Cabeçalho do arquivo e, em cada protótipo do `.h`, um bloco `/** */` com `Parâmetros` e `Retorna` quando houver:

```c
/**
 * indice.h
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Protótipo do TAD Indice. Monta, para uma ED, as três
 *            estruturas da navegação e responde às buscas de cada nível.
 */

/** Busca uma página do intervalo da chave num nível
 *
 * Parâmetros:
 * Indice* ix: ponteiro para o índice (a busca pode reorganizar a ED)
 * Nivel nivel: nível consultado
 * int offset: itens do intervalo a pular
 *
 * Retorna Resultado*: a página; resultado_total dá o intervalo inteiro
 */
```

Os pontos em que o código muda o algoritmo clássico levam o marcador `// MODIFICACAO`.

### Python

Docstring de módulo com `Autores`, `Descrição` e `Uso`; funções com docstring de uma linha:

```python
"""
preparar_dados.py
Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
Descrição: Converte o classes.csv do Kaggle WikiArt em metadados.csv.

Uso:
    uv run python scripts/preparar_dados.py
"""

def sanitizar(s: str) -> str:
    """Remove o separador ';' e normaliza espaços."""
```

Linha de até 100 colunas, verificada com `uv run ruff check .`.

### JavaScript

Cabeçalho `/** */` com `Autores` e `Descrição` e, antes de cada função não trivial, um comentário `/** */` que explica o porquê:

```js
/**
 * app.js
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Navegação estilo -> artista -> obras da interface web.
 */
```
