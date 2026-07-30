#include "display.h"
#include "app_config.h"
#include "state_machine.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "DISPLAY";

/* SSD1306 comando básico (implementación mínima sin librería externa) */
#define SSD1306_I2C_ADDR     (0x3C << 1)
#define SSD1306_LCDWIDTH     128
#define SSD1306_LCDHEIGHT    64

static uint8_t display_buf[SSD1306_LCDWIDTH * SSD1306_LCDHEIGHT / 8];

static void i2c_write_cmd(uint8_t cmd)
{
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, SSD1306_I2C_ADDR | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x00, true);
    i2c_master_write_byte(link, cmd, true);
    i2c_master_stop(link);
    i2c_master_cmd_begin(I2C_NUM_0, link, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(link);
}

static void i2c_write_data(uint8_t *data, size_t len)
{
    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, SSD1306_I2C_ADDR | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(link, 0x40, true);
    i2c_master_write(link, data, len, true);
    i2c_master_stop(link);
    i2c_master_cmd_begin(I2C_NUM_0, link, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(link);
}

static void ssd1306_init_seq(void)
{
    i2c_write_cmd(0xAE);
    i2c_write_cmd(0xD5); i2c_write_cmd(0x80);
    i2c_write_cmd(0xA8); i2c_write_cmd(0x3F);
    i2c_write_cmd(0xD3); i2c_write_cmd(0x00);
    i2c_write_cmd(0x40);
    i2c_write_cmd(0x8D); i2c_write_cmd(0x14);
    i2c_write_cmd(0x20); i2c_write_cmd(0x00);
    i2c_write_cmd(0xA1);
    i2c_write_cmd(0xC8);
    i2c_write_cmd(0xDA); i2c_write_cmd(0x12);
    i2c_write_cmd(0x81); i2c_write_cmd(0xCF);
    i2c_write_cmd(0xD9); i2c_write_cmd(0xF1);
    i2c_write_cmd(0xDB); i2c_write_cmd(0x40);
    i2c_write_cmd(0xA4);
    i2c_write_cmd(0xA6);
    i2c_write_cmd(0xAF);
}

static void ssd1306_update(void)
{
    i2c_write_cmd(0x21); i2c_write_cmd(0); i2c_write_cmd(127);
    i2c_write_cmd(0x22); i2c_write_cmd(0); i2c_write_cmd(7);
    i2c_write_data(display_buf, sizeof(display_buf));
}

static void ssd1306_clear_buf(void)
{
    memset(display_buf, 0, sizeof(display_buf));
}

static void ssd1306_draw_char(int x, int y, char c)
{
    static const uint8_t font[][5] = {
        {0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x5F,0x00,0x00},
        {0x00,0x07,0x00,0x07,0x00},
        {0x14,0x7F,0x14,0x7F,0x14},
        {0x24,0x2A,0x7F,0x2A,0x12},
        {0x23,0x13,0x08,0x64,0x62},
        {0x36,0x49,0x55,0x22,0x50},
        {0x00,0x05,0x03,0x00,0x00},
        {0x00,0x1C,0x22,0x41,0x00},
        {0x00,0x41,0x22,0x1C,0x00},
        {0x08,0x2A,0x1C,0x2A,0x08},
        {0x08,0x08,0x3E,0x08,0x08},
        {0x00,0x50,0x30,0x00,0x00},
        {0x08,0x08,0x08,0x08,0x08},
        {0x00,0x60,0x60,0x00,0x00},
        {0x20,0x10,0x08,0x04,0x02},
        {0x3E,0x51,0x49,0x45,0x3E},
        {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46},
        {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10},
        {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30},
        {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36},
        {0x06,0x49,0x49,0x29,0x1E},
        {0x00,0x36,0x36,0x00,0x00},
        {0x00,0x56,0x36,0x00,0x00},
        {0x00,0x08,0x14,0x22,0x41},
        {0x14,0x14,0x14,0x14,0x14},
        {0x41,0x22,0x14,0x08,0x00},
        {0x02,0x01,0x51,0x09,0x06},
        {0x32,0x49,0x79,0x41,0x3E},
        {0x7E,0x11,0x11,0x11,0x7E},
        {0x7F,0x49,0x49,0x49,0x36},
        {0x3E,0x41,0x41,0x41,0x22},
        {0x7F,0x41,0x41,0x22,0x1C},
        {0x7F,0x49,0x49,0x49,0x41},
        {0x7F,0x09,0x09,0x01,0x01},
        {0x3E,0x41,0x41,0x51,0x32},
        {0x7F,0x08,0x08,0x08,0x7F},
        {0x00,0x41,0x7F,0x41,0x00},
        {0x20,0x40,0x41,0x3F,0x01},
        {0x7F,0x08,0x14,0x22,0x41},
        {0x7F,0x40,0x40,0x40,0x40},
        {0x7F,0x02,0x04,0x02,0x7F},
        {0x7F,0x04,0x08,0x10,0x7F},
        {0x3E,0x41,0x41,0x41,0x3E},
        {0x7F,0x09,0x09,0x09,0x06},
        {0x3E,0x41,0x51,0x21,0x5E},
        {0x7F,0x09,0x19,0x29,0x46},
        {0x46,0x49,0x49,0x49,0x31},
        {0x01,0x01,0x7F,0x01,0x01},
        {0x3F,0x40,0x40,0x40,0x3F},
        {0x1F,0x20,0x40,0x20,0x1F},
        {0x7F,0x20,0x18,0x20,0x7F},
        {0x63,0x14,0x08,0x14,0x63},
        {0x03,0x04,0x78,0x04,0x03},
        {0x61,0x51,0x49,0x45,0x43},
        {0x00,0x00,0x7F,0x41,0x41},
        {0x02,0x04,0x08,0x10,0x20},
        {0x41,0x41,0x7F,0x00,0x00},
        {0x04,0x02,0x01,0x02,0x04},
        {0x80,0x80,0x80,0x80,0x80},
    };
    if (c < 32 || c > 126) c = ' ';
    c -= 32;
    for (int i = 0; i < 5; i++) {
        display_buf[x + i + y * SSD1306_LCDWIDTH] = font[(uint8_t)c][i];
    }
}

static void ssd1306_draw_string(int x, int y, const char *str)
{
    while (*str) {
        ssd1306_draw_char(x, y, *str);
        x += 6;
        if (x + 6 > SSD1306_LCDWIDTH) { x = 0; y += 8; }
        str++;
    }
}

void display_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    ssd1306_init_seq();
    ssd1306_clear_buf();
    ssd1306_update();
    ESP_LOGI(TAG, "Display initialized");
}

