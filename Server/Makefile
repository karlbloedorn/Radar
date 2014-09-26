default: processor

processor: processor.c
	gcc -Wall -O3 --std=gnu99 $< -lbz2 -lm -o $@

clean:
	-rm -f processor
