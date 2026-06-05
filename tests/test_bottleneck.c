/**
 * test_bottleneck.c
 * Suite de pruebas unitarias para libbottleneck_analyzer.so
 *
 * Compilar:
 *   gcc -o test_bottleneck test_bottleneck.c \
 *       -I../include -L../build -lbottleneck_analyzer -lm \
 *       -Wl,-rpath,../build
 *
 * Ejecutar:
 *   ./test_bottleneck
 *
 * Exit code:
 *   0 si todos los tests pasan.
 *   N si N tests fallaron.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bottleneck_analyzer.h"

/* ── Mini framework de testing ─────────────────────────────────────────────── */
static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ(label, expected, actual)                          \
    do {                                                            \
        tests_run++;                                                \
        if ((expected) == (actual)) {                               \
            tests_passed++;                                         \
            printf("  [PASS] %s\n", label);                        \
        } else {                                                    \
            tests_failed++;                                         \
            printf("  [FAIL] %s — expected %d, got %d\n",          \
                   label, (int)(expected), (int)(actual));          \
        }                                                           \
    } while(0)

#define ASSERT_STR_CONTAINS(label, haystack, needle)                \
    do {                                                            \
        tests_run++;                                                \
        if (strstr((haystack), (needle)) != NULL) {                 \
            tests_passed++;                                         \
            printf("  [PASS] %s\n", label);                        \
        } else {                                                    \
            tests_failed++;                                         \
            printf("  [FAIL] %s — '%s' not in '%s'\n",             \
                   label, needle, haystack);                        \
        }                                                           \
    } while(0)

#define ASSERT_FLOAT_RANGE(label, val, lo, hi)                      \
    do {                                                            \
        tests_run++;                                                \
        if ((val) >= (lo) && (val) <= (hi)) {                       \
            tests_passed++;                                         \
            printf("  [PASS] %s (%.1f in [%.1f, %.1f])\n",         \
                   label, (float)(val), (float)(lo), (float)(hi));  \
        } else {                                                    \
            tests_failed++;                                         \
            printf("  [FAIL] %s — %.1f not in [%.1f, %.1f]\n",     \
                   label, (float)(val), (float)(lo), (float)(hi));  \
        }                                                           \
    } while(0)

static void section(const char *name)
{
    printf("\n── %s ───────────────────────────────────────────────\n", name);
}

/* ── Tests ─────────────────────────────────────────────────────────────────── */

static void test_null_parameters(void)
{
    section("Null Parameter Handling");

    BottleneckResult result;
    HardwareConfig   cfg;
    memset(&cfg, 0, sizeof(cfg));

    ASSERT_EQ("null config returns -1",  -1, analyze_bottleneck(NULL,  &result));
    ASSERT_EQ("null result returns -1",  -1, analyze_bottleneck(&cfg,  NULL));
    ASSERT_EQ("both null returns -1",    -1, analyze_bottleneck(NULL,  NULL));
    ASSERT_EQ("by_model null cpu",       -1, analyze_bottleneck_by_model(NULL, "RTX 3070", 16, &result));
    ASSERT_EQ("by_model null gpu",       -1, analyze_bottleneck_by_model("Ryzen 5 5600X", NULL, 16, &result));
    ASSERT_EQ("by_model null result",    -1, analyze_bottleneck_by_model("Ryzen 5 5600X", "RTX 3070", 16, NULL));
}

static void test_invalid_scores(void)
{
    section("Invalid Score Handling");

    HardwareConfig cfg;
    BottleneckResult result;
    memset(&cfg, 0, sizeof(cfg));

    cfg.cpu_score = -100;
    cfg.gpu_score = 10000;
    ASSERT_EQ("negative cpu_score returns -2", -2, analyze_bottleneck(&cfg, &result));

    cfg.cpu_score = 10000;
    cfg.gpu_score = -50;
    ASSERT_EQ("negative gpu_score returns -2", -2, analyze_bottleneck(&cfg, &result));
}

static void test_cpu_bottleneck(void)
{
    section("CPU Bottleneck Detection");

    HardwareConfig cfg;
    BottleneckResult result;
    memset(&cfg, 0, sizeof(cfg));

    strncpy(cfg.cpu_model, "Ryzen 3 3200G", sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "RTX 4070",      sizeof(cfg.gpu_model) - 1);
    cfg.cpu_score = 4300;
    cfg.gpu_score = 22000;
    cfg.ram_gb    = 16;

    int ret = analyze_bottleneck(&cfg, &result);
    ASSERT_EQ("returns 0",                      0,                    ret);
    ASSERT_EQ("status is CPU_BOTTLENECK",        STATUS_CPU_BOTTLENECK, result.status);
    ASSERT_FLOAT_RANGE("bottleneck > 30%",       result.bottleneck_pct, 30.0f, 99.9f);
    ASSERT_STR_CONTAINS("message has CPU",       result.message, "CPU");
    ASSERT_STR_CONTAINS("message has RTX 4070",  result.message, "RTX 4070");
    ASSERT_STR_CONTAINS("advice has Upgrade",    result.advice,  "Upgrade");
}

static void test_gpu_bottleneck(void)
{
    section("GPU Bottleneck Detection");

    HardwareConfig cfg;
    BottleneckResult result;
    memset(&cfg, 0, sizeof(cfg));

    strncpy(cfg.cpu_model, "Core i9-13900K", sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "GTX 1060",       sizeof(cfg.gpu_model) - 1);
    cfg.cpu_score = 28000;
    cfg.gpu_score = 6000;
    cfg.ram_gb    = 64;

    int ret = analyze_bottleneck(&cfg, &result);
    ASSERT_EQ("returns 0",                       0,                    ret);
    ASSERT_EQ("status is GPU_BOTTLENECK",        STATUS_GPU_BOTTLENECK, result.status);
    ASSERT_FLOAT_RANGE("bottleneck > 50%",       result.bottleneck_pct, 50.0f, 99.9f);
    ASSERT_STR_CONTAINS("message has GPU",       result.message, "GPU");
    ASSERT_STR_CONTAINS("advice has drivers",    result.advice,  "drivers");
}

