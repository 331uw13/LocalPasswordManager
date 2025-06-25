#include <stddef.h>

#include "utils.h"


static int g_seed = 0;


void set_random_gen_seed(int seed) {
    g_seed = seed;
}

int random_gen() {
    g_seed = 0x343FD * g_seed + 0x269EC3;
    return (g_seed >> 16) & 0x7FFF;
}

int random_int(int min, int max) {
    return random_gen() % (max - min) + min;
}

void random_chars(char* output, size_t max_size) {
    for(size_t i = 0; i < max_size; i++) {
        output[i] = random_int(0x23, 0x7E);
    }
}
