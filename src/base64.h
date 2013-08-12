#ifndef _B64_H
#define _B64_H
int base64(const char* message, int len, char** buffer);
int debase64(char* b64message, char** buffer);
#endif
