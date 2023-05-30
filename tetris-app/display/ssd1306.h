#ifndef TETRIS_SSD1306_H
#define TETRIS_SSD1306_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#define FIELD_WIDTH 128
#define FIELD_HEIGHT 8

void ssd1306_command(int i2c_fd, uint8_t cmd);
void ssd1306_data(int i2c_fd, const uint8_t *data, size_t size);
void ssd1306_init(int i2c_fd);
void update_full(int i2c_fd, uint8_t *data);
void update_area(int i2c_fd, const uint8_t *data, int x, int y, int x_len, int y_len);

#endif //TETRIS_SSD1306_H
