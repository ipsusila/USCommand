// modify buffer size to 64-bytes
// must be called before include.
// #define USC_BUFSIZE 64
#include <USCommand.h>

#define DEVICE 11
usc::Command cmd;

void printErrorThenClear(int res, char ch) {
  Serial.println();
  Serial.print("Error:");
  Serial.print(res);
  Serial.println(ch);
  cmd.clear();
}

void printCommand() {
  Serial.print(F("Broadcast     : "));
  Serial.println(cmd.isBroadcast() ? F("YES") : F("NO"));
  Serial.print(F("Device        : "));
  Serial.println(cmd.device());
  Serial.print(F("Module        : "));
  Serial.println(cmd.module());

  if (cmd.hasDesignation()) {
    Serial.print(F("Designation   : "));
    Serial.println(cmd.designation());
  }

  // Note: do not iterate param, since it's one time operation
  // it overwrite buffer content, and it's not reversible (rewindable)
}

void handleCommand(bool verbose) {
  if (verbose) {
    printCommand();
  }

  if (!cmd.isBroadcast() && cmd.device() != DEVICE) {
    Serial.print(F("Only handle broadcast command on device: "));
    Serial.println(DEVICE);
    return;
  }

  // module
  // 0: print version
  // 1: motor
  // 
  // handle:
  // !11:1/forward?s=10$
  switch(cmd.module()) {
  case 0:
    Serial.println(cmd.beginResponse());
    Serial.println(F("Action v1.0.0"));
    Serial.println(cmd.endResponse());
    break;
  case 1:
    if (!cmd.hasDesignation()) {
      Serial.println(F("Designation not defined!"));
      return;
    }
    if (strcmp_P(cmd.designation(), PSTR("forward")) == 0) {
      int step = 0;
      if (cmd.nextParam() && cmd.param().keyChar() == 's') {
        step = cmd.param().valueInt();
      }
      Serial.println(cmd.beginResponse());
      Serial.print(F("MOVE FORWARD="));
      Serial.println(step);
      Serial.println(cmd.endResponse());
    } else {
      Serial.print(F("Unknown designation:"));
      Serial.println(cmd.designation());
    }
    break;
  default:
    Serial.print(F("Unknown module:"));
    Serial.print(cmd.module());
    break;
  }

  cmd.clear();
}

void cmdLoop() {
  usc::Result res;
  int ch = Serial.read();
  while (ch != -1) {
    res = cmd.parse(ch);
    switch (res) {
      case usc::OK:
        if (cmd.isResponse()) {
          Serial.println(F("Retrive response data"));
        } else {
          handleCommand(true);
          cmd.clear();
        }
        break;
      case usc::Next:
        break;
      default:
        printErrorThenClear(res, ch);
        break;
    }
    ch = Serial.read();
  }
}

void setup() {
  Serial.begin(9600);
  cmd.clear();

  Serial.println(F("Print USCommand contents"));
}

void loop() {
  if (Serial.available()) {
    cmdLoop();
  }
}
