CommandParser
=============

An Arduino library for parsing commands of the form `COMMAND_NAME ARG1 ARG2 ARG3 ...`.

Features:

* **No dynamic memory allocation**.
* Compile-time-**configurable resource limits**.
* Strongly typed arguments with **strict input validation** (including integer overflow detection!).
* Friendly **error messages** for invalid inputs (e.g., `parse error: invalid double for arg 2` if the command's second argument cannot be parsed as a double).
* Support for **escape sequences** in string arguments (e.g., `SOME_COMMAND "\"\x41\r\n\t\\"`).

This library works with all Arduino-compatible boards.

This library has a higher RAM footprint compared to similar libraries, because it fully parses each argument instead of leaving them as strings. An instance of `CommandParser<>` in RAM is 456 bytes on a 32-bit Arduino Nano 33 BLE.

Quickstart
----------

Search for "CommandParser" in the Arduino Library Manager, and install it. Now you can try a quick example sketch that accepts commands over serial:

```cpp
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
```

More examples:

* [Parsing commands over Serial](examples/SerialCommands/SerialCommands.ino)
* [Customizing resource limits](examples/CustomizeParameters/CustomizeParameters.ino)

Grammar
-------

Commands are null-terminated strings that largely follow this [PEG grammar](https://en.wikipedia.org/wiki/Parsing_expression_grammar):

```
COMMAND <- COMMAND_NAME (' '+ (ARG_STRING / ARG_DOUBLE / ARG_INT64 / ARG_UINT64))* ' '*
COMMAND_NAME <- (!' ')+
ARG_STRING <- ('"' STRING_CHAR_QUOTED* '"') / (STRING_CHAR_UNQUOTED+)
ARG_DOUBLE <- SIGN? ((DEC+ '.' DEC*) / ('.' DEC+)) (('e' / 'E') SIGN? DEC+)?
ARG_INT64 <- SIGN? ARG_UINT64
ARG_UINT64 <- ('0x' HEX+) / ('0o' OCT+) / ('0b' BIN+) / DEC+
STRING_CHAR_QUOTED <- STRING_CHAR_ESCAPE / (!'"')
STRING_CHAR_UNQUOTED <- STRING_CHAR_ESCAPE / (!' ')
STRING_CHAR_ESCAPE <- ( '\\x' HEX HEX ) / '\\n' / '\\r' / '\\t' / '\\"' / '\\\\'
HEX <- [0-9a-fA-F]
DEC <- [0-9]
OCT <- [0-7]
BIN <- [0-1]
SIGN <- '+' / '-'
```

This grammar is a superset of what the parser will actually accept, since the `ARG_STRING / ARG_DOUBLE / ARG_INT64 / ARG_UINT64` choice is predetermined when `COMMAND_NAME` is registered (i.e., a given command always accepts the same number and types of arguments). Additionally, there are other limits (string arguments that are longer than `COMMAND_ARG_SIZE`, int64 arguments that would underflow/overflow a 64-bit signed integer, and so on) that further restrict what inputs are accepted.

Here are some examples of strings that match the `COMMAND` rule in the above grammar:

* `test_command "Hello, \"World\"! \\\x31\x32\x33"` where `test_command` is registered with arg types `s`: one string argument with value `Hello, "World"! \123`.
* `! yes no` where `!` is registered with arg types `ss`: two string arguments, one with value `yes` and the other `no`.
* `ABC 0.1 +.1 -123 0. 0 12.3e-1 1E10` where `ABC` is registered with arg types `dddddd`: six double arguments with values `0.1`, `0.1`, `-123`, `0`, `0`, `1.23`, `10000000000`.
* `SomeCommand 0 +0x1F 0b101 0o77 -26` where `SomeCommand` is registered with arg types `iiiii`: five int64 arguments with values `0`, `31`, `5`, `63`, `-26`.
* `SomeCommand 0 0x1F 0b101 0o77 26` where `SomeCommand` is registered with arg types `uuuuu`: five uint64 arguments with values `0`, `31`, `5`, `63`, `26`.

Comparison
----------

Other libraries for parsing commands:

* [CmdArduino](https://github.com/joshmarinacci/CmdArduino):
    * CmdArduino has hardcoded size limits for arguments/commands, only supports string arguments, and doesn't support string escape sequences. It works well as the most minimal option.
    * CommandParser has configurable size limits, supports multiple argument types, and supports string escape sequences.
* [CmdParser](https://github.com/pvizeli/CmdParser):
    * CmdParser's command syntax is more configurable, with options such as setting the separator character, named parameters, and quoting behaviour. It works well as the most configurable option.
    * CommandParser's command syntax is not configurable, but additionally supports escape sequences in string arguments, non-string arguments such as doubles and integers that are automatically parsed/validated, and empty string arguments.
    * Also, CommandParser checks all inputs for validity (including syntax errors when invalid input is given), and doesn't assume the use of Serial.
* [SerialCommand](https://github.com/kroimon/Arduino-SerialCommand):
    * SerialCommand only accepts input from Serial, only supports string arguments, doesn't validate that the number of arguments, and doesn't support strings that contain the argument delimiter or any escape sequences. It works well for simpler Serial-specific use cases.
    * CommandParser accepts input from any char array, supports multiple argument types, validates that all arguments parse correctly and that the correct syntax is used, and supports escape sequences in strings.

CommandParser is likely the best choice when you need strict input validation, error checking, and configurable resource usage.

CommandParser is likely not the best choice when you need to customize the command syntax, use optional/named arguments, or are very tight on RAM.

Reference
---------

### `CommandParser<COMMANDS, COMMAND_ARGS, COMMAND_NAME_LENGTH, COMMAND_ARG_SIZE, RESPONSE_SIZE>`

This accepts several template arguments that limit how much RAM is required:

* `size_t COMMANDS = 16` - up to 16 commands can be registered. Past this limit, `registerCommand` will return `false`.
* `size_t COMMAND_ARGS = 4` - up to 4 arguments are supported for any given command. Past this limit, `registerCommand` will return `false`.
* `size_t COMMAND_NAME_LENGTH = 10` - command names can be up to 10 bytes. Past this limit, `registerCommand` will return `false`.
* `size_t COMMAND_ARG_SIZE = 32` - arguments passed to commands can be up to 32 bytes in size. Note that there may be more than 32 characters used to represent the argument; for example, a string argument `"\x41\x42\x43"` is 14 characters but the argument would only be 3 bytes, 0x41, 0x42, and 0x43. Past this limit, `processCommand` will return `false`.
* `size_t RESPONSE_SIZE = 64` - responses from command callback can be up to 64 characters in length. Past this limit, there is a risk of buffer overflow - always use bounded string handling functions such as `strlcpy` and `snprintf` when writing responses.

To avoid writing these arguments in multiple places in your code, typically you'll want to do something like `typedef CommandParser<16, 4, 10, 32, 64> MyCommandParser;`, and then use `MyCommandParser` everywhere else in your code.

These properties are then also available as static class variables: `CommandParser::MAX_COMMANDS`. `CommandParser::MAX_COMMAND_ARGS`, `CommandParser<...>::MAX_COMMAND_NAME_LENGTH`, `CommandParser<...>::MAX_COMMAND_ARG_SIZE`, `CommandParser<...>::MAX_RESPONSE_SIZE`.

### `CommandParser<...>::Argument`

This union struct represents a single argument value. Access the correct field to get the right value out - if `arg` is a `CommandParser<...>::Argument` representing an `int64_t` argument, then `arg.asInt64` is the `int64_t` value that it contains.

Command callbacks are passed an array of these, as well as a buffer to write their response into.

### `bool CommandParser<...>::registerCommand(const char *name, const char *argTypes, void (*callback)(union Argument *args, char *response))`

Registers a new command with name `name` and argument types `argTypes`. When `CommandParser<...>::processCommand` processes input containing this command, it calls `callback` with the parsed arguments.

Each character in `argTypes` represents the type of its corresponding positional argument:

* `d` represents a `double`.
* `i` represents an `int64_t`.
* `u` represents an `uint64_t`.
* `s` represents a `char *` (as a null-terminated string).

So if `argTypes` is `"sdiu"`, that represents four arguments, where the first is a string, the second is a double, the third is a 64-bit signed integer, and the fourth is a 64-bit unsigned integer.

Returns `true` if the command was successfully registered, `false` otherwise (usually because it exceeds the `CommandParser<...>` limits).

### `bool CommandParser<...>::processCommand(const char *command, char *response)`

Processes a string `command` that contains a command previously registed using `CommandParser<...>::registerCommand`, parsing any arguments and looking up the command's callback.

If `command` is fully parsed, this calls the command's callback with an array of `CommandParser<...>::Argument` instances, as well as a response buffer `response`, which the callback may choose to write in (`response` is initialized to an empty string before the callback is called), then returns `true`.

Otherwise, `command` could not be fully parsed, so `processCommand` will write a descriptive error message to `response`, no callbacks will be called, and this returns `false`.
