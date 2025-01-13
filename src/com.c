#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <usbdrvce.h>
#include <debug.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_NETWORKS 20
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 64

#define USB_TIMEOUT 1000 // timeout after one second of no response from the USB device
#define USB_TRANSFER_TYPE_BULK 0x02 // definition of transfer type

char ssids[MAX_NETWORKS][MAX_SSID_LEN];
int network_count = 0;
usb_device_t device = NULL;
usb_endpoint_t endpoint_in = NULL, endpoint_out = NULL;

// Function declarations
void scan_wifi(void);
void draw_no_device_screen(void);
void draw_networks(void);
void draw_password_screen(const char *ssid, char *password);
void connect_wifi(const char *ssid, const char *password);
char get_ascii_from_kb_data(void);
usb_error_t usb_event_handler(usb_event_t event, void *event_data, void *callback_data);

//draw the "No USB device connected" screen
void draw_no_device_screen() {
    gfx_FillScreen(gfx_white);
    gfx_SetTextFGColor(gfx_black);
    gfx_PrintStringXY("No USB device connected.", 10, 10);
    gfx_PrintStringXY("Please connect a USB device.", 10, 30);
    gfx_SwapDraw();
    dbg_printf("Displayed 'No USB device connected' screen\n");
}

//draw the list of networks
void draw_networks() {
    gfx_FillScreen(gfx_white);
    gfx_SetTextFGColor(gfx_black);

    for (int i = 0; i < network_count; i++) {
        gfx_PrintStringXY(ssids[i], 10, 10 + i * 10);
    }

    gfx_SwapDraw();
    dbg_printf("Displayed network list\n");
}

//draw the password input screen
void draw_password_screen(const char *ssid, char *password) {
    gfx_FillScreen(gfx_white);
    gfx_SetTextFGColor(gfx_black);

    gfx_PrintStringXY("SSID:", 10, 10);
    gfx_PrintStringXY(ssid, 50, 10);

    gfx_PrintStringXY("Password:", 10, 30);
    gfx_PrintStringXY(password, 100, 30);

    gfx_SwapDraw();
    dbg_printf("Displayed password input screen for SSID: %s\n", ssid);
}

//handling USB events
usb_error_t usb_event_handler(usb_event_t event, void *event_data, void *callback_data) {
    dbg_printf("USB event: %d\n", event);
    if (event == USB_DEVICE_CONNECTED_EVENT) {
        device = (usb_device_t) event_data;
        usb_ResetDevice(device);
        dbg_printf("USB device connected\n");
    } else if (event == USB_DEVICE_ENABLED_EVENT) {
        if (device == (usb_device_t) event_data) {
            dbg_printf("USB device enabled\n");
            usb_configuration_descriptor_t config;
            size_t transferred;
            usb_GetConfigurationDescriptor(device, 0, &config, sizeof(config), &transferred);
            usb_SetConfiguration(device, &config, transferred);

            //get the interface descriptor
            usb_interface_descriptor_t *interface_desc = (usb_interface_descriptor_t *)((uint8_t *)&config + config.bLength);
            usb_endpoint_descriptor_t *endpoint_desc = (usb_endpoint_descriptor_t *)((uint8_t *)interface_desc + interface_desc->bLength);

            //find and assign bulk IN and OUT endpoints
            for (int i = 0; i < interface_desc->bNumEndpoints; i++, endpoint_desc = (usb_endpoint_descriptor_t *)((uint8_t *)endpoint_desc + endpoint_desc->bLength)) {
                if ((endpoint_desc->bmAttributes & 0x03) == USB_TRANSFER_TYPE_BULK) {
                    if (endpoint_desc->bEndpointAddress & 0x80) {  // IN endpoint
                        endpoint_in = usb_GetDeviceEndpoint(device, endpoint_desc->bEndpointAddress);
                        dbg_printf("Found IN endpoint\n");
                    } else {  // OUT endpoint
                        endpoint_out = usb_GetDeviceEndpoint(device, endpoint_desc->bEndpointAddress);
                        dbg_printf("Found OUT endpoint\n");
                    }
                }
            }
        }
    } else if (event == USB_DEVICE_DISCONNECTED_EVENT) {
        device = NULL;
        dbg_printf("USB device disconnected\n");
    }
    return USB_SUCCESS;
}

//USB Initialization
void init_usb() {
    usb_Init(usb_event_handler, NULL, NULL, USB_USE_OS_HEAP | USB_INIT_FLSZ_256 | USB_INIT_ASST_1 | USB_INIT_EOF1_3 | USB_INIT_EOF2_0);
    dbg_printf("USB initialized\n");
}

//sending commands to Python script
void send_command(const char *command) {
    if (endpoint_out) {
        usb_ScheduleBulkTransfer(endpoint_out, (void *)command, strlen(command), NULL, NULL);
        dbg_printf("Sent command: %s\n", command);
    } else {
        dbg_printf("Error: OUT endpoint not available\n");
    }
}

