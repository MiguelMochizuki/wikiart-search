"""
gerar_indice.py
Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
Descrição: Gera o índice estático estilo -> artistas consumido pela interface web.

A engine expõe apenas /api/busca, que responde obras. Listar os estilos e os
artistas de um estilo por ali exigiria baixar o gênero inteiro (13k obras no
Impressionismo) só para extrair nomes. Este índice troca isso por um arquivo de
~80 KB, deixando a API para o que ela existe: a busca em si.

Formato de saída:
    {"total_obras": int, "estilos": [
        {"nome": str, "obras": int,
         "artistas": [{"nome": str, "obras": int}, ...]}, ...]}

Uso:
    uv run python scripts/gerar_indice.py
"""

from __future__ import annotations

import csv
import json
from collections import Counter, defaultdict
from pathlib import Path

ENTRADA = Path("engine/data/metadados.csv")
SAIDA = Path("web/indice.json")


def main() -> None:
    if not ENTRADA.exists():
        raise SystemExit(
            f"{ENTRADA} não encontrado. Rode antes: uv run python scripts/preparar_dados.py"
        )

    # Um Counter por estilo: conta obras de cada artista sem carregar as obras.
    por_estilo: dict[str, Counter[str]] = defaultdict(Counter)
    total = 0

    with ENTRADA.open(encoding="utf-8", newline="") as f:
        for linha in csv.DictReader(f, delimiter=";"):
            genero = (linha.get("genero") or "").strip()
            artista = (linha.get("artista") or "").strip()
            if not genero or not artista:
                continue
            por_estilo[genero][artista] += 1
            total += 1

    estilos = [
        {
            "nome": estilo,
            "obras": sum(artistas.values()),
            # Mais prolíficos primeiro: é a ordem útil para quem está navegando.
            "artistas": [
                {"nome": nome, "obras": n} for nome, n in artistas.most_common()
            ],
        }
        for estilo, artistas in sorted(por_estilo.items())
    ]

    SAIDA.parent.mkdir(parents=True, exist_ok=True)
    with SAIDA.open("w", encoding="utf-8") as f:
        json.dump({"total_obras": total, "estilos": estilos}, f, ensure_ascii=False)

    kb = SAIDA.stat().st_size / 1024
    print(f"{SAIDA}: {len(estilos)} estilos, {total} obras, {kb:.1f} KB")


if __name__ == "__main__":
    main()
