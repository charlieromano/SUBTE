rm server
gcc -pthread server.c rs232.c SerialManager.c -o server
chmod a+x server
