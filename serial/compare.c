#include <stdio.h>
#include <sys/time.h>

#include "btc.h"

#define PGM_MAX_PIXEL_DIFF 10
 
void compare_pgm(struct img* img1, struct img* img2){
	int i, nr_diff_pixels = 0, max_pixel_diff = 0;
	unsigned char c1, c2, diff;
	
	if (img1->width != img2->width){
		printf("Image 1 has a width of %d, while image 2 has a width of %d\n", 
			img1->width, img2->width);
		return;
	}
	if (img1->height != img2->height){
		printf("Image 1 has a height of %d, while image 2 has a height of %d\n", 
			img1->height, img2->height);
		return;
	}
	
	for (i = 0; i < img1->width * img1->height; i++){
		c1 = img1->pixels[i];
		c2 = img2->pixels[i];
		if (c1 > c2)
			diff = c1 - c2;
		else
			diff = c2 - c1;
		if (diff > PGM_MAX_PIXEL_DIFF)
			nr_diff_pixels++;
		if (diff > max_pixel_diff)
			max_pixel_diff = diff;
	}
	
	printf("%s: Percentage of significantly different pixels is %lf\n", __func__, 
		( 100 * (float)nr_diff_pixels) / (img1->width * img1->height)); 
	printf("%s: Maximum pixel difference is %d\n", __func__, max_pixel_diff); 
}

void compare_btc(struct c_img* img1, struct c_img* img2){
	int i, j, nr_blocks, nr_diff_a = 0, nr_diff_b = 0, nr_diff_bits = 0;
	unsigned char c1, c2, diff;
	
	if (img1->width != img2->width){
		printf("Image 1 has a width of %d, while image 2 has a width of %d\n", 
			img1->width, img2->width);
		return;
	}
	if (img1->height != img2->height){
		printf("Image 1 has a height of %d, while image 2 has a height of %d\n", 
			img1->height, img2->height);
		return;
	}
	
	nr_blocks = img1->width * img1->height / (BLOCK_SIZE * BLOCK_SIZE);
	
	for (i = 0; i < nr_blocks; i++){
		//compare a values
		c1 = img1->blocks[i].a;
		c2 = img2->blocks[i].a;
		if (c1 > c2)
			diff = c1 - c2;
		else
			diff = c2 - c1;
		if (diff > PGM_MAX_PIXEL_DIFF)
			nr_diff_a++;
		//compare b values
		c1 = img1->blocks[i].b;
		c2 = img2->blocks[i].b;
		if (c1 > c2)
			diff = c1 - c2;
		else
			diff = c2 - c1;
		if (diff > PGM_MAX_PIXEL_DIFF)
			nr_diff_b++;
		for (j = 0; j < BLOCK_SIZE * BLOCK_SIZE; j++){
			if (img1->blocks[i].bitplane[j] != img2->blocks[i].bitplane[j]){
				nr_diff_bits++;
			}
		}		
	}
	
	printf("%s: Significantly different a values percentage = %f\n", 
		__func__, 100.0 * ((float)  nr_diff_a) / nr_blocks);
	printf("%s: Significantly different b values percentage = %f\n", 
		__func__, 100.0 * ((float)  nr_diff_b) / nr_blocks);
	printf("%s: Percentage of different bits in bitplanes = %f\n", 
		__func__, 100.0 * ((float)  nr_diff_bits) / 
		(nr_blocks * BLOCK_SIZE * BLOCK_SIZE));

}

int main(int argc, char** argv){
	struct img img1, img2;
	struct c_img c_img1, c_img2;
	struct timeval t1, t2, t3, t4;
	float total_time = 0.0, scale_time = 0.0;

	if (argc != 4){
		printf("Usage: %s mode file1 file2\n", argv[0]);
		return 0;
	}

	gettimeofday(&t1, NULL);

	if (!strcmp(argv[1], "pgm")){
		read_pgm(argv[2], &img1);
		read_pgm(argv[3], &img2);
		gettimeofday(&t3, NULL);
		compare_pgm(&img1, &img2);
		gettimeofday(&t4, NULL);
		free_pgm(&img1);
		free_pgm(&img2);
	}

	if (!strcmp(argv[1], "btc")){
		read_btc(argv[2], &c_img1);
		read_btc(argv[3], &c_img2);
		gettimeofday(&t3, NULL);
		compare_btc(&c_img1, &c_img2);
		gettimeofday(&t4, NULL);
		free_btc(&c_img1);
		free_btc(&c_img2);
	}
	gettimeofday(&t2, NULL);

	scale_time += GET_TIME_DELTA(t3, t4);
	total_time += GET_TIME_DELTA(t1, t2);

	printf("Compare time: %lf\n", scale_time);
	printf("Total time: %lf\n", total_time);

	return 0;
}

