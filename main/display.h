#pragma once
#include "esp_err.h"
#include "esp_lcd_panel_rgb.h"
#include <stdint.h>

// Liefert Render-FB
uint16_t* display_get_render_fb(void);

// Präsentiert Front-FB
esp_err_t display_swap_and_present(void);

// Init
esp_err_t display_init(void);

// 🟢 Letzte gerenderte Farbe (für Logging)
extern uint16_t last_render_color;