//receive responses from Python script
void receive_response(char *response, size_t max_len) {
    size_t len = 0;
    while (len < max_len - 1) {
        if (endpoint_in) {
            size_t received;
            usb_BulkTransfer(endpoint_in, response + len, max_len - len - 1, USB_TIMEOUT, &received);
            if (received > 0) {
                len += received;
            } else {
                break;
            }
        } else {
            dbg_printf("Error: IN endpoint not available\n");
            break;
        }
    }
    response[len] = '\0'; // Ensure null termination
    dbg_printf("Received response: %s\n", response);
}

//scan for Wi-Fi networks
void scan_wifi(void) {
    send_command("SCAN");
    char response[1024];
    receive_response(response, sizeof(response));

    network_count = 0;
    char *token = strtok(response, ",");
    while (token != NULL && network_count < MAX_NETWORKS) {
        strncpy(ssids[network_count++], token, MAX_SSID_LEN);
        token = strtok(NULL, ",");
    }
    dbg_printf("Scanned %d networks\n", network_count);
}

//connect to Wi-Fi network
void connect_wifi(const char *ssid, const char *password) {
    char command[128];
    snprintf(command, sizeof(command), "CONNECT %s %s", ssid, password);
    send_command(command);

    char response[128];
    receive_response(response, sizeof(response));

    if (strstr(response, "CONNECTED") != NULL) {
        gfx_PrintStringXY("Connected successfully!", 10, 50);
        dbg_printf("Connected successfully to %s\n", ssid);
    } else {
        gfx_PrintStringXY("Connection failed!", 10, 50);
        dbg_printf("Connection failed to %s\n", ssid);
    }

    gfx_SwapDraw();
}

//get ASCII character from keyboard(keypad) data
char get_ascii_from_kb_data(void) {
    kb_Scan();

    // Check for modifiers first
    bool shift = kb_IsDown(kb_Key2nd);
    bool alpha = kb_IsDown(kb_KeyAlpha);

    for (uint8_t i = 1; i < 7; i++) {
        uint8_t group = kb_Data[i];
        if (!group) continue;

        for (uint8_t j = 0; j < 8; j++) {
            if (group & (1 << j)) {
                // Handle group 1
                if (i == 1) {
                    if (j == 0 && shift) return '<'; // Example for shifted [2nd]
                    if (j == 0 && !shift) return '2';
                    if (j == 1 && shift) return '>';
                    if (j == 1 && !shift) return 'm';
                    if (j == 2) return '\b'; // [del] acts as backspace
                    if (j == 3 && alpha && shift) return '\"'; // Example for shifted [alpha]
                    if (j == 3 && alpha && !shift) return '?';
                    if (j == 3 && !alpha && !shift) return 'x';
                    if (j == 4 && shift) return '^';
                    if (j == 4 && !shift) return 'x';
                    if (j == 5 && shift) return 'M';
                    if (j == 5 && !shift) return 'm';
                    if (j == 6 && shift) return 'A';
                    if (j == 6 && !shift) return 'a';
                    if (j == 7 && shift) return 'P';
                    if (j == 7 && !shift) return 'p';
                }
                // Handle group 2
                else if (i == 2) {
                    if (j == 0 && alpha && shift) return ' ';
                    if (j == 0 && alpha && !shift) return ' ';
                    if (j == 0 && !alpha && !shift) return ' ';
                    if (j == 1 && shift) return 'S';
                    if (j == 1 && !shift) return 's';
                    if (j == 2 && shift) return 'I';
                    if (j == 2 && !shift) return 'i';
                    if (j == 3 && shift) return ',';
                    if (j == 3 && !shift) return ',';
                    if (j == 4 && shift) return '&';
                    if (j == 4 && !shift) return '7';
                    if (j == 5 && shift) return '$';
                    if (j == 5 && !shift) return '4';
                    if (j == 6 && shift) return '!';
                    if (j == 6 && !shift) return '1';
                    if (j == 7 && shift) return ' ';
                    if (j == 7 && !shift) return '0';
                }
                // Handle group 3
                else if (i == 3) {
                    if (j == 0 && shift) return 'G';
                    if (j == 0 && !shift) return 'g';
                    if (j == 1 && shift) return 'T';
                    if (j == 1 && !shift) return 't';
                    if (j == 2 && shift) return '=';
                    if (j == 2 && !shift) return 'y';
                    if (j == 3 && shift) return 'W';
                    if (j == 3 && !shift) return 'w';
                    if (j == 4 && shift) return 'Z';
                    if (j == 4 && !shift) return 'z';
                    if (j == 5 && shift) return 'T';
                    if (j == 5 && !shift) return 't';
                    if (j == 6) return 0; // Clear does nothing in this context
                }
                // Handle group 4
                else if (i == 4) {
                    if (j == 0 && shift) return '~';
                    if (j == 0 && !shift) return '^';
                    if (j == 1 && shift) return '|';
                    if (j == 1 && !shift) return '/';
                    if (j == 2 && shift) return 'X';
                    if (j == 2 && !shift) return 'x';
                    if (j == 3 && shift) return '_';
                    if (j == 3 && !shift) return 's';
                    if (j == 4 && shift) return '%';
                    if (j == 4 && !shift) return 'l';
                    if (j == 5 && shift) return '#';
                    if (j == 5 && !shift) return 'l';
                    if (j == 6 && shift) return '>';
                    if (j == 6 && !shift) return 's';
                    if (j == 7) return 0; // [on] does nothing in this context
                }
                // Handle group 5
                else if (i == 5) {
                    if (j == 0 && shift) return ';';
                    if (j == 0 && !shift) return '/';
                    if (j == 1 && shift) return '*';
                    if (j == 1 && !shift) return '*';
                    if (j == 2 && shift) return 'T';
                    if (j == 2 && !shift) return 't';
                    if (j == 3 && shift) return '"';
                    if (j == 3 && !shift) return ')';
                    if (j == 4 && shift) return '^';
                    if (j == 4 && !shift) return '9';
                    if (j == 5 && shift) return '@';
                    if (j == 5 && !shift) return '6';
                    if (j == 6 && shift) return '#';
                    if (j == 6 && !shift) return '3';
                    if (j == 7 && shift) return '_';
                    if (j == 7 && !shift) return '-';
                }
                // Handle group 6
                else if (i == 6) {
                    if (j == 0) return '\n'; // [enter] acts as newline
                    if (j == 1 && shift) return '-';
                    if (j == 1 && !shift) return '-';
                    if (j == 2 && shift) return '+';
                    if (j == 2 && !shift) return '+';
                    if (j == 3 && shift) return '=';
                    if (j == 3 && !shift) return '=';
                    if (j == 4 && shift) return '.';
                    if (j == 4 && !shift) return '.';
                    if (j == 5 && shift) return 'A';
                    if (j == 5 && !shift) return 'a';
                }
            }
        }
    }

    return 0; // No valid character found
}

