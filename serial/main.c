#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "btc.h"

void btc_decompress_serial(struct img* image, struct c_img* c_image){
	int block_row_start, block_col_start, i, j, nr_blocks, k;
	unsigned char a, b;

	nr_blocks = c_image->width * c_image->height / (BLOCK_SIZE * BLOCK_SIZE);

	image->width = c_image->width;
	image->height = c_image->height;
	image->pixels = _alloc(image->width * image->height * sizeof(short int));
	block_row_start = block_col_start = 0;
	for (i=0; i<nr_blocks; i++){
		k = block_row_start * image->width + block_col_start;
		a = c_image->blocks[i].a;
		b = c_image->blocks[i].b;
		for (j=0; j<BLOCK_SIZE * BLOCK_SIZE; j++){
			image->pixels[k++] = (c_image->blocks[i].bitplane[j] ? b : a);
			if ((j + 1) % BLOCK_SIZE == 0){
				k -= BLOCK_SIZE; //back to the first column of the block
				k += image->width ; //go to the next line
			}
		}
		block_col_start += BLOCK_SIZE;
		if (block_col_start >= image->width){
			block_col_start = 0;
			block_row_start += BLOCK_SIZE;
		}
	}
}

void btc_compress_serial(struct img* image, struct c_img* c_image){
	int row, col, bl_row, bl_col, bl_index, bt_index, nr_blocks;
	float f1, f2, m, q, mean, stdev, a, b;
	unsigned char tmp;

	c_image->width = image->width;
	c_image->height = image->height;
	nr_blocks = image->width * image->height / (BLOCK_SIZE * BLOCK_SIZE);
	m = BLOCK_SIZE * BLOCK_SIZE;

	c_image->blocks = (struct block*) _alloc(nr_blocks * sizeof(struct block));

	bl_index = 0;
	for (bl_row = 0; bl_row < image->height; bl_row += BLOCK_SIZE){
		for (bl_col = 0; bl_col < image->width; bl_col += BLOCK_SIZE){
			//process 1 block from input image

			//compute mean
			mean = 0;
			for (row = bl_row; row < bl_row + BLOCK_SIZE; row++){
				for (col = bl_col; col < bl_col + BLOCK_SIZE; col++){
					mean += image->pixels[row * image->width + col];
				}
			}
			mean /= (BLOCK_SIZE * BLOCK_SIZE);

			//compute standard deviation
			stdev = 0;
			for (row = bl_row; row < bl_row + BLOCK_SIZE; row++){
				for (col = bl_col; col < bl_col + BLOCK_SIZE; col++){
					stdev += (image->pixels[row * image->width + col] - mean) * 
						(image->pixels[row * image->width + col] - mean);
				}
			}
			stdev /= (BLOCK_SIZE * BLOCK_SIZE);
			stdev = sqrt (stdev);

			//compute bitplane
			bt_index = 0;
			q = 0;
			for (row = bl_row; row < bl_row + BLOCK_SIZE; row++){
				for (col = bl_col; col < bl_col + BLOCK_SIZE; col++){
					tmp = (image->pixels[row * image->width + col] > mean ? 1 : 0);
					c_image->blocks[bl_index].bitplane[bt_index++] = tmp;
					q += tmp;
				}
			}

			//compute a and b
            if (q == 0){
                a = b = mean;
            }
            else {
	    		f1 = sqrt(q / (m - q));
                f2 = sqrt((m - q) / q);
        		a = (mean - stdev * f1);
    			b = (mean + stdev * f2);
            }
	        
            //avoid conversion issues due to precision errors
            if (a < 0)
                a = 0;
            if (b > 255)
                b = 255;

			c_image->blocks[bl_index].a = (unsigned char)a;
			c_image->blocks[bl_index].b = (unsigned char)b;
			bl_index++;
		}
	}	
}

int main(int argc, char** argv){
	struct img image, image2;
	struct c_img c_image;
	struct timeval t1, t2, t3, t4;
	double total_time = 0, scale_time = 0;

	if (argc != 4){
		printf("Usage: %s in.pgm out.btc out.pgm\n", argv[0]);
		return 0;
	}
	
	gettimeofday(&t3, NULL);	

	read_pgm(argv[1], &image);	

	gettimeofday(&t1, NULL);	
	btc_compress_serial(&image, &c_image);
	btc_decompress_serial(&image2, &c_image);
	gettimeofday(&t2, NULL);	

	write_btc(argv[2], &c_image);
	write_pgm(argv[3], &image2);

	free_btc(&c_image);
	free_pgm(&image);
	free_pgm(&image2);

	gettimeofday(&t4, NULL);

	total_time += GET_TIME_DELTA(t3, t4);
	scale_time += GET_TIME_DELTA(t1, t2);

	printf("Encoding / Decoding time: %lf\n", scale_time);
	printf("Total time: %lf\n", total_time);
}	



