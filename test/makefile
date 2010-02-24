CC = cc
CFLAGS = -g -Wall -I.. -I../lib -I../libmisc\
         -DDBSUPPORT -DSCTPSUPPORT  # -I/usr/local/include
LDFLAGS = -L../lib -L../libmisc -lipfix -lmisc \
          -lsctp -lmysqlclient # -lsocket -lnsl -lgnugetopt
TARGETS = exporter exporter_sb ipfixtee

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

collector: collector.c makefile
	$(CC) $(CFLAGS) -DDBSUPPORT -o collector collector.c $(LDFLAGS)

exporter: exporter.c makefile
	$(CC) $(CFLAGS) -o exporter exporter.c  $(LDFLAGS)

ipfixtee: ipfixtee.c makefile
	$(CC) $(CFLAGS) -o ipfixtee ipfixtee.c  $(LDFLAGS) -lhashish

exporter_sb: exporter_sb.c makefile
	$(CC) $(CFLAGS) -o exporter_sb exporter_sb.c  $(LDFLAGS)

exporter_t1: exporter_t1.c makefile
	$(CC) $(CFLAGS) -o exporter_t1 exporter_t1.c  $(LDFLAGS)

buggy_exporter: buggy_exporter.c makefile
	$(CC) $(CFLAGS) -o buggy_exporter buggy_exporter.c  $(LDFLAGS)