void display_clear(void)
{
    ssd1306_clear_buf();
    ssd1306_update();
}

void display_show_message(const char *line1, const char *line2)
{
    ssd1306_clear_buf();
    if (line1) ssd1306_draw_string(0, 0, line1);
    if (line2) ssd1306_draw_string(0, 16, line2);
    ssd1306_update();
}

void display_update(state_machine_t *sm)
{
    char line1[22], line2[22], line3[22];
    ssd1306_clear_buf();

    const char *state_str = sm_state_str(sm->current_state);
    snprintf(line1, sizeof(line1), "Estado: %s", state_str);
    ssd1306_draw_string(0, 0, line1);

    if (sm_is_moving(sm->current_state) || sm->current_state == STATE_STOPPED) {
        uint32_t pct = 0;
        if (sm->target_pulses > 0) {
            pct = (sm->position_pulses * 100) / sm->target_pulses;
        }
        snprintf(line2, sizeof(line2), "Posicion: %lu%%", (unsigned long)pct);
    } else if (sm->current_state == STATE_IDLE_CLOSED) {
        snprintf(line2, sizeof(line2), "Posicion: CERRADO");
    } else if (sm->current_state == STATE_IDLE_OPEN) {
        snprintf(line2, sizeof(line2), "Posicion: ABIERTO");
    } else if (sm->current_state == STATE_ERROR) {
        snprintf(line2, sizeof(line2), "*** FALLO ***");
    } else if (sm->current_state == STATE_CALIBRATION) {
        snprintf(line2, sizeof(line2), "CALIBRANDO...");
    } else {
        snprintf(line2, sizeof(line2), "Posicion: %lu", (unsigned long)sm->position_pulses);
    }
    ssd1306_draw_string(0, 16, line2);

    if (sm->current_state == STATE_INIT) {
        snprintf(line3, sizeof(line3), "Iniciando...");
    } else if (sm->current_state == STATE_ERROR) {
        snprintf(line3, sizeof(line3), "Reinicie sistema");
    } else if (sm->current_state == STATE_FTC_BLOCKED) {
        snprintf(line3, sizeof(line3), "FTC BLOQUEADO");
    }
    ssd1306_draw_string(0, 32, line3);

    ssd1306_update();
}
