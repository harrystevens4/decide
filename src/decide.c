#include <stdlib.h>
#include "ipc.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, const char **argv) {
	int socket = new_socket_client();
	if (socket < 0) return EXIT_FAILURE;
	if (send_files(socket,argv+1,argc-1) < 0) {
		close(socket);
		return EXIT_FAILURE;
	}
	int response = receive_response(socket);
	close(socket);
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
