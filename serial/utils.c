#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int _open_for_read(char* path){
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0){
		fprintf(stderr, "%s: Error opening %s\n", __func__, path);
		exit(0);
	}
	return fd;
}

int _open_for_write(char* path){
	int fd;

	fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (fd < 0){
		fprintf(stderr, "%s: Error opening %s\n", __func__, path);
		exit(0);
	}
	return fd;
}

void _write_buffer(int fd, void* buf, int size){
	char *ptr;
	int left_to_write, bytes_written;

	ptr = (char*)buf;
	left_to_write = size;

	while (left_to_write > 0){
		bytes_written = write(fd, ptr, left_to_write);
		if (bytes_written <= 0){
			fprintf(stderr, "%s: Error writing buffer. "
					"fd=%d left_to_write=%d size=%d bytes_written=%d\n", 
					__func__, fd, left_to_write, size, bytes_written);
			exit(0);
		}
		left_to_write -= bytes_written;
		ptr += bytes_written;
	}
}

void _read_buffer(int fd, void* buf, int size){
	char *ptr;
	int left_to_read, bytes_read;

	ptr = (char*) buf;
	left_to_read = size;

	while (left_to_read > 0){
		bytes_read = read(fd, ptr, left_to_read);
		if (bytes_read <= 0){
			fprintf(stderr, "%s: Error reading buffer. "
					"fd=%d left_to_read=%d size=%d bytes_read=%d\n", 
					__func__, fd, left_to_read, size, bytes_read);
			exit(0);
		}
		left_to_read -= bytes_read;
		ptr += bytes_read;
	}
}

void* _alloc(int size){
	void *res;

	res = malloc(size);
	if (!res){
		fprintf(stderr, "%s: Failed to allocated %d bytes\n", __func__,
				size);
		exit(0);
	}

	return res;
}
