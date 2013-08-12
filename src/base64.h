#ifndef _B64_H
#define _B64_H
char *base64(const unsigned char *input, int length);
int debase64(char* b64message, char** buffer);
#endif
