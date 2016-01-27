#ifndef IPOIRC_BASE64_H
#define IPOIRC_BASE64_H
#include <string>

int base64(const char* message, int len, char** buffer);
int debase64(const std::string &const_b64msg, char** buffer);
#endif
