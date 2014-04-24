OBJS=../base64.o base64_test.o
CFLAGS=-I .. -std=c99
CC=gcc

base64_test: $(OBJS)
	$(CC) $(CFLAGS) -o base64_test $^

clean:
	rm -f $(OBJS) base64_test