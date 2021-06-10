/*
 * inverse_bmp [--debug] [file1.bmp] [file2.bmp] . . 
 * 
 * Author: Travis Banken
 * 
 * Description:
 * This Program takes in bitmap images, img.bmp, as program arguments and 
 * inverses the colors, creating a new image INV_img.bmp
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct bmp_header {
	uint16_t type;			// Magic Identifier: 0x4d42
	uint32_t size;			// File size in bytes
	//uint16_t reserved1; 	// Not used
	//uint16_t reserved2; 	// Not used
	uint32_t offset;		// Offset to image data in bytes from beg of file
	uint32_t dib_header_size; // DIB header size in bytes
	int32_t width_px;		// width of image
	int32_t height_px;		// height of image
	uint16_t num_planes;	// num of color planes
	uint16_t bits_per_pixel;// bits per pixel
	uint32_t compression;	// compression type
	uint32_t image_size_bytes; // image size in bytes
	int32_t x_resolution_ppm; // pixels per meter
	int32_t y_resolution_ppm; // pixels per meter
	uint32_t num_colors;	// Number of colors
	uint32_t important_colors; // important colors
};

int inverse_bmp(char *filename, int debug); // прототипы функции
struct bmp_header* read_header(FILE *file);
int check_header(struct bmp_header *header);
void print_header(const struct bmp_header);

int main(int argc, char *argv[])
{
	if (argc == 1 || (argc == 2 && strcmp(argv[1], "--debug") == 0)) {
		fprintf(stderr, "Usage: inverse_bmp [--debug] [img1.bmp]\n");
		return 1;
	}

	int debug = 0;
	if (strcmp(argv[1], "--debug") == 0)
		debug = 1;

	for (int i = 1; i < argc; i++) {
		printf("Inverting %s . . . ", argv[i]);
		fflush(stdout);

		if (inverse_bmp(argv[i], debug) != 0) {
			fprintf(stderr, "Error: could not inverse %s\n", argv[i]);
			continue;
		}

		printf("DONE\n");
	}
	
	return 0;
}

/*
 * Takes a bitmap file and inverses the colors, writing the new image to a new 
 * file.
 * 
 * params: filename - bmp file name to be inverted.
 *         debug    - 1 for debug mode, 0 otherwise
 * 
 * return: 1 if error occured, 0 otherwise.
 */
int inverse_bmp(char *filename, int debug)
{
	FILE *image = fopen(filename, "r");
	if (image == NULL) {
		perror("fopen");
		return 1;
	}

	struct bmp_header *header = read_header(image);
	if (header == NULL) {
		fprintf(stderr, "Error: failed to read header\n");
		return 1;
	}
	if (check_header(header) != 0) {
		fprintf(stderr, "Error: Invalid header\n");
		free(header);
		return 1;
	}

	fclose(image);
	image = fopen(filename, "r");
	if (image == NULL) {
		perror("fopen");
		fclose(image);
		free(header);
		return 1;
	}

	// add INV_ prefix to outfile
	char prefix[strlen(filename) + 5];
	strcpy(prefix, "INV_");
	FILE *inv_image = fopen(strcat(prefix, filename), "w");

	uint8_t *imgbytes = malloc(54);
	fread(imgbytes, 1, 54, image);

	fwrite(imgbytes, 1, 54, inv_image);

	free(imgbytes);

	// if (debug == 1)
	// 	print_header(*header);
	
	// read pixels and invert, writing to new file
	uint8_t pixel = 0;
	for (int h = 0; h < header->height_px; h++) {
		for (int w = 0; w < 3 * header->width_px; w++) {
			fread(&pixel, 1, 1, image);
			pixel = (uint8_t)(255 - pixel);
			fwrite(&pixel, 1, 1, inv_image);
		}
		// padding
		fread(&pixel, 1, 1, image);
		fwrite(&pixel, 1, 1, inv_image);
		fread(&pixel, 1, 1, image);
		fwrite(&pixel, 1, 1, inv_image);
	}

	fclose(image);
	fclose(inv_image);
	free(header);

	return 0;
}

/*
 * Fills in the bmp header bits into a struct. The reference to this struct is
 * returned.
 * 
 * params: file - pointer to the file
 * 
 * return: reference to the bmp header struct
 */
struct bmp_header* read_header(FILE *file)
{
	struct bmp_header *header = malloc(sizeof(struct bmp_header));

