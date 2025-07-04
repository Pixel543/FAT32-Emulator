
CC = gcc


CFLAGS = -g -Wall


TARGET = f32disk


SRCS = fat32_emulator.c


all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)


clean:
	rm -f $(TARGET)


run: all
	./$(TARGET) test.disk

.PHONY: all clean run