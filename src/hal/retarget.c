#include "stm32f10x.h"


int _write(int fd, char *ptr, int len) {
    int i;

    for (i = 0; i < len; i++) {
        ITM_SendChar(ptr[i]);
    }

    return len;
}
