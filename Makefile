CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = src
BUILD = build

OBJS = $(BUILD)/vedalloc.o

all: test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/vedalloc.o: $(SRC)/vedalloc.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: src/benchmark.c src/vedalloc.c
	$(CC) $(CFLAGS) $^ -o build/bench

runbench:
	./build/bench

test: $(SRC)/main.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BUILD)/test

run:
	./build/test

clean:
	rm -rf $(BUILD)