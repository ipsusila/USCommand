#include <USCommand.h>

USCommand cmd;

void setup() {
  Serial.begin(9600);
  cmd.reset();

  Serial.println(F("Print USCommand contents"));
}

void printCommand() {
  Serial.print(F("Device        : "));
  Serial.println(cmd.device());
  Serial.print(F("Module        : "));
  Serial.println(cmd.module());
  
  if (cmd.hasDesignation()) {
    Serial.print(F("Designation   : "));
    Serial.println(cmd.designation());
  }
  if (cmd.hasParam()) {
    USParam *p;
    while(cmd.nextParam()) {
      p = cmd.param();
      Serial.print(F("  Param> "));
      Serial.print(p->key());
      if (p->hasValue()) {
        Serial.print(':');
        Serial.print(p->value());
      }
      Serial.println();
    }
  }

  // Example send response
  char *d = cmd.data();
  *d = '@';
  Serial.println("Response:");
  Serial.println(d);
  Serial.print(F("<<\nANY DATA\n"));
  Serial.print('$');
}

void loop() {
  if (Serial.available()) {
    USC_Result res;
    int ch = Serial.read();
    while (ch != -1) {
      Serial.print((char)ch);
      res = cmd.parse(ch);
      switch(res) {
      case USC_OK:
        Serial.println();
        printCommand();
        cmd.reset();
        break;
      case USC_Next:
        break;
      default:
        Serial.println();
        Serial.print("Error:");
        Serial.print((int)res);
        Serial.println((char)ch);
        cmd.reset();
        break;
      }

      ch = Serial.read();
    }
  }
}
