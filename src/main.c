#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lsxd.h"
#include "config.h"



void main_loop(struct lsxd_t* st) {
   
    while(st->running) {

        const size_t max_i = st->scroll + st->num_rows;
        int line_count = 0;

        // TODO: Make configurable top bar

        printf("\e[2K\e[90m%c %s\e[0m\n", (st->scroll > 0) ? '^' : '-', st->cwd);
        line_count++;

        for(size_t i = st->scroll; i < max_i; i++) {
            if(i >= st->buf.num_files) {
                break;
            }
            int is_selected = (i == (size_t)st->cursori);
            print_file_info(st, &st->buf.files[i], is_selected);
            line_count++;
        }

        printf("\e[2K%s\n", (max_i == st->buf.num_files) ? st->cfg.bottom_line : "\e[2K");
        line_count++;


        if(st->last_line_count > line_count) {
            // Current line count is smaller.
            // Clear all lines that are not needed anymore.
            int rem = st->last_line_count - line_count;
            for(int i = 0; i < rem; i++) {
                printf("\e[2K\n");
            }
            line_count += rem;
        }


        st->last_line_count = line_count;
        handle_input(st);
        mv_cursor_up(line_count);
    }
}


// Escape sequence start '\e' has to be replaced with
// 0x1B(escape) because config reader reads it as literal '\e'
void replace_esc_byte(char* array, size_t size) {

    for(size_t i = 0; i < size; i++) {
        if(i+2 >= size) {
            break;
        }

        if(array[i] == '\\' && array[i+1] == 'e') {
            memmove(
                    &array[i],
                    &array[i+1],
                    size - i
                    );

            array[i] = 0x1B;
            // last byte of array should be already zero.
        }
    }
}

#define CFGBUF_SIZE 256
int parse_config(struct lsxd_t* st) {
    int result = 0; 
    struct config_file_t cfgfile;
    if(!open_config_file(&cfgfile, "./config")) {
        fprintf(stderr, "'%s': Config file failed to open. return 1\n",
                __func__);
        goto error;
    }

    char buf[CFGBUF_SIZE];

    if(read_cfgvar(&cfgfile, "regfile_color", buf, CFGBUF_SIZE)) {
        st->cfg.regfile_color = atoi(buf);
    }
    if(read_cfgvar(&cfgfile, "directory_color", buf, CFGBUF_SIZE)) {
        st->cfg.directory_color = atoi(buf);
    }
    if(read_cfgvar(&cfgfile, "symlink_color", buf, CFGBUF_SIZE)) {
        st->cfg.symlink_color = atoi(buf);
    }
    if(read_cfgvar(&cfgfile, "blockdev_color", buf, CFGBUF_SIZE)) {
        st->cfg.blockdev_color = atoi(buf);
    }
    if(read_cfgvar(&cfgfile, "chardev_color", buf, CFGBUF_SIZE)) {
        st->cfg.chardev_color = atoi(buf);
    }
    if(read_cfgvar(&cfgfile, "socket_color", buf, CFGBUF_SIZE)) {
        st->cfg.socket_color = atoi(buf);
    }
    if(read_cfgvar(&cfgfile, "fifo_color", buf, CFGBUF_SIZE)) {
        st->cfg.fifo_color = atoi(buf);
    }

    memset(st->cfg.selected_prefix,    0, CFGSTRV_SIZE+1);
    memset(st->cfg.un_selected_prefix, 0, CFGSTRV_SIZE+1);
    memset(st->cfg.selected_suffix,    0, CFGSTRV_SIZE+1);
    memset(st->cfg.un_selected_suffix, 0, CFGSTRV_SIZE+1);
    memset(st->cfg.bottom_line,        0, CFGSTRV_SIZE+1);

    read_cfgvar(&cfgfile, "selected_prefix", st->cfg.selected_prefix, CFGSTRV_SIZE);
    read_cfgvar(&cfgfile, "un_selected_prefix", st->cfg.un_selected_prefix, CFGSTRV_SIZE);
    
    read_cfgvar(&cfgfile, "selected_suffix", st->cfg.selected_suffix, CFGSTRV_SIZE);
    read_cfgvar(&cfgfile, "un_selected_suffix", st->cfg.un_selected_suffix, CFGSTRV_SIZE);
    
    read_cfgvar(&cfgfile, "bottom_line", st->cfg.bottom_line, CFGSTRV_SIZE);
   

    replace_esc_byte(st->cfg.selected_prefix, CFGSTRV_SIZE);
    replace_esc_byte(st->cfg.un_selected_prefix, CFGSTRV_SIZE);
    replace_esc_byte(st->cfg.selected_suffix, CFGSTRV_SIZE);
    replace_esc_byte(st->cfg.un_selected_suffix, CFGSTRV_SIZE);
    replace_esc_byte(st->cfg.bottom_line, CFGSTRV_SIZE);

    close_config_file(&cfgfile);

    result = 1;
error:
    return result;
}

int main(int argc, char** argv) {
    struct lsxd_t st;


    if(!parse_config(&st)) {
        return 1;
    }

    if(!init_lsxd(&st, DEFAULT_MAX_ROWS)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }

    main_loop(&st);
    quit_lsxd(&st);
    mv_cursor_down(st.last_line_count);
    return 0;
}



