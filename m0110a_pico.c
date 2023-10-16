#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "m0110a_pico.h"

const uint LED_PIN = 25;

const uint ROWS = 10;
const uint COLS = 8;

const uint CLK_PIN = 26;
const uint DATA_PIN = 28;

const uint ROW_PINS[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const uint COL_PINS[] = {10, 11, 12, 13, 14, 15, 16, 17};

const uint OPT_PIN = 18;
const uint CMD_PIN = 19;
const uint LCK_PIN = 20;
const uint SHF_PIN = 21;

// Rows: GPI0 - GP9 -> output
const uint ROW_INIT_MASK = 0x000003FF;
const uint ROW_DIR_MASK = 0x000003FF;

// Cols: GP10 - GP17 -> input
const uint COL_INIT_MASK = 0x0003FC00;
const uint COL_DIR_MASK = 0x00000000;

// Modifiers: GP18 - GP21 -> input
const uint MOD_INIT_MASK = 0x03C0000;
const uint MOD_DIR_MASK = 0X00000000;

const uint MATRIX_INIT_MASK = ROW_INIT_MASK | COL_INIT_MASK | MOD_INIT_MASK;
const uint MATRIX_DIR_MASK = ROW_DIR_MASK | COL_DIR_MASK | MOD_DIR_MASK;

int main()
{

    bi_decl(bi_program_description("M0110A controller replacement"));
    // bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));

    stdio_init_all();

    // gpio_init(LED_PIN);
    // gpio_set_dir(LED_PIN, GPIO_OUT);

    // comms pins are managed by pio

    gpio_init_mask(MATRIX_INIT_MASK);
    gpio_set_dir_masked(MATRIX_INIT_MASK, MATRIX_DIR_MASK);

    for (uint col = 0; col < COLS; col++)
    {
        gpio_pull_up(COL_PINS[col]);
    }

    while (1)
    {
        for(uint r = 0; r < ROWS; r++)
        {
            uint row = 1 << r;
            uint active_row = ~row;
            gpio_put_masked(ROW_INIT_MASK, active_row);

            // Read all GPIOs, mask out cols, and shift to LSB
            uint raw_cols = gpio_get_all();
            uint masked_cols = raw_cols & COL_INIT_MASK;
            char col_values = ~(masked_cols >> 10);
            // printf("RowIdx: %d, Row:  0x%04X, Active row:  0x%04X\n", r, row, active_row);

            char key_code = 0xFF;
            for(char c = 0; c < COLS; c++) {
                if(col_values & (1 << c)) {
                    key_code = KEYMAP[r][c];
                    printf("Row: 0x%04X - [%d, %d] = Key code: 0x%02X\n", row, r, c, key_code);
                }
            }
        }
    }
}
