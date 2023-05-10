// modify buffer size to 64-bytes
// must be called before include.
#define USC_BUFSIZE 64
#include <USCommand.h>

#define DEVICE_ADDR 11
#define COMP_SYSTEM 0
#define COMP_MOTOR  1

usc::Command cmd(DEVICE_ADDR);

template <typename T>
void print(bool silent, T v) {
    if (!silent) {
        Serial.print(v);
    }
}
template <typename T>
void println(bool silent, T v) {
    if (!silent) {
        Serial.println(v);
    }
}

void onCommand(bool broadcast, uint16_t component, const char *action, usc::Params &pars) {
    switch(component) {
    case COMP_SYSTEM:
        if (*action == 0 || strcmp_P(action, PSTR("version")) == 0) {
            println(broadcast, F("Callback sample v1.0.0"));
        }
        break;
    case COMP_MOTOR:
        if (strcmp_P(action, PSTR("home")) == 0) {
            println(broadcast, F("Move to HOME"));
        } else if (strcmp_P(action, PSTR("forward")) == 0) {
            int step = 0;
            while (pars.next()) {
                if (strcmp_P(pars.kv().key(), PSTR("step")) == 0) {
                    step = pars.kv().valueInt();
                    break;
                }
            }
            print(broadcast, F("Forward movement: "));
            print(broadcast, step);
            println(broadcast, F(" step(s)"));
        } else if (strcmp_P(action, PSTR("reverse")) == 0) {
            int step = 0;
            while (pars.next()) {
                if (strcmp_P(pars.kv().key(), PSTR("step")) == 0) {
                    step = pars.kv().valueInt();
                    break;
                }
            }
            print(broadcast, F("Reverse movement: "));
            print(broadcast, step);
            println(broadcast, F(" step(s)"));
        }
    }
}

void onError(usc::Result res, usc::Command &c) {
    print(c.isBroadcast(), F("Error:"));
    println(c.isBroadcast(), (int)res);
}

void setup() {
    Serial.begin(9600);
    cmd.attachCallback(onCommand, onError);
}

void loop() {
    if (Serial.available()) {
        int ch = Serial.read();
        while (ch != -1) {
            cmd.process((char)ch);
            ch = Serial.read();
        }
    }
}