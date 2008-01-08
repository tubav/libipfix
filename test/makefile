CC = cc
CFLAGS = -g -Wall -I../lib \
         -DDBSUPPORT -DSCTPSUPPORT  # -I/usr/local/include
LDFLAGS = -L../lib -lipfix -lmisc \
          -lsctp -lmysqlclient # -lsocket -lnsl -lgnugetopt
TARGETS = collector exporter exporter_sb exporter_t1

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

collector: collector.c makefile
	$(CC) $(CFLAGS) -DDBSUPPORT -o collector collector.c $(LDFLAGS)

exporter: exporter.c makefile
	$(CC) $(CFLAGS) -o exporter exporter.c  $(LDFLAGS)

exporter_sb: exporter_sb.c makefile
	$(CC) $(CFLAGS) -o exporter_sb exporter_sb.c  $(LDFLAGS)

exporter_t1: exporter_t1.c makefile
	$(CC) $(CFLAGS) -o exporter_t1 exporter_t1.c  $(LDFLAGS)

buggy_exporter: buggy_exporter.c makefile
	$(CC) $(CFLAGS) -o buggy_exporter buggy_exporter.c  $(LDFLAGS)

