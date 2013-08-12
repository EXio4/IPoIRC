#include <math.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "base64.h"


// encoding/decoding code from: http://doctrina.org/Base64-With-OpenSSL-C-API.html


int base64(const char* message, int len, char** buffer) {
  BIO *bio, *b64;
  FILE* stream;
  int encodedSize = 4*ceil((double)len/3);
  *buffer = (char *)malloc(encodedSize+1);

  stream = fmemopen(*buffer, encodedSize+1, "w");
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new_fp(stream, BIO_NOCLOSE);
  bio = BIO_push(b64, bio);
  //BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  int ret = BIO_write(bio, message, len);
  BIO_flush(bio);
  BIO_free_all(bio);
  fclose(stream);

  return ret;
}


int calcDecodeLength(const char* b64input) {
    int len = strlen(b64input);
    int padding = 0;

    if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
        padding = 2;
    else if (b64input[len-1] == '=') //last char is =
        padding = 1;

    return (int)len*0.75 - padding;
}


int debase64(char* b64message, char** buffer) {
    BIO *bio, *b64;
    int decodeLen = calcDecodeLength(b64message),
    len = 0;
    *buffer = (char*)malloc(decodeLen+1);
    FILE* stream = fmemopen(b64message, strlen(b64message), "r");

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    len = BIO_read(bio, *buffer, strlen(b64message));
    //Can test here if len == decodeLen - if not, then return an error
    (*buffer)[len] = '\0';

    BIO_free_all(bio);
    fclose(stream);

    return len; //success
}


#ifdef TESTS

// yes, macros are awesome!

#define INIT_TESTS  int main(void) {\
                        int res=0;\
                        do {\
                            int count=0;\

#define TEST(len, str)  do {\
                int x=__test(count++, len, str);\
                res+=x;\
            } while(0);

#define END_TESTS       } while(0);\
                return res;\
            }

int __test(int count, int len, char *str) {
    printf("test %d .. ", count);
    char exp[len+1];
    snprintf(exp, len, str);
    char *x, *z;
    base64(str, len, &x);
    debase64(x, &z);
    int i, fx=0;
    for (i=0;i<len;i++) {
        if (str[i] != z[i]) {
            printf("\n\t%c != %c .. ", str[i], z[i]);
            fx++;
        }
    }
    if (fx==0) {
        printf("passed\n");
    } else {
        printf("\n{failed}\n");
        printf("\terrors: %d\nbase64 generated: %s\n", fx, x);
    }
    free(x);
    free(z);
    return fx;
}

INIT_TESTS

    TEST(3, "abc")
    TEST(3, "a\x0\x0")
    TEST(10, "\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9")
    TEST(64, "+JQ;%NB19FON02!UK'>.P4LD-LME1&96+W;K:,S/WAPQ654B##3\"<N9K7")

END_TESTS

#endif
