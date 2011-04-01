CC      = /usr/bin/gcc
CFLAGS  = -Wall -g -DSINGLEFLOW
LDFLAGS = 

OBJ = ipfixtofile.o thread2.o
BIN = ipfixtofile

ipfixtofile: $(OBJ)
	$(CC) $(CFLAGS) -o ipfixtofile $(OBJ) $(LDFLAGS)

ipfixtofile.o: ipfixtofile.c
	$(CC) $(CFLAGS) -c ipfixtofile.c

thread2.o: ../thread2.c
	$(CC) $(CFLAGS) -c ../thread2.c

clean:
	rm -rf $(BIN) $(OBJ)


