#ifndef BOTTLENECK_ANALYZER_H
#define BOTTLENECK_ANALYZER_H

/**
 * bottleneck_analyzer.h
 * Cabecera pública de la biblioteca dinámica bottleneck-analyzer-lib.
 *
 * Analiza configuraciones de hardware y detecta cuellos de botella
 * entre CPU y GPU, retornando estado, porcentaje estimado y recomendaciones.
 *
 * Uso típico:
 *   HardwareConfig cfg;
 *   cfg.cpu_cores      = 4;
 *   cfg.cpu_freq_ghz   = 3.6f;
 *   cfg.cpu_score      = 6500;
 *   cfg.gpu_score      = 18000;
 *   cfg.ram_gb         = 16;
 *   strncpy(cfg.cpu_model, "Ryzen 3 3200G", sizeof(cfg.cpu_model)-1);
 *   strncpy(cfg.gpu_model, "RTX 4070",      sizeof(cfg.gpu_model)-1);
 *
 *   BottleneckResult result;
 *   analyze_bottleneck(&cfg, &result);
 *   print_result(&result);
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constantes de estado ──────────────────────────────────────────────────── */

/** El sistema está balanceado: ningún componente limita significativamente a otro */
#define STATUS_BALANCED       0

/** La CPU es el cuello de botella: no puede alimentar a la GPU lo suficientemente rápido */
#define STATUS_CPU_BOTTLENECK 1

/** La GPU es el cuello de botella: es demasiado lenta para la capacidad de la CPU */
#define STATUS_GPU_BOTTLENECK 2

/** Datos insuficientes o inconsistentes para determinar el estado */
#define STATUS_UNKNOWN        3

/* ── Longitudes máximas de strings ────────────────────────────────────────── */
#define MAX_MODEL_LEN    64
#define MAX_MESSAGE_LEN  512
#define MAX_ADVICE_LEN   512

/* ── Estructuras de datos ──────────────────────────────────────────────────── */

/**
 * HardwareConfig — Descripción del hardware a analizar.
 *
 * cpu_score y gpu_score son puntuaciones de rendimiento normalizadas
 * (pueden venir de benchmarks como PassMark, Cinebench, etc.).
 * La biblioteca incluye una tabla interna de modelos comunes para
 * rellenar estos valores automáticamente cuando se usa
 * analyze_bottleneck_by_model().
 */
typedef struct {
    char  cpu_model[MAX_MODEL_LEN]; /**< Nombre del modelo de CPU (ej: "Ryzen 5 5600X") */
    int   cpu_cores;                /**< Número de núcleos físicos                       */
    float cpu_freq_ghz;             /**< Frecuencia base en GHz                          */
    int   cpu_score;                /**< Puntuación de rendimiento de la CPU (0..99999)  */

    char  gpu_model[MAX_MODEL_LEN]; /**< Nombre del modelo de GPU (ej: "RTX 4070")       */
    int   gpu_score;                /**< Puntuación de rendimiento de la GPU (0..99999)  */

    int   ram_gb;                   /**< Memoria RAM instalada en GB                     */
} HardwareConfig;

/**
 * BottleneckResult — Resultado del análisis de cuello de botella.
 *
 * status          : una de las constantes STATUS_*
 * bottleneck_pct  : porcentaje estimado de cuello de botella (0.0 .. 100.0)
 *                   0.0 significa sistema perfectamente balanceado
 * message         : descripción legible del problema encontrado
 * advice          : recomendaciones concretas de mejora
 */
typedef struct {
    int   status;
    float bottleneck_pct;
    char  message[MAX_MESSAGE_LEN];
    char  advice[MAX_ADVICE_LEN];
} BottleneckResult;

/* ── API pública ───────────────────────────────────────────────────────────── */

/**
 * analyze_bottleneck
 * Analiza la configuración de hardware y rellena el resultado.
 *
 * @param config  Puntero a la configuración de hardware (entrada).
 * @param result  Puntero al resultado del análisis (salida).
 * @return  0 si el análisis se completó sin errores,
 *         -1 si config o result son NULL,
 *         -2 si los datos son inválidos (scores negativos, etc.).
 */
int analyze_bottleneck(const HardwareConfig *config, BottleneckResult *result);

/**
 * analyze_bottleneck_by_model
 * Versión simplificada: busca las puntuaciones en la tabla interna
 * a partir del nombre del modelo. Conveniente para pruebas rápidas.
 *
 * @param cpu_model  Nombre del modelo de CPU (insensible a mayúsculas).
 * @param gpu_model  Nombre del modelo de GPU (insensible a mayúsculas).
 * @param ram_gb     RAM instalada en GB.
 * @param result     Puntero al resultado del análisis (salida).
 * @return  0 éxito,
 *         -1 parámetros NULL,
 *         -3 si algún modelo no se encuentra en la tabla interna.
 */
int analyze_bottleneck_by_model(const char *cpu_model,
                                const char *gpu_model,
                                int         ram_gb,
                                BottleneckResult *result);

/**
 * get_status_label
 * Retorna un string legible para una constante STATUS_*.
 *
 * @param status  Valor STATUS_BALANCED, STATUS_CPU_BOTTLENECK, etc.
 * @return  Puntero a string estático (no liberar).
 */
const char *get_status_label(int status);

/**
 * print_result
 * Imprime el resultado formateado en stdout.
 * Útil para debugging y ejemplos de consola.
 *
 * @param result  Puntero al resultado a imprimir.
 */
void print_result(const BottleneckResult *result);

/**
 * get_library_version
 * Retorna la versión de la biblioteca como string "MAJOR.MINOR.PATCH".
 *
 * @return  Puntero a string estático (no liberar).
 */
const char *get_library_version(void);

#ifdef __cplusplus
}
#endif

#endif /* BOTTLENECK_ANALYZER_H */
