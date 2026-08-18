all: main.o cept.o
	gcc -Wall ./*.o -o pageload -lcodebananas 


main.o: main.c
	gcc -Wall -c main.c

cept.o: cept.c
	gcc -Wall -c cept.c

clean:
	rm ./*.o
	rm ./pageload

install:
	cp ./pageload /usr/local/bin
