
CFLAGS = -O3 
count_pairs.ex: count_pairs.c Makefile
	mpicc $(CFLAGS) -o count_pairs.ex count_pairs.c

clean:
	rm -f count_pairs.ex
