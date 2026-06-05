# bottleneck-analyzer-lib

> Biblioteca dinámica en C para detectar cuellos de botella (bottlenecks) entre CPU y GPU en configuraciones de hardware de PC.

---

## ¿Qué es un cuello de botella?

Un cuello de botella ocurre cuando un componente no puede procesar información lo suficientemente rápido para aprovechar la capacidad de otro. El ejemplo clásico:

```
GPU muy potente : RTX 4070  (score ~22000)
CPU básica      : Ryzen 3 3200G  (score ~4300)
```

En este caso la CPU no puede enviar datos a la GPU con suficiente velocidad, limitando el rendimiento global aunque la GPU sea excelente. La biblioteca detecta esta situación y estima el porcentaje de desaprovechamiento.

---

## Arquitectura

```
bottleneck-analyzer-lib/
│
├── include/
│   └── bottleneck_analyzer.h   ← Cabecera pública (API de la biblioteca)
│
├── src/
│   └── bottleneck_analyzer.c   ← Implementación completa
│
├── examples/
│   ├── example_basic.c         ← 5 casos de uso con scores manuales
│   └── example_interactive.c   ← Entrada de datos por consola
│
├── tests/
│   └── test_bottleneck.c       ← Suite de pruebas unitarias
│
├── build/                      ← Artefactos compilados (generado por make)
│
├── Makefile
├── Dockerfile
├── docker-compose.yml
└── README.md
```

