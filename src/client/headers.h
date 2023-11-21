#ifndef __CLIENT_HEADERS_H
#define __CLIENT_HEADERS_H

i8 get_operation();
bool path_error(const char *path);
void read_path(char *path_buffer);
void print_error(enum status code);
void print_mode(mode_t mode);
void fill_rd_path(const i32 i, const char *path, char *buf);
void delete_rd_paths(const i32 nm_sockfd, enum operation op, const char *path);
void print_metadata(metadata meta);

#endif
