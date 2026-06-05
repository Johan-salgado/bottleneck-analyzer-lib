# ═══════════════════════════════════════════════════════════════════════════
# Makefile — bottleneck-analyzer-lib
#
# Targets principales:
#   make          → compila la biblioteca y los ejemplos
#   make lib      → solo compila la biblioteca (.so)
#   make examples → compila los programas de ejemplo
#   make tests    → compila y ejecuta los tests
#   make clean    → elimina artefactos compilados
#   make install  → instala la biblioteca en /usr/local/lib (requiere sudo)
#   make uninstall→ elimina la biblioteca instalada
#   make info     → muestra información de la biblioteca compilada
# ═══════════════════════════════════════════════════════════════════════════

# ── Compilador y flags ─────────────────────────────────────────────────────
CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c99 -O2
LDFLAGS := -lm

# Flags específicas para la biblioteca dinámica:
#   -shared  : produce un .so (shared object / biblioteca dinámica)
#   -fPIC    : Position Independent Code (obligatorio para .so)
LIB_CFLAGS := $(CFLAGS) -shared -fPIC

# ── Nombres de archivos ────────────────────────────────────────────────────
LIB_NAME    := bottleneck_analyzer
LIB_FILE    := libbottleneck_analyzer.so
LIB_VERSION := 1.0.0

# ── Directorios ───────────────────────────────────────────────────────────
SRC_DIR     := src
INC_DIR     := include
BUILD_DIR   := build
EXAMPLES_DIR:= examples
TESTS_DIR   := tests

# ── Archivos fuente ────────────────────────────────────────────────────────
LIB_SRC     := $(SRC_DIR)/bottleneck_analyzer.c
LIB_OBJ     := $(BUILD_DIR)/bottleneck_analyzer.so

EXAMPLE_SRCS := $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLE_BINS := $(patsubst $(EXAMPLES_DIR)/%.c, $(BUILD_DIR)/%, $(EXAMPLE_SRCS))

