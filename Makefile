nacc: nacc.c

test: nacc
		./nacc -test
		./test.sh

clean:
		rm -f nacc *.o *~ tmp*
