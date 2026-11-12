#include <stdlib.h>
#include "ipc.h"
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

int main(int argc, const char **argv) {
	int socket = new_socket_client();
	if (socket < 0) return EXIT_FAILURE;
	//get absolute filepaths
	char *absolute_paths[argc-1]; //lowk if you stack overflow here its on you
	for (int i = 0; i < argc-1; i++) {
		absolute_paths[i] = alloca(PATH_MAX);
		realpath(argv[i+1],absolute_paths[i]);
	}
	//send them off
	if (send_files(socket,(const char **)absolute_paths,argc-1) < 0) {
		close(socket);
		return EXIT_FAILURE;
	}
	//receive response
	int response = receive_response(socket);
	//cleanup
	close(socket);
	//print status
	switch (response) {
	case -1:
		return EXIT_FAILURE;
	case STATUS_OK:
		printf("Ok\n");
		return EXIT_SUCCESS;
	case STATUS_TOO_MANY_FILES:
		fprintf(stderr,"Too many files provided\n");
		return EXIT_FAILURE;
	default:
		return EXIT_FAILURE;
	}
}
