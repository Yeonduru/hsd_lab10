#include "fpga_api.h"
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

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

	float *vec_cpy;
	float *mat_cpy;
	
	// copy to a new matrix and vector
	memcpy(vec_cpy, vec, SIZE * sizeof(float));
	memcpy(mat_cpy, mat, SIZE * SIZE * sizeof(float));
	
	
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
			vec_calc = c *  SIZE * sizeof(float) + vec_cpy;
			if (c == last_c-1){
				memcpy(this->data_, vec_calc, remainder_c * sizeof(float));
				memset(this->data_ + remainder_c * sizeof(float),'\0', (SIZE-remainder_c) * sizeof(float));
			}
			else
				memcpy(this->data_, vec_calc, SIZE * sizeof(float));
			
			//-----------------Assign Matrix--------------------------------
			
			//start of the matrix of ith row address to be put
			float* addr = this->data_ + SIZE * sizeof(float);
			
			for (int i = 0; i < SIZE; ++i){
				// start of the matrix to get
                float* mat_start = mat + ((r + i) * N + c) * sizeof(float);
				
                if (r + i >= M)
                    memset(addr, '\0', SIZE * sizeof(float));
                else {
                    if (c == last_c-1){
                        memcpy(addr, mat_start, remainder_c * sizeof(float));
                        memset(addr + remainder * sizeof(float), '\0', (SIZE-remainder_c)*sizeof(float));
                    }
                    else
                        memcpy(addr, mat_start, SIZE * sizeof(float));
                }
				addr += SIZE * sizeof(float);
			}
			

			// get result
			const float *out_calc = this->run();
			
			float* prev_result;
			memcpy(prev_result, out + c * sizeof(float), SIZE * sizeof(float));
			for (int i = 0; i < SIZE; ++i)
				(&prev_result) += (&prev_result) + (&out_calc);

			// copy the previous result and add back to the new out
			memcpy(out + c * sizeof(float), prev_result, SIZE * sizeof(float));
		}
	
	}

    // write down your code here.
}
