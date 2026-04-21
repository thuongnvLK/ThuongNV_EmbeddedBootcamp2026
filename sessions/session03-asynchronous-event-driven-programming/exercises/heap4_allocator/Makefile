# ================= CONFIG =================
CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = app

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(OBJS:.o=.d)

# ================= BUILD =================
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c 
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

# ================= CLEAN =================
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

# ================= CLEAN =================
run: $(TARGET)
	./$(TARGET)