TEST_SRCS    := $(wildcard $(TESTS_DIR)/*.c)
TEST_BINS    := $(patsubst $(TESTS_DIR)/%.c, $(BUILD_DIR)/%, $(TEST_SRCS))

# ── Install paths ─────────────────────────────────────────────────────────
INSTALL_LIB_DIR    := /usr/local/lib
INSTALL_HEADER_DIR := /usr/local/include

# ════════════════════════════════════════════════════════════════════════════
# Target por defecto: compilar biblioteca y ejemplos
# ════════════════════════════════════════════════════════════════════════════
.PHONY: all
all: $(BUILD_DIR) lib examples
	@echo ""
	@echo "  ✓  Build complete."
	@echo "     Library : $(BUILD_DIR)/$(LIB_FILE)"
	@echo "     Examples: $(EXAMPLE_BINS)"
	@echo ""

# ── Crear directorio de salida ─────────────────────────────────────────────
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ════════════════════════════════════════════════════════════════════════════
# Biblioteca dinámica
# ════════════════════════════════════════════════════════════════════════════
.PHONY: lib
lib: $(BUILD_DIR) $(BUILD_DIR)/$(LIB_FILE)

$(BUILD_DIR)/$(LIB_FILE): $(LIB_SRC) $(INC_DIR)/bottleneck_analyzer.h
	@echo "  CC  [lib]  $< → $@"
	$(CC) $(LIB_CFLAGS) -I$(INC_DIR) -o $@ $< $(LDFLAGS)
	@echo "  ✓  Library compiled: $@"

# ════════════════════════════════════════════════════════════════════════════
# Ejemplos
# ════════════════════════════════════════════════════════════════════════════
.PHONY: examples
examples: lib $(EXAMPLE_BINS)

$(BUILD_DIR)/%: $(EXAMPLES_DIR)/%.c $(BUILD_DIR)/$(LIB_FILE)
	@echo "  CC  [example] $< → $@"
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $< \
	    -L$(BUILD_DIR) -l$(LIB_NAME) $(LDFLAGS) \
	    -Wl,-rpath,$(BUILD_DIR)

# ════════════════════════════════════════════════════════════════════════════
# Tests
# ════════════════════════════════════════════════════════════════════════════
.PHONY: tests
tests: lib $(TEST_BINS)
	@echo ""
	@echo "  Running tests..."
	@echo ""
	@PASS=0; FAIL=0; \
	for t in $(TEST_BINS); do \
	    echo "  → $$t"; \
	    $$t; \
	    if [ $$? -eq 0 ]; then PASS=$$((PASS+1)); \
	    else FAIL=$$((FAIL+1)); fi; \
	done; \
	echo ""; \
	echo "  Test suites passed: $$PASS  |  failed: $$FAIL"; \
	echo ""

$(BUILD_DIR)/%: $(TESTS_DIR)/%.c $(BUILD_DIR)/$(LIB_FILE)
	@echo "  CC  [test]    $< → $@"
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $< \
	    -L$(BUILD_DIR) -l$(LIB_NAME) $(LDFLAGS) \
	    -Wl,-rpath,$(BUILD_DIR)

# ════════════════════════════════════════════════════════════════════════════
# Información de la biblioteca compilada
# ════════════════════════════════════════════════════════════════════════════
.PHONY: info
info: $(BUILD_DIR)/$(LIB_FILE)
	@echo ""
	@echo "  ── Library info ─────────────────────────────────────"
	@echo "  File    : $(BUILD_DIR)/$(LIB_FILE)"
	@echo "  Version : $(LIB_VERSION)"
	@echo "  Size    : $$(du -sh $(BUILD_DIR)/$(LIB_FILE) | cut -f1)"
	@echo ""
	@echo "  ── Exported symbols (T = text/code) ─────────────────"
	@nm -D $(BUILD_DIR)/$(LIB_FILE) | grep " T " || echo "  (none found)"
	@echo ""
	@echo "  ── File type ────────────────────────────────────────"
	@file $(BUILD_DIR)/$(LIB_FILE)
	@echo ""

# ════════════════════════════════════════════════════════════════════════════
# Instalación del sistema
# ════════════════════════════════════════════════════════════════════════════
.PHONY: install
install: lib
	@echo "  Installing $(LIB_FILE) → $(INSTALL_LIB_DIR)"
	cp $(BUILD_DIR)/$(LIB_FILE) $(INSTALL_LIB_DIR)/$(LIB_FILE)
	@echo "  Installing header → $(INSTALL_HEADER_DIR)"
	cp $(INC_DIR)/bottleneck_analyzer.h $(INSTALL_HEADER_DIR)/
	ldconfig
	@echo "  ✓  Installed. Use -lbottleneck_analyzer to link."

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_LIB_DIR)/$(LIB_FILE)
	rm -f $(INSTALL_HEADER_DIR)/bottleneck_analyzer.h
	ldconfig
	@echo "  ✓  Uninstalled."

# ════════════════════════════════════════════════════════════════════════════
# Limpieza
# ════════════════════════════════════════════════════════════════════════════
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "  ✓  Clean complete."

# ════════════════════════════════════════════════════════════════════════════
# Ayuda
# ════════════════════════════════════════════════════════════════════════════
.PHONY: help
help:
	@echo ""
	@echo "  bottleneck-analyzer-lib — Makefile targets"
	@echo ""
	@echo "  make            Build library + examples"
	@echo "  make lib        Build library only"
	@echo "  make examples   Build example programs"
	@echo "  make tests      Build and run test suite"
	@echo "  make info       Show symbol table and file info"
	@echo "  make install    Install to /usr/local (needs sudo)"
	@echo "  make uninstall  Remove from /usr/local (needs sudo)"
	@echo "  make clean      Remove build artifacts"
	@echo "  make help       Show this message"
	@echo ""
