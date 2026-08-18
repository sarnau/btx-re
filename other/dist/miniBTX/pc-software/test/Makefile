all: cept.o main.o
	gcc -Werror ./*.o -o miniBTX -lcodebananas 

main.o: main.c
	gcc -Werror -c main.c

cept.o: cept.c
	gcc -Werror -c cept.c


clean:
	rm ./*.o
	rm ./miniBTX