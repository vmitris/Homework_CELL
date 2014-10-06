#include <stdio.h>
#include <stdlib.h>

#include "btc.h"

void write_btc(char* path, struct c_img* out_img){
	int i, nr_blocks, j, fd, k;
	struct bits tmp;
	char *buf;

	fd = _open_for_write(path);

	write(fd, &out_img->width, sizeof(int));
	write(fd, &out_img->height, sizeof(int));

	nr_blocks = out_img->width * out_img->height / (BLOCK_SIZE * BLOCK_SIZE);
	buf = _alloc(nr_blocks * (2 + BLOCK_SIZE * BLOCK_SIZE / BITS_IN_BYTE));

	k = 0;
	for (i=0; i<nr_blocks; i++){
		//write a and b
		buf[k++] = out_img->blocks[i].a;
		buf[k++] = out_img->blocks[i].b;		
		//from bytes to bits
		j = 0;
		while (j < BLOCK_SIZE * BLOCK_SIZE){
			tmp.bit0 = out_img->blocks[i].bitplane[j++];
			tmp.bit1 = out_img->blocks[i].bitplane[j++];
			tmp.bit2 = out_img->blocks[i].bitplane[j++];
			tmp.bit3 = out_img->blocks[i].bitplane[j++];
			tmp.bit4 = out_img->blocks[i].bitplane[j++];
			tmp.bit5 = out_img->blocks[i].bitplane[j++];
			tmp.bit6 = out_img->blocks[i].bitplane[j++];
			tmp.bit7 = out_img->blocks[i].bitplane[j++];						
			buf[k++] = *((char*)&tmp);
		}
		//write bitplane
	}

	_write_buffer(fd, buf, 
		nr_blocks * (2 + BLOCK_SIZE * BLOCK_SIZE / BITS_IN_BYTE));

	free(buf);
	close(fd);
}

void read_btc(char* path, struct c_img* out_img){
	int fd, nr_blocks, i, j = 0, k, ii;
	char *big_buf;
	struct bits tmp;

	fd = _open_for_read(path);

	read(fd, &out_img->width, sizeof(int));
	read(fd, &out_img->height, sizeof(int));

	nr_blocks = out_img->width * out_img->height / (BLOCK_SIZE * BLOCK_SIZE);
	out_img->blocks = (struct block*) _alloc(nr_blocks * sizeof(struct block));

	big_buf = (char*) _alloc(nr_blocks * (2 + BLOCK_SIZE * BLOCK_SIZE / BITS_IN_BYTE));

	_read_buffer(fd, big_buf, nr_blocks * (2 + BLOCK_SIZE * BLOCK_SIZE / BITS_IN_BYTE));

	for (i=0; i<nr_blocks; i++){
		//read a and b
		out_img->blocks[i].a = big_buf[j++];
		out_img->blocks[i].b = big_buf[j++];
		//read bitplane
		k = 0;
		for (ii=0; ii<BLOCK_SIZE * BLOCK_SIZE / BITS_IN_BYTE; ii++){
			tmp = *((struct bits*)&big_buf[j++]);
			out_img->blocks[i].bitplane[k++] = tmp.bit0;
			out_img->blocks[i].bitplane[k++] = tmp.bit1;
			out_img->blocks[i].bitplane[k++] = tmp.bit2;
			out_img->blocks[i].bitplane[k++] = tmp.bit3;
			out_img->blocks[i].bitplane[k++] = tmp.bit4;
			out_img->blocks[i].bitplane[k++] = tmp.bit5;
			out_img->blocks[i].bitplane[k++] = tmp.bit6;
			out_img->blocks[i].bitplane[k++] = tmp.bit7;			
		}
	}

	free(big_buf);
	close(fd);
}

void free_btc(struct c_img* image){
	free(image->blocks);
}
