/* 
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

	int i, j, blockRow, blockCol, tmp, d;
	int transSquare(int bsize){
		for(blockCol = 0; blockCol < N; blockCol += bsize){
			for(blockRow = 0; blockRow < N; blockRow += bsize){
				for(i = blockRow; i < blockRow + bsize; i ++){
					for(j = blockCol; j < blockCol + bsize; j ++){
						if(i != j){
							B[j][i] = A[i][j];
						}
						else{
							tmp = A[i][j];
							d = i;
						}
					}
					if(blockRow == blockCol){
						B[d][d] = tmp;
					}
				}
			}
		}
		return 1;
	}

	int transRect(int bsize){
		for(blockCol = 0; blockCol < M; blockCol += bsize){
			for(blockRow = 0; blockRow < N; blockRow += bsize){
				//only difference between transSquare and transRect is that there are cases when 
				for(i = blockRow; (i < blockRow + bsize) && (i < N); i ++){
					for(j = blockCol; (j < blockCol + bsize) && (j < M); j ++){
						if(i != j){
							B[j][i] = A[i][j];
						}
						else{
							tmp = A[i][j];
							d = i;
						}
					}
					if(blockRow == blockCol){
						B[d][d] = tmp;
					}
				}
			}
		}
		return 1;
	}

	switch(N){
		case 32:
			transSquare(8);
			break;
		case 64:
			transSquare(4);
			break;
		default:
			transRect(16);
			break;		
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