La arquitectura sigue el patrón de la guía [Biblioteca Dinámica + JNA + Docker](https://codelabs.denkitronik.com/arquitectura-biblioteca-dinamica-jna/#0):

- Cabecera pública separada del código fuente
- Compilación con `-shared -fPIC` para posibilitar carga dinámica
- Makefile con targets diferenciados (`lib`, `examples`, `tests`, `install`)
- Dockerfile multi-stage: stage de build con GCC → stage de runtime sin compilador

---

## API pública

```c
/* Analizar con scores numéricos */
int analyze_bottleneck(const HardwareConfig *config, BottleneckResult *result);

/* Analizar por nombre de modelo (busca en tabla interna) */
int analyze_bottleneck_by_model(const char *cpu_model,
                                const char *gpu_model,
                                int         ram_gb,
                                BottleneckResult *result);

/* Imprimir resultado formateado en stdout */
void print_result(const BottleneckResult *result);

/* Etiqueta legible del estado */
const char *get_status_label(int status);

/* Versión de la biblioteca */
const char *get_library_version(void);
```

### Estructuras

```c
typedef struct {
    char  cpu_model[64];   /* Ej: "Ryzen 5 5600X"  */
    int   cpu_cores;       /* Número de núcleos     */
    float cpu_freq_ghz;    /* Frecuencia base en GHz*/
    int   cpu_score;       /* Score de rendimiento  */
    char  gpu_model[64];   /* Ej: "RTX 3070"        */
    int   gpu_score;       /* Score de rendimiento  */
    int   ram_gb;          /* RAM instalada en GB   */
} HardwareConfig;

typedef struct {
    int   status;              /* STATUS_BALANCED | STATUS_CPU_BOTTLENECK | STATUS_GPU_BOTTLENECK */
    float bottleneck_pct;      /* 0.0 .. 100.0                                                    */
    char  message[256];        /* Diagnóstico descriptivo                                         */
    char  advice[512];         /* Recomendaciones de mejora                                       */
} BottleneckResult;
```

### Constantes de estado

| Constante | Valor | Significado |
|---|---|---|
| `STATUS_BALANCED` | 0 | CPU y GPU bien pareadas |
| `STATUS_CPU_BOTTLENECK` | 1 | La CPU limita a la GPU |
| `STATUS_GPU_BOTTLENECK` | 2 | La GPU limita a la CPU |
| `STATUS_UNKNOWN` | 3 | Datos insuficientes |

---

## Cómo compilar (Linux / Ubuntu)

### Requisitos

```bash
sudo apt-get update
sudo apt-get install -y gcc make binutils
```

### Compilar todo

```bash
make
```

Esto genera:
- `build/libbottleneck_analyzer.so` — la biblioteca dinámica
- `build/example_basic` — ejemplo básico
- `build/example_interactive` — ejemplo interactivo

### Solo la biblioteca

```bash
make lib
```

### Ver símbolos exportados

```bash
make info
```

Salida esperada:
```
── Exported symbols (T = text/code) ─────────────
0000000000001234 T analyze_bottleneck
0000000000001567 T analyze_bottleneck_by_model
0000000000001890 T get_library_version
0000000000001abc T get_status_label
0000000000001def T print_result
```

---

## Cómo usar la biblioteca

### Desde un programa en C

```c
#include "bottleneck_analyzer.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    HardwareConfig cfg;
    BottleneckResult result;

    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.cpu_model, "Ryzen 3 3200G", 63);
    strncpy(cfg.gpu_model, "RTX 4070",      63);
    cfg.cpu_score = 4300;
    cfg.gpu_score = 22000;
    cfg.ram_gb    = 16;

    analyze_bottleneck(&cfg, &result);
    print_result(&result);
    return 0;
}
```

Compilar enlazando la biblioteca:
```bash
gcc -o mi_programa mi_programa.c \
    -I./include -L./build -lbottleneck_analyzer -lm \
    -Wl,-rpath,./build
```

### Usando nombres de modelos

```c
BottleneckResult result;
int ret = analyze_bottleneck_by_model("Ryzen 5 5600X", "RTX 3080", 32, &result);
if (ret == 0) {
    print_result(&result);
} else if (ret == -3) {
    printf("Modelo no encontrado. Use analyze_bottleneck() con scores manuales.\n");
}
```

Modelos soportados:
- **CPU:** Ryzen 3/5/7/9 (varios), Core i3/i5/i7/i9 (generaciones 9..14)
- **GPU:** GTX 10/16xx, RTX 20/30/40xx, RX 5/6/7xxx

---

## Ejecutar ejemplos y tests

```bash
# Ejemplo con 5 configuraciones predefinidas
./build/example_basic

# Ejemplo donde tú ingresas CPU y GPU
./build/example_interactive

# Suite de tests unitarios
make tests
```

Salida de `example_basic` (fragmento):
```
  Status          : CPU Bottleneck
  Bottleneck %    : 89.0%

  ── Diagnosis ──────────────────────────────────────
  CPU Bottleneck Detected.
  Estimated bottleneck: 89.0%
  GPU RTX 4070 (score 22000) is being limited by Ryzen 3 3200G (score 4300).
  The CPU cannot feed data to the GPU fast enough.

  ── Advice ─────────────────────────────────────────
  Recommendations to reduce CPU bottleneck (89.0%):
  1. Upgrade your CPU to one with more cores or higher frequency.
  2. Enable XMP/EXPO memory profile in BIOS for faster RAM speed.
  ...
```

---

## Cómo ejecutar con Docker

### Build de la imagen

```bash
docker build -t bottleneck-analyzer-lib:1.0 .
```

Durante el build verás:
1. **Stage builder:** GCC compila la biblioteca, ejemplos y tests
2. **Stage runtime:** imagen final sin compilador

### Ejecutar el ejemplo básico

```bash
docker run --rm bottleneck-analyzer-lib:1.0
```

### Ejecutar los tests dentro de Docker

```bash
docker run --rm --entrypoint ./test_bottleneck bottleneck-analyzer-lib:1.0
```

### Shell interactiva (explorar el contenedor)

```bash
docker run --rm -it --entrypoint /bin/bash bottleneck-analyzer-lib:1.0

# Dentro del contenedor:
./example_basic
./example_interactive
./test_bottleneck
ldconfig -p | grep bottleneck
nm -D /usr/local/lib/libbottleneck_analyzer.so
```

### Con docker compose

```bash
# Ejemplo básico
docker compose up analyzer

# Tests
docker compose up analyzer-test

# Shell interactiva
docker compose run --rm analyzer-shell
```

---

## Instalar en el sistema (opcional)

```bash
sudo make install
# Instala en /usr/local/lib/ y /usr/local/include/
# Ejecuta ldconfig automáticamente
```

Luego puedes enlazar desde cualquier directorio:
```bash
gcc -o mi_app mi_app.c -lbottleneck_analyzer -lm
```

Desinstalar:
```bash
sudo make uninstall
```

---

## Lógica de detección

El análisis se basa en el **ratio** entre los scores de CPU y GPU:

```
ratio = cpu_score / gpu_score
```

| Condición | Estado |
|---|---|
| `ratio < 0.55` | CPU Bottleneck (CPU muy débil frente a la GPU) |
| `0.55 ≤ ratio ≤ 1.80` | Balanced |
| `ratio > 1.80` | GPU Bottleneck (GPU muy débil frente a la CPU) |

El porcentaje de bottleneck se estima como:

```
CPU bottleneck: pct = (1 - ratio / 0.55) × 100
GPU bottleneck: pct = (1 - 1.80 / ratio) × 100
```

Si la RAM es menor a 8 GB, se agrega una advertencia adicional en el campo `advice`.

---

## Contribuciones

Los scores de la tabla interna son aproximaciones basadas en benchmarks públicos. Para agregar modelos o ajustar scores, edita las tablas `CPU_TABLE` y `GPU_TABLE` en `src/bottleneck_analyzer.c`.

---

## Licencia

MIT — libre de usar, modificar y distribuir.
