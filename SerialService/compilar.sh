rm serialService
gcc -pthread main.c rs232.c SerialManager.c -o serialService
chmod a+x serialService
