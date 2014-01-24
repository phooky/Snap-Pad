OBJS=../ecc.o ecc_test.o
CFLAGS=-I .. -std=c99
CC=gcc

ecc_test: $(OBJS)
	$(CC) $(CFLAGS) -o ecc_test $^

clean:
	rm -f $(OBJS) ecc_test