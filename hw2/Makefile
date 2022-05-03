CC = g++

all: logger.so logger

logger.so: logger.cpp
	$(CC) -o logger.so -shared -fPIC logger.cpp -ldl

logger: logger.so hw2.cpp
	$(CC) hw2.cpp -o logger

clean:
	rm logger.so logger