CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
TARGET = park
SRC = park.c queues.c monitor.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