static void test_balanced(void)
{
    section("Balanced System Detection");

    HardwareConfig cfg;
    BottleneckResult result;
    memset(&cfg, 0, sizeof(cfg));

    strncpy(cfg.cpu_model, "Ryzen 5 5600X", sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "RTX 3070",      sizeof(cfg.gpu_model) - 1);
    cfg.cpu_score = 15500;
    cfg.gpu_score = 17000;
    cfg.ram_gb    = 32;

    int ret = analyze_bottleneck(&cfg, &result);
    ASSERT_EQ("returns 0",               0,               ret);
    ASSERT_EQ("status is BALANCED",      STATUS_BALANCED,  result.status);
    ASSERT_FLOAT_RANGE("bottleneck < 15%", result.bottleneck_pct, 0.0f, 15.0f);
    ASSERT_STR_CONTAINS("message says balanced", result.message, "balanced");
}

static void test_by_model_found(void)
{
    section("analyze_bottleneck_by_model — Model Found");

    BottleneckResult result;

    /* Ryzen 7 5800X (score 18000) vs RTX 3080 (score 21000) → should be balanced */
    int ret = analyze_bottleneck_by_model("Ryzen 7 5800X", "RTX 3080", 32, &result);
    ASSERT_EQ("returns 0",   0, ret);
    ASSERT_EQ("not unknown", 1, result.status != STATUS_UNKNOWN);

    /* Ryzen 3 3200G vs RTX 4090 → clear CPU bottleneck */
    ret = analyze_bottleneck_by_model("Ryzen 3 3200G", "RTX 4090", 16, &result);
    ASSERT_EQ("returns 0",              0,                    ret);
    ASSERT_EQ("CPU bottleneck by name", STATUS_CPU_BOTTLENECK, result.status);
}

static void test_by_model_not_found(void)
{
    section("analyze_bottleneck_by_model — Model Not Found");

    BottleneckResult result;

    int ret = analyze_bottleneck_by_model("CPU XYZ 9999",
                                           "GPU ABC 1234",
                                           16, &result);
    ASSERT_EQ("returns -3",         -3,            ret);
    ASSERT_EQ("status is UNKNOWN",   STATUS_UNKNOWN, result.status);
    ASSERT_STR_CONTAINS("message has NOT FOUND", result.message, "NOT FOUND");
}

static void test_ram_warning(void)
{
    section("RAM Warning (< 8 GB)");

    HardwareConfig cfg;
    BottleneckResult result;
    memset(&cfg, 0, sizeof(cfg));

    strncpy(cfg.cpu_model, "Core i3-10100", sizeof(cfg.cpu_model) - 1);
    strncpy(cfg.gpu_model, "RTX 2080",      sizeof(cfg.gpu_model) - 1);
    cfg.cpu_score = 6500;
    cfg.gpu_score = 14000;
    cfg.ram_gb    = 4;

    int ret = analyze_bottleneck(&cfg, &result);
    ASSERT_EQ("returns 0", 0, ret);
    ASSERT_STR_CONTAINS("advice warns about RAM", result.advice, "WARNING");
}

static void test_zero_scores(void)
{
    section("Zero Score Edge Case");

    HardwareConfig cfg;
    BottleneckResult result;
    memset(&cfg, 0, sizeof(cfg));

    cfg.cpu_score = 0;
    cfg.gpu_score = 15000;

    int ret = analyze_bottleneck(&cfg, &result);
    ASSERT_EQ("returns 0",          0,              ret);
    ASSERT_EQ("status is UNKNOWN",  STATUS_UNKNOWN,  result.status);
}

static void test_get_status_label(void)
{
    section("get_status_label()");

    ASSERT_STR_CONTAINS("balanced label",  get_status_label(STATUS_BALANCED),       "Balanced");
    ASSERT_STR_CONTAINS("cpu bn label",    get_status_label(STATUS_CPU_BOTTLENECK), "CPU");
    ASSERT_STR_CONTAINS("gpu bn label",    get_status_label(STATUS_GPU_BOTTLENECK), "GPU");
    ASSERT_STR_CONTAINS("unknown label",   get_status_label(STATUS_UNKNOWN),        "Unknown");
    ASSERT_STR_CONTAINS("invalid label",   get_status_label(99),                    "Unknown");
}

static void test_version(void)
{
    section("Library Version");

    const char *ver = get_library_version();
    ASSERT_EQ("version is not NULL", 1, ver != NULL);
    ASSERT_STR_CONTAINS("version has dot", ver, ".");
}

/* ── Main ──────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║    Bottleneck Analyzer — Test Suite v%-6s          ║\n",
           get_library_version());
    printf("╚══════════════════════════════════════════════════════╝\n");

    test_null_parameters();
    test_invalid_scores();
    test_cpu_bottleneck();
    test_gpu_bottleneck();
    test_balanced();
    test_by_model_found();
    test_by_model_not_found();
    test_ram_warning();
    test_zero_scores();
    test_get_status_label();
    test_version();

    /* ── Resumen ──────────────────────────────────────────────────────────── */
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  Results: %d/%d tests passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf("  (%d FAILED)", tests_failed);
    }
    printf("\n");
    printf("══════════════════════════════════════════════════════\n\n");

    return tests_failed;
}
