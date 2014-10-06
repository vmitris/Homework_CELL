#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "btc.h"

void read_line(int fd, char* path, char* buf, int buf_size){
	char c = 0;
	int i = 0;
	while (c != '\n'){
		if (read(fd, &c, 1) == 0){
			fprintf(stderr, "Error reading from %s\n", path);
			exit(0);
		}
		if (i == buf_size){
			fprintf(stderr, "Unexpected input in %s\n", path);
			exit(0);
		}
		buf[i++] = c;
	}
	buf[i] = '\0';
}

void read_pgm(char* path, struct img* in_img){
	int fd, i;
	char buf[BUF_SIZE], *token;
	unsigned char* tmp_pixels;

	fd = _open_for_read(path);

	//read file type; expecting P5
	read_line(fd, path, buf, BUF_SIZE);
	if (strncmp(buf, "P5", 2)){
		fprintf(stderr, "Expected binary PGM (P5 type), got %s\n", path);
		exit(0);
	}

	//read comment line
	read_line(fd, path, buf, BUF_SIZE);

	//read image width and height
	read_line(fd, path, buf, BUF_SIZE);
	token = strtok(buf, " ");
	if (token == NULL){
		fprintf(stderr, "Expected token when reading from %s\n", path);
		exit(0);
	}	
	in_img->width = atoi(token);
	token = strtok(NULL, " ");
	if (token == NULL){
		fprintf(stderr, "Expected token when reading from %s\n", path);
		exit(0);
	}
	in_img->height = atoi(token);
	if (in_img->width < 0 || in_img->height < 0){
		fprintf(stderr, "Invalid width or height when reading from %s\n", path);
		exit(0);
	}

	//read max value
	read_line(fd, path, buf, BUF_SIZE);

	//allocate memory for image pixels
	tmp_pixels = (char*) _alloc(in_img->width * in_img->height);	
	in_img->pixels = (short int*) _alloc(in_img->width * in_img->height *
			sizeof (short int));

	_read_buffer(fd, tmp_pixels, in_img->width * in_img->height);

	//from char to short int
	for (i=0; i<in_img->width * in_img->height; i++){
		in_img->pixels[i] = (short int)tmp_pixels[i];
	}

	free(tmp_pixels);
	close(fd);
}

void write_pgm(char* path, struct img* out_img){
	int fd, bytes_written, left_to_write; 
	char *ptr, buf[BUF_SIZE];
	unsigned char* tmp_pixels;
	int i;

	fd = _open_for_write(path);

	//write image type
	strcpy(buf, "P5\n");
	_write_buffer(fd, buf, strlen(buf));

	//write comment 
	strcpy(buf, "#Created using BTC\n");
	_write_buffer(fd, buf, strlen(buf));

	//write image width and height
	sprintf(buf, "%d %d\n", out_img->width, out_img->height);
	_write_buffer(fd, buf, strlen(buf));

	//write max value
	strcpy(buf, "255\n");
	_write_buffer(fd, buf, strlen(buf));

	tmp_pixels = (char*) calloc(out_img->width * out_img->height, sizeof (char));
	if (!tmp_pixels){
		fprintf(stderr, "Error allocating memory when reading from %s\n", path);
		exit(0);
	}

	for (i=0; i<out_img->width * out_img->height; i++){
		tmp_pixels[i] = (char)out_img->pixels[i];
	}

	//write image pixels
	_write_buffer(fd, tmp_pixels, out_img->width * out_img->height);

	free(tmp_pixels);
	close(fd);
}

void free_pgm(struct img* image){
	free(image->pixels);
}
