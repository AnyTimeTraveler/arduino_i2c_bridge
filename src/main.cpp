#include <Arduino.h>
#include <Wire.h>

#define STX 0x02
#define ETX 0x03

enum State {
    EXPECT_STX,
    EXPECT_OP,
    EXPECT_FLAGS,
    EXPECT_ADDRESS,
    EXPECT_LENGTH,
    EXPECT_DATA,
    EXPECT_ETX,
};

enum Operation {
    READ,
    WRITE,
};

#define FLAG_SEND_STOP (1 << 1)

State cmd_state = EXPECT_STX;
Operation cmd_op;
uint8_t cmd_flags = 0;
uint8_t cmd_address = 0;
size_t cmd_pos = 0;
size_t cmd_len = 0;
uint8_t *cmd_buf = nullptr;
uint8_t cmd_status = 0;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
//    Serial.begin(9600);
    Serial.begin(115200);
    pinMode(LED_BUILTIN, false);
    while (!Serial) {
        // wait for serial to be ready
    }
    pinMode(LED_BUILTIN, true);
    Wire.begin();
//    Wire.setClock(100000);
}

void send_reply() {
    Serial.write(byte(STX));
    switch (cmd_op) {
        case READ: {
            Serial.write(byte('r'));
            break;
        }
        case WRITE: {
            Serial.write(byte('w'));
            break;
        }
    }
    Serial.write(cmd_flags);
    Serial.write(cmd_address);
    Serial.write(cmd_status);
    while (cmd_len > 0xFF) {
        Serial.write(0xFF);
        cmd_len -= 0xFF;
    }
    Serial.write(cmd_len);
    Serial.write(cmd_buf, cmd_len);
    Serial.write(byte(ETX));
    Serial.flush();
}

void send_message(const char *error_message, bool error) {
    Serial.write(byte(STX));
    if (error) {
        Serial.write(byte('e'));
    } else {
        Serial.write(byte('d'));
    }
    size_t len = strlen(error_message);
    Serial.write(len);
    Serial.write(error_message, len);
    Serial.write(byte(ETX));
    Serial.flush();
}


void process_cmd() {
    char msg[50];
    uint8_t send_stop = (cmd_flags & FLAG_SEND_STOP) != 0;
    switch (cmd_op) {
        case READ: {
            cmd_status = Wire.requestFrom(cmd_address, cmd_len, send_stop);
            sprintf(msg, "READ: %02X", cmd_status);
            send_message(msg, false);
            Wire.readBytes(cmd_buf, cmd_len);
            break;
        }
        case WRITE: {
            Wire.beginTransmission(cmd_address);
            Wire.write(cmd_buf, cmd_len);
            cmd_status = Wire.endTransmission(send_stop);
            sprintf(msg, "WRITE: %02X", cmd_status);
            send_message(msg, false);
            break;
        }
    }
}

// Commands:
//
// Write x bytes:
// [STX] [w] [send stop] [write address] [write length] [write data] [ETX]
// [STX] [r] [send stop] [read address]  [read length]               [ETX]
//
// Replies:
// [STX] [w] [write address]  [status]       [write length] [write data] [ETX]
// [STX] [r] [read address]   [status]       [read length]  [read data]  [ETX]
// [STX] [e] [error length]   [error text]                               [ETX]
// [STX] [d] [message length] [message text]                             [ETX]
//
void loop() {
    if (Serial.available()) {
        uint8_t byte = Serial.read();
        switch (cmd_state) {
            case EXPECT_STX: {
                if (byte == STX) {
                    cmd_state = EXPECT_OP;
                    cmd_len = 0;
                } else {
                    send_message("Invalid byte, expected STX. Resetting...", true);
                }
                break;
            }
            case EXPECT_OP: {
                if (byte == 'r') {
                    cmd_op = READ;
                    cmd_state = EXPECT_FLAGS;
                } else if (byte == 'w') {
                    cmd_op = WRITE;
                    cmd_state = EXPECT_FLAGS;
                } else {
                    send_message("Invalid byte, expected 'r' or 'w'. Resetting...", true);
                    cmd_state = EXPECT_STX;
                }
                break;
            }
            case EXPECT_FLAGS: {
                cmd_flags = byte;
                cmd_state = EXPECT_ADDRESS;
                break;
            }
            case EXPECT_ADDRESS: {
                cmd_address = byte;
                cmd_state = EXPECT_LENGTH;
                break;
            }
            case EXPECT_LENGTH: {
                cmd_len += byte;
                if (byte != 0xFF) {
                    realloc(cmd_buf, cmd_len);
                    cmd_pos = 0;

                    switch (cmd_op) {
                        case READ:
                            cmd_state = EXPECT_ETX;
                            break;
                        case WRITE:
                            cmd_state = EXPECT_DATA;
                            break;
                    }
                    if (cmd_len == 0) {
                        cmd_state = EXPECT_ETX;
                    }
                }
                break;
            }
            case EXPECT_DATA: {
                if (cmd_pos < cmd_len) {
                    cmd_buf[cmd_pos++] = byte;
                }
                if (cmd_pos >= cmd_len) {
                    cmd_state = EXPECT_ETX;
                }
                break;
            }
            case EXPECT_ETX: {
                if (byte == ETX) {
                    process_cmd();
                    send_reply();
                } else {
                    send_message("Invalid byte, expected ETX. Resetting...", true);
                }
                cmd_state = EXPECT_STX;
                break;
            }
        }
        char msg[50];
        sprintf(msg, "Got byte: %02X", byte);
        send_message(msg, false);
        switch (cmd_state) {
            case EXPECT_STX:
                send_message("EXPECT_STX", false);
                break;
            case EXPECT_OP:
                send_message("EXPECT_OP", false);
                break;
            case EXPECT_FLAGS:
                send_message("EXPECT_FLAGS", false);
                break;
            case EXPECT_ADDRESS:
                send_message("EXPECT_ADDRESS", false);
                break;
            case EXPECT_LENGTH:
                send_message("EXPECT_LENGTH", false);
                break;
            case EXPECT_DATA:
                send_message("EXPECT_DATA", false);
                break;
            case EXPECT_ETX:
                send_message("EXPECT_ETX", false);
                break;
        }
    }
}
