all: main.o cept.o
	gcc -Wall ./*.o -o btxasm -lcodebananas 


main.o: main.c
	gcc -Wall -c main.c

cept.o: cept.c
	gcc -Wall -c cept.c

clean:
	rm ./*.o
	rm ./btxasm

install:
	cp ./btxasm /usr/local/bin
