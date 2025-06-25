
#include <openssl/evp.h>

#include "cryptography.h"



int scrypt_user_secret(
        char* input,  size_t input_size,
        char* salt,   size_t salt_size,
        char* output, size_t output_max_size
){
    int res = 0;

    if(!input) {
        fprintf(stderr, "%s: input cant be NULL\n", __func__);
        goto error;
    }

    if(input_size == 0) {
        fprintf(stderr, "%s: input_size cant be zero\n", __func__);
        goto error;
    }

    if(!output) {
        fprintf(stderr, "%s: output cant be NULL\n", __func__);
        goto error;
    }


    const uint64_t N = 262144; // 2 ^ 18
    const uint64_t R = 8;
    const uint64_t P = 1;
    const uint64_t max_mem = 1024 * 1024 * 512;  // ~525 MB

    res = EVP_PBE_scrypt(
            input, input_size,
            salt, salt_size,
            N, R, P, max_mem,
            output, output_max_size
            );

error:
    return res;
}


void encrypt_aes256(
        char* key,
        char* iv,
        char* data,   size_t data_size,
        char* output, size_t output_max_size
){
    int out_size = 0;
    int tmp_size = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    
    EVP_EncryptUpdate(ctx, output, &out_size, data, data_size);
    EVP_EncryptFinal_ex(ctx, output + out_size, &tmp_size);

    EVP_CIPHER_CTX_free(ctx);
}


void decrypt_aes256(
        char* key,
        char* iv,
        char* data,   size_t data_size,
        char* output, size_t output_max_size
){
    int out_size = 0;
    int tmp_size = 0;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    EVP_DecryptUpdate(ctx, output, &out_size, data, data_size);
    EVP_DecryptFinal_ex(ctx, output + out_size, &tmp_size);

    EVP_CIPHER_CTX_free(ctx);
}


