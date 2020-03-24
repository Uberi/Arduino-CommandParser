/*
  CommandParser.h - Library for parsing commands of the form "COMMAND_NAME ARG1 ARG2 ARG3 ...".

  Copyright 2020 Anthony Zhang (Uberi) <me@anthonyz.ca>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __COMMAND_PARSER_H__
#define __COMMAND_PARSER_H__

#include <cstring>

template<size_t COMMANDS = 16, size_t COMMAND_ARGS = 4, size_t COMMAND_NAME_LENGTH = 10, size_t COMMAND_ARG_SIZE = 32, size_t RESPONSE_SIZE = 64>
class CommandParser {
    public:
        static const size_t MAX_COMMANDS = COMMANDS;
        static const size_t MAX_COMMAND_ARGS = COMMAND_ARGS;
        static const size_t MAX_COMMAND_NAME_LENGTH = COMMAND_NAME_LENGTH;
        static const size_t MAX_COMMAND_ARG_SIZE = COMMAND_ARG_SIZE;
        static const size_t MAX_RESPONSE_SIZE = RESPONSE_SIZE;

        union Argument {
            double asDouble;
            uint64_t asUInt64;
            int64_t asInt64;
            char asString[MAX_COMMAND_ARG_SIZE + 1];
        };
    private:
        struct Command {
            char name[MAX_COMMAND_NAME_LENGTH + 1];
            char argTypes[MAX_COMMAND_ARGS + 1];
            void (*callback)(union Argument *args, char *response);
        };

        union Argument commandArgs[MAX_COMMAND_ARGS];
        struct Command commandDefinitions[MAX_COMMANDS];
        size_t numCommands = 0;

        bool parseString(const char *buf, char *output, size_t *readCount) {
            size_t pos = 0;

            if (buf[0] != '"') { return false; }
            pos ++; // move past the opening quote

            size_t i = 0;
            for (; i < MAX_COMMAND_ARG_SIZE && buf[pos] != '"' && buf[pos] != '\0'; i ++) { // loop through each character of the string literal
                if (buf[pos] == '\\') { // start of the escape sequence
                    pos ++; // move past the backslash
                    switch (buf[pos]) { // check what kind of escape sequence it is, turn it into the correct character
                        case 'n': output[i] = '\n'; pos ++; break;
                        case 'r': output[i] = '\r'; pos ++; break;
                        case 't': output[i] = '\t'; pos ++; break;
                        case '"': output[i] = '"'; pos ++; break;
                        case '\\': output[i] = '\\'; pos ++; break;
                        case 'x': { // hex escape, of the form \xNN where NN is a byte in hex
                            pos ++; // move past the "x" character
                            char hexCode[3] = {buf[pos], buf[pos] == '\0' ? '\0' : buf[pos + 1], '\0'}; // copy the hex code into its own string
                            char *afterHexCode; output[i] = (char)strtol(hexCode, &afterHexCode, 16);
                            if (hexCode == afterHexCode || afterHexCode[0] != '\0') { return false; } // invalid escape sequence, number wasn't fully parsed
                            pos += afterHexCode - hexCode;
                            break;
                        }
                        default: // unknown escape sequence
                            return false;
                    }
                } else { // non-escaped character
                    output[i] = buf[pos];
                    pos ++;
                }
            }
            output[i] = '\0';

            if (buf[pos] != '"') { return false; }
            pos ++; // move past the ending quote

            *readCount = pos;
            return true;
        }
    public:
        bool registerCommand(const char *name, const char *argTypes, void (*callback)(union Argument *args, char *response)) {
            if (numCommands == MAX_COMMANDS) { return false; }
            if (strlen(name) > MAX_COMMAND_NAME_LENGTH) { return false; }
            if (strlen(argTypes) > MAX_COMMAND_ARGS) { return false; }
            if (callback == nullptr) { return false; }
            for (size_t i = 0; argTypes[i] != '\0'; i ++) {
                switch (argTypes[i]) {
                    case 'd':
                    case 'u':
                    case 'i':
                    case 's':
                        break;
                    default:
                        return false;
                }
            }

            strlcpy(commandDefinitions[numCommands].name, name, MAX_COMMAND_NAME_LENGTH + 1);
            strlcpy(commandDefinitions[numCommands].argTypes, argTypes, MAX_COMMAND_ARGS + 1);
            commandDefinitions[numCommands].callback = callback;
            numCommands ++;
            return true;
        }

        bool processCommand(const char *command, char *response) {
            // retrieve the command name
            char name[MAX_COMMAND_NAME_LENGTH + 1];
            size_t i = 0;
            for (; i < MAX_COMMAND_NAME_LENGTH && *command != ' ' && *command != '\0'; i ++, command ++) { name[i] = *command; }
            name[i] = '\0';

            // look up the command argument types and callback
            char *argTypes = nullptr;
            void (*callback)(union Argument *, char *) = nullptr;
            for (size_t i = 0; i < numCommands; i ++) {
                if (strcmp(commandDefinitions[i].name, name) == 0) {
                    argTypes = commandDefinitions[i].argTypes;
                    callback = commandDefinitions[i].callback;
                    break;
                }
            }
            if (argTypes == nullptr) {
                snprintf(response, MAX_RESPONSE_SIZE, "parse error: unknown command name %s", name);
                return false;
            }

            // parse each command
            for (size_t i = 0; argTypes[i] != '\0'; i ++) {
                // require and skip 1 or more whitespace characters
                if (*command != ' ') {
                    snprintf(response, MAX_RESPONSE_SIZE, "parse error: missing whitespace before arg %d", i + 1);
                    return false;
                }
                do { command ++; } while (*command == ' ');

                switch (argTypes[i]) {
                    case 'd': { // double argument
                        char *after;
                        commandArgs[i].asDouble = strtod(command, &after);
                        if (after == command || (*after != ' ' && *after != '\0')) {
                            snprintf(response, MAX_RESPONSE_SIZE, "parse error: invalid double for arg %d", i + 1);
                            return false;
                        }
                        command = after;
                        break;
                    }
                    case 'u': { // uint64_t argument
                        char *after;
                        commandArgs[i].asUInt64 = strtoull(command, &after, 0);
                        if (after == command || (*after != ' ' && *after != '\0')) {
                            snprintf(response, MAX_RESPONSE_SIZE, "parse error: invalid uint64_t for arg %d", i + 1);
                            return false;
                        }
                        command = after;
                        break;
                    }
                    case 'i': { // int64_t argument
                        char *after;
                        commandArgs[i].asInt64 = strtoll(command, &after, 0);
                        if (after == command || (*after != ' ' && *after != '\0')) {
                            snprintf(response, MAX_RESPONSE_SIZE, "parse error: invalid int64_t for arg %d", i + 1);
                            return false;
                        }
                        command = after;
                        break;
                    }
                    case 's': {
                        size_t readCount = 0;
                        if (!parseString(command, commandArgs[i].asString, &readCount)) {
                            snprintf(response, MAX_RESPONSE_SIZE, "parse error: invalid string for arg %d", i + 1);
                            return false;
                        }
                        command += readCount;
                        break;
                    }
                    default:
                        snprintf(response, MAX_RESPONSE_SIZE, "parse error: invalid argtype %c for arg %d", argTypes[i], i + 1);
                        return false;
                }
            }

            // skip whitespace
            while (*command == ' ') { command ++; }

            // ensure that we're at the end of the command
            if (*command != '\0') {
                snprintf(response, MAX_RESPONSE_SIZE, "parse error: too many args (expected %d)", strlen(argTypes));
                return false;
            }

            // set response to empty string
            response[0] = '\0';

            // invoke the command
            (*callback)(commandArgs, response);
            return true;
        }
};

#endif
