/**
 * example_interactive.c
 * Ejemplo interactivo: el usuario ingresa CPU y GPU por consola.
 *
 * Compilar:
 *   gcc -o example_interactive example_interactive.c \
 *       -I../include -L../build -lbottleneck_analyzer -lm \
 *       -Wl,-rpath,../build
 *
 * Ejecutar:
 *   ./example_interactive
 */

#include <stdio.h>
#include <string.h>
#include "bottleneck_analyzer.h"

int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║      Bottleneck Analyzer Library v%-6s             ║\n",
           get_library_version());
    printf("║      Interactive Mode                                 ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Enter your hardware components.\n");
    printf("Press Ctrl+C to exit at any time.\n\n");

    char cpu_model[MAX_MODEL_LEN];
    char gpu_model[MAX_MODEL_LEN];
    int  ram_gb;

    /* Leer CPU */
    printf("CPU model (e.g. Ryzen 5 5600X, Core i7-12700K): ");
    if (fgets(cpu_model, sizeof(cpu_model), stdin) == NULL) {
        printf("Input error.\n");
        return 1;
    }
    /* Eliminar newline */
    cpu_model[strcspn(cpu_model, "\n")] = '\0';

    /* Leer GPU */
    printf("GPU model (e.g. RTX 3070, RX 6700 XT):         ");
    if (fgets(gpu_model, sizeof(gpu_model), stdin) == NULL) {
        printf("Input error.\n");
        return 1;
    }
    gpu_model[strcspn(gpu_model, "\n")] = '\0';

    /* Leer RAM */
    printf("RAM installed (GB, e.g. 16):                    ");
    if (scanf("%d", &ram_gb) != 1) {
        printf("Invalid RAM value.\n");
        return 1;
    }

    printf("\n  Analyzing configuration...\n");

    BottleneckResult result;
    int ret = analyze_bottleneck_by_model(cpu_model, gpu_model, ram_gb, &result);

    if (ret == -1) {
        printf("Error: NULL parameters.\n");
        return 1;
    }

    if (ret == -3) {
        printf("\nModel not found in database. Try manual scores.\n\n");
        printf("CPU score (0-30000, e.g. 15000): ");
        int cpu_score = 0, gpu_score = 0;
        if (scanf("%d", &cpu_score) != 1) cpu_score = 0;
        printf("GPU score (0-30000, e.g. 17000): ");
        if (scanf("%d", &gpu_score) != 1) gpu_score = 0;

        HardwareConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        snprintf(cfg.cpu_model, sizeof(cfg.cpu_model), "%s", cpu_model);
        snprintf(cfg.gpu_model, sizeof(cfg.gpu_model), "%s", gpu_model);
        cfg.cpu_score = cpu_score;
        cfg.gpu_score = gpu_score;
        cfg.ram_gb    = ram_gb;

        ret = analyze_bottleneck(&cfg, &result);
    }

    if (ret == 0) {
        print_result(&result);
    } else {
        printf("Analysis failed with code %d.\n", ret);
        return 1;
    }

    return 0;
}
