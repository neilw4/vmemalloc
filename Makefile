CC=gcc
OUT=out
FLAGS= -O3 -Wall -Wextra -std=gnu99
LINK_FLAGS=-lm
LIB_NAME=vmemalloc

all: tests $(LIB_NAME)

$(OUT):
	mkdir -p $(OUT)

tests: tests.o $(LIB_NAME) $(OUT)
	$(CC) $(FLAGS) -o $(OUT)/tests  $(OUT)/tests.o -L$(OUT) -l$(LIB_NAME) $(LINK_FLAGS)

$(LIB_NAME): vmemalloc.o vmemalloc_large.o vmemalloc_small.o logger.o $(OUT)
	ar -cvr $(OUT)/lib$(LIB_NAME).a $(OUT)/vmemalloc.o $(OUT)/vmemalloc_large.o $(OUT)/vmemalloc_small.o $(OUT)/logger.o

%.o: %.c $(OUT)
	$(CC) $(FLAGS) -o $(OUT)/$@ -c $<

clean:
	/bin/rm -f experiment* test_*.txt
	/bin/rm -rf $(OUT)
