//
//  jpegconv.hpp
//
/*
20120710 Created.
*/

#ifndef jpegconv_hpp
#define jpegconv_hpp

#include "dicom.hpp"

#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>
#endif
#ifdef HAVE_LIBJASPER
#include <jasper/jasper.h>
#endif
#ifdef HAVE_LIBOPENJPEG
#include <openjpeg.h>
#endif

#define		Uint8	unsigned char
#define		Uint16	unsigned short
#define		Uint32	unsigned int

#ifdef HAVE_J2K // JPEG 2000 stuff
extern	int	DebugLevel;
#endif //End HAVE_J2K

#ifdef HAVE_LIBJPEG
METHODDEF(void) joutput_message (j_common_ptr cinfo);
METHODDEF(void) jerror_exit (j_common_ptr cinfo);
METHODDEF(void) jdoes_nothing (j_decompress_ptr cinfo);
#else// Use jpeg_encoder.cpp
UINT32 encode_image (UINT8 *input_ptr, UINT8 *output_ptr, UINT32 quality_factor, UINT32 image_format, UINT32 image_width, UINT32 image_height);
#endif
BOOL ToJPG(DICOMDataObject* pDDO, char *filename, int size, int append, int level, int window, unsigned int frame);
#ifdef HAVE_LIBJPEG
BOOL CompressJPEGL(DICOMDataObject* pDDO, int comp = '1', int jpegQuality = 95 );
METHODDEF(boolean) jfill_input_buffer (j_decompress_ptr cinfo);
METHODDEF(void) jskip_input_data (j_decompress_ptr cinfo, long num_bytes);
BOOL DecompressJPEGL(DICOMDataObject* pDDO);
#endif //End HAVE_LIBJEPG
#ifdef HAVE_LIBJASPER
BOOL CompressJPEG2K(DICOMDataObject* pDDO, int j2kQuality);
BOOL DecompressJPEG2K(DICOMDataObject* pDDO);
#endif //End LIBJASPER
#ifdef HAVE_LIBOPENJPEG
void error_callback(const char *msg, void *client_data);
void warning_callback(const char *msg, void *client_data);
void info_callback(const char *msg, void *client_data);
BOOL CompressJPEG2Ko(DICOMDataObject* pDDO, int j2kQuality);
BOOL DecompressJPEG2Ko(DICOMDataObject* pDDO);
#endif //End LIBOPENJPEG


#endif
