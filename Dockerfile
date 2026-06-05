# ═══════════════════════════════════════════════════════════════════════════
# Dockerfile — bottleneck-analyzer-lib
#
# Arquitectura multi-stage:
#
#   STAGE 1 (builder): Ubuntu + GCC + Make
#     → Compila libbottleneck_analyzer.so
#     → Compila ejemplos y tests
#     → Ejecuta la suite de tests
#
#   STAGE 2 (runtime): Ubuntu mínimo
#     → Solo tiene la biblioteca .so y los ejemplos compilados
#     → Sin GCC, sin código fuente
#     → Imagen final liviana (~80 MB)
#
# Uso:
#   docker build -t bottleneck-analyzer-lib:1.0 .
#   docker run --rm bottleneck-analyzer-lib:1.0
#   docker run --rm bottleneck-analyzer-lib:1.0 ./example_interactive
# ═══════════════════════════════════════════════════════════════════════════

# ════════════════════════════════════════════════════════════════════════════
# STAGE 1 — Build
# ════════════════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS builder

LABEL stage="builder"
LABEL description="Compila libbottleneck_analyzer.so, ejemplos y tests"

# Evitar prompts interactivos durante apt-get
ENV DEBIAN_FRONTEND=noninteractive

# Instalar herramientas de compilación
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc \
        make \
        binutils \
        file \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copiar el proyecto completo
COPY . .

# Compilar la biblioteca y los ejemplos
RUN make all

# ── Mostrar información de la biblioteca compilada ─────────────────────────
RUN echo "" && \
    echo "=== Tabla de símbolos exportados de libbottleneck_analyzer.so ===" && \
    nm -D build/libbottleneck_analyzer.so | grep " T " && \
    echo "" && \
    echo "=== Tipo de archivo ===" && \
    file build/libbottleneck_analyzer.so && \
    echo ""

# ── Ejecutar la suite de tests dentro del build ────────────────────────────
RUN echo "=== Ejecutando tests ===" && \
    make tests && \
    echo "=== Tests completados ==="

# ════════════════════════════════════════════════════════════════════════════
# STAGE 2 — Runtime (imagen final)
# ════════════════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS runtime

LABEL maintainer="bottleneck-analyzer-lib"
LABEL version="1.0.0"
LABEL description="Bottleneck Analyzer Library — Runtime image"

ENV DEBIAN_FRONTEND=noninteractive

# Solo las librerías de sistema necesarias para ejecutar binarios C
# (libc ya viene en ubuntu:22.04; libm es la única dependencia adicional)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libm-dev \
        binutils && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# ── Copiar solo los artefactos compilados desde el builder ─────────────────
COPY --from=builder /build/build/libbottleneck_analyzer.so /usr/local/lib/
COPY --from=builder /build/build/example_basic             ./
COPY --from=builder /build/build/example_interactive       ./
COPY --from=builder /build/build/test_bottleneck           ./

# Registrar la biblioteca en el enlazador dinámico del sistema
# ldconfig lee /usr/local/lib y actualiza /etc/ld.so.cache
RUN ldconfig

# ── Verificar que la biblioteca es accesible ──────────────────────────────
RUN ldconfig -p | grep bottleneck && \
    echo "Library registered successfully."

# El entrypoint por defecto ejecuta el ejemplo básico
# Se puede sobrescribir con: docker run <img> ./example_interactive
ENTRYPOINT ["./example_basic"]
