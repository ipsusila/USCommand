// modify buffer size to 64-bytes
// must be called before include.
#define USC_BUFSIZE 64
#include <USCommand.h>

usc::Command cmd;

void setup() {
  Serial.begin(9600);

  Serial.print(F("Print USCommand contents:"));
  Serial.println(USC_BUFSIZE);
}

void printCommand() {
  Serial.print(F("Broadcast     : "));
  Serial.println(cmd.isBroadcast() ? F("YES") : F("NO"));
  Serial.print(F("Device        : "));
  Serial.println(cmd.device());
  Serial.print(F("Component     : "));
  Serial.println(cmd.component());
  
  if (cmd.hasAction()) {
    Serial.print(F("Action      : "));
    Serial.println(cmd.action());
  }

  usc::Params &par = cmd.params().begin();
  while(par.next()) {
    const usc::KeyVal &kv = par.kv();
    Serial.print(F("  Param> "));
    Serial.print(kv.key());
    if (kv.hasValue()) {
      Serial.print(':');
      Serial.print(kv.value());
    }
    Serial.println();
  }
  
  Serial.println("Response:");
  Serial.println(cmd.beginResponse());
  Serial.print(F("<<\nANY DATA\n"));
  Serial.print(cmd.endResponse());
}

void loop() {
  if (Serial.available()) {
    usc::Result res;
    int ch = Serial.read();
    while (ch != -1) {
      Serial.print((char)ch);
      res = cmd.process(ch);
      switch(res) {
      case usc::OK:
        if (cmd.isResponse()) {
          Serial.println(F("Retrive response data"));
        } else {
          Serial.println();
          printCommand();
        }
        break;
      case usc::Next:
        break;
      default:
        Serial.println();
        Serial.print("Error:");
        Serial.print((int)res);
        Serial.println((char)ch);
        break;
      }

      ch = Serial.read();
    }
  }
}
