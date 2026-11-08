#ifndef _IPC_H
#define _IPC_H

#include <stdint.h>

enum result_status {
	STATUS_OK,
	STATUS_TOO_MANY_FILES,
	STATUS_NO_FILES_PROVIDED,
};

int new_socket_server();
int new_socket_client();
void free_files(char **files);
int receive_files(int fd, char ***output, size_t *output_len);
int send_files(int fd, const char **filenames, size_t filename_count);
int send_response(int fd, uint64_t status);
int receive_response(int fd);

#endif
