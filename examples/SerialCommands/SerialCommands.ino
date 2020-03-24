#include <CommandParser.h>

typedef CommandParser<> MyCommandParser;

MyCommandParser parser;

void cmd_test(MyCommandParser::Argument *args, char *response) {
  Serial.print("string: "); Serial.println(args[0].asString);
  Serial.print("double: "); Serial.println(args[1].asDouble);
  Serial.print("int64: "); Serial.println(args[2].asInt64);
  Serial.print("uint64: "); Serial.println(args[3].asUInt64);
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  parser.registerCommand("TEST", "sdiu", &cmd_test);
  Serial.println("registered command: TEST <string> <double> <int64> <uint64>");
  Serial.println("example: TEST \"\\x41bc\\ndef\" -1.234e5 -123 123");
}

void loop() {
  if (Serial.available()) {
    char line[128];
    size_t lineLength = Serial.readBytesUntil('\n', line, 127);
    line[lineLength] = '\0';

    char response[MyCommandParser::MAX_RESPONSE_SIZE];
    parser.processCommand(line, response);
    Serial.println(response);
  }
}