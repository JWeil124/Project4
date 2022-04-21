CC=g++
CFLAGS=-O1 -g -Wall -Wextra -std=c++11

TARGETS = sender receiver

RDT_LIB_OBJS = ReliableSocket.o rdt_time.o

all: $(TARGETS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $^

sender: sender.cpp $(RDT_LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

receiver: receiver.cpp $(RDT_LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS) $(RDT_LIB_OBJS)
