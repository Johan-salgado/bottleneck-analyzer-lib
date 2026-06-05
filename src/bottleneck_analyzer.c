/**
 * bottleneck_analyzer.c
 * Implementación de la biblioteca dinámica bottleneck-analyzer-lib.
 *
 * Lógica:
 *   1. Se define una tabla estática de modelos conocidos con sus scores.
 *   2. El análisis calcula el ratio CPU/GPU.
 *   3. Si el ratio está fuera del rango de tolerancia, se determina
 *      qué componente es el cuello de botella y se estima el porcentaje.
 *   4. Se generan mensajes y consejos descriptivos.
 *
 * Compilar como biblioteca dinámica:
 *   gcc -shared -fPIC -O2 -Wall -Wextra \
 *       -I../include -o libbottleneck_analyzer.so bottleneck_analyzer.c
 */

#include "bottleneck_analyzer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* ── Versión ───────────────────────────────────────────────────────────────── */
#define LIB_VERSION "1.0.0"

/* ── Umbrales de balanceo ──────────────────────────────────────────────────── */
/*
 * Si el ratio cpu_score/gpu_score cae fuera del rango
 * [RATIO_LOW_THRESHOLD, RATIO_HIGH_THRESHOLD], se considera desbalanceado.
 *
 * Explicación intuitiva:
 *   ratio = 1.0  → CPU y GPU tienen el mismo score (perfectamente balanceados)
 *   ratio < 0.5  → La CPU es mucho más débil que la GPU → CPU bottleneck
 *   ratio > 2.0  → La GPU es mucho más débil que la CPU → GPU bottleneck
 *
 * Estos umbrales son una aproximación práctica; en hardware real
 * otros factores (bus PCIe, latencia RAM, driver, API) también influyen.
 */
#define RATIO_LOW_THRESHOLD  0.55f   /* Por debajo: CPU bottleneck             */
#define RATIO_HIGH_THRESHOLD 1.80f   /* Por encima: GPU bottleneck             */
#define RATIO_IDEAL          1.00f   /* Ratio ideal CPU/GPU                    */

/* Penalización adicional si la RAM es menor a este umbral (GB) */
#define RAM_MIN_GB           8

/* ── Tabla de modelos conocidos ────────────────────────────────────────────── */
/*
 * Scores aproximados basados en benchmarks públicos (PassMark, UserBenchmark).
 * Se usan para analyze_bottleneck_by_model().
 * Los scores de CPU representan rendimiento multi-hilo normalizado.
 * Los scores de GPU representan rendimiento en cargas 3D típicas.
 */

typedef struct {
    const char *model;
    int         score;
} ModelScore;

