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

	memcpy(vec_cpy, vec, SIZE * sizeof(float));
	memcpy(mat_cpy, mat, SIZE * SIZE * sizeof(float));
	
	int last = SIZE / 64;
	int remainder = SIZE % 64;
	if (remainder == 0)
		remainder = 64;
	for (int r = 0; r < last; ++r){
		for (int c = 0; c < last; ++c) {
			
			float * vec_calc;
			vec_calc = c *  64 * sizeof(float) + vec_cpy;
			if (c == last-1){
				memcpy(this->data_, vec_calc, remainder * sizeof(float));
				memset(this->data_ + remainder*sizeof(float),'\0', (64-remainder) * sizeof(float));
			}
			else
				memcpy(this->data_, vec_calc, 64 * sizeof(float));
		
			float* addr = this->data_ + 64 * sizeof(float);
			for (int i = 0; i < 64; ++i){
                float* mat_start = mat + (r * SIZE + c + SIZE * i) * sizeof(float);
                if (r * 64 + i >= SIZE)
                    memset(addr, '\0', 64 * sizeof(float));
                else {
                    if (c == last-1){
                        memcpy(addr, mat_start, remainder * sizeof(float));
                        memset(addr + remainder * sizeof(float), '\0', (64-remainder)*sizeof(float));
                    }
                    else
                        memcpy(addr, mat_start, 64 * sizeof(float));
                }
				
				
				addr += 64 * sizeof(float);
			}
			const float *out_calc = this->run();
			memcpy(out + c * sizeof(float), out_calc, 64 * sizeof(float));

		}
	
	}

    // write down your code here.
}
