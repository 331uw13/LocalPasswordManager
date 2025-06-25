
#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <time.h>
#include <libconfig.h>

#include "manager.h"
#include "utils.h"
#include "cryptography.h"


#define USER_INPUT_MAX_SIZE 512

#define KEY_SIZE 32
#define MAX_SALT_SIZE 32
#define AES_IV_SIZE 32





static struct termios g_old_termcfg;
static int g_seed = 0;


void set_input_echo(int option) {
    if(option == ECHO_DISABLED) {
    
        // Modify local modes.
        struct termios cfg = g_old_termcfg;
        cfg.c_lflag &= ~(ISIG | ECHO); // Disable signals and echo.

        if(tcsetattr(0, TCSANOW, &cfg) < 0) {
            fprintf(stderr, "Warning: Failed to disable echoing. Input will stay visible!\n");
        }
    }
    else
    if(option == ECHO_ENABLED) {
        tcsetattr(0, TCSANOW, &g_old_termcfg);
    }
}


int init_manager() {
    int res = 0;

    set_random_gen_seed(time(0));

    if(tcgetattr(0, &g_old_termcfg) < 0) {
        fprintf(stderr, "%s: Could not get current term config.\n",
                __func__);
    }

    res = 1;
    return res;
}





static int get_user_secret_input(char* output, ssize_t* output_size) {
    int res = 0;
    if(!output || !output_size) {
        goto error;
    }

    printf("Enter passphrase: ");
    fflush(0);

    *output_size = read(1, output, USER_INPUT_MAX_SIZE);
    if(*output_size <= 1) {
        fprintf(stderr, "\n\033[31mPassphrase cant be empty.\033[0m\n");
    }

    res = 1;
    printf("\n");
error:
    return res;
}


static int add_config_group_data(struct config_setting_t* group, const char* name, const char* value) {
    int res = 0;
    config_setting_t* setting = config_setting_add(group, name, CONFIG_TYPE_STRING);
    
    if(!setting) {
        fprintf(stderr, "Failed to add value to group.\n",
                __func__);
        goto error;
    }

    config_setting_set_string(setting, value);


    res = 1;
error:
    return res;
}


void manager_write_entry(char* ent_name, size_t ent_name_size, const char* filename) {

    printf("\033[36m(WARNING! Your input will be visible)\033[0m\n\n");

    printf("Data for \"%s\": ",
            ent_name,
            USER_INPUT_MAX_SIZE);
    fflush(0);

    char ent_data[USER_INPUT_MAX_SIZE+1] = { 0 };
    ssize_t ent_data_size = read(1, ent_data, USER_INPUT_MAX_SIZE+1);
    if(ent_data_size <= 1) {
        fprintf(stderr, "\033[31mEntry data cant be empty.\033[0m\n");
        return;
    }

    // read() should not create a buffer overflow
    // but it is a good idea here to give the user warning
    // if the data is too large to be stored.
    if(ent_data_size >= USER_INPUT_MAX_SIZE) {
        fprintf(stderr, "\033[31mEntry data is too big.\033[0m\n");
        return;
    }


    char user_pass[USER_INPUT_MAX_SIZE] = { 0 };
    ssize_t user_pass_size = 0;
    if(!get_user_secret_input(user_pass, &user_pass_size)) {
        return;
    }


    char salt[MAX_SALT_SIZE+1] = { 0 };
    random_chars(salt, MAX_SALT_SIZE);


    char key[KEY_SIZE+1] = { 0 };

    scrypt_user_secret(
            user_pass, user_pass_size,
            salt, MAX_SALT_SIZE,
            key,  KEY_SIZE);


    // Copy ent_data and add random bytes to the end.
    // This is because if the user data is shorter than 128 bytes,
    // The secret's _length_ is known for everyone reading the file.

    const size_t data_size = ent_data_size + 128;
    char* data = malloc(data_size);
   
    for(int i = 0; i < ent_data_size; i++) {
        data[i] = ent_data[i];
    }

    // Null byte for reading later.
    data[ent_data_size] = 0;

    char padding[128] = { 0 };
    random_chars(padding, 128);

    for(int i = ent_data_size+1; i < ent_data_size+128; i++) {
        data[i] = padding[i - ent_data_size];
    }


    char cipher[128+1] = { 0 };

    char aes_iv[AES_IV_SIZE+1] = { 0 };
    random_chars(aes_iv, AES_IV_SIZE);

    encrypt_aes256(
            key,
            aes_iv,
            data, data_size,
            cipher, 128);


    free(data);
    data = NULL;


    // Convert bytes to hex.

    const char* hex = "0123456789ABCDEF";

    char final_cipher[128*8+1] = { 0 };

    int k = 0;
    int nminus = 0;
    for(int i = 0; i < 128; i++) { 
        if(cipher[k] < 0) {
            final_cipher[i*3+nminus] = '-';
            nminus++;
        }
        final_cipher[i*3+0+nminus] = hex[ (abs(cipher[k]) >> 4) % 0xF ];
        final_cipher[i*3+1+nminus] = hex[ (abs(cipher[k]) & 0xF) ];
        
        if(i+1 < 128) {
            final_cipher[i*3+2+nminus] = ':';
        }
        
        k++;
        //printf("%i, ", cipher[i]);
    }

    //printf("\n%s\n", final_cipher);

    
    
    // Write data to the file.

    struct config_t config;
    config_init(&config);
    if(config_read_file(&config, filename) == CONFIG_FALSE) {
        fprintf(stderr, "Failed to read file \"%s\"\n", filename);
        config_destroy(&config);
        return;
    }

    config_setting_t* config_root = config_root_setting(&config);
    if(!config_root) {
        fprintf(stderr, "Failed to get config root setting.\n");
        config_destroy(&config);
        return;
    }
    
    config_setting_t* group = config_setting_add(config_root, ent_name, CONFIG_TYPE_GROUP);
    if(!group) {
        fprintf(stderr, "Failed to add new group '%s'\n", ent_name);
        config_destroy(&config);
        return;
    }

    add_config_group_data(group, "data", final_cipher);
    add_config_group_data(group, "salt", salt);
    add_config_group_data(group, "iv", aes_iv);

    printf("\033[32mAdded new entry.\033[0m\n");


    config_write_file(&config, filename);
    config_destroy(&config);

}

