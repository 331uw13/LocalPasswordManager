#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H


int scrypt_user_secret(
        char* input,  size_t input_size,
        char* salt,   size_t salt_size,
        char* output, size_t output_max_size);

void encrypt_aes256(
        char* key,
        char* iv,
        char* data,   size_t data_size,
        char* output, size_t output_max_size);

void decrypt_aes256(
        char* key,
        char* iv,
        char* data,   size_t data_size,
        char* output, size_t output_max_size);


#endif
