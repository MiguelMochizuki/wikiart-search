"""
plotar_benchmark.py
Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
Descrição: Lê o CSV de saída do benchmark e gera gráficos de barras:
           tempo médio e comparações médias entre as EDs, um par por
           operação (gênero, artistas do gênero, obras do artista e
           primeira página de obras); o efeito de consultas com localidade;
           e o custo de carga com a ordem embaralhada e com a ordenada.

Uso:
    uv run python scripts/plotar_benchmark.py <caminho_csv> [<saida_dir>]
"""

from __future__ import annotations

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

OPERACOES = (
    "buscar_genero",
    "buscar_artistas_genero",
    "buscar_obras_artista",
    "buscar_obras_pagina",
)


ESTRUTURAS = ("tabela_ord", "arvore_afunilada")
COR = {"tabela_ord": "#3b82f6", "arvore_afunilada": "#f59e0b"}


def gerar_grafico(df: pd.DataFrame, operacao: str, metrica: str,
                  titulo: str, eixo_y: str, saida: Path) -> None:
    """Gera um gráfico de barras para uma operação e uma métrica."""
    recorte = df[(df["operacao"] == operacao) & df["estrutura"].isin(ESTRUTURAS)].copy()
    if recorte.empty:
        print(f"Sem dados para operacao={operacao}, pulando.")
        return

    recorte = recorte.sort_values(metrica, ascending=True)

    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.barh(recorte["estrutura"], recorte[metrica], color="#3b82f6")
    ax.set_xlabel(eixo_y)
    ax.set_ylabel("Estrutura de dados")
    ax.set_title(f"{titulo} ({operacao})")
    ax.grid(axis="x", linestyle="--", alpha=0.4)

    for i, valor in enumerate(recorte[metrica]):
        ax.text(valor, i, f"  {valor:.4g}",
                va="center", fontsize=9)

    fig.tight_layout()
    fig.savefig(saida, dpi=150)
    plt.close(fig)
    print(f"Gerado: {saida}")


def gerar_localidade(df: pd.DataFrame, saida: Path) -> None:
    """Compara consultas aleatórias e com localidade, por estrutura."""
    ops = ("buscar_genero", "buscar_artistas_genero",
           "buscar_obras_artista", "buscar_obras_pagina")
    fig, eixos = plt.subplots(1, 2, figsize=(11, 4.5))
    for eixo, (metrica, rotulo, escala) in zip(
        eixos,
        (("media_ms", "Tempo médio (µs)", 1000), ("media_rotacoes", "Rotações médias", 1)),
        strict=True,
    ):
        largura = 0.2
        pos = range(len(ops))
        for j, (est, sufixo, hatch) in enumerate(
            (e, suf, h) for e in ESTRUTURAS for suf, h in (("", ""), ("_local", "//"))
        ):
            valores = [
                df[(df["estrutura"] == est) & (df["operacao"] == op + sufixo)][metrica].iloc[0]
                * escala
                for op in ops
            ]
            eixo.bar([p + (j - 1.5) * largura for p in pos], valores, largura,
                     color=COR[est], hatch=hatch, edgecolor="white",
                     label=f"{est} ({'local' if sufixo else 'aleatória'})")
        eixo.set_xticks(list(pos))
        eixo.set_xticklabels([o.replace("buscar_", "") for o in ops], rotation=15, fontsize=8)
        eixo.set_ylabel(rotulo)
        eixo.set_yscale("symlog")
        eixo.grid(axis="y", linestyle="--", alpha=0.4)
    eixos[0].legend(fontsize=7)
    fig.suptitle("Consultas aleatórias x com localidade")
    fig.tight_layout()
    fig.savefig(saida, dpi=150)
    plt.close(fig)
    print(f"Gerado: {saida}")


def gerar_carga(df: pd.DataFrame, saida: Path) -> None:
    """Compara o custo de carga com a ordem embaralhada e com a ordenada."""
    linhas = df.drop_duplicates("estrutura")[["estrutura", "insercao_ms"]]
    ordem = ["tabela_ord", "tabela_ord_carga_ordenada",
             "arvore_afunilada", "arvore_afunilada_carga_ordenada"]
    linhas = linhas.set_index("estrutura").loc[ordem].reset_index()

    fig, ax = plt.subplots(figsize=(8, 4.5))
    cores = [COR[e.replace("_carga_ordenada", "")] for e in linhas["estrutura"]]
    ax.barh(linhas["estrutura"], linhas["insercao_ms"], color=cores)
    ax.set_xscale("log")
    ax.set_xlabel("Carga dos três níveis (ms, escala log)")
    ax.set_title("Custo de carga: ordem embaralhada x ordenada")
    ax.grid(axis="x", linestyle="--", alpha=0.4)
    for i, valor in enumerate(linhas["insercao_ms"]):
        ax.text(valor, i, f"  {valor:.4g}", va="center", fontsize=9)
    fig.tight_layout()
    fig.savefig(saida, dpi=150)
    plt.close(fig)
    print(f"Gerado: {saida}")


def main() -> None:
    if len(sys.argv) < 2:
        raise SystemExit(
            "Uso: python plotar_benchmark.py <caminho_csv> [<saida_dir>]"
        )

    entrada = Path(sys.argv[1])
    saida_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("resultados/graficos")
    saida_dir.mkdir(parents=True, exist_ok=True)

    if not entrada.exists():
        raise SystemExit(f"Arquivo nao encontrado: {entrada}")

    df = pd.read_csv(entrada)
    if df.empty:
        raise SystemExit("CSV do benchmark esta vazio.")

    base = entrada.stem

    for operacao in OPERACOES:
        gerar_grafico(
            df, operacao, "media_ms",
            "Tempo medio de busca", "Tempo (ms)",
            saida_dir / f"{base}_{operacao}_tempo.png",
        )
        gerar_grafico(
            df, operacao, "media_comparacoes",
            "Comparacoes medias", "Numero de comparacoes",
            saida_dir / f"{base}_{operacao}_comparacoes.png",
        )

    if "buscar_obras_artista_local" in set(df["operacao"]):
        gerar_localidade(df, saida_dir / f"{base}_localidade.png")
    if "tabela_ord_carga_ordenada" in set(df["estrutura"]):
        gerar_carga(df, saida_dir / f"{base}_carga.png")

    print(f"\nTodos os graficos foram salvos em: {saida_dir}")


if __name__ == "__main__":
    main()
