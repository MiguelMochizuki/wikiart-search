"""
plotar_benchmark.py
Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
Descrição: Lê o CSV de saída do benchmark e gera gráficos de barras
           comparando tempo médio e comparações médias entre as EDs, um par
           por nível da navegação (gênero, artistas do gênero, obras do
           artista).

Uso:
    uv run python scripts/plotar_benchmark.py <caminho_csv> [<saida_dir>]
"""

from __future__ import annotations

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def gerar_grafico(df: pd.DataFrame, operacao: str, metrica: str,
                  titulo: str, eixo_y: str, saida: Path) -> None:
    """Gera um gráfico de barras para uma operação e uma métrica."""
    recorte = df[df["operacao"] == operacao].copy()
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

    for operacao in ("buscar_genero", "buscar_artistas_genero", "buscar_obras_artista"):
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

    print(f"\nTodos os graficos foram salvos em: {saida_dir}")


if __name__ == "__main__":
    main()
