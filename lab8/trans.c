/* 
 * 5130379013 Zhu Chaojie
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */


char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32) {
        int ii, jj, i, j, tmp;
        for (ii = 0; ii < N; ii += 8)
            for (jj = 0; jj < M; jj += 8)
                for (i = ii; i < ii + 8 && i < N; i++)
                    for (j = jj; j < jj + 8 && j < M; j++) {
                        if (ii == jj) {
                            tmp = A[i][j];
                            B[M - 1 - j][N - 1 - i] = tmp;
                        }
                        else {
                            tmp = A[i][j];
                            B[j][i] = tmp;
                        }
                    }
        for (ii = 0; ii < N / 2; ii += 8)
            for (i = ii; i < ii + 8 && i < M; i++)
                for (j = ii; j < ii + 8 && j < N; j++) {
                    tmp = B[i][j];
                    B[i][j] = B[M - 1 - i][N - 1 - j];
                    B[M - 1 - i][N - 1 - j] = tmp;
                }
    }
    else if (M == 64) {
        int i, j;
        for (i = 0; i < 64; i += 8)
            for (j = 0; j < 64; j += 8) {
                int ii, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

                tmp0 = A[j][i];
                tmp1 = A[j][i + 1];
                tmp2 = A[j][i + 2];
                tmp3 = A[j][i + 3];

                B[i][j] = tmp0;
                B[i][j + 1] = A[j + 1][i];
                B[i][j + 2] = A[j + 2][i];
                B[i][j + 3] = A[j + 3][i];

                tmp4 = A[j + 1][i + 1];
                tmp5 = A[j + 1][i + 2];
                tmp6 = A[j + 1][i + 3];

                B[i + 1][j] = tmp1;
                B[i + 1][j + 1] = tmp4;
                B[i + 1][j + 2] = A[j + 2][i + 1];
                B[i + 1][j + 3] = A[j + 3][i + 1];

                tmp7 = A[j + 2][i + 2];
                tmp0 = A[j + 2][i + 3];

                B[i + 2][j] = tmp2;
                B[i + 2][j + 1] = tmp5;
                B[i + 2][j + 2] = tmp7;
                B[i + 2][j + 3] = A[j + 3][i + 2];

                tmp4 = A[j + 3][i + 3];

                B[i + 3][j] = tmp3;
                B[i + 3][j + 1] = tmp6;    
                B[i + 3][j + 2] = tmp0;
                B[i + 3][j + 3] = tmp4;

                tmp0 = A[j][i + 4];
                tmp1 = A[j][i + 5];
                tmp2 = A[j][i + 6];
                tmp3 = A[j][i + 7];

                B[i][j + 4] = tmp0;
                B[i][j + 5] = A[j + 1][i + 4];
                B[i][j + 6] = A[j + 2][i + 4];
                B[i][j + 7] = A[j + 3][i + 4];

                tmp4 = A[j + 1][i + 5];
                tmp5 = A[j + 1][i + 6];
                tmp6 = A[j + 1][i + 7];

                B[i + 1][j + 4] = tmp1;
                B[i + 1][j + 5] = tmp4;
                B[i + 1][j + 6] = A[j + 2][i + 5];
                B[i + 1][j + 7] = A[j + 3][i + 5];

                tmp7 = A[j + 2][i + 6];
                tmp0 = A[j + 2][i + 7];

                B[i + 2][j + 4] = tmp2;
                B[i + 2][j + 5] = tmp5;
                B[i + 2][j + 6] = tmp7;
                B[i + 2][j + 7] = A[j + 3][i + 6];

                tmp4 = A[j + 3][i + 7];

                B[i + 3][j + 4] = tmp3;
                B[i + 3][j + 5] = tmp6;    
                B[i + 3][j + 6] = tmp0;
                B[i + 3][j + 7] = tmp4;

                for (ii = i; ii < i + 4; ii++) {
                    tmp0 = B[ii][j + 4];
                    tmp1 = B[ii][j + 5];
                    tmp2 = B[ii][j + 6];
                    tmp3 = B[ii][j + 7];
                    B[ii][j + 4] = A[j + 4][ii];
                    B[ii][j + 5] = A[j + 5][ii];
                    B[ii][j + 6] = A[j + 6][ii];
                    B[ii][j + 7] = A[j + 7][ii];
                    B[ii + 4][j] = tmp0;
                    B[ii + 4][j + 1] = tmp1;
                    B[ii + 4][j + 2] = tmp2;
                    B[ii + 4][j + 3] = tmp3;
                }


                tmp0 = A[j + 4][i + 4];
                tmp1 = A[j + 4][i + 5];
                tmp2 = A[j + 4][i + 6];
                tmp3 = A[j + 4][i + 7];

                B[i + 4][j + 4] = tmp0;
                B[i + 4][j + 5] = A[j + 5][i + 4];
                B[i + 4][j + 6] = A[j + 6][i + 4];
                B[i + 4][j + 7] = A[j + 7][i + 4];

                tmp4 = A[j + 5][i + 5];
                tmp5 = A[j + 5][i + 6];
                tmp6 = A[j + 5][i + 7];

                B[i + 5][j + 4] = tmp1;
                B[i + 5][j + 5] = tmp4;
                B[i + 5][j + 6] = A[j + 6][i + 5];
                B[i + 5][j + 7] = A[j + 7][i + 5];

                tmp7 = A[j + 6][i + 6];
                tmp0 = A[j + 6][i + 7];

                B[i + 6][j + 4] = tmp2;
                B[i + 6][j + 5] = tmp5;
                B[i + 6][j + 6] = tmp7;
                B[i + 6][j + 7] = A[j + 7][i + 6];

                tmp4 = A[j + 7][i + 7];

                B[i + 7][j + 4] = tmp3;
                B[i + 7][j + 5] = tmp6;    
                B[i + 7][j + 6] = tmp0;
                B[i + 7][j + 7] = tmp4;
            }

    }
    else {
        int ii, jj, i, j;
        for (ii = 0; ii < N; ii += 16)
            for (jj = 0; jj < M; jj += 16)
                for (i = ii; i < ii + 16 && i < N; i++)
                    for (j = jj; j < jj + 16 && j < M; j++) {
                        B[j][i] = A[i][j];
                    }
    }

}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple ii-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

