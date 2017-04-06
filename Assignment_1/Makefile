all: clean create

test: clean create run

create:
	gcc -Wall sws.c -o sws
run:
	./sws 8000 www
clean:
	rm -f sws
