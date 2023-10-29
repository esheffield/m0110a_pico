#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "m0110a_pico.h"

const uint LED_PIN = 25;

const uint ROWS = 10;
const uint COLS = 8;
const uint MODS = 4;

const uint CLK_PIN = 26;
const uint DATA_PIN = 28;

const uint ROW_PINS[] = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
const uint COL_PINS[] = {0, 1, 2, 3, 4, 5, 6, 7};

const uint OPT_PIN = 18;
const uint OPT_PIN_MASK = 0x01 << 18;
const uint CMD_PIN = 19;
const uint CMD_PIN_MASK = 0x01 << 19;
const uint LCK_PIN = 20;
const uint LCK_PIN_MASK = 0x01 << 20;
const uint SHF_PIN = 21;
const uint SHF_PIN_MASK = 0x01 << 21;

const uint MOD_PINS[] = {OPT_PIN, CMD_PIN, LCK_PIN, SHF_PIN};

// Rows: GPI8 - GP17 -> output
const uint ROW_INIT_MASK = 0x0003FF00;
const uint ROW_DIR_MASK = 0x00000000; // 0x0003FF00;

// Cols: GP0 - GP7 -> input
const uint COL_INIT_MASK = 0x000000FF;
const uint COL_DIR_MASK = 0x00000000;

// Modifiers: GP18 - GP21 -> input
const uint MOD_INIT_MASK = OPT_PIN_MASK | CMD_PIN_MASK | LCK_PIN_MASK | SHF_PIN_MASK;
const uint MOD_DIR_MASK = 0X00000000;

const uint MATRIX_INIT_MASK = ROW_INIT_MASK | COL_INIT_MASK | MOD_INIT_MASK;
const uint MATRIX_DIR_MASK = ROW_DIR_MASK | COL_DIR_MASK | MOD_DIR_MASK;

char key_states[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char opt_state = 0;
char cmd_state = 0;
char lck_state = 0;
char shf_state = 0;

bool is_keypad(uint row, char col);
bool is_keypad_shift(uint row, char col);
void print_key_code(char key_code);
char check_modifier(uint masked_value, char mod_key_code, char *state);

int main()
{

    bi_decl(bi_program_description("M0110A controller replacement"));

    stdio_init_all();

    // comms pins are managed by pio

    gpio_init_mask(MATRIX_INIT_MASK);
    gpio_set_dir_masked(MATRIX_INIT_MASK, MATRIX_DIR_MASK);

    // Set pull-downs on the column inputs
    for (uint col = 0; col < COLS; col++)
    {
        gpio_pull_down(COL_PINS[col]);
    }

    // Set pull-ups on the modifier keys
    for (uint mod = 0; mod < MODS; mod++)
    {
        gpio_pull_up(MOD_PINS[mod]);
    }

    // Just here to give USB connection a chance to finish connecting for debugging printfs
    sleep_ms(2000);
    printf("Begin scanning...\n");

    char row = 0;
    while (1)
    {
        uint active_row = 1 << (8 + row);

        // Set only the active row to output, others input so will effectively float
        gpio_set_dir_masked(ROW_INIT_MASK, active_row);
        sleep_ms(5);
        gpio_put_masked(ROW_INIT_MASK, active_row);
        sleep_ms(5);

        volatile uint raw_cols = gpio_get_all();

        char key_code = KEY_NULL;

        uint mod_values = (~raw_cols) & MOD_INIT_MASK;

        // Code is getting printed from check_modifier for now, in future will add the return to the buffer
        key_code = check_modifier((OPT_PIN_MASK & mod_values), KEY_OPT, &opt_state);
        key_code = check_modifier((CMD_PIN_MASK & mod_values), KEY_CMD, &cmd_state);
        key_code = check_modifier((LCK_PIN_MASK & mod_values), KEY_LOCK, &lck_state);
        key_code = check_modifier((SHF_PIN_MASK & mod_values), KEY_SHIFT, &shf_state);

        key_code = KEY_NULL;
        char col_values = raw_cols & COL_INIT_MASK;

        char key_col_states = key_states[row];
        for (char col = 0; col < COLS; col++)
        {
            char mask = 1 << col;
            key_code = KEYMAP[row][col];
            char last_key_state = key_col_states & mask;
            char cur_key_state = col_values & mask;

            if (last_key_state && !cur_key_state) // key transition from pressed to released
            {
                // Set bit 7 to indicate key release
                key_code |= KEY_TXN_UP;
            }
            else if (cur_key_state == last_key_state) // key state unchanged
            {
                key_code = KEY_NULL;
            }

            if (key_code)
            {
                if (is_keypad_shift(active_row, col))
                {
                    print_key_code(KEY_SHIFT);
                    print_key_code(KP_MOD);
                }
                else if (is_keypad(active_row, col))
                {
                    print_key_code(KP_MOD);
                }
            }
            print_key_code(key_code);
        }

        key_states[row] = col_values;
        row = (row + 1) % 10;
    }
}

// Check if the key is a keypad or arrow key needing a "keypad" prefix (0x79)
bool is_keypad(uint active_row, char col)
{
    uint kp_row = KEYPAD_MAP_BY_COL[col];
    return active_row && kp_row;
}

// Check if the key is one of the keypad keys that needs a "shift" prefix (0x71)
bool is_keypad_shift(uint active_row, char col)
{
    return (col == 5 && (0x020000 & active_row))
        || (col == 6 && (0x000C00 & active_row));
}

// Debug method to print out a key code with transition flag
void print_key_code(char key_code)
{
    if (key_code)
    {
        char *txn = (key_code & KEY_TXN_UP) ? "Released" : "Pressed";
        printf("Key code: %02X %s\n", key_code, txn);
    }
}

// Checks if a given modifier key (Option, Command, Shift, Shift-Lock)
// has transitioned and returns the key_code with transition flag if so
char check_modifier(uint masked_value, char mod_key_code, char *state)
{
    char key_code = mod_key_code;
    if (!masked_value && *state)
    {
        key_code |= KEY_TXN_UP;
        *state = 0;
    }
    else if (masked_value && !(*state))
    {
        *state = 1;
    }
    else
    {
        key_code = KEY_NULL;
    }

    print_key_code(key_code);

    return key_code;
}
