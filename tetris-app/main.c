#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <memory.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <syslog.h>
#include <errno.h>
#include "display/ssd1306.h"
#include "bitmap/bitmap.h"

/**
 * the I2C device address for the SSD1306 OLED display module
 */
#define SSD1306_I2C_DEV 0x3C
/**
 * The amount of moving unit in horizontal direction
 */
#define MOVE_Y 1
/**
 * The amount of moving unit in vertical direction
 */
#define MOVE_X 8
#define max(a, b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/**
 * 2-dimensional array to save title animation
 */
static uint8_t **title;
/**
 * File descriptor for I2C communication
 */
static int i2c_fd;
/**
 * File descriptor for rpikey character device
 */
static int rpi_fd;
/**
 * Position of the tetromino
 */
static int mino_x = 40, mino_y = 0;
/**
 * Type and angle of the tetromino
 */
static int mino_type = 0, mino_angle = 0;
/**
 * Playing field
 */
static uint8_t field[FIELD_HEIGHT][FIELD_WIDTH - 8];
/**
 * Display buffer
 */
static uint8_t display_buffer[FIELD_HEIGHT][FIELD_WIDTH - 8];
/**
 * Buffer to clear the field
 */
static uint8_t clear[FIELD_HEIGHT][FIELD_WIDTH];
/**
 * Player's score
 */
static int score;
/**
 * File pointer for score file
 */
static FILE *score_file;
/**
 * The character read from the rpikey character driver
 */
static char buf;

static void initialize_title_animation(void);
static void show_game(void);
static void show_score(void);
static void show_top_score(void);
static void clear_display(void);
static void reset_mino(void);
static int is_hit(int mino_x_next, int mino_y_next, int _mino_type, int _mino_angle);
static int get_random(int min, int max);
static void title_handler(int sig);

