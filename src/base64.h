#ifndef IPOIRC_BASE64_H
#define IPOIRC_BASE64_H
#include <string>

std::string base64(const char* message, int len);
int debase64(const std::string &const_b64msg, char** buffer);
#endif
