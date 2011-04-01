CC      = /usr/bin/gcc
CFLAGS  = -Wall -g \
	  -I../../trunk \
	  -I../../trunk/lib \
	  -I../../trunk/libmisc
LDFLAGS = -L../../trunk/lib -lipfix \
	  -L../../trunk/libmisc -lmisc \
	  -lpthread

OBJ = reexporter.o utilities.o thread2.o
BIN = reexporter

reexporter: $(OBJ)
	$(CC) $(CFLAGS) -o reexporter $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(BIN) $(OBJ)
