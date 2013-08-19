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

    *buffer = (char*)malloc(len*5+1); // avoiding problems with missing chars when splitting the strings (added newlines)

    stream = fmemopen(*buffer, (len*5)+1, "w");
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
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
    len = BIO_read(bio, *buffer, strlen(b64message));
    (*buffer)[len] = '\0';

    BIO_free_all(bio);
    fclose(stream);

    if (len != decodeLen) {
        return -1;
    }

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

    // pwgen -ns 64 50 | sed -e 's/.*/TEST(64, "&")/'

    TEST(64, "NLRxlRDv7F1PNGEXRtVg1bh2XFLNmSoafJaW9QWDcD4MIFJrMKMLEwXzr4sOtOYB")
    TEST(64, "riZPhkpWKaJBIFwsgZCUunsSRhHusCJ0gKN9o6HA7ZjhQfJg42V8qUnqk6gWWatd")
    TEST(64, "9w2dxc8QgKzS0zTHokpd0t2HbAp7oxgY2SCWGzLcYKIGv9r6VM3kaRHOz7pzBXSi")
    TEST(64, "u6xyyjcua0YoTM1FFLpQgdDPbkGKLtijD5gwopX2s5B6nsKv8pUdVJQFBEAKCtvv")
    TEST(64, "Tx984AvibFUcdBDwGfVnbsdfXSNOMpqvwxMuqH4b1mRAWzIFGQBL1IFu3tptCXxw")
    TEST(64, "s5P7nq5BWphPjfEnvrELfeiHoETFgg0UEmweDx0MxZdhcRf5C5HkMV0StkUriOl6")
    TEST(64, "1xVm0ozSSpZoB1gpHqj0GISFZgjcbMLTf46dBfsS7cshPSzXNCTAMO9AQWy0zCXS")
    TEST(64, "K6na1vVKZs0S8ovwVs9ejsClIytfVSkcsEXVkfxDP4ntze6oJ3mcxklcD041gViU")
    TEST(64, "hqRfpSEe96hxIRqmVgmSJbyYJWXopokVLgudJGj9phuKkxOtGI8Dd9GkftI7oRD6")
    TEST(64, "cOuoQYg1h5Q3coKmenZW1IdWROmPnaiXxTw2qq9lPe3QpZ06JWjISl5TPs6CzMoE")
    TEST(64, "rB4uTbAdb0zVqB3MJVbheOdbYmrl6fFF3BviNtEPaC3zWLXk5JngqvgYu7wdBLYg")
    TEST(64, "442KrHE88aaQxccuAwGR17uBWQFVYMtFkpekxCrZIjVVTd65PPGcPu2Viix2ZTq4")
    TEST(64, "yNOBloODmyNQBPx04zXsiWyVxoLSLWHnhsxWidKEQw7IkjDy3YWPnEcQ7qYmT6Xk")
    TEST(64, "6abq1wVvTtDP9CTVAiJ1yhnMJNGGoCj6Nx4IM9eI92m8e4MuxLCTfUcHB9GOQpUk")
    TEST(64, "mrNeHKH09m4BHrZqwEMRbskMdALjogBmfutyi9Yfhl0Yu9ZwLovqd5d8KSofpSA7")
    TEST(64, "r1VldP53yLCgPRTjEHs6XVx9DcwQSgCI4MK4ZEmD8YXTDkglTdVWAHOPsSxsuprR")
    TEST(64, "DhR0pyiDRvbIF5zq3kdWpjL8T2UqVp7Vr8Ii8NuDTcUTwsuajFuYE81JctxnkeHE")
    TEST(64, "eg5hdeuXhHosJgf3LML0bhbo8UmvG3nDiJHzBLA9HOubMofkRMePPGeW4tE51flu")
    TEST(64, "66XORHqczne0S5KvdXpJYMx1mICfzgvEQKvCB2XwJEZsbiUF8Gaha9kHaZCBzKeB")
    TEST(64, "OijK4Rjic1ciuroYTcozdjd9oqgaIdxrhv6iykuu81Fm0hO9d8fE5d6c7sYBrzTv")
    TEST(64, "2Mnc0APZNTQHIWDQUxfMfQhTmKbMeeR97tBpflLy9Us7xzMOUMdqcUorBQ1uXKvv")
    TEST(64, "7twJkAQMyGNr0PXPtm5zmlBOE3hV6O17KHda9j4GB0DW1DHokmFZetb0ZEkSl0HV")
    TEST(64, "FYJTGPnH652eMa1yTWJ86a4eIz00IQh5X0rpDw5g5tukt3nJT1tgpfUrhNP5myU4")
    TEST(64, "a3vPRfALvLx0kIpJr6nopcwXVqULTX37NHIR9a5i375q175e21mBbnW2wD2GvO6a")
    TEST(64, "NeuLTh1tqFTGWROVI3GB43z2zGpDP1vhItgKeslRcvH2ey0bipXxkGF72vRmsiBw")
    TEST(64, "0eohaue9Z65l0dbPszaCpB6DpbXBdsTxfj6pWLJcEqsIxLihfpXiWInbwPF55nrE")
    TEST(64, "l3p52T2O2kVT0YEWUTQZeM09LkqJDZLnMCOlbBQyDAkQgtKw9WwaxZhWBw9mAF2j")
    TEST(64, "DuQrMJo0K3OY9NfCDmX0JuLuwoS6dScca6aaAFmyeLMyWQXedQGwAIRck2zQE0xv")
    TEST(64, "oP7aAhO53moHCA0K9AwmbGcATndA7o82D0E8LfVSal5evhfne2Z1iGqIfFwjzFVO")
    TEST(64, "U1F7U1WqjKiackVF20zRs1tD8E1upCcY6ioeoR9rWg6cfWb0U0946wsorzMI12pM")
    TEST(64, "lzknmZxvePD2jpaK5UoTkMWzPd6pawADr8LI5MNhl6jJzpdUccYuTr0XD6txoTA6")
    TEST(64, "5XpfQvVUTlW2vkKPOAVPrzcheKeeNCa31nn2uVRerq53c1tm95vtBLB628mR8yz5")
    TEST(64, "yw4iRZGPJ2TiGBBaTq06dLnB0WMwTFmXJ4EB250vypR0MtJxBmHRJJk9SiWm2UI0")
    TEST(64, "10mQJqy49t85G1oiIpN7yBR17UnNsX12Y7X3ZbNRDUISfO6yeidOCPuk4Dz6qhgX")
    TEST(64, "P7k40HlFpRvv9iLwKHTMpkMFHT8XWbWY4ZnUSWb9nEORM49DhvPcEMJzrSs8BlRZ")
    TEST(64, "aKnJUzR7WxF1M1SZf4XQKOyX3C7cXfMj5Z0B3MFjy2cJj1TLO3h4EN0UkIK8W9BZ")
    TEST(64, "NKb4XmJXr1HlG4dEEmvYTuwbpr3FL2x9nbMP38FrSbuth09uhrtW24kmYecZI80M")
    TEST(64, "CpMwsme4KsElFOsqs8pg6kj3ee0ZKpt7SbODOTCJfZM265jI8bmpIz8jLooWlrLS")
    TEST(64, "qCfx0Ab9v50dCo01nkoSP2ajBJwamczBJn74Ql4j1xUWTwVf6QKCRQ5kPsET7FUY")
    TEST(64, "cunZk2BL7ECsSatAVjtYQcEX6rmD28E8RtTkDqs1Bq5AIp2mewHNOU5QNfpr31HV")
    TEST(64, "IXpxIEQtg71JWbinL7wzxvF66PdnlatyAEG3pa9sROpz8RcJvFO909FEMes6aPSs")
    TEST(64, "8bWS5J1qwYhxcyr64AenXKnFL9EzwejnyOWfYpoySWnqUaX0lOnd337yxe0IiSKv")
    TEST(64, "FlHf8mBkZ8uwfsRKEatawAhPOIYRjHjKTB2A0j9cFOFuWCYGJq21TJ46dks3aC96")
    TEST(64, "G9ZtXpPPHeb6r2LmdjbduuaL5gUqn9X5Vi9ul5BG9wxlCDsuWYLaZDUq5CM5HU26")
    TEST(64, "TRBtcQwGnqSSgDvpYPf4PpjMktfJYAYPS3SCwMLOFUC4w03zDZRxoITwabYbbwSD")
    TEST(64, "Mb5N4pkJtOHraqe5e8DEtPZZMkNI94xiLbBNJytLBCZkVyNydXAwSrIgUVWlycpE")
    TEST(64, "Y4oxSAaEfrSuiN1aHjdN8i6kxHmztUQO0NHINxxGJLtcyeB2WeIH25nxryG8FR13")
    TEST(64, "PBy1g7w5PeF4DlIYlqs9FzkryPB9ztMqi2eDnKxnBtTdOFvT4sZF7VCqoRPYuecX")
    TEST(64, "xEaeVC5nhh0dndDMQ1YIlLR5vchVcG0B31s7AU8oOCarS64kL9rXjXaDG3HgZp63")
    TEST(64, "Xks7gLCEL0wJ5IMZmHUaaMQuXvr7obD4FuZLttClqRoaTtWUvdt92RZtAX5YOUyW")


END_TESTS

#endif
