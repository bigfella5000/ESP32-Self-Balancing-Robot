#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "pin_mappings.h"
#include "lcd.h"

#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320
#define FACE_BG COLOR_BLACK
#define FACE_FG COLOR_GREEN

static spi_device_handle_t spi;

// Sends a single command byte
static void ili9341_cmd(const uint8_t cmd) {
    gpio_set_level(LCD_DC_PIN, 0); // DC Low for Command
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd
    };
    spi_device_polling_transmit(spi, &t);
}

// Sends an array of data bytes
static void ili9341_data(const uint8_t *data, int len) {
    if (len == 0) return;
    gpio_set_level(LCD_DC_PIN, 1); // DC High for Data
    spi_transaction_t t = {
        .length = len * 8, // Length is in bits
        .tx_buffer = data
    };
    spi_device_polling_transmit(spi, &t);
}

void ili9341_init(void) {
    // Initialize non-SPI GPIOs
    gpio_reset_pin(LCD_DC_PIN);
    gpio_set_direction(LCD_DC_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LCD_RST_PIN);
    gpio_set_direction(LCD_RST_PIN, GPIO_MODE_OUTPUT);

    // Hardware reset
    gpio_set_level(LCD_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(LCD_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = LCD_MISO_PIN,
        .mosi_io_num = LCD_MOSI_PIN,
        .sclk_io_num = LCD_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 26 * 1000 * 1000, 
        .mode = 0,
        .spics_io_num = LCD_CS_PIN,
        .queue_size = 7
    };

    spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI3_HOST, &devcfg, &spi);

    // Send ILI9341 initialization sequence
    ili9341_cmd(0x01); // Software Reset
    vTaskDelay(pdMS_TO_TICKS(150));

    ili9341_cmd(0x11); // Sleep Out
    vTaskDelay(pdMS_TO_TICKS(150));

    ili9341_cmd(0x36); // Memory Access Control
    uint8_t madctl = 0x48; // MX, BGR order
    ili9341_data(&madctl, 1);

    ili9341_cmd(0x3A); // Pixel Format Set
    uint8_t pixFmt = 0x55; // 16-bit / pixel
    ili9341_data(&pixFmt, 1);

    ili9341_cmd(0x29); // Display ON
    vTaskDelay(pdMS_TO_TICKS(100));
}

// Defines the drawing area
static void ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];
    
    ili9341_cmd(0x2A); // Column Address Set
    data[0] = x0 >> 8; data[1] = x0 & 0xFF;
    data[2] = x1 >> 8; data[3] = x1 & 0xFF;
    ili9341_data(data, 4);

    ili9341_cmd(0x2B); // Page Address Set
    data[0] = y0 >> 8; data[1] = y0 & 0xFF;
    data[2] = y1 >> 8; data[3] = y1 & 0xFF;
    ili9341_data(data, 4);

    ili9341_cmd(0x2C); // Memory Write
}

void ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if(x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT) return;
    if(x + w > ILI9341_WIDTH) w = ILI9341_WIDTH - x;
    if(y + h > ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    ili9341_set_window(x, y, x + w - 1, y + h - 1);
    
    // SPI hardware sends MSB first, so we swap RGB565 endianness for the IC
    uint16_t swapped_color = (color >> 8) | (color << 8);
    
    // Allocate a line buffer in DMA-capable memory to avoid fragmentation
    uint16_t *line = heap_caps_malloc(w * sizeof(uint16_t), MALLOC_CAP_DMA);
    for (int i = 0; i < w; i++) {
        line[i] = swapped_color;
    }
    
    // Send data line-by-line
    for (int i = 0; i < h; i++) {
        ili9341_data((uint8_t*)line, w * 2);
    }
    free(line);
}

void ili9341_fill_screen(uint16_t color) {
    ili9341_fill_rect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

// Helper to clear the face area without clearing the whole screen
static void clear_face() {
    // Overwrite the eye area
    ili9341_fill_rect(30, 40, 180, 100, FACE_BG);
    // Overwrite the mouth area
    ili9341_fill_rect(60, 180, 120, 80, FACE_BG);
}

void draw_face_reaction(const char* gesture) {
    clear_face();

    if (strcmp(gesture, "palm") == 0) {
        // "Stop" / Surprised Face
        // Wide open eyes
        ili9341_fill_rect(40, 40, 50, 80, FACE_FG);  // Left eye
        ili9341_fill_rect(150, 40, 50, 80, FACE_FG); // Right eye
        // "O" mouth
        ili9341_fill_rect(95, 180, 50, 60, FACE_FG);
        ili9341_fill_rect(105, 190, 30, 40, FACE_BG); // Hollow out the mouth

    } else if (strcmp(gesture, "fist") == 0) {
        // Angry / Aggressive Face
        // Angled eyes (simulated with rectangles for simplicity)
        ili9341_fill_rect(40, 60, 60, 30, FACE_FG);  // Left eye
        ili9341_fill_rect(140, 60, 60, 30, FACE_FG); // Right eye
        // Flat, wide mouth
        ili9341_fill_rect(70, 200, 100, 10, FACE_FG);

    } else if (strcmp(gesture, "point") == 0) {
        // Happy / Attentive Face
        // Standard eyes
        ili9341_fill_rect(50, 50, 40, 60, FACE_FG);
        ili9341_fill_rect(150, 50, 40, 60, FACE_FG);
        // Smile (U-shape block)
        ili9341_fill_rect(70, 180, 20, 40, FACE_FG); // Left lip
        ili9341_fill_rect(150, 180, 20, 40, FACE_FG); // Right lip
        ili9341_fill_rect(70, 200, 100, 20, FACE_FG); // Bottom lip

    } else {
        // Neutral / Waiting Face
        // Blink lines for eyes
        ili9341_fill_rect(50, 80, 40, 10, FACE_FG);
        ili9341_fill_rect(150, 80, 40, 10, FACE_FG);
        // Small neutral mouth
        ili9341_fill_rect(100, 200, 40, 10, FACE_FG);
    }
}