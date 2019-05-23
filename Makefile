nacc: nacc.c

test: nacc
		./test.sh

clean:
		rm -f nacc *.o *~ tmp*
