#pragma once
#define FW_VERSION "1.11.0"
#if defined(BOARD_175)
#define DEVICE_ID "pana-amoled175"
// redonda 466x466: una página a la vez (el avatar · consola · grafo), se cambia deslizando (ui175.cpp)
#define SCR_W 466
#define SCR_H 466
#elif defined(BOARD_P4)
#define DEVICE_ID "pana-p4-4c"
// redonda 720x720, esfera viva con páginas de homelab, agenda y redes, y grafo (ui720.cpp)
#define SCR_W 720
#define SCR_H 720
#elif defined(BOARD_S3_ILI9341)
#define DEVICE_ID "pana-s3-ili9341"
// apaisada 320x240: el avatar a la izquierda, lo que dice Hermes y los botones a la derecha (ui320.cpp)
#define SCR_W 320
#define SCR_H 240
#endif

#ifndef LCD_ROTATION
#define LCD_ROTATION 0
#endif

// Zona horaria de la placa (formato POSIX; la hora exacta llega del servidor). «COT5» = UTC-5 sin horario de verano
// (Colombia, Perú, Ecuador). República Dominicana: «AST4».
#ifndef TZ_POSIX
#if defined(BOARD_S3_ILI9341)
#define TZ_POSIX "COT5"
#else
#define TZ_POSIX "AST4"
#endif
#endif

#define PTT_MS 300                     // mantener la cara este tiempo = hablar
#define FRAME_MS 33                    // ~30 fps
