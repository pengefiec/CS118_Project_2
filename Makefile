CC=gcc
CFLAGS=-I.
DEPS = # header file 
OBJ = my-router.o

%.o: %.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

my-router: $(OBJ)
	$(CC) -g -o $@ $^ $(CFLAGS)