	uint8_t *imgbytes = malloc(54);
	size_t num_read = fread(imgbytes, 1, 54, file);
	if (num_read != 54) {
		free(header);
		free(imgbytes);
		return NULL;
	}
	
	// fill struct with bmp header bytes
	header->type =             (imgbytes[0] <<  8) | (imgbytes[1] <<  0);

	header->size =             (imgbytes[2] <<  0) | (imgbytes[3] <<  8) |
	                           (imgbytes[4] << 16) | (imgbytes[5] << 24);

	//header->reserved1 =        (imgbytes[6] <<  8) | (imgbytes[7] <<  0);
	//header->reserved2 =        (imgbytes[8] <<  8) | (imgbytes[9] <<  0);
	
	header->offset =           (imgbytes[10] <<  0) | (imgbytes[11] <<  8) |
				               (imgbytes[12] << 16) | (imgbytes[13] << 24);

	header->dib_header_size =  (imgbytes[14] <<  0) | (imgbytes[15] <<  8) |
				               (imgbytes[16] << 16) | (imgbytes[17] << 24);

	header->width_px =         (imgbytes[18] <<  0) | (imgbytes[19] <<  8) |
				               (imgbytes[20] << 16) | (imgbytes[21] << 24);


	header->height_px =        (imgbytes[22] <<  0) | (imgbytes[23] <<  8) |
				               (imgbytes[24] << 16) | (imgbytes[25] << 24);

	header->num_planes =       (imgbytes[26] <<  0) | (imgbytes[27] <<  8);

	header->bits_per_pixel =   (imgbytes[28] <<  0) | (imgbytes[29] <<  8);

	header->compression =      (imgbytes[30] <<  0) | (imgbytes[31] <<  8) |
				               (imgbytes[32] << 16) | (imgbytes[33] << 24);
	
	header->image_size_bytes = (imgbytes[34] <<  0) | (imgbytes[35] <<  8) |
				               (imgbytes[36] << 16) | (imgbytes[37] << 24);

	header->x_resolution_ppm = (imgbytes[38] <<  0) | (imgbytes[39] <<  8) |
				               (imgbytes[40] << 16) | (imgbytes[41] << 24);

	header->y_resolution_ppm = (imgbytes[42] <<  0) | (imgbytes[43] <<  8) |
				               (imgbytes[44] << 16) | (imgbytes[45] << 24);

	header->num_colors =       (imgbytes[46] <<  0) | (imgbytes[47] <<  8) |
				               (imgbytes[48] << 16) | (imgbytes[49] << 24);

	header->important_colors = (imgbytes[50] <<  0) | (imgbytes[51] <<  8) |
				               (imgbytes[52] << 16) | (imgbytes[53] << 24);

	free(imgbytes);
	return header;
}

/*
 * Checks if the given header is a valid bmp header.
 * 
 * params: header - bmp header struct
 * 
 * return: 1 if header OK, 0 otherwise
 */
int check_header(struct bmp_header *header)
{
	// check magic number
	if (header->type == 0x4d42) {
		return 1;
	}

	// other checks?..

	return 0;
}

// // debug method which prints out the contents of the bmp header struct
// void print_header(const struct bmp_header header)
// {
// 	printf("\n");
// 	printf("-------------------------------------------\n");
// 	printf("BMP Header Data\n");
// 	printf("-------------------------------------------\n");
// 	printf("Mgk num:          0x%04x\n", header.type);
// 	printf("size:             0x%08x\n", header.size);
// 	printf("offset:           0x%08x\n", header.offset);
// 	printf("dib_header_size:  0x%08x\n", header.dib_header_size);
// 	printf("width_px:         0x%08x\n", header.width_px);
// 	printf("height_px:        0x%08x\n", header.height_px);
// 	printf("num_planes:       0x%04x\n", header.num_planes);
// 	printf("bits/pix:         0x%04x\n", header.bits_per_pixel);
// 	printf("compression:      0x%08x\n", header.compression);
// 	printf("image_size_bytes: 0x%08x\n", header.image_size_bytes);
// 	printf("x_resolution_ppm: 0x%08x\n", header.x_resolution_ppm);
// 	printf("y_resolution_ppm: 0x%08x\n", header.y_resolution_ppm);
// 	printf("num_colors:       0x%08x\n", header.num_colors);
// 	printf("important_colors: 0x%08x\n", header.important_colors);
// 	printf("-------------------------------------------\n");
// }