int main(void) {
    gfx_Begin();
    kb_EnableOnLatch();
    dbg_printf("Initializing USB...\n");

    init_usb();

    // Main loop
    while (true) {
        kb_Scan();

        // del to esc
        if (kb_IsDown(kb_KeyDel)) {
            break;
        }

        // Handle USB events per loop iteration
        usb_HandleEvents();

        // Checks if a USB device is connected
        if (device == NULL) {
            draw_no_device_screen();
        } else {
            dbg_printf("Scanning for Wi-Fi networks...\n");
            scan_wifi();
            draw_networks();

            int selected_network = 0;
            char password[MAX_PASS_LEN] = {0};
            int pass_len = 0;

            while (true) {
                kb_Scan();

                // Check if the Del key is pressed to quit the app
                if (kb_IsDown(kb_KeyDel)) {
                    gfx_End();
                    usb_Cleanup();
                    return 0;
                }

                if (kb_IsDown(kb_KeyUp) && selected_network > 0) {
                    selected_network--;
                    draw_networks();
                    gfx_SetColor(gfx_black);
                    gfx_FillRectangle_NoClip(0, 10 + selected_network * 10, 320, 10);
                    gfx_SetTextFGColor(gfx_white);
                    gfx_PrintStringXY(ssids[selected_network], 10, 10 + selected_network * 10);
                    gfx_SwapDraw();
                    gfx_SetTextFGColor(gfx_black);
                } else if (kb_IsDown(kb_KeyDown) && selected_network < network_count - 1) {
                    selected_network++;
                    draw_networks();
                    gfx_SetColor(gfx_black);
                    gfx_FillRectangle_NoClip(0, 10 + selected_network * 10, 320, 10);
                    gfx_SetTextFGColor(gfx_white);
                    gfx_PrintStringXY(ssids[selected_network], 10, 10 + selected_network * 10);
                    gfx_SwapDraw();
                    gfx_SetTextFGColor(gfx_black);
                } else if (kb_IsDown(kb_KeyEnter)) {
                    unsigned int timer = 0;
                    while (timer < 5000) {
                        kb_Scan();

                        // more del to esc
                        if (kb_IsDown(kb_KeyDel)) {
                            gfx_End();
                            usb_Cleanup();
                            return 0;
                        }

                        draw_password_screen(ssids[selected_network], password);

                        if (kb_IsDown(kb_KeyClear)) {
                            pass_len = 0;
                            password[0] = '\0'; // Clear the password
                            break;
                        } else if (kb_IsDown(kb_KeyEnter)) {
                            connect_wifi(ssids[selected_network], password);
                            break;
                        } else if (kb_AnyKey()) {
                            char ascii_char = get_ascii_from_kb_data();
                            if (ascii_char && pass_len < MAX_PASS_LEN - 1) {
                                password[pass_len++] = ascii_char;
                                password[pass_len] = '\0';
                                draw_password_screen(ssids[selected_network], password); // Redraw to show new character
                            }
                        }
                        timer += 10;
                    }

                    draw_networks();
                    gfx_SetColor(gfx_black);
                    gfx_FillRectangle_NoClip(0, 10 + selected_network * 10, 320, 10);
                    gfx_SetTextFGColor(gfx_white);
                    gfx_PrintStringXY(ssids[selected_network], 10, 10 + selected_network * 10);
                    gfx_SwapDraw();
                    gfx_SetTextFGColor(gfx_black);
                }
            }
        }
    }

    gfx_End();
    usb_Cleanup();
    return 0;
}