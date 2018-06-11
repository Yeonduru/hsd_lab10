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
	
	// copy to a new matrix and vector
	memcpy(vec_cpy, input, M * sizeof(float));
	memcpy(mat_cpy, input + M , M * N * sizeof(float));
	
	//set output to zero
	memset(output, '\0', M * sizeof(float)); 

	int last_r = M / SIZE;
	int remainder_r = M % SIZE;
	int last_c = N / SIZE;
	int remainder_c = N % SIZE;

	if (remainder_r == 0)
		remainder_r = 64;
	if (remainder_c == 0)
		remainder_c = 64;

	for (int r = 0; r < last_r; ++r){
		for (int c = 0; c < last_c; ++c) {
			
			// ----------------Assign Vector-----------------------------
			float * vec_calc;
			vec_calc = c *  SIZE + vec_cpy;
			if (c == last_c){
				memcpy(vec, vec_calc, remainder_c * sizeof(float));
				memset(vec + remainder_c * sizeof(float),'\0', (SIZE-remainder_c) * sizeof(float));
			}
			else
				memcpy(vec, vec_calc, SIZE * sizeof(float));
			printf("Vector assigned\n");

			}
			//-----------------Assign Matrix--------------------------------
			
			//start of the matrix of ith row address to be put
			float* addr = mat;
			printf("Matrix assign");
			for (int i = 0; i < SIZE; ++i){
				// start of the matrix to get
                float* mat_start = mat_cpy + ((r + i) * N + c);
				
                if (r + i >= M)
                    memset(addr, '\0', SIZE * sizeof(float));
                else {
                    if (c == last_c){
                        memcpy(addr, mat_start, remainder_c * sizeof(float));
                        memset(addr + remainder_c , '\0', (SIZE-remainder_c)*sizeof(float));
                    }
                    else {
						printf("here?\n");
                        memcpy(addr, mat_start, SIZE * sizeof(float));
					}
                }
				addr += SIZE * sizeof(float);
			}

			// get result
			const float *out_calc = this->run();
			
			float* prev_result;
			memcpy(prev_result, output + c, SIZE * sizeof(float));
			for (int i = 0; i < SIZE; ++i)
				(*prev_result) += (*prev_result) + (*out_calc);

			// copy the previous result and add back to the new out
			memcpy(output + c, prev_result, SIZE * sizeof(float));
		}
	
	}

    // write down your code here.
}
