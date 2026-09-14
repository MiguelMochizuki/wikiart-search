"""
Converte o classes.csv do Kaggle WikiArt em metadados.csv.

Formato de saída (separador ';'):
    id;titulo;artista;genero;ano;caminho
"""
from pathlib import Path

ENTRADA = Path("classes.csv")
SAIDA = Path("engine/data/metadados.csv")


def main() -> None:
    if not ENTRADA.exists():
        raise SystemExit(
            f"Arquivo {ENTRADA} não encontrado na raiz do projeto.\n"
            "Baixe o classes.csv do Kaggle antes de rodar."
        )
    SAIDA.parent.mkdir(parents=True, exist_ok=True)
    print(f"TODO: converter {ENTRADA} -> {SAIDA}")


if __name__ == "__main__":
    main()
