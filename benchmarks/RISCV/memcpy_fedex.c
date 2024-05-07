#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#define DIM 8
#define SHIFT 2
#define SIZE (64L << SHIFT)
#define ALIGN (4L << SHIFT)
#define ITERATIONS 1
#define MEMCPY(src, dest, len) \
    asm volatile ("memcpy %0, %1, %2" : : "r" (src), "r" (dest), "r" (len));

volatile int a[DIM][DIM] __attribute__((aligned(64)));
volatile int b[DIM][DIM] __attribute__((aligned(64)));
volatile int c[DIM][DIM] __attribute__((aligned(64)));

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
    volatile char buf_src[SIZE] __attribute__((aligned(64)));
    volatile char buf_dst[SIZE] __attribute__((aligned(64)));
    int size = SIZE;
    int dim = DIM;
    double sec;

    char *src_p = (char*) buf_src;
    char *dst_p = (char*) buf_dst;

    for (unsigned long i = 0; i < size; ++i) {
        src_p[i] = i % 64 + 32;
    }

    for (unsigned i = 0; i < DIM; ++i) {
        for (unsigned j = 0; j < DIM; ++j) {
            a[i][j] = 2 * i + j + 1;
            b[i][j] = i + 2 * j - 1;
        }
    }

    // clock_t start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        MEMCPY(src_p, dst_p, size);
        // matrix_multiply(dim);
    }
    for (volatile int i = 0; i< 10000;i++);
    // sec = (clock() - start) / (double)CLOCKS_PER_SEC;
    // printf("%f \n", sec*1000000000.0 / ITERATIONS);
    if (memcmp((const void*)src_p, (const void*)dst_p, size) != 0) {
        printf("[[FAILED]]\n");
    }
    printf("[[PASSED]]\n");
    // printf("src: %s\n", src_p);
    // printf("dst: %s\n", dst_p);
    return 0;
}
