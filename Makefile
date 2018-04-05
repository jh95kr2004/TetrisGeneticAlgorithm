CC = g++-7
TARGET = a.out

$(TARGET): main.cpp main.h
	$(CC) -Wall -lncurses -fopenmp -o $(TARGET) main.cpp

clean:
	rm *.o $(TARGET) output.txt
