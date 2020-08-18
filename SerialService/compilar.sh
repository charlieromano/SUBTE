rm serialService
gcc -pthread server.c rs232.c SerialManager.c -o serialService
chmod a+x serialService
