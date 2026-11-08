//============ protocol overview ============
/*
 * decided                    decide
 *  |  packet_selected_files   |   
 *  |<-------------------------|         
 *  |                          |
 *  |        packet_file       |   
 *  |<-------------------------|         
 *  |<-------------------------|         
 *  |<-------------------------|         
 *  |<-------------------------|         
 *  |                          |
 *  |      packet_result       |
 *  |------------------------->|
 *  |                          |
 *
 *
 */

#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <stdint.h>
#include "ipc.h"

//============ structs and enums ============
struct packet_selected_files {
	uint64_t number_of_files;
};
struct packet_file {
	uint64_t filename_len;
	char filename[];
};
struct packet_result {
	uint64_t status;
};

//============ server functions =============
int new_socket_server() {
	//create socket
	int fd = socket(AF_UNIX,SOCK_STREAM,0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}
	//bind socket path
	struct sockaddr_un address = {0};
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path,108,"/run/user/%d/decide.socket",getuid());
	//unlink an old one
	unlink(address.sun_path);
	//bind our new one
	int result = bind(fd,(struct sockaddr *)&address,sizeof(struct sockaddr_un));
	if (result < 0) {
		perror("bind");
		return -1;
	}
	else return fd;
}

int receive_files(int fd, char ***output, size_t *output_len) {
	//receive initial header
	struct packet_selected_files files_info = {0};
	if (read(fd,&files_info,sizeof(files_info)) < 0) {
		perror("read");
		return -1;
	}
	//receive each file name
	char **filename_array = malloc(sizeof(char *)*files_info.number_of_files+1);
	memset(filename_array,0,sizeof(char *)*files_info.number_of_files+1);
	*output_len = files_info.number_of_files;
	for (uint64_t i = 0; i < files_info.number_of_files; i++) {
		//grab the length
		struct packet_file *file = alloca(sizeof(struct packet_file));
		if (read(fd,file,sizeof(uint64_t)) < 0) {
			perror("read");
			free_files(filename_array);
			return -1;
		}
		//read it for realzies
		size_t file_info_length = file->filename_len;
		file = malloc(file_info_length);
		if (read(fd,file,file_info_length) < 0) {
			free(file);
			free_files(filename_array);
			perror("read");
			return -1;
		}
		filename_array[i] = (char *)file;
	}
	*output = filename_array;
	return 0;
}

void free_files(char **files) {
	for (;*files != NULL; files++) free(*files);
}

int send_response(int fd, uint64_t status) {
	if (write(fd,&status,sizeof(status)) < 0) {
		perror("write");
		return -1;
	}
	return 0;
}

//============ client functions =============
int new_socket_client() {
	//create socket
	int fd = socket(AF_UNIX,SOCK_STREAM,0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}
	//connect socket path
	struct sockaddr_un address = {0};
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path,108,"/run/user/%d/decide.socket",getuid());
	int result = connect(fd,(struct sockaddr *)&address,sizeof(struct sockaddr_un));
	if (result < 0) {
		perror("connect");
		return -1;
	}
	else return fd;
}

int send_files(int fd, const char **filenames, size_t filename_count) {
	//send initial header
	struct packet_selected_files files_info = {
		.number_of_files = filename_count,
	};
	if (write(fd,&files_info,sizeof(files_info)) < 0) {
		perror("write");
		return -1;
	}
	for (size_t i = 0; i < filename_count; i++) {
		size_t filename_info_size = strlen(filenames[i]) + 1 + sizeof(struct packet_file);
		struct packet_file *filename_info = alloca(filename_info_size);
		filename_info->filename_len = strlen(filenames[i])+1;
		strcpy(filename_info->filename,filenames[i]);
		if (write(fd,filename_info,filename_info_size) < 0) {
			perror("write");
			return -1;
		}
	}
	return 0;
}

int receive_response(int fd) {
	uint64_t status;
	if (read(fd,&status,sizeof(status)) < 0) {
		perror("read");
		return -1;
	}
	return status;
}
