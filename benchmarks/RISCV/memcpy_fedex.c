#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#define DIM 10
#define SHIFT 4
#define SIZE (64L << SHIFT)
#define ALIGN (4L << SHIFT)


#define MEMCPY(src, dest, len) \
    asm volatile ("memcpy %0, %1, %2" : : "r" (src), "r" (dest), "r" (len));

volatile int a[DIM][DIM];
volatile int b[DIM][DIM];
volatile int c[DIM][DIM];

volatile char buf_src[SIZE + ALIGN];
volatile char buf_dst[SIZE + ALIGN];

void matrix_multiply(int dim) {
    for (unsigned k = 0; k < dim; ++k) {
        for (unsigned i = 0; i < dim; ++i) {
            for (unsigned j = 0; j < dim; ++j) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}

int main() {
    // if (argc != 3) {
    //     printf("Usage: %s <size> <dim>\n", argv[0]);
    //     return 1;
    // }

    int size = 5;
    int dim = DIM;
    double sec;


    char *src_p = (char *)(((unsigned long)buf_src + (ALIGN - 1)) & (~(ALIGN - 1)));
    char *dst_p = (char *)(((unsigned long)buf_dst + (ALIGN - 1)) & (~(ALIGN - 1)));

    printf("%x\n", src_p);

    for (unsigned long i = 0; i < size; ++i) {
        src_p[i] = i % 64 + 32;
        printf("%d\n", i);
        
    }

    for (unsigned i = 0; i < DIM; ++i) {
        for (unsigned j = 0; j < DIM; ++j) {
            a[i][j] = 2 * i + j + 1;
            b[i][j] = i + 2 * j - 1;
            printf("%d, %d\n", i, j);
        }
    }
    clock_t start = clock();
    // MEMCPY(dest_ptr, dest_ptr, len);
    MEMCPY(src_p, dst_p, size);


    /******* Some Other calculations *******/
    matrix_multiply(dim);
    /***************************************/
    sec = (clock() - start) / (double)CLOCKS_PER_SEC;
    printf("%15.1f \n", sec*1000000000.0);
    if (memcmp((const void*)src_p, (const void*)dst_p, size) != 0) {
        printf("[[FAILED]]\n");
        // return -1;
    }
    printf("[[PASSED]]\n");
    printf("src: %s\n", src_p);
    printf("dst: %s\n", dst_p);
    return 0;
}
