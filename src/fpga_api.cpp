#include "fpga_api.h"
#include <cstring>
#include <istream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cstdio>

#define DATA_SIZE SIZE*(SIZE+1)*sizeof(float) // fpga bram data size

#define min(x,y) (((x)<(y))?(x):(y))

FPGA::FPGA(off_t data_addr, off_t api_addr)
{
    fd_ = open("/dev/mem", O_RDWR);
    data_ = static_cast<float*>(mmap(NULL, DATA_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, data_addr));
    api_ = static_cast<unsigned int*>(mmap(NULL, sizeof(unsigned int), PROT_READ|PROT_WRITE, MAP_SHARED,fd_, api_addr));
}

FPGA::~FPGA()
{
    munmap(data_, DATA_SIZE );
    munmap(api_, sizeof(unsigned int));
    close(fd_);
}

float* FPGA::matrix(void)
{
	return data_ + SIZE;
}

float* FPGA::vector(void)
{
	return data_;
}

const float* __attribute__((optimize("O0"))) FPGA::run()
{
    *api_ = 0x5555;
    while(*api_ == 0x5555);

    return data_;    
}



void FPGA::largeMV(const float* large_mat, const float* input,
		float* output, int M, int N)
{
    // write down your code here.
    float* vec = this->vector();
    float* mat = this->matrix();

	float *vec_cpy = new float[M];
	float *mat_cpy = new float[M*N];
	
	//memcpy(vec, input, sizeof(float) * SIZE);
	//memcpy(mat, input + M, sizeof(float) * SIZE * SIZE);	

	// copy to a new matrix and vector
	memcpy(vec_cpy, input, M * sizeof(float));
	memcpy(mat_cpy, input + M , M * N * sizeof(float));
	
	//set output to zero
	float *output_temp = new float[M + SIZE];
	memset(output_temp, '\0', (M + SIZE) * sizeof(float)); 

	int quotient_r = M / SIZE;
	int remainder_r = M % SIZE;
	int quotient_c = N / SIZE;
	int remainder_c = N % SIZE;

	for (int r = 0; r <= quotient_r; ++r){
		for (int c = 0; c <= quotient_c; ++c) {

			float * vec_calc;
			vec_calc = c * SIZE + vec_cpy;

			if (c == quotient_c){
				memcpy(vec, vec_calc, remainder_c * sizeof(float));
				memset(vec + remainder_c * sizeof(float),'\0', (SIZE-remainder_c) * sizeof(float));
			}
			else
				memcpy(vec, vec_calc, SIZE * sizeof(float));
			
			// check vec 
			printf("vector of r: %d c: %d", r, c);
			for (int i = 0; i < SIZE; ++i)
				printf("%f\n", *(vec + i) );

			//-----------------Assign Matrix--------------------------------
			
			//start of the matrix of ith row address to be put
			float* addr = mat;
			for (int i = 0; i < SIZE; ++i){
				// start of the matrix to get
                float* mat_start = mat_cpy + ((r + i) * N + c);
				
                if (r + i >= M)
                    memset(addr, '\0', SIZE * sizeof(float));
                else {
                    if (c == quotient_c){
                        memcpy(addr, mat_start, remainder_c * sizeof(float));
                        memset(addr + remainder_c , '\0', (SIZE-remainder_c)*sizeof(float));
                    }
                    else {
                        memcpy(addr, mat_start, SIZE * sizeof(float));
					}
                }
				addr += SIZE;
			}

			// check vec 
			printf("Matrix of r: %d c: %d", r, c);
			for (int i = 0; i < SIZE; ++i){
				for (int j = 0; j < SIZE; ++j)
					printf("%f ", *(mat + i * SIZE + j));
				printf("\n");
			}
		
			// get result
			const float *out_calc = this->run();

			// check out 
			for (int i = 0 ; i < SIZE; ++i)
				printf("%f\n", *(out_calc + i));
				
			float* prev_result = new float[SIZE];
			memcpy(prev_result, output_temp + c * SIZE, SIZE * sizeof(float));
			for (int i = 0; i < SIZE; ++i)
				*(prev_result+i) = *(prev_result+i) + *(out_calc+i);

			// copy the previous result and add back to the new out
			memcpy(output_temp + c * SIZE, prev_result, SIZE * sizeof(float));
		}
	
	}
	memcpy(output, output_temp, M * sizeof(float));
    // write down your code here.
}
