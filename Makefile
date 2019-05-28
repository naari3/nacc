CFLAGS=-Wall -std=c11
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

nacc: $(OBJS)
		$(CC) -o nacc $(OBJS) $(LDFLAGS)

$(OBJS): nacc.h


test: nacc
		./nacc -test
		./test.sh

clean:
		rm -f nacc *.o *~ tmp*
