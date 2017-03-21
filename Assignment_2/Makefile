all: clean create

create:
	gcc -Wall rdps.c helper.c -o rdps
	gcc -Wall rdpr.c helper.c -o rdpr 
clean:
	rm -f rdps rdpr received.dat received.txt

recieve:
	./rdpr 10.10.1.100 8001 received.txt

send:
	./rdps 192.168.1.100 8001 10.10.1.100 8001 file.txt
