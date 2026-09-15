# Dockerfile
# Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
# Descrição: Build multi-estágio da engine C. Produz a imagem de runtime do
#            servidor HTTP e uma imagem auxiliar com a suíte de testes.

# ==============================
# Base de compilação
# ==============================
FROM debian:bookworm-slim AS base

RUN apt-get update \
 && apt-get install -y --no-install-recommends gcc make libc6-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# ==============================
# Build do servidor
# ==============================
FROM base AS build

COPY Makefile ./
COPY include/ ./include/
COPY src/ ./src/

RUN make

# ==============================
# Testes (munit + valgrind)
# ==============================
FROM base AS test

RUN apt-get update \
 && apt-get install -y --no-install-recommends valgrind \
 && rm -rf /var/lib/apt/lists/*

COPY Makefile ./
COPY include/ ./include/
COPY src/ ./src/
COPY test/ ./test/
COPY data/ ./data/

RUN make test_runner

CMD ["./test_runner"]

# ==============================
# Runtime
# ==============================
FROM debian:bookworm-slim AS runtime

RUN apt-get update \
 && apt-get install -y --no-install-recommends curl \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --create-home --uid 1000 wikiart

WORKDIR /app

COPY --from=build /build/wikiart_server ./wikiart_server
COPY data/ ./data/

USER wikiart

ENV WIKIART_CSV=data/metadados_fake.csv \
    WIKIART_PORTA=8080

EXPOSE 8080

# stdbuf -oL: o servidor usa printf sem flush. Sem isso o stdout fica
# totalmente bufferizado no pipe do Docker e `compose logs` vem vazio.
CMD ["sh", "-c", "exec stdbuf -oL ./wikiart_server \"$WIKIART_CSV\" --server \"$WIKIART_PORTA\""]
