CC = gcc

CFLAGS = -O2

TARGET = tmdout

SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

clean:
	rm -f $(TARGET)