// str_A is written first to output then str_B.
// IMPORTANT NOTE: obviously null termination is not set by this function.
static void join_string(
        char* output,
        size_t output_max_size,
        char* str_A, size_t str_A_size,
        char* str_B, size_t str_B_size
){

    if((str_A_size + str_B_size) >= output_max_size) {
        fprintf(stderr, "%s: Not enough memory allocated for output buffer.\n",
                __func__);
        return;
    }

    memmove(output,
            str_A,
            str_A_size);

    memmove(output + str_A_size,
            str_B,
            str_B_size); 
}

#define ENTRY_STR_SIZE (USER_INPUT_MAX_SIZE+16)
#define CONVERT_HEXTOBYTE_BUF_SIZE 16

void manager_read_entry(char* ent_name, size_t ent_name_size, const char* filename) {

    struct config_t config;

    const char* data_hex;
    const char* salt;
    const char* aes_iv;



    config_init(&config);
    if(config_read_file(&config, filename) == CONFIG_FALSE) {
        fprintf(stderr, "Failed to read file \"%s\"\n", filename);
        goto early_error;
    }

    char entry_str[ENTRY_STR_SIZE+1] = { 0 };

    // We will need the salt to encrypt the passphrase.
    // that encrypted passphrase is the key for AES-256.
    join_string(entry_str, ENTRY_STR_SIZE, ent_name, ent_name_size, ".salt", 5);
    if(config_lookup_string(&config, entry_str, &salt) == CONFIG_FALSE) {
        fprintf(stderr, "Failed to get entry %s\n", entry_str);
        goto early_error;
    }


    // Get initialization vector for the aes.
    memset(entry_str, 0, ENTRY_STR_SIZE+1);
    join_string(entry_str, ENTRY_STR_SIZE, ent_name, ent_name_size, ".iv", 3);
    if(config_lookup_string(&config, entry_str, &aes_iv) == CONFIG_FALSE) {
        fprintf(stderr, "Failed to get entry %s\n", entry_str);
        goto early_error;
    }

    // Get Encrypted data.
    memset(entry_str, 0, ENTRY_STR_SIZE+1);
    join_string(entry_str, ENTRY_STR_SIZE, ent_name, ent_name_size, ".data", 5);
    if(config_lookup_string(&config, entry_str, &data_hex) == CONFIG_FALSE) {
        fprintf(stderr, "Failed to get entry %s\n", entry_str);
        goto early_error;
    }


    // Convert from hex string back to byte array.

    const size_t hex_data_size = strlen(data_hex);
    if(hex_data_size <= 16) {
        fprintf(stderr, "Invalid data. Corrupted?\n");
        goto early_error;
    }

    char* data = malloc(hex_data_size);
    size_t data_size = 0;
    
    char hex_buf[CONVERT_HEXTOBYTE_BUF_SIZE+1] = { 0 };
    size_t hex_buf_i = 0;

    for(int i = 0; i < hex_data_size; i++) {
        
        char c = data_hex[i];
        if(c == ':') {
            // Next value will start next. write current one now.

            data[data_size] = strtol(hex_buf, NULL, 16);
            data_size++;

            memset(hex_buf, 0, CONVERT_HEXTOBYTE_BUF_SIZE);
            hex_buf_i = 0;
            continue;
        }

        hex_buf[hex_buf_i] = data_hex[i];
        hex_buf_i++;
        if(hex_buf_i >= CONVERT_HEXTOBYTE_BUF_SIZE) {
            fprintf(stderr, "Invalid data. Corrupted?\n");
            goto error_and_free;
        }
    }


    // Get user secret input.

    char user_pass[USER_INPUT_MAX_SIZE] = { 0 };
    ssize_t user_pass_size = 0;
    
    set_input_echo(ECHO_DISABLED);
    if(!get_user_secret_input(user_pass, &user_pass_size)) {
        set_input_echo(ECHO_ENABLED);
        goto error_and_free;
    }
    
    set_input_echo(ECHO_ENABLED);


    char key[KEY_SIZE+1] = { 0 };
    scrypt_user_secret(
            user_pass,   user_pass_size,
            (char*)salt, MAX_SALT_SIZE,
            key, KEY_SIZE);


    char output[USER_INPUT_MAX_SIZE+1] = { 0 };
    int out_size = 0;
    int tmp_size = 0;

    decrypt_aes256(
            key,
            (char*)aes_iv,
            data, data_size,
            output, USER_INPUT_MAX_SIZE
            );

    printf("%s\n", output);



error_and_free:
    free(data);
    data = NULL;

early_error:
    config_destroy(&config);

}



