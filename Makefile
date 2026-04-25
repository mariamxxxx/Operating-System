CC      = gcc
CFLAGS  = -g -Wall -Wextra \
          -Isrc \
          -Isrc/os \
          -Isrc/process \
          -Isrc/memory \
          -Isrc/interpreter \
          -Isrc/scheduler \
          -Isrc/synchronization
LDFLAGS = -lm

SRCS = src/main.c \
       src/process/process.c \
       src/os/syscalls.c \
       src/os/os_core.c \
       src/synchronization/mutex.c \
       src/memory/memory.c \
       src/scheduler/queue.c \
       src/scheduler/mlfq.c \
       src/scheduler/hrrn.c \
       src/scheduler/scheduler.c \
       src/scheduler/rr.c \
       src/interpreter/parser.c \
       src/interpreter/interpreter.c

OBJS    = $(SRCS:src/%.c=build/%.o)
TARGET  = build/os_sim

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf build
