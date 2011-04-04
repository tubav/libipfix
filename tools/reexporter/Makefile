CC      = /usr/bin/gcc
CFLAGS  = -Wall -g \
      -I../../ \
	  -I../../lib \
	  -I../../libmisc
LDFLAGS = -L../../lib -lipfix \
	  -L../../libmisc -lmisc \
	  -lpthread

OBJ = reexporter.o utilities.o thread2.o
BIN = reexporter

reexporter: $(OBJ)
	$(CC) $(CFLAGS) -o reexporter $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(BIN) $(OBJ)
