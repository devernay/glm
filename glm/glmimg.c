#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "glm.h"
/*
#define DEBUG
#define GLDEBUG
*/
#include "glmint.h"


/*
#undef HAVE_LIBJPEG
#undef HAVE_LIBSDL_IMAGE
*/

/* glmReadPPM: read a PPM raw (type P6) file.  The PPM file has a header
 * that should look something like:
 *
 *    P6
 *    # comment
 *    width height max_value
 *    rgbrgbrgb...
 *
 * where "P6" is the magic cookie which identifies the file type and
 * should be the only characters on the first line followed by a
 * carriage return.  Any line starting with a # mark will be treated
 * as a comment and discarded.   After the magic cookie, three integer
 * values are expected: width, height of the image and the maximum
 * value for a pixel (max_value must be < 256 for PPM raw files).  The
 * data section consists of width*height rgb triplets (one byte each)
 * in binary format (i.e., such as that written with fwrite() or
 * equivalent).
 *
 * The rgb data is returned as an array of unsigned chars (packed
 * rgb).  The malloc()'d memory should be free()'d by the caller.  If
 * an error occurs, an error message is sent to stderr and NULL is
 * returned.
 *
 * filename   - name of the .ppm file.
 * width      - will contain the width of the image on return.
 * height     - will contain the height of the image on return.
 *
 */
static GLubyte* 
glmReadPPM(const char* filename, GLboolean alpha, int* width, int* height, int *type)
{
    FILE* fp;
    int i, w, h, d;
    unsigned char* image;
    char head[70];          /* max line <= 70 in PPM (per spec). */
    
    fp = fopen(filename, "rb");
    if (!fp) {
        perror(filename);
        return NULL;
    }
    
    /* grab first two chars of the file and make sure that it has the
       correct magic cookie for a raw PPM file. */
    fgets(head, 70, fp);
    if (strncmp(head, "P6", 2)) {
	DBG_(__glmWarning("glmReadPPM() failed: %s: Not a raw PPM file", filename));
        return NULL;
    }
    
    /* grab the three elements in the header (width, height, maxval). */
    i = 0;
    while(i < 3) {
        fgets(head, 70, fp);
        if (head[0] == '#')     /* skip comments. */
            continue;
        if (i == 0)
            i += sscanf(head, "%d %d %d", &w, &h, &d);
        else if (i == 1)
            i += sscanf(head, "%d %d", &h, &d);
        else if (i == 2)
            i += sscanf(head, "%d", &d);
    }
    
    /* grab all the image data in one fell swoop. */
    image = (unsigned char*)malloc(sizeof(unsigned char)*w*h*3);
    fread(image, sizeof(unsigned char), w*h*3, fp);
    fclose(fp);
    
    *type = GL_RGB;
    *width = w;
    *height = h;
    return image;
}




/* don't try alpha=GL_FALSE: gluScaleImage implementations seem to be buggy */
GLuint
glmLoadTexture(GLMmodel* model, const char *name, GLboolean alpha, GLboolean repeat, GLboolean filtering, GLboolean mipmaps)
{
    GLuint tex;
    char *dir;
    char *filename;
    int width, height,pixelsize;
    int type;
    int filter_min, filter_mag;
    GLubyte *data, *rdata;
    GLint glMaxTexDim ;
    double xPow2, yPow2;
    int ixPow2, iyPow2;
    int xSize2, ySize2;
    GLint retval;

    dir = __glmDirName(model->pathname);
    filename = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(name) + 1));
    strcpy(filename, dir);
    strcat(filename, name);
    free(dir);

    /* fallback solution (PPM only) */
    data = glmReadPPM(filename, alpha, &width, &height, &type);
    if(data != NULL) {
	DBG_(__glmWarning("glmLoadTexture(): got PPM for %s",filename));
	goto DONE;
    }

#ifdef HAVE_LIBJPEG
    data = glmReadJPG(filename, alpha, &width, &height, &type);
    if(data != NULL) {
	DBG_(__glmWarning("glmLoadTexture(): got JPG for %s",filename));
	goto DONE;
    }
#endif
#ifdef HAVE_LIBPNG
    data = glmReadPNG(filename, alpha, &width, &height, &type);
    if(data != NULL) {
	DBG_(__glmWarning("glmLoadTexture(): got PNG for %s",filename));
	goto DONE;
    }
#endif
#ifdef HAVE_LIBSDL_IMAGE
    data = glmReadSDL(filename, alpha, &width, &height, &type);
    if(data != NULL) {
	DBG_(__glmWarning("glmLoadTexture(): got SDL for %s",filename));
	goto DONE;
    }
