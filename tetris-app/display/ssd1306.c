#include "ssd1306.h"

/**
 * Sends a command to the SSD1306 OLED display module via I2C.
 *
 * @param i2c_fd The file descriptor for the I2C device.
 * @param cmd    The command to be sent to the SSD1306.
 */
void ssd1306_command(int i2c_fd, uint8_t cmd) {
    uint8_t buffer[2];
    // Co=0, D/C#=0 (command mode)
    buffer[0] = (0 << 7) | (0 << 6);
    buffer[1] = cmd;

    // Write the command to the I2C device
    if (write(i2c_fd, buffer, 2) != 2) {
        syslog(LOG_ERR, "Error i2c write: %s\n", strerror(errno));
    }
}

/**
 * Sends data to the SSD1306 OLED display module via I2C.
 *
 * @param i2c_fd The file descriptor for the I2C device.
 * @param data   The data to be sent.
 * @param size   The size of the data in bytes.
 */
void ssd1306_data(int i2c_fd, const uint8_t *data, size_t size) {
    // Allocate memory for the buffer
    uint8_t *buffer = (uint8_t *) malloc(size + 1);

    // Set the control byte (Co = 0, D/C = 1)
    buffer[0] = (0 << 7) | (1 << 6);

    // Copy the data to the buffer
    memcpy(buffer + 1, data, size);

    // Write the buffer to the I2C device
    if (write(i2c_fd, buffer, size + 1) != size + 1) {
        syslog(LOG_ERR, "Error i2c write: %s\n", strerror(errno));
    }

    // Free the allocated memory
    free(buffer);
}

/**
 * Initializes the SSD1306 OLED display module by sending a series of commands via I2C.
 *
 * @param i2c_fd The file descriptor for the I2C device.
 */
void ssd1306_init(int i2c_fd) {
    // Set mux ratio
    ssd1306_command(i2c_fd, 0xA8);
    ssd1306_command(i2c_fd, 0x3f);

    // Set display offset
    ssd1306_command(i2c_fd, 0xD3);
    ssd1306_command(i2c_fd, 0x00);

    // Set display start line
    ssd1306_command(i2c_fd, 0x40);

    // Set segment re-map
    ssd1306_command(i2c_fd, 0xA0);

    // Set com output scan direction
    ssd1306_command(i2c_fd, 0xC0);

    // Set com pins hardware configuration
    ssd1306_command(i2c_fd, 0xDA);
    ssd1306_command(i2c_fd, 0x12);

    // Set contrast control
    ssd1306_command(i2c_fd, 0x81);
    ssd1306_command(i2c_fd, 0x7F);

    // Disable entire display on
    ssd1306_command(i2c_fd, 0xA4);

    // Set normal display
    ssd1306_command(i2c_fd, 0xA6);

    // Set osc frequency
    ssd1306_command(i2c_fd, 0xD5);
    ssd1306_command(i2c_fd, 0x80);

    // Enable charge pmup regulator
    ssd1306_command(i2c_fd, 0x8D);
    ssd1306_command(i2c_fd, 0x14);

    // Display on
    ssd1306_command(i2c_fd, 0xAF);
}

/**
 * Updates the entire display with the provided data.
 *
 * @param i2c_fd The file descriptor for the I2C device.
 * @param data   The data representing the display contents.
 */
void update_area(int i2c_fd, const uint8_t *data, int x, int y, int x_len, int y_len) {
    // Addressing mode
    ssd1306_command(i2c_fd, 0x20);
    // Horizontal addressing mode
    ssd1306_command(i2c_fd, 0x00);

    // Set column start/end address
    ssd1306_command(i2c_fd, 0x21);
    ssd1306_command(i2c_fd, x);
    ssd1306_command(i2c_fd, x + x_len - 1);
    // Set page start/end address
    ssd1306_command(i2c_fd, 0x22);
    ssd1306_command(i2c_fd, y);
    ssd1306_command(i2c_fd, y + y_len - 1);

    ssd1306_data(i2c_fd, data, x_len * y_len);
}

/**
 * Updates the entire display with the provided data.
 *
 * @param i2c_fd The file descriptor for the I2C device.
 * @param data   The data representing the display contents.
 */
void update_full(int i2c_fd, uint8_t *data) {
    // Addressing mode
    ssd1306_command(i2c_fd, 0x20);
    // Horizontal addressing mode
    ssd1306_command(i2c_fd, 0x00);

    // Set column start/end address
    ssd1306_command(i2c_fd, 0x21);
    ssd1306_command(i2c_fd, 0);
    ssd1306_command(i2c_fd, FIELD_WIDTH - 1);

    // Set page start_end address
    ssd1306_command(i2c_fd, 0x22);
    ssd1306_command(i2c_fd, 0);
    ssd1306_command(i2c_fd, FIELD_HEIGHT - 1);

    ssd1306_data(i2c_fd, data, FIELD_WIDTH * FIELD_HEIGHT);
}