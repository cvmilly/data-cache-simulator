CC=g++
CFLAGS = -std=c++11 -g -Wall -Werror
DEBUG=-O0 -g
TARGET = data

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) $(DEBUG) -o $(TARGET).exe $(TARGET).cpp

clean:
	$(RM) $(TARGET).exe