#endif
#ifdef HAVE_LIBSIMAGE
    data = glmReadSimage(filename, alpha, &width, &height, &type);
    if(data != NULL) {
	DBG_(__glmWarning("glmLoadTexture(): got simage for %s",filename));
	goto DONE;
    }
#endif

    __glmWarning("glmLoadTexture() failed: Unable to load texture from %s!", filename);
    DBG_(__glmWarning("glmLoadTexture() failed: tried PPM"));
#ifdef HAVE_LIBJPEG
    DBG_(__glmWarning("glmLoadTexture() failed: tried JPEG"));
#endif
#ifdef HAVE_LIBSDL_IMAGE
    DBG_(__glmWarning("glmLoadTexture() failed: tried SDL_image"));
#endif
    free(filename);
    return 0;

  DONE:
/*#define FORCE_ALPHA*/
#ifdef FORCE_ALPHA
    if(alpha && type == GL_RGB) {
	/* if we really want RGBA */
	const unsigned int size = width * height;
	
	unsigned char *rgbaimage;
	unsigned char *ptri, *ptro;
	int i;
	
	rgbaimage = (unsigned char*)malloc(sizeof(unsigned char)* size * 4);
	ptri = data;
	ptro = rgbaimage;
	for(i=0; i<size; i++) {
	    *(ptro++) = *(ptri++);
	    *(ptro++) = *(ptri++);
	    *(ptro++) = *(ptri++);
	    *(ptro++) = 255;
	}
	free(data);
	data = rgbaimage;
	type = GL_RGBA;
    }
#endif /* FORCE_ALPHA */
    switch(type) {
    case GL_LUMINANCE:
	pixelsize = 1;
	break;
    case GL_RGB:
    case GL_BGR:
	pixelsize = 3;
	break;
    case GL_RGBA:
    case GL_BGRA:
	pixelsize = 4;
	break;
    default:
	__glmFatalError( "glmLoadTexture(): unknown type 0x%x", type);
	pixelsize = 0;
	break;
    }

    if((pixelsize*width) % 4 == 0)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    else
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* scale image to power of 2 in height and width */
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glMaxTexDim);
    if(glMaxTexDim > GLM_MAX_TEXTURE_SIZE)
        glMaxTexDim = GLM_MAX_TEXTURE_SIZE;
    
    if (width <= glMaxTexDim)
	xPow2 = log((double)width) / log(2.0);
    else
	xPow2 = log((double)glMaxTexDim) / log(2.0);

    if (height <= glMaxTexDim)
	yPow2 = log((double)height) / log(2.0);
    else
	yPow2 = log((double)glMaxTexDim) / log(2.0);

    ixPow2 = (int)xPow2;
    iyPow2 = (int)yPow2;

    if (xPow2 != (double)ixPow2)
	ixPow2++;
    if (yPow2 != (double)iyPow2)
	iyPow2++;

    xSize2 = 1 << ixPow2;
    ySize2 = 1 << iyPow2;

#if 0				/* no rescaling */
    xSize2 = width;
    ySize2 = height;
#else
    if((width != xSize2) || (height != ySize2)) {
	rdata = (GLubyte*)malloc(sizeof(GLubyte) * xSize2 * ySize2 * pixelsize);
	if (!rdata)
	    return 0;
       
	retval = gluScaleImage(type, width, height,
		      GL_UNSIGNED_BYTE, data,
		      xSize2, ySize2, GL_UNSIGNED_BYTE,
		      rdata);

	free(data);
	data = rdata;
    }       
#endif

    glGenTextures(1, &tex);		/* Generate texture ID */
    glBindTexture(GL_TEXTURE_2D, tex);
    DBG_(__glmWarning("building texture %d",tex));
   
    if(filtering) {
	filter_min = (mipmaps) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
	filter_mag = GL_LINEAR;
    }
    else {
	filter_min = (mipmaps) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
	filter_mag = GL_NEAREST;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mag);
   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (repeat) ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (repeat) ? GL_REPEAT : GL_CLAMP);
    if(mipmaps) {
	retval = gluBuild2DMipmaps(GL_TEXTURE_2D, type, xSize2, ySize2, type, 
				   GL_UNSIGNED_BYTE, data);
    }
    else {
	glTexImage2D(GL_TEXTURE_2D, 0, type, xSize2, ySize2, 0, type, 
		     GL_UNSIGNED_BYTE, data);
    }
   
   
    /* Clean up and return the texture ID */
    free(data);
    free(filename);
   
    return tex;
}
