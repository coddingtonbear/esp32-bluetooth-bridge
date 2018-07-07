#pragma once

void setupCommands();
void commandPrompt();
void commandLoop();
void commandByte(char);

bool monitorBridgeEnabled();
bool escapeIsEnabled();

void resetUC();
void flashUC();
void monitorBridge();
void boot0();
void setRst();
void enableEscape();
void unescape();
void unrecognized(const char *cmd);