/* CPU scores (multi-thread, normalizados ~1000..30000) */
static const ModelScore CPU_TABLE[] = {
    /* AMD Ryzen 3 */
    { "ryzen 3 1200",    3200 },
    { "ryzen 3 2200g",   4100 },
    { "ryzen 3 3100",    5800 },
    { "ryzen 3 3200g",   4300 },
    { "ryzen 3 3300x",   6200 },
    { "ryzen 3 4100",    6800 },
    { "ryzen 3 5300g",   8500 },
    { "ryzen 3 5300ge",  8200 },

    /* AMD Ryzen 5 */
    { "ryzen 5 1600",    8000 },
    { "ryzen 5 2600",    9200 },
    { "ryzen 5 3600",   12000 },
    { "ryzen 5 3600x",  12500 },
    { "ryzen 5 5600",   15000 },
    { "ryzen 5 5600x",  15500 },
    { "ryzen 5 7600",   18000 },
    { "ryzen 5 7600x",  19000 },

    /* AMD Ryzen 7 */
    { "ryzen 7 2700x",  11000 },
    { "ryzen 7 3700x",  14500 },
    { "ryzen 7 3800x",  15000 },
    { "ryzen 7 5700x",  17000 },
    { "ryzen 7 5800x",  18000 },
    { "ryzen 7 7700x",  22000 },
    { "ryzen 7 7800x3d",24000 },

    /* AMD Ryzen 9 */
    { "ryzen 9 5900x",  23000 },
    { "ryzen 9 5950x",  26000 },
    { "ryzen 9 7900x",  26000 },
    { "ryzen 9 7950x",  30000 },

    /* Intel Core i3 */
    { "core i3-10100",   6500 },
    { "core i3-10105",   7000 },
    { "core i3-12100",   9000 },
    { "core i3-12100f",  9000 },
    { "core i3-13100",  10000 },
    { "core i3-13100f", 10000 },

    /* Intel Core i5 */
    { "core i5-9600k",   9500 },
    { "core i5-10400",  10500 },
    { "core i5-10600k", 12000 },
    { "core i5-11400",  12500 },
    { "core i5-11600k", 13500 },
    { "core i5-12400",  15000 },
    { "core i5-12600k", 17000 },
    { "core i5-13400",  16500 },
    { "core i5-13600k", 20000 },

    /* Intel Core i7 */
    { "core i7-9700k",  11500 },
    { "core i7-10700k", 14000 },
    { "core i7-11700k", 15000 },
    { "core i7-12700k", 20000 },
    { "core i7-13700k", 24000 },
    { "core i7-14700k", 25000 },

    /* Intel Core i9 */
    { "core i9-10900k", 18000 },
    { "core i9-11900k", 17500 },
    { "core i9-12900k", 23000 },
    { "core i9-13900k", 28000 },
    { "core i9-14900k", 29000 },

    /* Sentinel */
    { NULL, 0 }
};

/* GPU scores (rendimiento 3D normalizado ~1000..30000) */
static const ModelScore GPU_TABLE[] = {
    /* NVIDIA GTX 10xx */
    { "gtx 1050",        3500 },
    { "gtx 1050 ti",     4200 },
    { "gtx 1060",        6000 },
    { "gtx 1060 6gb",    6200 },
    { "gtx 1070",        8500 },
    { "gtx 1070 ti",     9200 },
    { "gtx 1080",       10500 },
    { "gtx 1080 ti",    12500 },

    /* NVIDIA GTX 16xx */
    { "gtx 1650",        5000 },
    { "gtx 1650 super",  6500 },
    { "gtx 1660",        7500 },
    { "gtx 1660 super",  8200 },
    { "gtx 1660 ti",     8500 },

    /* NVIDIA RTX 20xx */
    { "rtx 2060",        9500 },
    { "rtx 2060 super", 11000 },
    { "rtx 2070",       11500 },
    { "rtx 2070 super", 13000 },
    { "rtx 2080",       14000 },
    { "rtx 2080 super", 15000 },
    { "rtx 2080 ti",    17000 },

    /* NVIDIA RTX 30xx */
    { "rtx 3050",        8000 },
    { "rtx 3060",       12000 },
    { "rtx 3060 ti",    15000 },
    { "rtx 3070",       17000 },
    { "rtx 3070 ti",    18500 },
    { "rtx 3080",       21000 },
    { "rtx 3080 ti",    23000 },
    { "rtx 3090",       24000 },
    { "rtx 3090 ti",    25000 },

    /* NVIDIA RTX 40xx */
    { "rtx 4060",       16000 },
    { "rtx 4060 ti",    19000 },
    { "rtx 4070",       22000 },
    { "rtx 4070 super", 25000 },
    { "rtx 4070 ti",    26000 },
    { "rtx 4070 ti super", 27500 },
    { "rtx 4080",       28000 },
    { "rtx 4080 super", 29000 },
    { "rtx 4090",       30000 },

    /* AMD RX 5xx */
    { "rx 570",          4500 },
    { "rx 580",          5500 },
    { "rx 590",          6000 },

    /* AMD RX 5xxx */
    { "rx 5500 xt",      6000 },
    { "rx 5600 xt",      8000 },
    { "rx 5700",        10000 },
    { "rx 5700 xt",     11000 },

    /* AMD RX 6xxx */
    { "rx 6600",        12000 },
    { "rx 6600 xt",     13500 },
    { "rx 6650 xt",     14000 },
    { "rx 6700",        15000 },
    { "rx 6700 xt",     16500 },
    { "rx 6750 xt",     17000 },
    { "rx 6800",        19000 },
    { "rx 6800 xt",     21000 },
    { "rx 6900 xt",     22000 },
    { "rx 6950 xt",     23000 },

    /* AMD RX 7xxx */
    { "rx 7600",        15500 },
    { "rx 7700 xt",     19000 },
    { "rx 7800 xt",     21000 },
    { "rx 7900 xt",     26000 },
    { "rx 7900 xtx",    28000 },

    /* Sentinel */
    { NULL, 0 }
};

