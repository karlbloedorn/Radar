default: processor

processor: processor.c
	gcc -Wall -ggdb --std=gnu99 $< -lbz2 -lm -lz -o $@

clean:
	-rm -f processor
