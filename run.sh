make parser
gcc -Wall -std=c11 -g -I/usr/include/libxml2 -c src/main.c -o bin/main.o
gcc -Wall -std=c11 -g -I/usr/include/libxml2 -Iinclude/ -L. -Lbin/ -o exe bin/main.o -lgpxparser -lxml2
export LD_LIBRARY_PATH=:./bin
./exe simple.gpx
rm bin/main.o exe
make clean
