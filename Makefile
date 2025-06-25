FLAGS = -O2 -Wall -Wextra
CC = gcc

TARGET_NAME = lpwmanager


SRC  = $(shell find ./src -type f -name *.c)
OBJS = $(SRC:.c=.o)

all: $(TARGET_NAME)


%.o: %.cpp
	@echo "compile"
	$(CC) $(FLAGS) -c $< -o $@ 

$(TARGET_NAME): $(OBJS)
	@echo "link"
	$(CC) $(OBJS) -o $@ $(FLAGS) -lssl -lcrypto -lconfig

clean:
	rm $(OBJS) $(TARGET_NAME)

.PHONY: all clean

