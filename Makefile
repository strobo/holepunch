CC=clang

all: holepunch

holepunch: holepunch.o hppacket.o nodepool.o hpserver.o hpnode.o hputils.o
	$(CC) -g -Wall -L/opt/local/lib -levent -o $@ $^

.c.o:
	$(CC) -g -Wall -c $<

clean:
	@rm -rf *.o holepunch

.PHONY : clean
