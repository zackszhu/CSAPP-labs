/* 
 * author: YP
 *
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

#define SUPERBLOCK 8
#define SMALLBLOCK 4
#define BLOCKSIZE 17

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{

    
    int block_row, block_column, i;
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int flag = 0;
    
    //32*32 Matrix Transpose
    if (M == 32 && N == 32) {
        //block bits=5 indicating each line can hold 32 bytes(8 integers)
        //integers in each row in the matrix could be stored into 4 sets with 8 integers per set
        //the cache could be filled with 8 rows, use cache blocking with blocksize 8
        //Each block selected from matrix A follows left->right, top->bottom access pattern
        //Each block selected from matrix b follows top->bottom, left->right access pattern
        for (block_row = 0; block_row < N; block_row += SUPERBLOCK) {
            for (block_column = 0; block_column < M; block_column += SUPERBLOCK) {
                for (int tmp0 = block_row; tmp0 < block_row + SUPERBLOCK; tmp0 ++) {
                    for (int tmp1 = block_column; tmp1 < block_column + SUPERBLOCK; tmp1 ++) {
                        if (tmp0 != tmp1) {
                            B[tmp1][tmp0] = A[tmp0][tmp1];
                        } else {
                            // store array element along the diagonal to a local variable
                            // to minimize conflict miss
                            tmp2 = A[tmp0][tmp1];
                            tmp3 = tmp0;
                            flag = 1;
                        }
                    }
                    // load the elements in B to replace those in A
                    // that share the same set/line with B
                    if (flag) {
                        B[tmp3][tmp3] = tmp2;
                        flag = 0;
                    }
                }
            }
        }
    }
    
    //64*64 Matrix Transpose
    if (M == 64 && N == 64) {
        //block bits=5 indicating each line can hold 32 bytes(8 integers)
        //integers in each row in the matrix could be stored into 8 sets with 8 integers per set
        //the cache could be filled with 4 rows
        //the matrix can be divided into 8*8 super block with 4*4 small block within each super block
        //Each super block selected from matrix A follows left->right, top->bottom access pattern
        //Each super block selected from matrix b follows top->bottom, left->right access pattern
        for (block_row = 0; block_row < N; block_row += SUPERBLOCK) {
            for (block_column = 0; block_column < M; block_column += SUPERBLOCK) {
                
                //Transpose the upper left small block
                //Each loop takes a row of 8 elements from A, do transpose for the first half
                //Buffer the second half in B for transpose in next step
                for (i = block_column; i < block_column + SMALLBLOCK; i ++) {
                    tmp0 = A[i][block_row];
                    tmp1 = A[i][block_row+1];
                    tmp2 = A[i][block_row+2];
                    tmp3 = A[i][block_row+3];
                    tmp4 = A[i][block_row+4];
                    tmp5 = A[i][block_row+5];
                    tmp6 = A[i][block_row+6];
                    tmp7 = A[i][block_row+7];
                    B[block_row][i] = tmp0;
                    B[block_row][i+4] = tmp4;
                    B[block_row+1][i] = tmp1;
                    B[block_row+1][i+4] = tmp5;
                    B[block_row+2][i] = tmp2;
                    B[block_row+2][i+4] = tmp6;
                    B[block_row+3][i] = tmp3;
                    B[block_row+3][i+4] = tmp7;
                }
                
                //Transpose the bottom left small block and
                //the upper right small block within one loop
                //Transpose the bottom left one using the data buffered in B
                for (i = block_row; i < block_row + SMALLBLOCK; i ++) {
                    tmp0 = B[i][block_column+4];
                    tmp1 = B[i][block_column+5];
                    tmp2 = B[i][block_column+6];
                    tmp3 = B[i][block_column+7];
                    tmp4 = A[block_column+4][i];
                    tmp5 = A[block_column+5][i];
                    tmp6 = A[block_column+6][i];
                    tmp7 = A[block_column+7][i];
                    B[i][block_column+4] = tmp4;
                    B[i][block_column+5] = tmp5;
                    B[i][block_column+6] = tmp6;
                    B[i][block_column+7] = tmp7;
                    B[i+4][block_column] = tmp0;
                    B[i+4][block_column+1] = tmp1;
                    B[i+4][block_column+2] = tmp2;
                    B[i+4][block_column+3] = tmp3;
                }
                
                //Transpose the bottom right small block at last
                for (i = block_row + SMALLBLOCK; i < block_row + SUPERBLOCK; i ++) {
                    tmp0 = A[block_column + 4][i];
                    tmp1 = A[block_column + 5][i];
                    tmp2 = A[block_column + 6][i];
                    tmp3 = A[block_column + 7][i];
                    B[i][block_column + 4] = tmp0;
                    B[i][block_column + 5] = tmp1;
                    B[i][block_column + 6] = tmp2;
                    B[i][block_column + 7] = tmp3;
                }
            }
        }
    }
    
    //67*61 Matrix Transpose
    if (M == 61 && N == 67) {
        // Use cache blocking with blocksize 17
        for (block_column = 0; block_column < M; block_column += BLOCKSIZE) {
            for (block_row = 0; block_row < N; block_row += BLOCKSIZE) {
                for (tmp0 = block_row; tmp0 < block_row + BLOCKSIZE && tmp0 < N; tmp0 ++) {
                    for (tmp1 = block_column; tmp1 < block_column + BLOCKSIZE && tmp1 < M; tmp1 ++) {
                        tmp2 = A[tmp0][tmp1];
                        B[tmp1][tmp0] = tmp2;
                    }
                }
            }
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
char trans_desc[] = "Simple row-wise scan transpose";
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



int w = A[j][i];
                int s = A[j][i + 1];
                int t = A[j][i + 3];
                int r = A[j][i + 2];

                B[i][j] = w;
                B[i][j + 1] = A[j + 1][i];
                B[i][j + 2] = A[j + 2][i];
                B[i][j + 3] = A[j + 3][i];

                int x = A[j + 1][i + 1];
                int v = A[j + 1][i + 3];
                int u = A[j + 1][i + 2];


                B[i + 1][j] = s;
                B[i + 1][j + 1] = x;
                B[i + 1][j + 2] = A[j + 2][i + 1];
                B[i + 1][j + 3] = A[j + 3][i + 1];

                int y = A[j + 2][i + 2];
                w = A[j + 2][i + 3];

                B[i + 2][j] = r;
                B[i + 2][j + 1] = u;
                B[i + 2][j + 2] = y;
                B[i + 2][j + 3] = A[j + 3][i + 2];

                int z = A[j + 3][i + 3];

                B[i + 3][j] = t;
                B[i + 3][j + 1] = v;    
                B[i + 3][j + 2] = w;
                B[i + 3][j + 3] = z;