int main() {
    // Open I2C and rpikey devices
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    rpi_fd = open("/dev/rpikey", O_RDWR, O_NONBLOCK);

    // Check if the device files were opened successfully
    if (i2c_fd < 0) {
        syslog(LOG_ERR, "Err opening i2c device: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (rpi_fd < 0) {
        syslog(LOG_ERR, "Err opening rpikey device: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set the I2C slave address for communication with SSD1306 display
    if (ioctl(i2c_fd, I2C_SLAVE, SSD1306_I2C_DEV) < 0) {
        syslog(LOG_ERR, "Err setting i2c slave address: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Initialize ssd1306 before drawing anything
    ssd1306_init(i2c_fd);

    // Initialize 2-dimensional array to save title animation
    title = (uint8_t **) malloc(TITLE_NUM_FRAME * sizeof(uint8_t * ));
    for (int i = 0; i < TITLE_NUM_FRAME; i++) {
        title[i] = (uint8_t *) malloc(TITLE_WIDTH * TITLE_HEIGHT);
    }

    // Initialize the title animation
    initialize_title_animation();

    title_screen:
    // Read characters from the 'rpi_fd' file descriptor until there are no more characters
    while (read(rpi_fd, &buf, 1) != 0);
    // Update a specific area on a display with the press start bitmap
    update_area(i2c_fd, press_start, 64, 0, 64, 8);
    // Register the signal callback function to handle the SIGALRM signal
    signal(SIGALRM, title_handler);
    // Set an alarm to trigger the SIGALRM signal every 80000 microseconds (800 milliseconds)
    ualarm(80000, 80000);
    while (1) {
        // Read a single character from the 'rpi_fd' file descriptor
        if (read(rpi_fd, &buf, 1) == 1) {
            // Disable the alarm
            ualarm(0, 0);
            // Start the game
            if (buf == 'f')
                goto start;
            // Show the top score
            else if (buf == 'r')
                goto top_score;
            // Exit the application
            goto exit;
        }
    }

    top_score:
    // Show the top score on this device
    show_top_score();
    while (1) {
        // Read a single character from the 'rpi_fd' file descriptor
        if (read(rpi_fd, &buf, 1) == 1) {
            // Any character input will lead to the title screen
            goto title_screen;
        }
    }

    start:
    score = 0;
    // Fill the 'field' array with zeros, effectively clearing it
    memset(field, 0, sizeof(field));
    // Fill the 'clear' array with zeros, effectively clearing it
    memset(clear, 0, sizeof(clear));

    // Reset mino (its position, type, and angle)
    reset_mino();

    // Clear display
    clear_display();

    // Show main game screen
    show_game();
    // Get the current time and store it
    time_t t = time(NULL);
    // Set the 'interval' variable to 1000000 microseconds (1 second)
    time_t interval = 1000000;
    struct timespec start, end;
    // Get the current value of the system clock and store it in the 'start' variable
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    // Discard any input from the 'rpi_fd' file descriptor until it returns 0, effectively clearing the input buffer
    while (read(rpi_fd, &buf, 1) != 0);
    while (1) {
        // Read a single character from the 'rpi_fd' file descriptor
        if (read(rpi_fd, &buf, 1) == 1) {
            // Check the value of 'buf' and execute the corresponding case
            switch (buf) {
                case 'l':
                    // If 'buf' is 'l', check if moving the 'mino' object left by 1 is a valid move
                    // If it is, update the 'mino_y' coordinate accordingly
                    if (!is_hit(mino_x, mino_y + 1, mino_type, mino_angle)) {
                        mino_y += 1;
                    }
                    break;
                case 'r':
                    // If 'buf' is 'r', check if moving the 'mino' object right by 1 is a valid move
                    // If it is, update the 'mino_y' coordinate accordingly
                    if (!is_hit(mino_x, mino_y - 1, mino_type, mino_angle)) {
                        mino_y -= 1;
                    }
                    break;
                case 'd':
                    // If 'buf' is 'd', check if moving the 'mino' object down by 'MOVE_X' units is a valid move
                    // If it is, update the 'mino_x' coordinate accordingly
                    if (!is_hit(mino_x + MOVE_X, mino_y, mino_type, mino_angle)) {
                        mino_x += MOVE_X;
                    }
                    break;
                case 'u':
                    // If 'buf' is 'u', check if rotating the 'mino' object to the next angle is a valid move
                    // If it is, update the 'mino_angle' accordingly
                    if (!is_hit(mino_x, mino_y, mino_type, (mino_angle + 1) % MINO_ANGLE_MAX)) {
                        mino_angle = (mino_angle + 1) % MINO_ANGLE_MAX;
                    }
                    break;
                case 'f':
                    // If 'buf' is 'f', attempt to move the 'mino' object to the furthest down position possible
                    // within the 'FIELD_WIDTH' limit by repeatedly moving it right by 'MOVE_X' units until a collision occurs
                    for (int i = MOVE_X; i < FIELD_WIDTH; i += MOVE_X) {
                        if (!is_hit(mino_x + MOVE_X, mino_y, mino_type, mino_angle)) {
                            mino_x += MOVE_X;
                        }
                    }
                    break;
            }
            // Display the updated game state
            show_game();
        }

        // Get the current value of the system clock and store it in the 'end' variable
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        // Calculate the time difference in microseconds between 'end' and 'start'
        uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;

        // Check if the time difference exceeds or is equal to the 'interval'
        if (delta_us >= interval) {
            // Update the value of 'start' with the current value of 'end'
            start = end;
            int num_of_line_filled = 0;
            int is_game_over = 0;

            // Check if the 'mino' object has hit the bottom
            if (is_hit(mino_x, mino_y, mino_type, mino_angle)) {
                is_game_over = 1;
            }
            // Check if the 'mino' object has hit the bottom wall or another block
            if (is_hit(mino_x + MOVE_X, mino_y, mino_type, mino_angle)) {

                // Add mino to the field
                for (int i = 0; i < MINO_HEIGHT; i++) {
                    for (int j = 0; j < MINO_WIDTH; j++) {
                        field[mino_y + i][mino_x + j] |= mino_shape[mino_type][mino_angle][i][j];
                    }
                }

                // Check if any rows are filled with non-zero values in the 'field' array
                for (int i = 0; i < FIELD_WIDTH; i = i + MOVE_X) {
                    int y_line_fill = 1;
                    for (int j = 0; j < FIELD_HEIGHT; j = j + MOVE_Y) {
                        if (field[j][i + 1] != 0x7e)
                            y_line_fill = 0;
                    }
                    // If a row is filled, shift the above lines down and update the 'num_of_line_filled' variable
                    if (y_line_fill) {
                        num_of_line_filled++;
                        for (int j = i; j >= 8; j = j - MOVE_X) {
                            for (int k = 0; k < FIELD_HEIGHT; k++) {
                                field[k][j] = field[k][j - MOVE_X];
                                field[k][j + 1] = field[k][j - MOVE_X + 1];
                                field[k][j + 2] = field[k][j - MOVE_X + 2];
                                field[k][j + 3] = field[k][j - MOVE_X + 3];
                                field[k][j + 4] = field[k][j - MOVE_X + 4];
                                field[k][j + 5] = field[k][j - MOVE_X + 5];
                                field[k][j + 6] = field[k][j - MOVE_X + 6];
                                field[k][j + 7] = field[k][j - MOVE_X + 7];
                            }
                        }
                        // Adjust the 'interval' to make the game faster
                        interval = max(interval - 100000, 300000);
                    }
                }

                // Check game over condition
                for (int j = 0; j < FIELD_HEIGHT; j = j + MOVE_Y) {
                    if (field[j][1] == 0x7e)
                        is_game_over = 1;
                }

                // Perform game over action
                if (is_game_over) {
                    // Draw bottom part of the game over screen
                    for (int i = FIELD_WIDTH - MOVE_X; i >= 8; i -= MOVE_X) {
                        update_area(i2c_fd, line, i, 0, 8, 8);
                    }

                    // Save the score to the file
                    score_file = fopen("score.txt", "a");
                    fprintf(score_file, "%d\n", score);
                    fclose(score_file);

                    // Draw upper part of the game over screen
                    update_area(i2c_fd, game_over, 32, 0, GAME_OVER_WIDTH, GAME_OVER_HEIGHT);

                    // Wait for user input to take next action
                    while (1) {
                        // Read a single character from the 'rpi_fd' file descriptor
                        if (read(rpi_fd, &buf, 1) == 1) {
                            // If the input is 'f', then restart the game
                            if (buf == 'f')
                                goto start;

                            // Otherwise, clear the display and go to title screen
                            clear_display();
                            goto title_screen;
                        }
                    }
                }

                // Otherwise, the game continues
                // Reset mino (its position, type, and angle)
                reset_mino();
                // Score for alive
                score += 10;
                if (num_of_line_filled == 1)
                    // Score for one line fill
                    score += 90;
                else if (num_of_line_filled == 2)
                    // Score for two lines fill
                    score += 390;
                else if (num_of_line_filled == 3)
                    // Score for three lines fill
                    score += 690;
                else if (num_of_line_filled == 4)
                    // Score for four lines fill
                    score += 990;
            } else {
                mino_x += MOVE_X;
            }
            // Display the updated game state
            show_game();
        }
    }

    exit:
    // Clear the display
    clear_display();
    // Close the file descriptor
    close(i2c_fd);
    close(rpi_fd);
    // Disable the alarm
    ualarm(0, 0);
    return 0;
}

/**
 * Clears the display by making the screen black.
 */
void clear_display() {
    update_full(i2c_fd, (uint8_t *) clear);
}

/**
 * Displays the current score on the screen.
 * The score is divided into digits and each digit is displayed separately.
 */
void show_score() {
    int q = score / 10;
    int r = score % 10;
    int display_start_y = 2;
    // Update the display with the digit image for the current remainder (r)
    while (q != 0 || r != 0) {
        update_area(i2c_fd, number[r], 0, display_start_y, 8, 1);
        // Calculate the next digit by setting r to the remainder of q divided by 10
        // and update q by dividing it by 10
        r = q % 10;
        q = q / 10;
        // Move to the next position for displaying the digit
        display_start_y++;
    }
    // If there are leading zeros, fill the remaining positions with the digit image for zero
    if (display_start_y <= 7) {
        while (display_start_y <= 7) {
            update_area(i2c_fd, number[0], 0, display_start_y, 8, 1);
            display_start_y++;
        }
    }
}

/**
 * Updates the display buffer with the current state of the game field and the falling tetromino.
 * The display buffer is then sent to the display module for rendering.
 * Additionally, the current score is shown on the screen.
 */
void show_game() {
    // Copy the contents of the game field to the display buffer
    memcpy(display_buffer, field, sizeof(field));
    // Iterate over the cells of the falling tetromino
    for (int i = 0; i < MINO_HEIGHT; i++) {
        for (int j = 0; j < MINO_WIDTH; j++) {
            // Check if the cell is within the visible area of the display
            if (mino_y + i >= 0) {
                // Set the corresponding bit in the display buffer based on the tetromino shape
                display_buffer[mino_y + i][mino_x + j] |= mino_shape[mino_type][mino_angle][i][j];
            }
        }
    }

    // Update the display module with the contents of the display buffer
    update_area(i2c_fd, (uint8_t *) display_buffer, 8, 0, 120, 8);
    // Show the current score on the screen
    show_score();
}

/**
 * Generates a random integer between the specified minimum and maximum values.
 * Uses a static flag to initialize the random number generator if necessary.
 *
 * @param min The minimum value for the random number (inclusive).
 * @param max The maximum value for the random number (inclusive).
 * @return An integer between min and max.
 */
int get_random(int min, int max) {

    static int flag;

    // Check if the random number generator needs to be initialized
    if (flag == 0) {
        // Set the seed for the random number generator using the current time
        srand((unsigned int) time(NULL));
        flag = 1;
    }

    // Generate a random number between 0 and RAND_MAX,
    // scale it to the desired range, and add the minimum value
    return min + (int) (rand() * (max - min + 1.0) / (1.0 + RAND_MAX));
}

/**
 * Resets the current falling mino to its initial state.
 * Sets the mino's position, type, and angle to random values.
 * Uses the get_random function to generate random values.
 */
void reset_mino(void) {
    mino_x = 0; // Set the mino's x-coordinate to 0 (uppermost position)
    mino_y = 3; // Set the mino's y-coordinate to 3

    // Set the mino's type to a random value between 0 and MINO_TYPE_MAX - 1
    mino_type = get_random(0, MINO_TYPE_MAX - 1);

    // Set the mino's angle to a random value between 0 and MINO_ANGLE_MAX - 1
    mino_angle = get_random(0, MINO_ANGLE_MAX - 1);
}

/**
 * Checks if the next position of the mino will result in a collision or out-of-bounds condition.
 * Returns rue if the mino will hit an obstacle or go out of bounds, false otherwise.
 *
 * @param mino_x_next The x-coordinate of the next position to check.
 * @param mino_y_next The y-coordinate of the next position to check.
 * @param _mino_type  The type of the mino.
 * @param _mino_angle The angle of the mino.
 * @return True if the next position is a hit, false otherwise.
 */
int is_hit(int mino_x_next, int mino_y_next, int _mino_type, int _mino_angle) {
    for (int i = 0; i < MINO_HEIGHT; i++) {
        for (int j = 0; j < MINO_WIDTH; j++) {
            // Check if the mino shape at the next position is not zero and will hit an obstacle or go out of bounds
            if ((mino_shape[_mino_type][_mino_angle][i][j] != 0 &&
                 (mino_y_next + i >= FIELD_HEIGHT ||
                  mino_y_next + i < 0 ||
                  mino_x_next + j >= FIELD_WIDTH - 8 ||
                  mino_x_next + j < 0)) ||
                (mino_shape[_mino_type][_mino_angle][i][j] & field[mino_y_next + i][mino_x_next + j]) != 0) {
                // Collision or out-of-bounds condition detected
                return true;
            }
        }
    }
    // Collision or out-of-bounds condition detected
    return false;
}

/**
 * Handler function for the title animation.
 * Updates the display area with the next frame of the title animation.
 *
 * @param sig The signal received by the handler.
 */
void title_handler(int sig) {
    // Static variable to keep track of the current frame index
    static int i = 0;
    // Updates the display area with the next frame of the title animation
    update_area(i2c_fd, title[(i++) % TITLE_NUM_FRAME], 0, 0, TITLE_WIDTH, TITLE_HEIGHT);
}

/**
 * Displays the top score on the screen.
 * Reads the scores from a file and finds the maximum score.
 * Updates the display area with the best score.
 */
void show_top_score(void) {
    int value = 0;
    int max = 0;
    // Open the score file in read mode
    score_file = fopen("score.txt", "r");
    if (score_file != NULL) {
        while (fscanf(score_file, "%d", &value) != EOF) {
            // Find the maximum score
            if (value > max) {
                max = value;
            }
        }
        // Close the score file
        fclose(score_file);
    }

    // Clear the display
    clear_display();
    // Show the best score bitmap
    update_area(i2c_fd, best_score, 0, 0, 64, 8);

    int q = max / 10;
    int r = max % 10;
    int display_start_y = 1;
    // Update the display with the digit image for the current remainder (r)
    while (q != 0 || r != 0) {
        update_area(i2c_fd, number[r], 72, display_start_y, 8, 1);
        // Calculate the next digit by setting r to the remainder of q divided by 10
        // and update q by dividing it by 10
        r = q % 10;
        q = q / 10;
        // Move to the next position for displaying the digit
        display_start_y++;
    }
    // If there are leading zeros, fill the remaining positions with the digit image for zero
    if (display_start_y <= 6) {
        while (display_start_y <= 6) {
            update_area(i2c_fd, number[0], 72, display_start_y, 8, 1);
            display_start_y++;
        }
    }
}

/**
 * Initializes the title animation by assigning values to the 'title' array.
 *
 * @param title An array of pointers to uint8_t values representing the title animation frames.
 *              This parameter is passed by reference and modified within the function.
 *
 * @return void
 */
void initialize_title_animation() {
    title[0] = tetris_0;
    title[1] = tetris_1;
    title[2] = tetris_2;
    title[3] = tetris_3;
    title[4] = tetris_4;
    title[5] = tetris_5;
    title[6] = tetris_6;
    title[7] = tetris_7;
    title[8] = tetris_8;
    title[9] = tetris_9;
    title[10] = tetris_10;
    title[11] = tetris_11;
    title[12] = tetris_12;
    title[13] = tetris_13;
    title[14] = tetris_14;
    title[15] = tetris_15;
    title[16] = tetris_16;
    title[17] = tetris_17;
    title[18] = tetris_18;
    title[19] = tetris_19;
    title[20] = tetris_20;
    title[21] = tetris_21;
    title[22] = tetris_22;
    title[23] = tetris_23;
    title[24] = tetris_24;
    title[25] = tetris_25;
    title[26] = tetris_26;
    title[27] = tetris_27;
    title[28] = tetris_28;
    title[29] = tetris_29;
    title[30] = tetris_30;
    title[31] = tetris_31;
    title[32] = tetris_32;
    title[33] = tetris_33;
    title[34] = tetris_34;
    title[35] = tetris_35;
    title[36] = tetris_36;
    title[37] = tetris_37;
    title[38] = tetris_38;
    title[39] = tetris_39;
    title[40] = tetris_40;
    title[41] = tetris_41;
    title[42] = tetris_42;
    title[43] = tetris_43;
    title[44] = tetris_44;
    title[45] = tetris_45;
    title[46] = tetris_46;
    title[47] = tetris_47;
    title[48] = tetris_48;
    title[49] = tetris_49;
    title[50] = tetris_50;
    title[51] = tetris_51;
    title[52] = tetris_52;
    title[53] = tetris_53;
    title[54] = tetris_54;
    title[55] = tetris_55;
    title[56] = tetris_56;
    title[57] = tetris_57;
    title[58] = tetris_58;
    title[59] = tetris_59;
    title[60] = tetris_60;
    title[61] = tetris_61;
    title[62] = tetris_62;
    title[63] = tetris_63;
    title[64] = tetris_64;
    title[65] = tetris_65;
    title[66] = tetris_66;
    title[67] = tetris_67;
    title[68] = tetris_68;
    title[69] = tetris_69;
    title[70] = tetris_70;
    title[71] = tetris_71;
    title[72] = tetris_72;
    title[73] = tetris_73;
    title[74] = tetris_74;
    title[75] = tetris_75;
    title[76] = tetris_76;
    title[77] = tetris_77;
    title[78] = tetris_78;
    title[79] = tetris_79;
    title[80] = tetris_80;
    title[81] = tetris_81;
    title[82] = tetris_82;
    title[83] = tetris_83;
    title[84] = tetris_84;
    title[85] = tetris_85;
    title[86] = tetris_86;
    title[87] = tetris_87;
    title[88] = tetris_88;
    title[89] = tetris_89;
    title[90] = tetris_90;
    title[91] = tetris_91;
    title[92] = tetris_92;
    title[93] = tetris_93;
    title[94] = tetris_94;
    title[95] = tetris_95;
    title[96] = tetris_96;
    title[97] = tetris_97;
    title[98] = tetris_98;
    title[99] = tetris_99;
    title[100] = tetris_100;
    title[101] = tetris_101;
    title[102] = tetris_102;
    title[103] = tetris_103;
    title[104] = tetris_104;
    title[105] = tetris_105;
    title[106] = tetris_106;
    title[107] = tetris_107;
    title[108] = tetris_108;
    title[109] = tetris_109;
    title[110] = tetris_110;
    title[111] = tetris_111;
    title[112] = tetris_112;
}
