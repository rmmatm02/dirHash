CC = gcc
CFLAGS = -O3 -march=native -flto -fomit-frame-pointer -fopenmp -Wall
LDFLAGS = -lxxhash -luring
SRC = main.c bloom_filter.c file_list.c directory_traversal.c hashing.c progress.c
OBJS = $(SRC:.c=.o)
TARGET = dirHash

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
