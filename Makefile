CC = g++
CFLAGS = -I./ -std=c++11 -Wall -Wextra -Wpedantic -Weffc++

%.o: %.cc
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: run_test clean

run_test: tests/catch_test
	@tests/catch_test

tests/catch_test: tests/catch.o tests/test.cc
	$(CC) $(CFLAGS) $^ -o tests/catch_test

clean:
	-@rm -v tests/catch.o
	-@rm -v tests/catch_test
