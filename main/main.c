#include "display.h"
#include "board_8048S050C.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// kommt aus display.c
extern uint16_t last_render_color;

void app_main(void)
{
    ESP_ERROR_CHECK(display_init());

    static int phase = 0;

    while (1) {
        // 1️⃣ Render-FB holen
        uint16_t *fb = display_get_render_fb();

        // 2️⃣ Farbe + Name auswählen
        uint16_t color = 0;
        const char *name = "";

        switch (phase++ % 3) {
            case 0: color = 0xF800; name = "ROT";  break;
            case 1: color = 0x07E0; name = "GRUEN"; break;
            case 2: color = 0x001F; name = "BLAU"; break;
        }

        ESP_LOGI("MAIN", "Drawing color %04X (%s)", color, name);

        // 3️⃣ Framebuffer füllen
        for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
            fb[i] = color;
        }

        // 4️⃣ Für Display-Log merken
        last_render_color = color;

        // 5️⃣ Präsentieren
        ESP_ERROR_CHECK(display_swap_and_present());

        // 6️⃣ Pause
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
