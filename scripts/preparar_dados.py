"""
preparar_dados.py
Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
Descrição: Converte o classes.csv do Kaggle WikiArt em metadados.csv
           com o formato esperado pela engine C.

Formato de saída (separador ';'):
    id;titulo;artista;genero;ano;caminho

Uso:
    uv run python scripts/preparar_dados.py
"""

from __future__ import annotations

import csv
import re
from pathlib import Path

ENTRADA = Path("classes.csv")
SAIDA = Path("engine/data/metadados.csv")

# Regex para extrair o ano do final do filename: ...-1955.jpg
RE_ANO = re.compile(r"-(\d{4})\.(jpg|jpeg|png)$", re.IGNORECASE)


def limpar_genero(bruto: str) -> str:
    """Converte "['Abstract Expressionism']" em "Abstract Expressionism"."""
    if not bruto:
        return "Unknown"
    s = bruto.strip().strip("[]").replace("'", "").replace('"', "")
    if "," in s:
        s = s.split(",", 1)[0]
    return s.strip() or "Unknown"


def extrair_ano(filename: str) -> int:
    """Extrai o ano do final do filename. Devolve 0 se não encontrar."""
    m = RE_ANO.search(filename)
    return int(m.group(1)) if m else 0


def extrair_titulo(description: str, filename: str) -> str:
    """Usa description se disponível, senão o slug do filename."""
    if description and description.strip():
        return description.strip()
    base = filename.split("/")[-1].rsplit(".", 1)[0]
    partes = base.split("_", 1)
    return partes[1] if len(partes) > 1 else base


def sanitizar(s: str) -> str:
    """Remove o separador ';' e normaliza espaços."""
    return s.replace(";", ",").strip()


def main() -> None:
    if not ENTRADA.exists():
        raise SystemExit(
            f"Arquivo {ENTRADA} nao encontrado na raiz do projeto.\n"
            "Baixe o classes.csv do Kaggle (dataset steubk/wikiart)."
        )

    SAIDA.parent.mkdir(parents=True, exist_ok=True)

    with ENTRADA.open(encoding="utf-8") as fin, \
         SAIDA.open("w", newline="", encoding="utf-8") as fout:

        reader = csv.DictReader(fin)
        writer = csv.writer(fout, delimiter=";")
        writer.writerow(["id", "titulo", "artista", "genero", "ano", "caminho"])

        contador = 0
        ignorados = 0

        for row in reader:
            filename = (row.get("filename") or "").strip()
            if not filename:
                ignorados += 1
                continue

            contador += 1
            artista = sanitizar((row.get("artist") or "unknown").lower())
            genero = sanitizar(limpar_genero(row.get("genre", "")))
            titulo = sanitizar(extrair_titulo(row.get("description", ""), filename))
            ano = extrair_ano(filename)

            writer.writerow([contador, titulo, artista, genero, ano, filename])

    print(f"OK: {contador} obras escritas em {SAIDA}")
    if ignorados:
        print(f"    {ignorados} linhas ignoradas (sem filename)")


if __name__ == "__main__":
    main()
