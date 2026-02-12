#include "display.h"
#include "board_8048S050C.h"

#include <string.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"

// 🟢 Letzte gerenderte Farbe (für Logging)
uint16_t last_render_color = 0;


static const char *TAG = "DISPLAY";

#define FB_PIXELS (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define FB_BYTES  (FB_PIXELS * sizeof(uint16_t))

static uint16_t *fb_front  = NULL;
static uint16_t *fb_back   = NULL;
static uint16_t *fb_render = NULL;

static esp_lcd_panel_handle_t lcd_panel = NULL;

// ---------------------------------------------------------
// Framebuffer
// ---------------------------------------------------------
static esp_err_t fb_init(void)
{
    fb_front  = heap_caps_malloc(FB_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    fb_back   = heap_caps_malloc(FB_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    fb_render = heap_caps_malloc(FB_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!fb_front || !fb_back || !fb_render) {
        ESP_LOGE(TAG, "FB alloc failed");
        return ESP_ERR_NO_MEM;
    }

    memset(fb_front,  0, FB_BYTES);
    memset(fb_back,   0, FB_BYTES);
    memset(fb_render, 0, FB_BYTES);

    ESP_LOGI(TAG, "Triple FB initialized: front=%p back=%p render=%p",
             fb_front, fb_back, fb_render);
    return ESP_OK;
}

uint16_t *display_get_render_fb(void)
{
    return fb_render;
}

esp_err_t display_swap_and_present(void)
{
    uint16_t *tmp = fb_front;
    fb_front  = fb_render;
    fb_render = fb_back;
    fb_back   = tmp;

    ESP_LOGI(TAG, "Presenting FB %p", fb_front);

    return esp_lcd_panel_draw_bitmap(
        lcd_panel,
        0, 0,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT,
        fb_front
    );
}

// ---------------------------------------------------------
// Display Init
// ---------------------------------------------------------
esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Init RGB panel (%s)", BOARD_NAME);

    ESP_ERROR_CHECK(fb_init());

    esp_lcd_rgb_panel_config_t cfg = {
        .clk_src = ST7262_PANEL_CONFIG_CLK_SRC,

        .timings = {
            .pclk_hz = 14 * 1000000,   // Demo: 14 MHz PCLK
            .h_res   = DISPLAY_WIDTH,
            .v_res   = DISPLAY_HEIGHT,

            .hsync_pulse_width = 4,
            .hsync_back_porch  = 8,
            .hsync_front_porch = 8,

            .vsync_pulse_width = 4,
            .vsync_back_porch  = 8,
            .vsync_front_porch = 8,

            .flags = {
                .hsync_idle_low  = true,
                .vsync_idle_low  = true,
                .de_idle_high    = true,
                .pclk_active_neg = true,   // falling edge
                .pclk_idle_high  = false,
            },
        },

        .data_width        = 16,
        .bits_per_pixel    = 16,
        .num_fbs           = 1,          // Wir nutzen unsere eigenen 3 FB
        .bounce_buffer_size_px = 0,
        .sram_trans_align  = ST7262_PANEL_CONFIG_SRAM_TRANS_ALIGN,
        .psram_trans_align = ST7262_PANEL_CONFIG_PSRAM_TRANS_ALIGN,

        .hsync_gpio_num = ST7262_PANEL_CONFIG_HSYNC,
        .vsync_gpio_num = ST7262_PANEL_CONFIG_VSYNC,
        .de_gpio_num    = ST7262_PANEL_CONFIG_DE,
        .pclk_gpio_num  = ST7262_PANEL_CONFIG_PCLK,
        .disp_gpio_num  = ST7262_PANEL_CONFIG_DISP,

        .data_gpio_nums = {
            ST7262_PANEL_CONFIG_DATA_R0, ST7262_PANEL_CONFIG_DATA_R1,
            ST7262_PANEL_CONFIG_DATA_R2, ST7262_PANEL_CONFIG_DATA_R3,
            ST7262_PANEL_CONFIG_DATA_R4,
            ST7262_PANEL_CONFIG_DATA_G0, ST7262_PANEL_CONFIG_DATA_G1,
            ST7262_PANEL_CONFIG_DATA_G2, ST7262_PANEL_CONFIG_DATA_G3,
            ST7262_PANEL_CONFIG_DATA_G4, ST7262_PANEL_CONFIG_DATA_G5,
            ST7262_PANEL_CONFIG_DATA_B0, ST7262_PANEL_CONFIG_DATA_B1,
            ST7262_PANEL_CONFIG_DATA_B2, ST7262_PANEL_CONFIG_DATA_B3,
            ST7262_PANEL_CONFIG_DATA_B4
        },

        .flags = {
            .fb_in_psram = ST7262_PANEL_CONFIG_FLAGS_FB_IN_PSRAM,
            .double_fb   = false,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&cfg, &lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel));

    // RGB-Panels unterstützen disp_on_off NICHT
    // ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel, true));

    gpio_set_direction(DISPLAY_BCKL, GPIO_MODE_OUTPUT);
    gpio_set_level(DISPLAY_BCKL, 1);

    ESP_LOGI(TAG, "RGB Panel ready with demo timings (14MHz, DE/HSYNC/VSYNC correctly set)");
    return ESP_OK;
}
