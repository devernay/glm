#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/gl.h>
#include "glm.h"
//#define DEBUG
//#define GLDEBUG
#include "glmint.h"


//#undef HAVE_LIBJPEG


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

#ifdef HAVE_LIBJPEG
#include <setjmp.h>
#include <jpeglib.h>

/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a bit more refined than the above, in that we show:
 *   (a) how to modify the JPEG library's standard error-reporting behavior;
 *   (b) how to allocate workspace using the library's memory manager.
 *
 * Just to make this example a little different from the first one, we'll
 * assume that we do not intend to put the whole image into an in-memory
 * buffer, but to send it line-by-line someplace else.  We need a one-
 * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
 * memory manager allocate it for us.  This approach is actually quite useful
 * because we don't need to remember to deallocate the buffer separately: it
 * will go away automatically when the JPEG object is cleaned up.
 */

/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

/*
 * Sample routine for JPEG decompression.  We assume that the source file name
 * is passed in.  We want to return 1 on success, 0 on error.
 */

static GLubyte* 
glmReadJPG(const char* filename, GLboolean alpha, int* w, int* h, int* type)
{
  GLubyte *data;
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my_error_mgr jerr;
  /* More stuff */
  FILE * infile;		/* source file */
  GLubyte *line[16], *ptr;
  int                 y, i, nbuf;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = fopen(filename, "rb")) == NULL) {
      DBG_(__glmWarning("glmReadJPG() failed: can't open %s", filename));
      return NULL;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return NULL;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */
  cinfo.do_fancy_upsampling = FALSE;
  cinfo.do_block_smoothing = FALSE;

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  *w = cinfo.output_width;
  *h = cinfo.output_height;
  data = (GLubyte*)malloc(*w * *h * cinfo.output_components);
  if(data == NULL) {
      jpeg_destroy_decompress(&cinfo);
      return NULL;
  }
  ptr = data;

  nbuf = cinfo.rec_outbuf_height;
  if (nbuf > 16) {
      __glmWarning("glmReadJPG(): JPEG uses line buffers > 16. Cannot load.");
      free(data);
      return NULL;
  }
  ptr = &data[(*h-1) * *w * cinfo.output_components];
  for (y = 0; y < *h; y += nbuf) {
      for (i = 0; i < nbuf; i++) {
	  line[i] = ptr;
	  ptr -= *w * cinfo.output_components;
      }
      jpeg_read_scanlines(&cinfo, line, nbuf);
  }
  switch(cinfo.output_components) {
  case 3:
      *type = GL_RGB;
  case 1:
      *type = GL_LUMINANCE;
      break;
  default:
      __glmWarning("glmReadJPG(): unhandled output_components value: %d", cinfo.output_components);
      free(data);
      return NULL;
  }
  /* Step 7: Finish decompression */
  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose(infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
  return data;
}

/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.doc for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.doc for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 */
#endif /* HAVE_LIBJPEG */

#ifdef HAVE_LIBPNG
static GLubyte* 
glmReadPNG(const char* filename, GLboolean alpha, int* w, int* h, int* type)
{
#warning Image reading with libpng is not yet implemented
    return NULL;
}
#endif	/* HAVE_LIBPNG */

#ifdef HAVE_LIBSDL_IMAGE
#include <SDL/SDL_image.h>

/* following routine is from the SDL documentation */
/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static inline Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

static GLubyte* 
glmReadSDL(const char* filename, GLboolean alpha, int* width, int* height, int* type)
{
    SDL_Surface *img = NULL;
    int dim;
    int pos;
    int x,y;
    GLubyte *data;

    /* Load the 'file' to SDL_Surface */
    DBG_(__glmWarning("loading texture %s",filename));
    img = IMG_Load(filename);
    if(img == NULL) {
	__glmWarning("glmLoadTexture() failed: Unable to load texture from %s!\n%s", filename, IMG_GetError());
        return NULL;
    }

    /* Build the texture from the surface */
    dim = img->w * img->h * ((alpha) ? 4 : 3);
    data = (GLubyte*)malloc(sizeof(GLubyte) * dim);
    if(!data) {
	__glmWarning("glmLoadTexture() failed: Unable to create a texture from %s!", filename);
        return NULL;
    }
    
    /* Traverse trough surface and grab the pixels */
    pos = 0;
    
    for(y=(img->h-1); y>-1; y--) {
	for(x=0; x<img->w; x++) {
	    Uint8 r,g,b,a;
	    Uint32 color = getpixel(img, x,y);
	    
	    if(!alpha)
		SDL_GetRGB(color, img->format, &r,&g,&b);
	    else
		SDL_GetRGBA(color, img->format, &r,&g,&b,&a);

	    data[pos] = r; pos++;
	    data[pos] = g; pos++;
	    data[pos] = b; pos++;
	    if(alpha) {
		data[pos] = a; pos++;
	    }
	}
    }

    *type = (alpha) ? GL_RGBA : GL_RGB;
    *width = img->w;
    *height = img->h;
    SDL_FreeSurface(img);

    return data;
}
#endif

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
