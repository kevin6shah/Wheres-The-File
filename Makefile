all:
	gcc -c data.c -lcrypto
	gcc WTF.c data.c -o WTF -pthread -lcrypto
	gcc WTFserver.c data.c -o WTFserver -pthread -lcrypto

clean:
	rm WTF WTFserver data.o

clean_all:
	rm WTF WTFserver data.o .*
