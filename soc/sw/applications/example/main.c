#include "uart.h"
#include "print.h"


int main() {
    uart_init();
    printf("Hello World!\n");
    uart_write_flush();
    return 1;
}
