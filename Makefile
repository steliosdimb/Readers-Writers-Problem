CC = gcc
CFLAGS = -Wall -Werror -pthread
LDFLAGS = -lrt

all: init_shm reader writer test delete_print

init_shm: init_shm.c shm.h account.h
	$(CC) $(CFLAGS) -o init_shm init_shm.c $(LDFLAGS)

reader: reader.c reader.h account.h
	$(CC) $(CFLAGS) -o reader reader.c $(LDFLAGS)

writer: writer.c writer.h account.h
	$(CC) $(CFLAGS) -o writer writer.c $(LDFLAGS)

test: test.c test.h
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS)

delete_print: delete_print.c
	$(CC) $(CFLAGS) -o delete_print delete_print.c $(LDFLAGS)

clean:
	rm -f init_shm reader writer test delete_print

