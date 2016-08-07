#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
//#include <GLUT/glut.h> /* Mac �̏ꍇ*/ 
#include <GL/glut.h>  /* Windows �̏ꍇ*/

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 800
//#define W 100             /* �n�ʂ̕��@�@�@�@�@�@�@ */
//#define D 400            /* �n�ʂ̒����@�@�@�@�@�@ */
#define W 100
#define D 100
#define QX -0.2           /* ���̏����ʒu�̂����W�l */
#define QY 1.5           /* ���̏����ʒu�̂����W�l */
#define QZ 0        /* ���̏����ʒu�̂����W�l */
#define G (-5.8)         /* �d�͉����x�@�@�@�@�@�@ */
#define A 0.8            /* �����W��               */
#define V 25.0           /* �����x�@�@�@�@�@�@�@�@ */
#define TSTEP 0.01       /* �t���[�����Ƃ̎��ԁ@�@ */
#define R 0.025           /* �{�[���̔��a�@�@�@�@�@ */
#define PI 3.14159       /* �~����                 */
#define DM 0.06          /* ミートカーソルの速度 */
#define VS 25            /* スイングスピード */

static void resize(int w, int h)
{
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	gluPerspective(20.0, (double)w / (double)h, 1.0, 100.0);

	glMatrixMode(GL_MODELVIEW);
}

unsigned char* loadBMPFile(char* filename, int *width, int *height)
{
	const int HEADERSIZE = 54;
	int i, j;
	int real_width;
	unsigned int color;
	FILE *fp;
	unsigned char header_buf[HEADERSIZE];
	unsigned char *tmp;
	unsigned char *pixel;
	errno_t err = fopen_s(&fp, filename, "rb");
	if (err != 0) {
		fprintf_s(stderr, "Cannot open file [ %s ].\n", filename);
		exit(-1);
	}

	fread(header_buf, sizeof(unsigned char), HEADERSIZE, fp);

	if (strncmp((char*)header_buf, "BM", 2)) {
		fprintf_s(stderr, "Error : [ %s ] is not bitmap file.\n", filename);
		exit(-1);
	}
	memcpy(width, header_buf + 18, sizeof(int));
	memcpy(height, header_buf + 22, sizeof(int));
	memcpy(&color, header_buf + 28, sizeof(unsigned int));

	if (color != 24) {
		fprintf_s(stderr, "Error : [ %s ] is not 24bit color image.\n", filename);
		exit(-1);
	}

	if (*height < 0) {
		*height *= -1;
		real_width = *width * 3 + *width % 4;
		tmp = new unsigned char[real_width];
		pixel = new unsigned char[3 * (*width) * (*height)];

		for (i = 0; i < *height; i++) {
			fread(tmp, 1, real_width, fp);
			for (j = 0; j < *width; j++) {
				pixel[3 * ((*height - i - 1) * (*width) + j)] = tmp[3 * j + 2];
				pixel[3 * ((*height - i - 1) * (*width) + j) + 1] = tmp[3 * j + 1];
				pixel[3 * ((*height - i - 1) * (*width) + j) + 2] = tmp[3 * j];
			}
		}
	}
	else {
		real_width = *width * 3 + *width % 4;
		tmp = new unsigned char[real_width];
		pixel = new unsigned char[3 * (*width) * (*height)];

		for (i = 0; i < *height; i++) {
			fread(tmp, 1, real_width, fp);
			for (j = 0; j < *width; j++) {
				pixel[3 * (i * (*width) + j)] = tmp[3 * j + 2];
				pixel[3 * (i * (*width) + j) + 1] = tmp[3 * j + 1];
				pixel[3 * (i * (*width) + j) + 2] = tmp[3 * j];
			}
		}
	}
	delete[] tmp;
	return pixel;
}

GLuint loadTexture(char* filename)
{
	GLuint texID;
	GLubyte *texture_image;
	int texture_width, texture_height;
	texture_image = loadBMPFile(filename, &texture_width, &texture_height);
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_image);
	delete[] texture_image;
	return texID;
}
