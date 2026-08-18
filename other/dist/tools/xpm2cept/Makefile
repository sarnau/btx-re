# (c) Philipp Fabain Benedikt Maier

BINARYNAME = xpm2cept
LINKERFLAGS = 

all: main.o
	gcc ./*.o -o $(BINARYNAME) $(LINKERFLAGS)

clean:
	rm ./*.o
	rm ./$(BINARYNAME)

install:
	cp ./$(BINARYNAME) /usr/local/bin
