# ═══════════════════════════════════════════════════════════════════════════
# Dockerfile — bottleneck-analyzer-lib
#
# Arquitectura multi-stage:
#
#   STAGE 1 (builder):
#     → Compila la biblioteca dinámica
#     → Compila ejemplos y tests
#     → Ejecuta la suite de pruebas
#
#   STAGE 2 (runtime):
#     → Imagen mínima solo con binarios y .so
#     → Sin código fuente
#     → Sin herramientas de compilación
#
# Uso:
#   docker build --no-cache -t bottleneck-analyzer-lib:1.0 .
#
# Ejecutar:
#   docker run --rm bottleneck-analyzer-lib:1.0
#
# Ejecutar ejemplo interactivo:
#   docker run -it --rm bottleneck-analyzer-lib:1.0 ./example_interactive
# ═══════════════════════════════════════════════════════════════════════════

# ═══════════════════════════════════════════════════════════════════════════
# STAGE 1 — BUILD
# ═══════════════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS builder

LABEL stage="builder"
LABEL description="Compilación de la biblioteca dinámica y tests"

ENV DEBIAN_FRONTEND=noninteractive

# ──────────────────────────────────────────────────────────────────────────
# Instalar herramientas de compilación y librerías de desarrollo
# ──────────────────────────────────────────────────────────────────────────
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        gcc \
        make \
        libc6-dev \
        binutils \
        file \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Directorio de trabajo
WORKDIR /build

# Copiar proyecto completo
COPY . .

# ──────────────────────────────────────────────────────────────────────────
# Compilar biblioteca, ejemplos y tests
# ──────────────────────────────────────────────────────────────────────────
RUN make all

# ──────────────────────────────────────────────────────────────────────────
# Mostrar información de la biblioteca compilada
# ──────────────────────────────────────────────────────────────────────────
RUN echo "" && \
    echo "=== Símbolos exportados ===" && \
    nm -D build/libbottleneck_analyzer.so | grep " T " && \
    echo "" && \
    echo "=== Información del archivo ===" && \
    file build/libbottleneck_analyzer.so && \
    echo ""

# ──────────────────────────────────────────────────────────────────────────
# Ejecutar tests
# ──────────────────────────────────────────────────────────────────────────
RUN echo "=== Ejecutando tests ===" && \
    make tests && \
    echo "=== Tests completados correctamente ==="

# ═══════════════════════════════════════════════════════════════════════════
# STAGE 2 — RUNTIME
# ═══════════════════════════════════════════════════════════════════════════
FROM ubuntu:22.04 AS runtime

LABEL maintainer="bottleneck-analyzer-lib"
LABEL version="1.0.0"
LABEL description="Runtime image for Bottleneck Analyzer Library"

ENV DEBIAN_FRONTEND=noninteractive

# ──────────────────────────────────────────────────────────────────────────
# Instalar solo dependencias mínimas de ejecución
# ──────────────────────────────────────────────────────────────────────────
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libc6 \
        binutils && \
    rm -rf /var/lib/apt/lists/*

# Directorio principal
WORKDIR /app

# ──────────────────────────────────────────────────────────────────────────
# Copiar únicamente artefactos compilados
# ──────────────────────────────────────────────────────────────────────────
COPY --from=builder /build/build/libbottleneck_analyzer.so /usr/local/lib/

COPY --from=builder /build/build/example_basic /app/
COPY --from=builder /build/build/example_interactive /app/
COPY --from=builder /build/build/test_bottleneck /app/

# ──────────────────────────────────────────────────────────────────────────
# Registrar biblioteca dinámica
# ──────────────────────────────────────────────────────────────────────────
RUN ldconfig

# Variable para asegurar búsqueda de librerías
ENV LD_LIBRARY_PATH=/usr/local/lib

# ──────────────────────────────────────────────────────────────────────────
# Verificar que la librería quedó registrada
# ──────────────────────────────────────────────────────────────────────────
RUN echo "=== Bibliotecas registradas ===" && \
    ldconfig -p | grep bottleneck && \
    echo "Library registered successfully."

# ──────────────────────────────────────────────────────────────────────────
# Ejecutable por defecto
# ──────────────────────────────────────────────────────────────────────────
ENTRYPOINT ["./example_basic"]