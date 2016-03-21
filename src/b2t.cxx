#include <string>
#include <regex>

#include <math.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "b2t.h"

namespace B2T {
    std::string encode_base64(const unsigned char* message, int len) {
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
        int l = strlen((const char*)buffer);
        for (int i=0; i<l; i++) {
            if (buffer[i] == '\n') continue;
            rest += buffer[i];
        };
        free(buffer);

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

    boost::optional<std::vector<uint8_t>> decode_base64(const std::string &const_b64msg) {
        BIO *bio, *b64;
        char *b64message = strdup(const_b64msg.c_str());
        int decodeLen = calcDecodeLength(b64message),
        len = 0;
        std::vector<uint8_t> buffer(decodeLen+1);
        FILE* stream = fmemopen(b64message, strlen(b64message), "r");

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_fp(stream, BIO_NOCLOSE);
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        len = BIO_read(bio, buffer.data(), strlen(b64message));
        buffer.resize(len);

        BIO_free_all(bio);
        fclose(stream);
        free(b64message);

        if (len != decodeLen) {
            return boost::none;
        }

        return buffer;
    }

    std::string encode(const std::vector<uint8_t>& m) {
        return encode_base64(m.data(), m.size());
    }
    boost::optional<std::vector<uint8_t>> decode(const std::string &const_msg) {
        return decode_base64(const_msg);
    }

}