CFLAGS=-std=gnu99 -Werror
OBJECTS=Sender.o Reliable.o ReliableImpl.o Util.o Queue.o Congestion.o
DEPS=Reliable.h ReliableImpl.h Util.h Queue.h Congestion.h

Sender: $(OBJECTS)
	gcc $(OBJECTS) -o Sender -lpthread $(CFLAGS)

Sender.o: Sender.c $(DEPS)
	gcc Sender.c -c $(CFLAGS)

Reliable.o: Reliable.c $(DEPS)
	gcc Reliable.c -c $(CFLAGS)

ReliableImpl.o: ReliableImpl.c $(DEPS)
	gcc ReliableImpl.c -c $(CFLAGS)

Congestion.o: Congestion.c $(DEPS)
	gcc Congestion.c -c $(CFLAGS)

Util.o: Util.c Util.h Queue.h
	gcc Util.c -c $(CFLAGS)

Queue.o: Queue.c Queue.h
	gcc Queue.c -c $(CFLAGS)

clean:
	rm *.o
	rm Sender
