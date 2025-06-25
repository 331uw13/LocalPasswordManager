#ifndef PASSWORD_MANAGER_H
#define PASSWORD_MANAGER_H




int init_manager();

#define ECHO_DISABLED 0
#define ECHO_ENABLED 1
void set_input_echo(int option);

void manager_write_entry (char* ent_name, size_t ent_name_size, const char* filename);
void manager_read_entry  (char* ent_name, size_t ent_name_size, const char* filename);

#endif
