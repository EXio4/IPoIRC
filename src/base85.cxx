#include <string>
#include <regex>

#include <math.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "base85.h"

namespace Base85 {
    static const char en85[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        '!', '#', '$', '%', '&', '(', ')', '*', '+', '-',
        ';', '<', '=', '>', '?', '@', '^', '_',	'`', '{',
        '|', '}', '~'
    };

    static char de85[256];
    static void prep_base85(void) {
        int i;
        if (de85['Z'])
            return;
        for (i = 0; i < 256; i++) {
            int ch = en85[i];
            de85[ch] = i + 1;
        }
    }

    int decode_85(char *dst, const char *buffer, int len) {
        prep_base85();
        while (len) {
            unsigned acc = 0;
            int de, cnt = 4;
            unsigned char ch;
            do {
                ch = *buffer++;
                de = de85[ch];
                if (--de < 0)
                    return -1;
                acc = acc * 85 + de;
            } while (--cnt);
            ch = *buffer++;
            de = de85[ch];
            if (--de < 0)
                return -1;
            /* Detect overflow. */
            if (0xffffffff / 85 < acc ||
                0xffffffff - de < (acc *= 85))
                return -1;
            acc += de;
            cnt = (len < 4) ? len : 4;
            len -= cnt;
            do {
                acc = (acc << 8) | (acc >> 24);
                *dst++ = acc;
            } while (--cnt);
        }
        return 0;
    }

    void encode_85(char *buf, const unsigned char *data, int bytes) {
        while (bytes) {
            unsigned int acc = 0;
            for (int cnt = 24; cnt >= 0; cnt -= 8) {
                unsigned int ch = *data++;
                acc |= ch << cnt;
                if (--bytes == 0)
                    break;
            }
            for (int cnt = 4; cnt >= 0; cnt--) {
                int val = acc % 85;
                acc /= 85;
                buf[cnt] = en85[val];
            }
            buf += 5;
        }

        *buf = 0;
    }

    #ifdef DEBUG_85
    int main(int ac, char **av)  {
        char buf[1024];

        if (!strcmp(av[1], "-e")) {
            int len = strlen(av[2]);
            encode_85(buf, av[2], len);
            if (len <= 26) len = len + 'A' - 1;
            else len = len + 'a' - 26 - 1;
            printf("encoded: %c%s\n", len, buf);
            return 0;
        }
        if (!strcmp(av[1], "-d")) {
            int len = *av[2];
            if ('A' <= len && len <= 'Z') len = len - 'A' + 1;
            else len = len - 'a' + 26 + 1;
            decode_85(buf, av[2]+1, len);
            printf("decoded: %.*s\n", len, buf);
            return 0;
        }
        if (!strcmp(av[1], "-t")) {
            char t[4] = { -1,-1,-1,-1 };
            encode_85(buf, t, 4);
            printf("encoded: D%s\n", buf);
            return 0;
        }
    }
    #endif


    std::string encode(const char* message, int len) {
        char *buffer = (char*)malloc((len * 6) / 4);
        encode_85(buffer, (const unsigned char*)message, len);
        std::string r = buffer;
        free(buffer);
        return std::to_string(len) + "." + r;
    };
    int decode(const std::string &const_msg, char* &buffer) {
        std::regex r { "([^0-9]*)\\.(.*)" } ;
        std::smatch p_match;
        if (std::regex_match(const_msg, p_match, r) && p_match.size() == 2+1) {
            int len = std::stoi(p_match[1]);
            buffer = (char*)malloc(sizeof(char) * len);
            return decode_85(buffer, ((std::string)p_match[2]).c_str(), len);
        } else { return -1; }
    }

    std::string encode_base64(const char* message, int len) {
        BIO *bio, *b64;
        FILE* stream;

        char *buffer = (char*)malloc(len*5+1); // avoiding problems with missing chars when splitting the strings (added newlines)

        stream = fmemopen(buffer, (len*5)+1, "w");
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_fp(stream, BIO_NOCLOSE);
        bio = BIO_push(b64, bio);
        BIO_write(bio, message, len);
        BIO_flush(bio);
        BIO_free_all(bio);
        fclose(stream);

        /* awful hacky shit until I manage to use a C++friendly base64 library */
        std::string rest;
        int l = strlen(buffer);
        for (int i=0; i<l; i++) {
            if (buffer[i] == '\n') continue;
            rest += buffer[i];
        };

        return rest;
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

    int decode_base64(const std::string &const_b64msg, char** buffer) {
        BIO *bio, *b64;
        char *b64message = strdup(const_b64msg.c_str());
        int decodeLen = calcDecodeLength(b64message),
        len = 0;
        *buffer = (char*)malloc(decodeLen+1);
        FILE* stream = fmemopen(b64message, strlen(b64message), "r");

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_fp(stream, BIO_NOCLOSE);
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        len = BIO_read(bio, *buffer, strlen(b64message));
        (*buffer)[len] = '\0';

        BIO_free_all(bio);
        fclose(stream);
        free(b64message);

        if (len != decodeLen) {
            return -1;
        }

        return len; //success
    }
}