/**
 * example_basic.c
 * Ejemplo básico de uso de libbottleneck_analyzer.so
 *
 * Demuestra analyze_bottleneck() con scores manuales y
 * analyze_bottleneck_by_model() con modelos de la tabla interna.
 *
 * Compilar (después de tener la .so):
 *   gcc -o example_basic example_basic.c \
 *       -I../include -L../build -lbottleneck_analyzer -lm \
 *       -Wl,-rpath,../build
 *
 * Ejecutar:
 *   ./example_basic
 */

#include <stdio.h>
#include <string.h>
#include "bottleneck_analyzer.h"

/* ── Helper: imprime separador ─────────────────────────────────────────────── */
static void separator(const char *title)
{
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  %s\n", title);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

int main(void)
{
    printf("\n");
    printf("  Bottleneck Analyzer Library v%s\n", get_library_version());
    printf("  Example: Basic Usage\n");

    HardwareConfig cfg;
    BottleneckResult result;
    int ret;

    /* ── CASO 1: CPU Bottleneck clásico ────────────────────────────────────── */
    separator("Case 1 — Classic CPU Bottleneck (Ryzen 3 + RTX 4070)");

    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.cpu_model, "Ryzen 3 3200G", sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "RTX 4070",      sizeof(cfg.gpu_model) - 1);
    cfg.cpu_cores    = 4;
    cfg.cpu_freq_ghz = 3.6f;
    cfg.cpu_score    = 4300;   /* Ryzen 3 3200G — score bajo */
    cfg.gpu_score    = 22000;  /* RTX 4070      — score alto */
    cfg.ram_gb       = 16;

    ret = analyze_bottleneck(&cfg, &result);
    if (ret == 0) {
        print_result(&result);
    } else {
        printf("Error analyzing bottleneck: code %d\n", ret);
    }

    /* ── CASO 2: Sistema Balanceado ────────────────────────────────────────── */
    separator("Case 2 — Balanced System (Ryzen 5 5600X + RTX 3070)");

    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.cpu_model, "Ryzen 5 5600X", sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "RTX 3070",      sizeof(cfg.gpu_model) - 1);
    cfg.cpu_cores    = 6;
    cfg.cpu_freq_ghz = 3.7f;
    cfg.cpu_score    = 15500;
    cfg.gpu_score    = 17000;
    cfg.ram_gb       = 32;

    ret = analyze_bottleneck(&cfg, &result);
    if (ret == 0) {
        print_result(&result);
    } else {
        printf("Error analyzing bottleneck: code %d\n", ret);
    }

    /* ── CASO 3: GPU Bottleneck ─────────────────────────────────────────────── */
    separator("Case 3 — GPU Bottleneck (Core i9-13900K + GTX 1060)");

    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.cpu_model, "Core i9-13900K", sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "GTX 1060",       sizeof(cfg.gpu_model) - 1);
    cfg.cpu_cores    = 24;
    cfg.cpu_freq_ghz = 3.0f;
    cfg.cpu_score    = 28000;
    cfg.gpu_score    = 6000;
    cfg.ram_gb       = 64;

    ret = analyze_bottleneck(&cfg, &result);
    if (ret == 0) {
        print_result(&result);
    } else {
        printf("Error analyzing bottleneck: code %d\n", ret);
    }

    /* ── CASO 4: Usando analyze_bottleneck_by_model() ──────────────────────── */
    separator("Case 4 — By Model Name (Ryzen 7 5800X + RTX 3080)");

    ret = analyze_bottleneck_by_model("Ryzen 7 5800X", "RTX 3080", 32, &result);
    if (ret == 0) {
        print_result(&result);
    } else if (ret == -3) {
        printf("Model not found in database. Details:\n");
        printf("%s\n", result.message);
    } else {
        printf("Error: code %d\n", ret);
    }

    /* ── CASO 5: RAM baja + CPU bottleneck ─────────────────────────────────── */
    separator("Case 5 — CPU Bottleneck + Low RAM Warning");

    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.cpu_model, "Core i3-10100",  sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "RTX 2080 Super", sizeof(cfg.gpu_model) - 1);
    cfg.cpu_cores    = 4;
    cfg.cpu_freq_ghz = 3.6f;
    cfg.cpu_score    = 6500;
    cfg.gpu_score    = 15000;
    cfg.ram_gb       = 4;   /* RAM muy baja — activa warning */

    ret = analyze_bottleneck(&cfg, &result);
    if (ret == 0) {
        print_result(&result);
    } else {
        printf("Error: code %d\n", ret);
    }

    printf("Example finished.\n\n");
    return 0;
}
