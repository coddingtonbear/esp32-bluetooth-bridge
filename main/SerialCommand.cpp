/**
 * SerialCommand - A Wiring/Arduino library to tokenize and parse commands
 * received over a serial port.
 * 
 * Copyright (C) 2012 Stefan Rado
 * Copyright (C) 2011 Steven Cogswell <steven.cogswell@gmail.com>
 *                    http://husks.wordpress.com
 * 
 * Version 20120522
 * 
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "SerialCommand.h"

/**
 * Constructor makes sure some things are set.
 */
SerialCommand::SerialCommand()
  : commandList(NULL),
    commandCount(0),
    defaultHandler(NULL),
    term('\n'),           // default terminator for commands, newline character
    last(NULL)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();

  serial = &Serial;
}

SerialCommand::SerialCommand(Stream* serialPort)
  : commandList(NULL),
    commandCount(0),
    defaultHandler(NULL),
    term('\n'),           // default terminator for commands, newline character
    last(NULL)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();

  serial = serialPort;
}

void SerialCommand::help() {
  serial->println("Available commands:");
  serial->println("===================");
  for (int i = 0; i < commandCount; i++) {
    serial->println(commandList[i].command);
  }
  serial->println("===================");
}
/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void SerialCommand::addCommand(const char *command, void (*function)()) {
  #ifdef SERIALCOMMAND_DEBUG
    serial->print("Adding command (");
    serial->print(commandCount);
    serial->print("): ");
    serial->println(command);
  #endif

  commandList = (SerialCommandCallback *) realloc(commandList, (commandCount + 1) * sizeof(SerialCommandCallback));
  strncpy(commandList[commandCount].command, command, SERIALCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].function = function;
  commandCount++;
}

void SerialCommand::disableEcho() {
  echoEnabled = false;
}

void SerialCommand::prompt() {
  if(echoEnabled) {
    serial->println();
    serial->print(SERIALCOMMAND_PROMPT);
  }
}

/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void SerialCommand::setDefaultHandler(void (*function)(const char *)) {
  defaultHandler = function;
}

void SerialCommand::readChar(char inChar) {
  if (inChar == term) {     // Check for the terminator (default '\r') meaning end of command
    if(echoEnabled) {
      serial->print(inChar);   // Echo back to serial stream
    }
    #ifdef SERIALCOMMAND_DEBUG
      serial->print("Received: ");
      serial->println(buffer);
    #endif

    char *command = strtok_r(buffer, delim, &last);   // Search for command at start of buffer
    if (command != NULL) {
      boolean matched = false;
      for (int i = 0; i < commandCount; i++) {
        #ifdef SERIALCOMMAND_DEBUG
          serial->print("Comparing [");
          serial->print(command);
          serial->print("] to [");
          serial->print(commandList[i].command);
          serial->println("]");
        #endif

        if (strncmp(command, "help", 4) == 0) {
          help();
          matched = true;
          break;
        }

        // Compare the found command against the list of known commands for a match
        if (strncmp(command, commandList[i].command, SERIALCOMMAND_MAXCOMMANDLENGTH) == 0) {
          #ifdef SERIALCOMMAND_DEBUG
            serial->print("Matched Command: ");
            serial->println(command);
          #endif

          // Execute the stored handler function for the command
          (*commandList[i].function)();
          matched = true;
          break;
        }
      }
      if (!matched && (defaultHandler != NULL)) {
        (*defaultHandler)(command);
      }
    }
    clearBuffer();
    prompt();
  } else if(inChar == 0x8 && bufPos > 0) {
    if(echoEnabled) {
      serial->write(0x08);
      serial->print(' ');
      serial->write(0x08);
    }
    bufPos--;
    buffer[bufPos] = '\0';
  } else if (isprint(inChar)) {     // Only printable characters into the buffer
    if(echoEnabled) {
      serial->print(inChar);   // Echo back to serial stream
    }
    if (bufPos < SERIALCOMMAND_BUFFER) {
      buffer[bufPos++] = inChar;  // Put character into buffer
      buffer[bufPos] = '\0';      // Null terminate
    } else {
      #ifdef SERIALCOMMAND_DEBUG
        serial->println("Line buffer is full - increase SERIALCOMMAND_BUFFER");
      #endif
    }
  }
}

/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void SerialCommand::readSerial() {
  while (serial->available() > 0) {
    char inChar = serial->read();   // Read single available character, there may be more waiting
    readChar(inChar);

  }
}

/*
 * Clear the input buffer.
 */
void SerialCommand::clearBuffer() {
  buffer[0] = '\0';
  bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *SerialCommand::next() {
  return strtok_r(NULL, delim, &last);
}
