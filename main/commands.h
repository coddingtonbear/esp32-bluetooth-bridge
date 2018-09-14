#pragma once

#define OTA_BUFFER_SIZE 1024

void setupCommands();
void commandPrompt();
void commandLoop();
void commandByte(char);

bool monitorBridgeEnabled();
bool escapeIsEnabled();

void flashEsp32();
void resetUC();
void flashUC();
void monitorBridge();
void boot0();
void setRst();
void enableEscape();
void unescape();
void unrecognized(const char *cmd);
