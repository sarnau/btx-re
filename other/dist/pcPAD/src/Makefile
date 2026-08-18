


all: main.o pad.o cept.o termctrl.o btx.o http.o
	gcc ./*.o -o pcpad -lcodebananas

main.o: main.c
	gcc -Wall -c main.c

pad.o: pad.c
	gcc -Wall -c pad.c

cept.o: cept.c
	gcc -Wall -c cept.c

btx.o: btx.c
	gcc -Wall -c btx.c

http.o: http.c
	gcc -Wall -c http.c



termctrl.o: termctrl.c
	gcc -Wall -c termctrl.c


clean:
	echo "removing files..."
	- rm ./*.o
	- rm ./*~
	- rm ./temp.btx

install:
	cp ./pcpad /usr/local/bin

