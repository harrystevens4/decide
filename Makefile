CFLAGS=-Wall -Wextra -g
LIBS=-fsanitize=address
decided : src/decided.c src/ipc.o
	$(CC) -o $@\
		$(CFLAGS)\
		$$(pkg-config --cflags glib-2.0 gio-2.0)\
		$^\
		$$(pkg-config --libs glib-2.0 gio-2.0)\
		$(LIBS)

decide : src/decide.o src/ipc.o
	$(CC) -o $@ $^


