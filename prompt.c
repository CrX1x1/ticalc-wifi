#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <string.h>

#define MAX_CODE_LEN 16
#define CORRECT_CODE "1234"

// Function to draw the code entry screen
void draw_code_screen(const char *code) {
    gfx_FillScreen(gfx_white);
    gfx_SetTextFGColor(gfx_black);
    gfx_PrintStringXY("Enter Code:", 10, 10);
    gfx_PrintStringXY(code, 10, 30);
    gfx_SwapDraw();
}

int main(void) {
    char code[MAX_CODE_LEN] = {0};
    int code_len = 0;
    bool key_released = true;

    gfx_Begin();
    kb_EnableOnLatch();

    while (1) {
        kb_Scan();

        // Check if the user pressed Enter
        if (kb_IsDown(kb_KeyEnter)) {
            if (strcmp(code, CORRECT_CODE) == 0) {
                break;  // Exit the program if the code is correct
            } else {
                // Clear the code if it's incorrect
                code_len = 0;
                code[0] = '\0';
            }
        } else if (kb_IsDown(kb_KeyClear)) {
            // Exit the program if Clear is pressed
            break;
        } else {
            // Add characters to the code
            if (key_released) {
                if (kb_IsDown(kb_Key0)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '0';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key1)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '1';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key2)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '2';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key3)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '3';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key4)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '4';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key5)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '5';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key6)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '6';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key7)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '7';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key8)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '8';
                        code[code_len] = '\0';
                    }
                } else if (kb_IsDown(kb_Key9)) {
                    if (code_len < MAX_CODE_LEN - 1) {
                        code[code_len++] = '9';
                        code[code_len] = '\0';
                    }
                }
                key_released = false;  // Set key_released to false
            }

            // Check if all keys are released
            if (!kb_AnyKey()) {
                key_released = true;
            }
        }

        draw_code_screen(code);
    }

    gfx_End();
    return 0;
}