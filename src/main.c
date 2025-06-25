#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "manager.h"



#define ACTION_SET_ENT 0
#define ACTION_GET_ENT 1


void print_usage(char* arg0) {
    printf(
            "%s set <entry> <file> : Write new encrypted entry to a file.\n"
            "%s get <entry> <file> : Read and decrypt existing entry.\n"
            "\n"
            "Optional flags:\n"
            "-c  :  Copy \"get\" action result to clipboard instead of printing it to stdout.\n"
            "\n"
            "\"set\" action will create a new file with permissions 600 if it doesnt exist.\n"
            ,
            arg0, arg0
            );
}



int main(int argc, char** argv) {

    if(argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    char* action_str = argv[1];
    char* entry_name = argv[2];
    char* filename = argv[3];

    int action = -1;

    const size_t entry_name_size = strlen(entry_name);
    const size_t action_str_size = strlen(action_str);

    if(action_str_size != 3) {
        fprintf(stderr, "\033[31mInvalid action \"%s\"\033[0m\n\n", action_str);
        print_usage(argv[0]);
        return 1;
    }

    if(strcmp(action_str, "get") == 0) {
        action = ACTION_GET_ENT;
    }
    else
    if(strcmp(action_str, "set") == 0) {
        action = ACTION_SET_ENT;
    }
    else {
        fprintf(stderr, "\033[31mInvalid action \"%s\"\033[0m\n\n", action_str);
        print_usage(argv[0]);
        return 1;
    }


    int file_exists = (access(filename, F_OK) == 0);

    if((action == ACTION_GET_ENT) && !file_exists) {
        fprintf(stderr, "\033[31mAction 'get' must have a valid file as input.\n"
                        "File \"%s\" not found.\033[0m\n\n", filename);
        print_usage(argv[0]);
        return 1;
    }
    else
    if((action == ACTION_SET_ENT) && !file_exists) {
        // Ask to create a new file.

        int valid_input = 0;

        printf("\"%s\" Doesnt exist. Create new file? [y/n]: ", filename);
        fflush(0);
        char read_buf[16+1] = { 0 };
        int read_size = read(1, read_buf, 16);

        int create_new = 0;

        if(read_size > 1) {
            if((read_buf[0] == 'y') || (read_buf[0] == 'Y')) {
                create_new = 1;
            }
        }

        if(!create_new) {
            printf("File not created.\n");
            return 0;
        }

        // Only user can read and write to the file.
        mode_t modes = S_IRUSR | S_IWUSR;
        creat(filename, modes);

    }


    // Confirm that it is a regular file.
    struct stat sb;
    if(stat(filename, &sb) != 0) {
        fprintf(stderr, "Could not get information about \"%s\"\n"
                "%s\n",
                filename, strerror(errno));
        return 1;
    }

    if((sb.st_mode & S_IFMT) != S_IFREG) {
        fprintf(stderr, "\"%s\" Is not a regular file.\n",
                filename);
        return 1;
    }

    // Confirm file permissions are okay.

    if((sb.st_mode & S_IXUSR) /* User execute permission */
    || (sb.st_mode & S_IRGRP) /* Group read permission */
    || (sb.st_mode & S_IWGRP) /* Group write permission */
    || (sb.st_mode & S_IXGRP) /* Group execute permission */
    || (sb.st_mode & S_IROTH) /* Others read permission */
    || (sb.st_mode & S_IWOTH) /* Others write permission */
    || (sb.st_mode & S_IXOTH) /* Others execute permission */
    ){
        fprintf(stderr, "\033[31m\"%s\" Has invalid permissions."
                        "\nOnly user write and read should be allowed (600)\033[0m\n\n",
                        filename);

        print_usage(argv[0]);

        return 1;
    }



    if(!init_manager()) {
        return 1;
    }

    if(action == ACTION_SET_ENT) {
        manager_write_entry(entry_name, entry_name_size, filename);
    }
    else
    if(action == ACTION_GET_ENT) {
        manager_read_entry(entry_name, entry_name_size, filename);
    }

    return 0;
}


