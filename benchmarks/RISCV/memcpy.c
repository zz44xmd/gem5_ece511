#include <stdio.h>
#include <string.h>

#define MEMCPY_SRC "Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!, Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!Hello, RISCV!"

#define MEMCPY(src, dest, len) \
    asm volatile ("memcpy %0, %1, %2" : : "r" (src), "r" (dest), "r" (len));

int main() {
    volatile char src[] = MEMCPY_SRC;
    volatile char dest[sizeof(MEMCPY_SRC)];
    int len = sizeof(MEMCPY_SRC);
    
    memset((void*)dest, 0, sizeof(dest));
    
    char *src_ptr = (char*)src;
    char *dest_ptr = (char*)dest;
    // MEMCPY(dest_ptr, dest_ptr, len);
    MEMCPY(src_ptr, dest_ptr, len);
    
    /******* Some Other calculations *******/
    //!TODO
    //Also the MEMCPY_SRC is TOOOOOOO Small to show the benefit
    /***************************************/


    if (memcmp((const void*)src_ptr, (const void*)dest_ptr, len) != 0) {
        printf("[[FAILED]]\n");
        return -1;
    }
    
    printf("[[PASSED]]\n");
    printf("src: %s\n", src_ptr);
    printf("dest: %s\n", dest_ptr);
    
    return 0;
}
