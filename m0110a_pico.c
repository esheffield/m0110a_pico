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

const uint ROW_PINS[] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
const uint COL_PINS[] = {0, 1, 2, 3, 4, 5, 6, 7};

const uint OPT_PIN = 18;
const uint CMD_PIN = 19;
const uint LCK_PIN = 20;
const uint SHF_PIN = 21;

// Rows: GPI8 - GP17 -> output
const uint ROW_INIT_MASK = 0x0003FF00;
const uint ROW_DIR_MASK = 0x0003FF00;

// Cols: GP0 - GP7 -> input
const uint COL_INIT_MASK = 0x000000FF;
const uint COL_DIR_MASK = 0x00000000;

// Modifiers: GP18 - GP21 -> input
const uint MOD_INIT_MASK = 0x03C0000;
const uint MOD_DIR_MASK = 0X00000000;

const uint MATRIX_INIT_MASK = ROW_INIT_MASK | COL_INIT_MASK | MOD_INIT_MASK;
const uint MATRIX_DIR_MASK = ROW_DIR_MASK | COL_DIR_MASK | MOD_DIR_MASK;

char key_states[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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
        gpio_pull_down(COL_PINS[col]);
    }

    char row = 0;
    while (1)
    {
        char *key_row = (char *)(KEYMAP + (COLS * row));
        gpio_put_masked(ROW_INIT_MASK, 1 << (8 + row));
        // slight delay for lines to settle
        sleep_ms(5);

        volatile uint raw_cols = gpio_get_all();
        char col_values = raw_cols & COL_INIT_MASK;

        char key_col_states = key_states[row];
        for (char col = 0; col < COLS; col++)
        {
            char mask = 1 << col;
            char key_code = key_row[col];
            char last_key_state = key_col_states & mask;
            char cur_key_state = col_values & mask;

            if(last_key_state && !cur_key_state) // key transition from pressed to released
            {
                // Set bit 7 to indicate key release
                key_code |= 0x80;
            }
            else if(cur_key_state == last_key_state) // key state unchanged
            {
                key_code = KEY_NULL;
            }

            // No special handling of released to pressed transition necessary

            if(key_code != KEY_NULL)
            {
                char* txn = (key_code & 0x80)? "Released" : "Pressed";
                printf("Key code: %02X %s\n", key_code, txn);
            }
        }

        key_states[row] = col_values;
        // printf("<Post> RowIdx: %d, Row: 0x%04X, Col_Values: 0x%02X\n", row, row, col_values);

        row = (row + 1) % 10;
    }
}
