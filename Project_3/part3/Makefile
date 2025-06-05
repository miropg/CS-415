CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -g
TARGET  = park

SRCS    = park.c queues.c
OBJS    = park.o queues.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lpthread

%.o: %.c queues.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
