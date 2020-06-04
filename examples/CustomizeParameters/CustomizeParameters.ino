#include <CommandParser.h>

// all of the template arguments below are optional, but it is useful to adjust them to save memory (by lowering the limits) or allow larger inputs (by increasing the limits)
// limit number of commands to at most 5
// limit number of arguments per command to at most 3
// limit length of command names to 10 characters
// limit size of all arguments to 15 bytes (e.g., the argument "\x41\x42\x43" uses 14 characters to represent the string but is actually only 3 bytes, 0x41, 0x42, and 0x43)
// limit size of response strings to 64 bytes
typedef CommandParser<5, 3, 10, 15, 64> MyCommandParser;

MyCommandParser parser;
int positionX = 0, positionY = 0;

void cmd_move(MyCommandParser::Argument *args, char *response) {
  positionX = args[0].asInt64;
  positionY = args[1].asInt64;
  Serial.print("MOVING "); Serial.print(args[0].asInt64); Serial.print(" "); Serial.println(args[1].asInt64);
  snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "moved to %d, %d", positionX, positionY);
}

void cmd_jump(MyCommandParser::Argument *args, char *response) {
  Serial.println("JUMPING!");
  snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "jumped at %d, %d", positionX, positionY);
}

void cmd_say(MyCommandParser::Argument *args, char *response) {
  Serial.print("SAYING "); Serial.println(args[0].asString);
  snprintf(response, MyCommandParser::MAX_RESPONSE_SIZE, "said %s at %d, %d", args[0].asString, positionX, positionY);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  parser.registerCommand("move", "ii", &cmd_move); // two int64_t arguments
  parser.registerCommand("jump", "", &cmd_jump); // no arguments
  parser.registerCommand("say", "s", &cmd_say); // one string argument

  char response[MyCommandParser::MAX_RESPONSE_SIZE];

  Serial.println("let's try a simple example...");
  parser.processCommand("move 45 -23", response);
  Serial.println(response);
  parser.processCommand("jump", response);
  Serial.println(response);
  parser.processCommand("say \"Hello, \\\"world!\\\"\"", response);
  Serial.println(response);

  Serial.println("now let's try some invalid inputs...");
  parser.processCommand("invalid", response); // bad command
  Serial.println(response);
  parser.processCommand("move", response); // missing args
  Serial.println(response);
  parser.processCommand("jump 123", response); // extra args
  Serial.println(response);
  parser.processCommand("move 123abc 456", response); // invalid int64 argument
  Serial.println(response);
  parser.processCommand("say abc", response); // invalid string argument
  Serial.println(response);
  parser.processCommand("say \"\\x\"", response); // invalid string argument
  Serial.println(response);
  parser.processCommand("say \"\\x5z\"", response); // invalid string escape
  Serial.println(response);
  parser.processCommand("say \"abc", response); // missing ending quote
  Serial.println(response);
}

void loop() {}
