#include <stdio.h>
#include <string.h>

#define DIM 1025

#define SIZE (64L << 20)
#define ALIGN (4L << 20)

#define MEMCPY(src, dest, len) \
    asm volatile ("memcpy %0, %1, %2" : : "r" (src), "r" (dest), "r" (len));

volatile int a[DIM][DIM];
volatile int b[DIM][DIM];
volatile int c[DIM][DIM];

int main() {
    volatile char buf_src[SIZE + ALIGN];
    volatile char buf_dst[SIZE + ALIGN];

    int size = 4L << 10;

    char *src_p = ((unsigned long)buf_src + (ALIGN - 1)) & (~(ALIGN - 1));
    char *dst_p = ((unsigned long)buf_dst + (ALIGN - 1)) & (~(ALIGN - 1));

    for (unsigned long i = 0; i < size; ++i) {
        src_p[i] = i % 64 + 32;
    }
    for (unsigned i = 0; i < DIM; ++i) {
        for (unsigned j = 0; j < DIM; ++j) {
            a[i][j] = 2 * i + j + 1;
            b[i][j] = i + 2 * j - 1;
        }
    }

    // MEMCPY(dest_ptr, dest_ptr, len);
    MEMCPY(src_ptr, dst_ptr, size);
    /******* Some Other calculations *******/
    for (unsigned k = 0; k < DIM; ++k) {
        for (unsigned i = 0; i < DIM; ++i) {
            for (unsigned j = 0; j < DIM; ++j) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    /***************************************/

    if (memcmp((const void*)src_ptr, (const void*)dst_ptr, size) != 0) {
        printf("[[FAILED]]\n");
        return -1;
    }
    printf("[[PASSED]]\n");
    printf("src: %s\n", src_ptr);
    printf("dst: %s\n", dst_ptr);
    return 0;
}
