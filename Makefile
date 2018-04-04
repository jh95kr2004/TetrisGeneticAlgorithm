CC = gcc
TARGET = main.out

$(TARGET): main.o
	$(CC) -Wall -o $(TARGET) -lncurses main.o

main.o: main.cpp main.h
	$(CC) -Wall -c main.cpp

clean:
	rm *.o $(TARGET)
