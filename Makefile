CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude

SRC = src
TEST = test
BUILD = build

OBJS = $(BUILD)/vedalloc.o $(BUILD)/vedalloc_v2.o

all: test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/vedalloc.o: $(SRC)/vedalloc.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/vedalloc_v2.o: $(SRC)/vedalloc_v2.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

bench: $(TEST)/benchmark.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BUILD)/bench

runbench: bench
	./$(BUILD)/bench

test: $(TEST)/main.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(BUILD)/test

run: test
	./$(BUILD)/test

clean:
	rm -rf $(BUILD)