/* ── Utilidades internas ───────────────────────────────────────────────────── */

/**
 * str_to_lower_copy — Copia src en dst convirtiendo a minúsculas.
 * dst debe tener al menos len bytes.
 */
static void str_to_lower_copy(char *dst, const char *src, size_t len)
{
    size_t i;
    for (i = 0; i < len - 1 && src[i] != '\0'; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    dst[i] = '\0';
}

/**
 * lookup_score — Busca el score de un modelo en una tabla.
 * La búsqueda es insensible a mayúsculas y usa coincidencia parcial
 * (la clave puede ser subcadena del nombre real).
 *
 * @return score si se encontró, -1 si no.
 */
static int lookup_score(const ModelScore *table, const char *model_lower)
{
    for (int i = 0; table[i].model != NULL; i++) {
        if (strstr(model_lower, table[i].model) != NULL ||
            strstr(table[i].model, model_lower) != NULL) {
            return table[i].score;
        }
    }
    return -1;
}

/**
 * clamp_float — Limita val al rango [min_val, max_val].
 */
static float clamp_float(float val, float min_val, float max_val)
{
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

/**
 * compute_bottleneck_pct — Calcula el porcentaje de cuello de botella
 * dado el ratio cpu_score/gpu_score y el umbral correspondiente.
 *
 * Fórmula:
 *   Para CPU bottleneck (ratio < RATIO_LOW):
 *     pct = (1 - ratio / RATIO_LOW) * 100
 *   Para GPU bottleneck (ratio > RATIO_HIGH):
 *     pct = (1 - RATIO_HIGH / ratio) * 100
 *
 * Resultado: 0..100 (se redondea a un decimal).
 */
static float compute_bottleneck_pct(float ratio, int status)
{
    float pct = 0.0f;

    if (status == STATUS_CPU_BOTTLENECK) {
        pct = (1.0f - ratio / RATIO_LOW_THRESHOLD) * 100.0f;
    } else if (status == STATUS_GPU_BOTTLENECK) {
        pct = (1.0f - RATIO_HIGH_THRESHOLD / ratio) * 100.0f;
    }

    pct = clamp_float(pct, 0.0f, 99.9f);
    /* Redondear a 1 decimal */
    pct = roundf(pct * 10.0f) / 10.0f;
    return pct;
}

/* ── Implementación de la API pública ─────────────────────────────────────── */

const char *get_library_version(void)
{
    return LIB_VERSION;
}

const char *get_status_label(int status)
{
    switch (status) {
        case STATUS_BALANCED:       return "Balanced";
        case STATUS_CPU_BOTTLENECK: return "CPU Bottleneck";
        case STATUS_GPU_BOTTLENECK: return "GPU Bottleneck";
        default:                    return "Unknown";
    }
}

int analyze_bottleneck(const HardwareConfig *config, BottleneckResult *result)
{
    /* ── Validación de parámetros ──────────────────────────────────────────── */
    if (config == NULL || result == NULL) {
        return -1;
    }

    if (config->cpu_score < 0 || config->gpu_score < 0 ||
        config->cpu_cores < 0 || config->ram_gb < 0) {
        return -2;
    }

    /* ── Inicializar resultado ─────────────────────────────────────────────── */
    memset(result, 0, sizeof(BottleneckResult));
    result->status = STATUS_UNKNOWN;

    /* Manejar el caso degenerado de scores cero */
    if (config->cpu_score == 0 || config->gpu_score == 0) {
        snprintf(result->message, MAX_MESSAGE_LEN,
            "Unable to determine bottleneck: CPU score=%d, GPU score=%d. "
            "Provide valid scores > 0.",
            config->cpu_score, config->gpu_score);
        snprintf(result->advice, MAX_ADVICE_LEN,
            "Use analyze_bottleneck_by_model() or provide cpu_score and "
            "gpu_score > 0.");
        return 0;
    }

    /* ── Cálculo del ratio y determinación del estado ─────────────────────── */
    float ratio = (float)config->cpu_score / (float)config->gpu_score;

    int status;
    if (ratio < RATIO_LOW_THRESHOLD) {
        status = STATUS_CPU_BOTTLENECK;
    } else if (ratio > RATIO_HIGH_THRESHOLD) {
        status = STATUS_GPU_BOTTLENECK;
    } else {
        status = STATUS_BALANCED;
    }

    float bottleneck_pct = compute_bottleneck_pct(ratio, status);

    /* Penalización por RAM insuficiente */
    int ram_warning = (config->ram_gb > 0 && config->ram_gb < RAM_MIN_GB);

    /* ── Construir mensajes según el estado ───────────────────────────────── */
    result->status         = status;
    result->bottleneck_pct = bottleneck_pct;

    const char *cpu = (config->cpu_model[0] != '\0') ? config->cpu_model : "Your CPU";
    const char *gpu = (config->gpu_model[0] != '\0') ? config->gpu_model : "Your GPU";

    switch (status) {
        case STATUS_BALANCED:
            snprintf(result->message, MAX_MESSAGE_LEN,
                "System is balanced.\n"
                "CPU (%s, score %d) and GPU (%s, score %d) are well matched.\n"
                "Estimated bottleneck: %.1f%%",
                cpu, config->cpu_score,
                gpu, config->gpu_score,
                bottleneck_pct);

            snprintf(result->advice, MAX_ADVICE_LEN,
                "Your build is well balanced. Recommendations:\n"
                "- Ensure adequate cooling for sustained performance.\n"
                "- For higher frame rates, consider upgrading both CPU and GPU together.\n"
                "%s",
                ram_warning
                    ? "- WARNING: Low RAM detected. Upgrade to at least 16 GB for best performance.\n"
                    : "- RAM looks adequate for this configuration.\n");
            break;

        case STATUS_CPU_BOTTLENECK:
            snprintf(result->message, MAX_MESSAGE_LEN,
                "CPU Bottleneck Detected.\n"
                "Estimated bottleneck: %.1f%%\n"
                "GPU %s (score %d) is being limited by %s (score %d).\n"
                "The CPU cannot feed data to the GPU fast enough.",
                bottleneck_pct,
                gpu, config->gpu_score,
                cpu, config->cpu_score);

            snprintf(result->advice, MAX_ADVICE_LEN,
                "Recommendations to reduce CPU bottleneck (%.1f%%):\n"
                "1. Upgrade your CPU to one with more cores or higher frequency.\n"
                "2. Enable XMP/EXPO memory profile in BIOS for faster RAM speed.\n"
                "3. Increase in-game resolution (shifts load toward GPU).\n"
                "4. Close background applications to free CPU resources.\n"
                "5. Check CPU temperatures — thermal throttling may reduce effective performance.\n"
                "%s",
                bottleneck_pct,
                ram_warning
                    ? "6. WARNING: Low RAM detected. Upgrade to at least 16 GB.\n"
                    : "");
            break;

        case STATUS_GPU_BOTTLENECK:
            snprintf(result->message, MAX_MESSAGE_LEN,
                "GPU Bottleneck Detected.\n"
                "Estimated bottleneck: %.1f%%\n"
                "CPU %s (score %d) is being limited by %s (score %d).\n"
                "The GPU cannot render frames fast enough to keep up with the CPU.",
                bottleneck_pct,
                cpu, config->cpu_score,
                gpu, config->gpu_score);

            snprintf(result->advice, MAX_ADVICE_LEN,
                "Recommendations to reduce GPU bottleneck (%.1f%%):\n"
                "1. Upgrade your GPU to a more powerful model.\n"
                "2. Lower in-game resolution or graphics settings to reduce GPU load.\n"
                "3. Enable DLSS/FSR/XeSS upscaling if supported by your GPU and game.\n"
                "4. Make sure GPU drivers are up to date.\n"
                "5. Check GPU temperatures — thermal throttling limits performance.\n"
                "%s",
                bottleneck_pct,
                ram_warning
                    ? "6. WARNING: Low RAM detected. Upgrade to at least 16 GB.\n"
                    : "");
            break;

        default:
            snprintf(result->message, MAX_MESSAGE_LEN,
                "Unable to determine system status.");
            snprintf(result->advice, MAX_ADVICE_LEN,
                "Provide valid cpu_score and gpu_score values.");
            break;
    }

    return 0;
}

int analyze_bottleneck_by_model(const char    *cpu_model,
                                const char    *gpu_model,
                                int            ram_gb,
                                BottleneckResult *result)
{
    if (cpu_model == NULL || gpu_model == NULL || result == NULL) {
        return -1;
    }

    /* Convertir modelos a minúsculas para búsqueda */
    char cpu_lower[MAX_MODEL_LEN];
    char gpu_lower[MAX_MODEL_LEN];
    str_to_lower_copy(cpu_lower, cpu_model, sizeof(cpu_lower));
    str_to_lower_copy(gpu_lower, gpu_model, sizeof(gpu_lower));

    /* Buscar scores en las tablas internas */
    int cpu_score = lookup_score(CPU_TABLE, cpu_lower);
    int gpu_score = lookup_score(GPU_TABLE, gpu_lower);

    if (cpu_score < 0 || gpu_score < 0) {
        memset(result, 0, sizeof(BottleneckResult));
        result->status = STATUS_UNKNOWN;
        snprintf(result->message, MAX_MESSAGE_LEN,
            "Model not found in database.\n"
            "CPU '%s': %s\n"
            "GPU '%s': %s\n"
            "Use analyze_bottleneck() with manual scores instead.",
            cpu_model, (cpu_score < 0) ? "NOT FOUND" : "found",
            gpu_model, (gpu_score < 0) ? "NOT FOUND" : "found");
        snprintf(result->advice, MAX_ADVICE_LEN,
            "Supported CPUs: Ryzen 3/5/7/9, Core i3/i5/i7/i9 (various generations).\n"
            "Supported GPUs: GTX 10/16xx, RTX 20/30/40xx, RX 5/6/7xxx.");
        return -3;
    }

    /* Construir HardwareConfig y delegar al análisis completo */
    HardwareConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.cpu_model, cpu_model, MAX_MODEL_LEN - 1);
    strncpy(cfg.gpu_model, gpu_model, MAX_MODEL_LEN - 1);
    cfg.cpu_score = cpu_score;
    cfg.gpu_score = gpu_score;
    cfg.ram_gb    = ram_gb;

    return analyze_bottleneck(&cfg, result);
}

void print_result(const BottleneckResult *result)
{
    if (result == NULL) {
        printf("[bottleneck-analyzer] Error: result is NULL\n");
        return;
    }

    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║         BOTTLENECK ANALYZER — ANALYSIS REPORT        ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("  Status          : %s\n", get_status_label(result->status));
    printf("  Bottleneck %%    : %.1f%%\n", result->bottleneck_pct);
    printf("\n");
    printf("  ── Diagnosis ──────────────────────────────────────\n");
    printf("%s\n", result->message);
    printf("\n");
    printf("  ── Advice ─────────────────────────────────────────\n");
    printf("%s\n", result->advice);
    printf("══════════════════════════════════════════════════════\n");
    printf("\n");
}
