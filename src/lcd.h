#ifndef LCD_H
#define LCD_H

#define COLOR_BLACK 0x0000
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE  0x001F
#define COLOR_WHITE 0xFFFF

void ili9341_init(void);
void ili9341_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ili9341_fill_screen(uint16_t color);
void draw_face_reaction(const char* gesture);

#endif