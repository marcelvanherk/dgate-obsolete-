//
//  jpegconv.cpp
//
/*
20120710 bcb Created.
20130218	mvh	JPEG compression did not set right sopclass!!!!
20130226 bcb Replaced gpps with IniValue class, fixed size warnings.
                Version to 1.4.18a and b.
*/

#include "jpegconv.hpp"
#include "nkiqrsop.hpp"
#include "dprintf.hpp"
#include "dbsql.hpp"

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif
#ifndef INT32_MAX
#define INT32_MAX 0x7FFFFFFF
#endif

#ifdef HAVE_J2K
#ifndef HAVE_LIBJPEG// Don't want it twice
//extern int gJpegQuality;// The quality of the lossy jpeg or j2k image from dicom.ini.
#endif
#endif
#ifdef HAVE_BOTH_J2KLIBS
//extern int gUseOpenJpeg;// If we have both Jasper and OpenJPEG, it lets the dicom.ini choose.
#endif

#define CLRSPC_FAM_UNKNOWN 0
#define CLRSPC_FAM_GRAYSCALE 1
#define CLRSPC_FAM_RGB 2
#define CLRSPC_FAM_YBR_FULL 3
//Skipped numbers for Jasper //
#define CLRSPC_FAM_YCBCR_ICT 6
#define CLRSPC_FAM_YCBCR_RCT 7
#define CLRSPC_FAM_YCBCR_F422 8
#define CLRSPC_FAM_YCBCR_422 9
#define CLRSPC_FAM_YCBCR_420 10
// JPEG2000 quality
//#ifndef DEFAULT_LOSSY
//#define DEFAULT_LOSSY "95"
//#endif
//#define DEFAULT_LOSSY_INT 95
#define MIN_QUALITY 1


#ifdef HAVE_LIBJPEG
//extern int gJpegQuality;// The quality of the lossy jpeg or j2k image from dicom.ini.

/* Here's the extended error handler struct for libjpeg:
 (from jpeg example.c)*/
struct jerror_mgr 
{ 
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct jerror_mgr * jerror_ptr;

typedef struct 
{ 
    struct jpeg_source_mgr pub;	/* public fields */
} jsource_mgr;

typedef jsource_mgr * jsrc_ptr;
#endif

#ifdef HAVE_LIBJPEG //The jpeg common stuff.
/* This is a replacement for the libjpeg message printer that normally
 *  goes to stderr. */
METHODDEF(void)
joutput_message (j_common_ptr cinfo)
{ 
    char buffer[JMSG_LENGTH_MAX];
    
    /* Create the message */
    (*cinfo->err->format_message) (cinfo, buffer);
    
    OperatorConsole.printf("***[JPEG Library]: %s\n", buffer);
}

/* This is a replacement for the libjpeg error handler that normally
 *  halts the program. */
METHODDEF(void)
jerror_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    jerror_ptr jerr = (jerror_ptr) cinfo->err;
    
    /* Always display the message */
    (*cinfo->err->output_message) (cinfo);
    
    /* Return control to the setjmp point */
    longjmp(jerr->setjmp_buffer, 1);
}

/* Just for routines we do not need or use. */
METHODDEF(void)
jdoes_nothing (j_decompress_ptr cinfo)
{
    /* Nothing */
    UNUSED_ARGUMENT(cinfo);
    return;
}
#else// Use jpeg_encoder.cpp
UINT32 encode_image (UINT8 *input_ptr, UINT8 *output_ptr, UINT32 quality_factor, UINT32 image_format, UINT32 image_width, UINT32 image_height);
#endif

BOOL ToJPG(DICOMDataObject* pDDO, char *filename, int size, int append, int level, int window, unsigned int frame)
{ int 			dimx, dimy;
    unsigned char 	*out;//, c;
    FILE 			*f;
    int 			i, Ratefps;
#ifdef HAVE_LIBJPEG
    struct                jpeg_compress_struct cinfo;
    unsigned int          rowWidth;
    struct jerror_mgr     jerr;
    register JSAMPROW     row_pointer;
#endif
    
    if (frame>10000 && GetNumberOfFrames(pDDO)>1)
    { char file[256], file2[256];
        DICOMDataObject *DDO2 = MakeCopy(pDDO);
        NewTempFile(file);
        for (i=0; i<GetNumberOfFrames(pDDO); i++)
        { sprintf(file2, "%s.%05d", file, i);
            ToJPG(DDO2, file2, size, 0, level, window, i);
            delete DDO2;
        }
        
        Ratefps = (frame % 100);
        if (Ratefps==0) Ratefps=1;
        
        // to convert a row of jpeg images to mpeg2 using mjpegtools:
        // jpeg2yuv -f 25 -I p -j dgate%03d.jpg | mpeg2enc -f 3 -b 1500 -o mpegfile.m2v
        //          rate  progr files                      mpg2  bitrate   file
        sprintf(file2, "jpeg2yuv -f %d -I p -j %s%%05d | mpeg2enc -f 3 -b 1500 -o %s", Ratefps, file, filename);
        system(file2);
        
        for (i=0; i<GetNumberOfFrames(pDDO); i++)
        { sprintf(file2, "%s.%05d", file, i);
            unlink(file2);
        }
        return TRUE;
    }
    
    if (To8bitMonochromeOrRGB(pDDO, size, &dimx, &dimy, &out, 2, level, window, frame))
    { if (append) f = fopen(filename, "ab");
    else        f = fopen(filename, "wb");
        if (!f)
        { free(out);
            return FALSE;
        }
#ifdef HAVE_LIBJPEG
        
        // Init the default handler.
        cinfo.err = jpeg_std_error(&jerr.pub);
        // change the error exit so libjpeg can't kill us.
        jerr.pub.error_exit = jerror_exit;
        // Use our methode for outputting messages.
        jerr.pub.output_message = joutput_message;
        if (setjmp(jerr.setjmp_buffer))
        {
            /* If we get here, the JPEG code has signaled an error.
             * We need to clean up the JPEG object and return. */
            jpeg_destroy_compress(&cinfo);
            if(f != NULL)
            {
                fclose(f);
                if (!append)unlink(filename);
                free(out);
            }
            return (FALSE);
        }
        //Look for multi-byte version 6c (63) from www.bitsltd.net
        jpeg_CreateCompress(&cinfo, 63, (size_t) sizeof(struct jpeg_compress_struct));
        // Color space must be set before a call to jpeg_set_defaults.
        cinfo.in_color_space = JCS_RGB; //To8bitMonochromeOrRGB is set for rgb, gray could be faster.
        jpeg_set_defaults(&cinfo);
        // Get all the image size stuff.
        cinfo.image_height =  dimy;
        cinfo.image_width =  dimx;
        cinfo.num_components = 3;
        cinfo.input_components = cinfo.num_components;
        cinfo.data_precision = 8;
        cinfo.data_precision_other = cinfo.data_precision;
        jpeg_set_quality(&cinfo, 95, true);//100 is silly in lossy and the same as 95.
        cinfo.jpeg_color_space = cinfo.in_color_space;
        jpeg_default_colorspace(&cinfo);
        // Set where to put it.
        jpeg_stdio_dest(&cinfo, f);
        rowWidth =  cinfo.image_width * cinfo.num_components;
        jpeg_start_compress(&cinfo, TRUE);
        while (cinfo.next_scanline < cinfo.image_height)
        {
            row_pointer = &(((JSAMPROW)out)[cinfo.next_scanline * rowWidth]);
            jpeg_write_scanlines(&cinfo, &row_pointer, 1);
        }
        // If here, finished the compression.
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
#else // Old way.
        unsigned char *output = (UINT8 *)malloc(dimx * dimy * 3);
        UINT32 len;
        len = encode_image (out, output, 256, 14, dimx, dimy);
        fwrite (output, 1, len, f);
        free(output);
#endif
        free(out);
        fclose(f);
        return TRUE;
    }
    
    return FALSE;
}

#ifdef HAVE_LIBJPEG
/* This routine will take all of the listed color spaces in 08_03 of the standard
 *  in little endian, uncompressed format and convert it to YBR_FULL and than compress
 *  it to jpeg.  YBR_FULL is the IJG standard for the Jpeg-6b library used everywhere.
 *  I did not use the Jpeg-6b version because it had to be compiled for 8, 12 or 16
 *  only.  So I wrote Jpeg-6c that can change bit width on the fly.  You can get it here:
 *  http://www.bitsltd.net/Software/Software-Repository/index.php
 *  I included spectral sel "1.2.840.10008.1.2.4.53" and progressive
 *  "1.2.840.10008.1.2.4.55", but both are obsolete and maybe I should remove them.
 *  jpegQuality is meaningless for lossless and  0 for use "iniValuePtr->sscscpPtr->LossyQuality" in IniValue Class.
 *  Maybe one day all of the format decoding should be moved into it's own routine.
 *  If I have made some mistakes (most likely) you can contact me bruce.barton
 *  (the mail symbol goes here) bitsltd.net.  Let me know where I can find a sample of
 *  the image that didn't work. */

BOOL CompressJPEGL(DICOMDataObject* pDDO, int comp, int jpegQuality)
{
    Array < DICOMDataObject* >	*ArrayPtr, *ArrayImage;
    DICOMDataObject		*DDO;
    VR				*pVR, *vrImage, *vrs;
    char				*colorASC, name[256], charY, charCb, charCr, *inData;
    FILE				*fp;
    BOOL				oddRow, buffer, progressive;
    UINT8				colorType;
    UINT16			frames;
    register unsigned int		rowWidth;
    register int			outInt, outInt1;
    size_t			fileLength, readLength;
    int				err, t;
    unsigned int			currFrame, frameSize, inputArrayCnt;
    unsigned int			byteWidthIn, byteWidth;
    register JSAMPROW		jbuffer, jbuffer_ptr, row_pointer[3];
    JSAMPARRAY			jarray;
    struct jpeg_compress_struct	cinfo;
    struct jerror_mgr		jerr;
    JDIMENSION			width, byteCnt;
    
    // If debug > 0, get start time.
	t=0;
    if (DebugLevel > 0)t = (unsigned int)time(NULL);
    if (DebugLevel > 1) SystemDebug.printf("JPEG compress started.\n");
    // Are there frames?
    currFrame = 0;
    inputArrayCnt = 0;
    // Look for the frames vr.
    if(!(frames = pDDO->Getatoi(0x0028, 0x0008))) frames = 1; // de.
    // Check and set the quality for lossy.
    if(jpegQuality < MIN_QUALITY)// Set to 0 to use dicom.ini value. 
    {
        jpegQuality = IniValue::GetInstance()->sscscpPtr->LossyQuality;//Use the default or dicom.ini value.
    }
    
    if(jpegQuality > 100) jpegQuality = 100;
    // Init the default handler.
    cinfo.err = jpeg_std_error(&jerr.pub);
    // change the error exit so libjpeg can't kill us.
    jerr.pub.error_exit = jerror_exit;
    // Use our methode for outputting messages.
    jerr.pub.output_message = joutput_message;
    
    fp = NULL;
    if (setjmp(jerr.setjmp_buffer))
    {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object and return. */
        jpeg_destroy_compress(&cinfo);
        if(fp != NULL)
        {
            fclose(fp);
            unlink(name);
        }
        return (FALSE);
    }
    
    /* Look for multi-byte version 6c (63) from www.bitsltd.net */
    jpeg_CreateCompress(&cinfo, 63,
                        (size_t) sizeof(struct jpeg_compress_struct));
    /* Color space must be set before a call to jpeg_set_defaults. */
    cinfo.in_color_space = JCS_RGB; //Default,just a guess
    jpeg_set_defaults(&cinfo);
    
    /* Get all the image size stuff. */
    // Get the Rows VR and check size
    if(!(cinfo.image_height = (JDIMENSION)pDDO->GetUINT(0x0028, 0x0010)))
    {
        SystemDebug.printf("***[CompressJPEGL]: failed to get image height.\n");
        jpeg_destroy_compress(&cinfo);
        return(FALSE);
    }
    if(cinfo.image_height > JPEG_MAX_DIMENSION)
    {
        OperatorConsole.printf("***[CompressJPEGL]: %d too high for the jpeg standard.\n",
                               cinfo.image_height);
        jpeg_destroy_compress(&cinfo);
        return(FALSE);
    }
    
    // Get the Columns VR and check size.
    if(!(cinfo.image_width = (JDIMENSION)pDDO->GetUINT(0x0028, 0x0011)))
    {
        OperatorConsole.printf("***[CompressJPEGL]: failed to get image width.\n");
        jpeg_destroy_compress(&cinfo);
        return(FALSE);
    }
    if(cinfo.image_width > JPEG_MAX_DIMENSION)
    {
        OperatorConsole.printf("***[CompressJPEGL]: %d too wide for the jpeg standard.\n",
                               cinfo.image_width);
        jpeg_destroy_compress(&cinfo);
        return(FALSE);
    }
    // Get the number of samples per pixel VR.
    if(!(cinfo.num_components = pDDO->GetBYTE(0x0028, 0x0002)))
        cinfo.num_components = 1;  // Gray default.
    cinfo.input_components = cinfo.num_components;
    // Get the number of bits allocated.
    if(!(cinfo.data_precision_other = pDDO->GetBYTE(0x0028, 0x0100)))
        cinfo.data_precision_other = 8; // 8 bit default.
    // Get the number of bits stored.
    if(!(cinfo.data_precision = pDDO->GetBYTE(0x0028, 0x0101)))
        cinfo.data_precision = 8; // 8 bit default.
    if(cinfo.data_precision > cinfo.data_precision_other)
        cinfo.data_precision_other = cinfo.data_precision; /* the bigger one. */
    if(cinfo.data_precision_other != 8 && cinfo.data_precision_other != 12
       && cinfo.data_precision_other != 16)
    {
        jpeg_destroy_compress(&cinfo);
        OperatorConsole.printf("***[CompressJPEGL]: Unsuported allocated bit width: %d.\n",
                               cinfo.data_precision_other);
        return(FALSE);
    }
    byteWidthIn = 1;
    if(cinfo.data_precision_other > 8) byteWidthIn = 2;
    byteWidth = 1;
    if(cinfo.data_precision > 8) byteWidth = 2;
    // Set the size of each image
    frameSize = cinfo.image_width * cinfo.image_height * cinfo.num_components *  byteWidthIn;
    buffer =  FALSE;
    // Set the defaults.
    colorASC = NULL;//Default gray.
    cinfo.in_color_space = JCS_GRAYSCALE;
    colorType = CLRSPC_FAM_GRAYSCALE;
    switch(cinfo.num_components)
    {
        case 1:// Leave the defaults.
            break;
        case 3:
            pVR = pDDO->GetVR(0x0028, 0x0004); // Get the color profile.
            if(pVR && pVR->Length > 2) colorASC = (char *)pVR->Data; //Might be YBR.
            cinfo.in_color_space = JCS_RGB;// Default color space.
            colorType = CLRSPC_FAM_RGB;
            break;
        default:
            jpeg_destroy_compress(&cinfo);
            OperatorConsole.printf("***[CompressJPEGL]: Unsuported number of components: %d.\n",
                                   cinfo.num_components );
            return(FALSE);
    }
    // Other color types can be added.
    if (colorType == CLRSPC_FAM_GRAYSCALE)
    {// Leave defaults
#if NATIVE_ENDIAN == LITTLE_ENDIAN //Little Endian
        if(cinfo.data_precision_other == 12) // Byte fix time
#else//Big Endian like Apple power pc.
            if(byteWidthIn == 2) // Byte swap time
#endif //Big Endian
                buffer = TRUE;
    }
    else if (cinfo.in_color_space == JCS_RGB &&
             (colorASC == NULL || strncmp(colorASC, "RGB",3)==0))
    {// Any problems should pick here.  Leave color defaults.
        // Planar configuration
        if(pDDO->GetBYTE(0x0028, 0x0006) == 1) buffer = TRUE;
    }
    else if (pVR->Length > 6 && strncmp(colorASC, "YBR_",4)==0)
    {
        if(strncmp(&colorASC[4], "ICT", 3)==0 || strncmp(&colorASC[4], "RCT", 3)==0)
        {
            OperatorConsole.printf
            ("Warn[CompressJPEGL]: Uncompressed colorspace can not be YBR_ICT or YBR_RCT. Trying RGB\n");
            cinfo.in_color_space = JCS_RGB;
            colorType = CLRSPC_FAM_RGB;
        }
        else if (pVR->Length > 7 && strncmp(&colorASC[4], "FULL", 4)==0)//YBR_FULL(_422)
        {
            cinfo.in_color_space = JCS_YCbCr;
            buffer = TRUE;
            if(pVR->Length > 11 && (strncmp(&colorASC[8], "_422", 4)==0))//YBR_FULL_422
                colorType = CLRSPC_FAM_YCBCR_F422;
            else // YBR_FULL
                colorType = CLRSPC_FAM_YBR_FULL;
        }
        else if (pVR->Length > 14 && strncmp(&colorASC[4], "PARTIAL_42", 10)==0)//YBR_PARTIAL_42x
        {
            cinfo.in_color_space = JCS_YCbCr;
            buffer = TRUE;
            if(colorASC[14] == '0')colorType = CLRSPC_FAM_YCBCR_420;
            else colorType = CLRSPC_FAM_YCBCR_422;
        }
    } // End if YBR_
    else
    {
        if((colorASC = pDDO->GetCString(0x0028, 0x0004)))
        {
            OperatorConsole.printf(
                                   "***[CompressJPEGL]: Unknown or unsuported color space %s.\n",colorASC);
            free(colorASC);
        }
        else OperatorConsole.printf(
                                    "***[CompressJPEGL]: Unknown or unsuported color space record.\n");
        jpeg_destroy_compress(&cinfo);
        return(FALSE);
    }
    
    // Get the data.
    pVR = pDDO->GetVR(0x7fe0, 0x0010); // Get the Image VR.
    vrImage = pVR;// Will test for null later.
    ArrayImage = NULL;// For warning.
    if(pVR && pVR->Length && pVR->SQObjectArray)
    {//This should not be for uncompressed.
        ArrayImage = (Array<DICOMDataObject *> *) pVR->SQObjectArray;
        while (inputArrayCnt < ArrayImage->GetSize())
        {
            DDO = ArrayImage->Get(inputArrayCnt++);//Get the array.
            vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data.
            if(vrImage && vrImage->Length >= frameSize)
            {
                OperatorConsole.printf("Warn[CompressJPEGL]:Raw image data in an encapsulation array.\n");
                break;
            }
        }
    }
    if(!vrImage || vrImage->Length < frameSize)// Have enough data for at least 1 frame?
    {
        OperatorConsole.printf("***[CompressJPEGL]: Could not find the image data.\n");
        jpeg_destroy_compress(&cinfo);
        return (FALSE);
    }
    // Use a temp file to hold the data, easier to get the compressed size(s)
    NewTempFileWExt(name, ".jpg");
    if((fp = fopen(name, "wb+")) == NULL )
    {
        OperatorConsole.printf("***[CompressJPEGL]: Could not open file %s for write.\n", name);
        jpeg_destroy_compress(&cinfo);
        return (FALSE);
    }
    jarray = &jbuffer;
    jbuffer = NULL; // Default is no buffer.
    // Any compression errors from here on are handled by jerror_exit.
    
    /* Check to see if we need a buffer, data in planes or 12 bits storage.
     *  We will let libjpeg allocate it, so if any errors it will go away. */
    if(buffer) jarray = (cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_PERMANENT,
         (JDIMENSION)( cinfo.image_width * cinfo.input_components
                      * byteWidthIn ) + 6, 1);//Add a little extra.
    jbuffer = *jarray;//jarray is an array of rows. We use just one.
    
    progressive = FALSE;
    switch(comp)
    {
        case '2':// lossless SV6 "1.2.840.10008.1.2.4.57" = jpeg 14 (33%)
            jpegQuality = 100;
            if(cinfo.data_precision > 8)cinfo.lossless = TRUE;
            comp = 2;
            break;
        case '3':// baseline   (8 bits ) "1.2.840.10008.1.2.4.50" = jpeg 1  (15%)
        case '4':// extended   (12 bits) "1.2.840.10008.1.2.4.51" = jpeg2, 4  (15%)
            if(cinfo.data_precision <= 12)// Else fall though 5 and 6 to 1.
            {
                if(cinfo.data_precision > 8)
                {
                    cinfo.data_precision = 12;
                    jpeg_set_quality(&cinfo, jpegQuality, false);
                    comp = 4;
                }
                else
                {
                    cinfo.data_precision = 8;
                    jpeg_set_quality(&cinfo, jpegQuality, true);
                    comp = 3;
                }
                break;
            }
        case '5': // spectral sel 	"1.2.840.10008.1.2.4.53" = jpeg 8  (15%) Obsolete!
        case '6': // progressive		"1.2.840.10008.1.2.4.55" = jpeg 12 (14%) Obsolete!
            if(cinfo.data_precision <= 12)// Else fall though.
            {
                progressive = TRUE;
                if(cinfo.data_precision > 8)
                {
                    cinfo.data_precision = 12;
                    comp = 6;
                }
                else
                {
                    cinfo.data_precision = 8;
                    comp = 5;
                }
                jpeg_set_quality(&cinfo, jpegQuality, false);
                jpeg_simple_progression(&cinfo);
                break;
            }
        default: //default lossless SV1 "1.2.840.10008.1.2.4.70" = jpeg 14 (33%)
            //16 bit jpeg id always lossless.
            comp = 1;
            jpegQuality = 100;
            if(cinfo.data_precision > 8)cinfo.lossless = TRUE;// Tell 8 bit after colorspace
    }
    // Setup for compress.
    cinfo.jpeg_color_space = cinfo.in_color_space;
    jpeg_default_colorspace(&cinfo);
    // lossless must be set after colorspace, but color space need to know lossless if not 8 bits.
    if(comp <= 2)// Lossless J1 or J2.
    {
        cinfo.lossless = TRUE;
        cinfo.lossless_scaling = FALSE;
        if(comp == 2)jpeg_simple_lossless(&cinfo, 6, 0);//(J2)Predictor = 6, point_transform = 0
        else jpeg_simple_lossless(&cinfo, 1, 0);//(J1)Predictor = 1, point_transform = 0
    }
    //  Set the obsolete progressive if called for.
    if(progressive)jpeg_simple_progression(&cinfo);
    // Print out some info for debug.
    if (DebugLevel > 2)
    {
        if(cinfo.lossless)SystemDebug.printf("JPEG Lossless");
        else SystemDebug.printf("JPEG Lossy Quality = %d", jpegQuality);
        if (DebugLevel > 3)
        {
            if(progressive)SystemDebug.printf(", progressive");
            if(!buffer)SystemDebug.printf(", unbuffered data");
            else SystemDebug.printf(", buffered data");
        }
        SystemDebug.printf(
                           "\n, H = %d, W = %d, Bits = %d in %d, Frames = %d, ",
                           cinfo.image_width, cinfo.image_height, cinfo.data_precision,
                           cinfo.data_precision_other, frames);
        if((colorASC = pDDO->GetCString(0x0028, 0x0004)))
        {
            SystemDebug.printf("color = %s\n", colorASC);
            free(colorASC);
        }
        else SystemDebug.printf("Unknown color space record.\n");
    }
    // Create the encapsulation array.
    ArrayPtr = new Array < DICOMDataObject * >;
    // The first blank object.
    DDO = new DICOMDataObject;
    vrs = new VR(0xfffe, 0xe000, 0, (void *)NULL, FALSE);
    DDO->Push(vrs);
    ArrayPtr->Add(DDO);
    // Start the frames loop
    while(TRUE)
    {
        //Start of the frame
        if( inputArrayCnt == 0)
        {
            inData = (char *)vrImage->Data + (currFrame * frameSize);
            if((++currFrame * frameSize) > vrImage->Length)
            {
                OperatorConsole.printf(
                                       "Warn[CompressJPEGL]: Ran out of image data on frame %d of %d.\n", ++currFrame, frames);
                break;
            }
        }
        else inData = (char *)vrImage->Data;// input array, a strange world.
        // Set where to put it.
        jpeg_stdio_dest(&cinfo, fp);
        // Get the library ready for data.
        jpeg_start_compress(&cinfo, TRUE);
        /* To send the data there are many routines used.  It is done this way for speed rather than code size.
         *  I try to not have any 'if' statements in the loops.  */
        if(!buffer)// Easy way, just send.
        {
            rowWidth =  cinfo.image_width * cinfo.num_components *  byteWidthIn;
            while (cinfo.next_scanline < cinfo.image_height)
            {
                row_pointer[0] = &(((JSAMPROW)inData)[cinfo.next_scanline * rowWidth]);
                jpeg_write_scanlines(&cinfo, &row_pointer[0], 1);
            }
        }
        else // Buffered, copy time.
        {
            row_pointer[0] = (JSAMPROW)inData;
            
            switch (colorType)
            {
                case JCS_GRAYSCALE:
#if NATIVE_ENDIAN == BIG_ENDIAN // Big Endian like Apple power pc
                    if(cinfo.data_precision_other == 16) // 16 Bits, byte swap time.
                    {
                        while (cinfo.next_scanline < cinfo.image_height)
                        {
                            jbuffer_ptr =jbuffer;
                            for(byteCnt = 0;byteCnt < cinfo.image_width; byteCnt++)
                            {
                                *jbuffer_ptr++ = row_pointer[0][1];
                                *jbuffer_ptr++ = *row_pointer[0]++;
                                row_pointer[0]++;
                            }
                            while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                        }
                    }
                    else //Buffer, no planes == 12 Bits allocated.
                    {
#endif
                        // Each loop gets two words.
                        cinfo.data_precision_other = 16;
                        oddRow =  FALSE;
                        if(cinfo.image_width & 1) oddRow = TRUE;
                        rowWidth =  cinfo.image_width >> 1;
                        if(oddRow) rowWidth++;// Get the odd bit.
                        while (cinfo.next_scanline < cinfo.image_height)
                        {
                            jbuffer_ptr =jbuffer;
                            for(byteCnt = 0;byteCnt < rowWidth; byteCnt++)
                            {
#if NATIVE_ENDIAN == BIG_ENDIAN // Big Endian like Apple power pc
                                *jbuffer_ptr = (row_pointer[0][1] & 0xF0) >> 4;
                                *jbuffer_ptr++ |= (*row_pointer[0] & 0x0F) << 4;
                                *jbuffer_ptr++ = (*row_pointer[0]++ & 0xF0) >> 4;
                                *jbuffer_ptr++ = row_pointer[0][1];
                                *jbuffer_ptr++ = (*row_pointer[0]++) & 0x0F;
                                row_pointer[0]++;
#else //Little Endian
                                *jbuffer_ptr++ = (*row_pointer[0] & 0xF0) >> 4;
                                *jbuffer_ptr = (*row_pointer[0]++ & 0x0F) << 4;
                                *jbuffer_ptr++ |= (*row_pointer[0] & 0xF0) >> 4;
                                *jbuffer_ptr++ = (*row_pointer[0]++) & 0x0F;
                                *jbuffer_ptr++ = *row_pointer[0]++;
#endif //Little Endian
                            }                
                            while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                            if(oddRow && (cinfo.next_scanline < cinfo.image_height))
                            {
                                jbuffer_ptr -= 2; //Back up over the second half of the odd.
                                for(byteCnt = 0; byteCnt < 2; byteCnt++)
                                    jbuffer[byteCnt] = *jbuffer_ptr++;
                                jbuffer_ptr = jbuffer + 2;//Reset the buffer pointer.
                                for(byteCnt = 2 ;byteCnt < rowWidth; byteCnt++)
                                {
#if NATIVE_ENDIAN == BIG_ENDIAN // Big Endian like Apple power pc
                                    *jbuffer_ptr++ = row_pointer[0][1];
                                    *jbuffer_ptr++ = (*row_pointer[0]++) & 0x0F;
                                    row_pointer[0]++;
                                    *jbuffer_ptr = (row_pointer[0][1] & 0xF0) >> 4;
                                    *jbuffer_ptr++ |= (*row_pointer[0] & 0x0F) << 4;
                                    *jbuffer_ptr++ = (*row_pointer[0]++ & 0xF0) >> 4;
#else //Little Endian
                                    *jbuffer_ptr++ = (*row_pointer[0]++) & 0x0F;
                                    *jbuffer_ptr++ = *row_pointer[0]++;
                                    *jbuffer_ptr++ = (*row_pointer[0] & 0xF0) >> 4;
                                    *jbuffer_ptr = (*row_pointer[0]++ & 0x0F) << 4;
                                    *jbuffer_ptr++ |= (*row_pointer[0] & 0xF0) >> 4;
#endif //Little Endian
                                }                
                                while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                            }
                        }
#if NATIVE_ENDIAN == BIG_ENDIAN // Big Endian like Apple power pc
                    } // 16 bit else in BIG_ENDIAN
#endif
                    break;
                case JCS_RGB:// 8 bit, Planes.  Regular RGB has no buffer.
                case JCS_YCbCr:
                    row_pointer[1] = row_pointer[0] + (cinfo.image_width * cinfo.image_height);
                    row_pointer[2] = row_pointer[1] + (cinfo.image_width * cinfo.image_height);
                    while (cinfo.next_scanline < cinfo.image_height)
                    {
                        jbuffer_ptr = jbuffer;
                        for(byteCnt = 0;byteCnt < cinfo.image_width; byteCnt ++)
                        {
                            *jbuffer_ptr++ = *row_pointer[0]++;
                            *jbuffer_ptr++ = *row_pointer[1]++;
                            *jbuffer_ptr++ = *row_pointer[2]++;
                        }                
                        while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                    }
                    break;
                case CLRSPC_FAM_YCBCR_F422: // Convert to "YBR_FULL"
                    width = cinfo.image_width > 1; // Done at the Cr,Cb width.
                    while (cinfo.next_scanline < cinfo.image_height)
                    {
                        jbuffer_ptr = jbuffer;
                        for(byteCnt = 0;byteCnt < width; byteCnt ++)
                        {
                            *jbuffer_ptr++ = *row_pointer[0]++;//Y1
                            charY = *row_pointer[0]++;//Y2
                            *jbuffer_ptr++ = charCb = *row_pointer[0]++;// Get the colors twice
                            *jbuffer_ptr++ = charCr = *row_pointer[0]++;
                            *jbuffer_ptr++ = charY;
                            *jbuffer_ptr++ = charCb;
                            *jbuffer_ptr++ = charCr;
                        }                
                        while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                    }
                    break;
                case CLRSPC_FAM_YCBCR_422:
                    width = cinfo.image_width > 1; // Done at the Cr,Cb width.
                    while (cinfo.next_scanline < cinfo.image_height)
                    {
                        jbuffer_ptr = jbuffer;
                        for(byteCnt = 0;byteCnt < width; byteCnt ++)
                        {
                            /* For Y the ratio is 256 / 220 (1.1636) and  for color 256 / 225 (1.1378).
                             *  Note: All values are multiplied by 1024 or 1 << 7.
                             *  Yo = 1.1636(Yi - 16)  == Yo = 1.1636Yi - 18.204 ==
                             *  Yo = [149Yi - 2330]/128 */
                            outInt = (((int)(*row_pointer[0]++)) * 149)
                            - 2330;//Y1
                            if(outInt & 0x10000)*jbuffer_ptr++ = 0;// Neg not allowed here.
                            else if(outInt & 0x8000)*jbuffer_ptr++ = 0xFF;// Over flow.
                            else *jbuffer_ptr++ = (char)((outInt >> 7) & 0xFF);
                            outInt = (((int)(*row_pointer[0]++)) * 149)
                            - 2330;//Y2
                            /*  Cxo = 1.1378(Cxi - 16)  == Cxo = 1.1378Cxi - 18.205 ==
                             *  Cxo = [73Cxi - 1152]/64 */
                            outInt1 = (((int)(*row_pointer[0]++)) * 73)
                            - 1152;//Cb
                            if(outInt1 & 0x8000) charCb = 0;// Neg not allowed here.
                            else if(outInt1 & 0x4000)charCb = 0xFF;// Over flow.
                            else charCb = (char)((outInt >> 6) & 0xFF);
                            outInt1 = ((((int)(*row_pointer[0]++)) >> 21) * 2440246)
                            - 37129657;//Cr
                            if(outInt1 & 0x8000) charCr = 0;// Neg not allowed here.
                            else if(outInt1 & 0x4000)charCr = 0xFF;// Over flow.
                            else charCr = (char)((outInt >> 6) & 0xFF);
                            *jbuffer_ptr++ = charCr;
                            // Put Y2 and Cb, Cr again.
                            if(outInt & 0x10000)*jbuffer_ptr++ = 0;// Neg not allowed here.
                            else if(outInt & 0x8000)*jbuffer_ptr++ = 0xFF;// Over flow.
                            else *jbuffer_ptr++ = (char)((outInt >> 7) & 0xFF);
                            *jbuffer_ptr++ = charCb;
                            *jbuffer_ptr++ = charCr;
                        }                
                        while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                    }
                    break;
                case CLRSPC_FAM_YCBCR_420:
                    width = cinfo.image_width > 1; // Done at the Cr,Cb width.
                    while (cinfo.next_scanline < cinfo.image_height)
                    {
                        jbuffer_ptr = jbuffer;
                        row_pointer[1] = row_pointer[0];// Set last row
                        for(byteCnt = 0;byteCnt < width; byteCnt ++)// First row with color
                        {
                            /* For Y the ratio is 256 / 220 (1.1636) and  for color 256 / 225 (1.1378).
                             *  Note: All values are multiplied by 1024 or 1 << 7.
                             *  Yo = 1.1636(Yi - 16)  == Yo = 1.1636Yi - 18.204 ==
                             *  Yo = [149Yi - 2330]/128 */
                            outInt = (((int)(*row_pointer[0]++)) * 149)
                            - 2330;//Y1
                            if(outInt & 0x10000)*jbuffer_ptr++ = 0;// Neg not allowed here.
                            else if(outInt & 0x8000)*jbuffer_ptr++ = 0xFF;// Over flow.
                            else *jbuffer_ptr++ = (char)((outInt >> 7) & 0xFF);
                            /*  Cxo = 1.1378(Cxi - 16)  == Cxo = 1.1378Cxi - 18.205 ==
                             *  Cxo = [73Cxi - 1152]/64 */
                            outInt1 = (((int)(*row_pointer[0]++)) * 73)
                            - 1152;//Cb
                            if(outInt1 & 0x8000) charCb = 0;// Neg not allowed here.
                            else if(outInt1 & 0x4000)charCb = 0xFF;// Over flow.
                            else charCb = (char)((outInt >> 6) & 0xFF);
                            *jbuffer_ptr++ = charCb;
                            outInt1 = (((int)(*row_pointer[0]++)) * 73)
                            - 1152;//Cr
                            if(outInt1 & 0x8000) charCr = 0;// Neg not allowed here.
                            else if(outInt1 & 0x4000)charCr = 0xFF;// Over flow.
                            else charCr = (char)((outInt >> 6) & 0xFF);
                            *jbuffer_ptr++ = charCr;
                            // Put Y2 and Cb, Cr again.
                            outInt = (((int)(*row_pointer[0]++)) * 149)
                            - 2330;//Y2
                            if(outInt & 0x10000)*jbuffer_ptr++ = 0;// Neg not allowed here.
                            else if(outInt & 0x8000)*jbuffer_ptr++ = 0xFF;// Over flow.
                            else *jbuffer_ptr++ = (char)((outInt >> 7) & 0xFF);
                            *jbuffer_ptr++ = charCb;
                            *jbuffer_ptr++ = charCr;
                        }                
                        while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                        jbuffer_ptr = jbuffer;
                        for(byteCnt = 0;byteCnt < width; byteCnt ++)// 2nd row without color.
                        {
                            outInt = (((int)(*row_pointer[0]++)) * 149)
                            - 2330;//Y1
                            if(outInt & 0x10000)*jbuffer_ptr++ = 0;// Neg not allowed here.
                            else if(outInt & 0x8000)*jbuffer_ptr++ = 0xFF;// Over flow.
                            else *jbuffer_ptr++ = (char)((outInt >> 7) & 0xFF);
                            jbuffer_ptr++;// Reusing the buffer let us skip over
                            jbuffer_ptr++;// the old values and use them again.
                            outInt = (((int)(*row_pointer[0]++)) * 149)
                            - 2330;//Y2
                            if(outInt & 0x10000)*jbuffer_ptr++ = 0;// Neg not allowed here.
                            else if(outInt & 0x8000)*jbuffer_ptr++ = 0xFF;// Over flow.
                            else *jbuffer_ptr++ = (char)((outInt >> 7) & 0xFF);
                            jbuffer_ptr++;// Reusing the buffer let us skip over
                            jbuffer_ptr++;// the old values and use them again.
                        }                
                        while( 1 != jpeg_write_scanlines(&cinfo, jarray, 1));// Not 1, try again.
                    }
                    break;
                default:// Should never get here!  Do nothing
                    break;
			}
        }
        
        // If here, finished the compression.
        jpeg_finish_compress(&cinfo);
        // Time to handle errors on our own again, the file is still open and cinfo still exists.
        if(!fp)
        {
            OperatorConsole.printf("***[CompressJPEGL]: jpeglib distroyed file %s\n", name);
            jpeg_destroy_compress(&cinfo);
            return ( FALSE );
        }
        fseek(fp, 0, SEEK_END);
        fileLength = ftell(fp);
        if(-1 == (signed int)fileLength)
        {
            OperatorConsole.printf("***[CompressJPEGL]: Could not get file size for %s\n", name);
            fclose(fp);
            unlink(name);
            jpeg_destroy_compress(&cinfo);
            return ( FALSE );
        }
        rewind(fp);
        
        // Jpeg is encapsulated, make a new vr to encapsulate.
        vrs = new VR(0xfffe, 0xe000, 0, (void *)NULL, FALSE);
        if(!vrs->ReAlloc((UINT32)(fileLength & INT32_MAX)))//Odd file length, ReAlloc will make it even.
        {
            OperatorConsole.printf("***[CompressJPEGL]: Failed to allocate memory.\n");
            fclose(fp);
            unlink(name);
            jpeg_destroy_compress(&cinfo);
            return ( FALSE );
        }
        //Zero the added byte, never hurts before the read.
        ((BYTE *)vrs->Data)[(vrs->Length) - 1] = 0;
        //Read the image data.
        readLength = fread(vrs->Data,1,fileLength,fp);
        if(readLength == 0)
        {
            err = ferror(fp);
            if(err) OperatorConsole.printf("***[CompressJPEGL]: File read error %d on %s.\n",err,name);
            else    OperatorConsole.printf("***[CompressJPEGL]: No compressed image data (0 length read).\n");
            fclose(fp);
            unlink(name);
            jpeg_destroy_compress(&cinfo);
            return ( FALSE );
        }
        if(readLength != fileLength)
        {
            OperatorConsole.printf("Warn[CompressJPEGL]: Only read %d bytes of %d of the jpeg2k file:%s, will try to save\n",
                                   readLength, fileLength, name);
        }
        fclose(fp);//File done
        // Encapsulate an image object.       
        DDO = new DICOMDataObject;        
        DDO->Push(vrs);
        ArrayPtr->Add(DDO);
        if(currFrame >= frames)break;//Finished while(TRUE) loop
        //Still here, setup for the next frame
        if((fp = fopen(name, "wb+")) == NULL )//Clear the file.
        {
            OperatorConsole.printf("***[CompressJPEGL]: Could not open file %s for write of frame %d.\n", name, currFrame + 1);
            jpeg_destroy_compress(&cinfo);
            return (FALSE);
        }
        // Deal with silly input arrays
        if(inputArrayCnt > 0)
        {
            // All in one array would be nice.
            if((++currFrame * frameSize) > vrImage->Length)
            {// Look for the next array
                while (inputArrayCnt < ArrayImage->GetSize())
                {
                    DDO = ArrayImage->Get(inputArrayCnt++);//Get the array.
                    vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
                    if(vrImage && vrImage->Length >= frameSize) break;// Found next one, break inner loop.
                }
            }
            if(!vrImage || vrImage->Length < frameSize) break;// Not enough data for at least 1 frame.
        }
    }//End of while(TRUE), go back for the next frame.
    unlink(name);//Done with the file.
    // All frames compressed, done with cinfo.
    jpeg_destroy_compress(&cinfo);
    // Should we kill it and keep the uncompressed data?
    if(currFrame < frames) OperatorConsole.printf(
                                                  "Warn[CompressJPEGL]: Only %d of %d frames saved.\n",currFrame, frames);
    // The end object.
    DDO = new DICOMDataObject;
    vrs = new VR(0xfffe, 0xe0dd, 0, (void *)NULL, FALSE);
    DDO->Push(vrs);
    ArrayPtr->Add(DDO);
    //  vrs = new VR(0x7fe0, 0x0010, 0, (void *)NULL, FALSE);
    pVR->Reset();//Clear the old image data including arrays.
    pVR->SQObjectArray = ArrayPtr;// Replace the data
    if(byteWidth > 1)pVR->TypeCode ='OW';
    else pVR->TypeCode ='OB';
    // Change the transfer syntax to JPEG!
    // Change the dicom parameters
    
    // Fix the bits allocated.
    // 20120624: do not change highbit and bitsstored
    //if(byteWidth == 1)pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)8, 'US');
    //else pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)16, 'US');
    
    //Change the transfer syntax to JPEG!  Many choices, do it old way.
    pVR = pDDO->GetVR( 0x0002, 0x0010 );
    if(pVR)
    {
        if(pVR->Length != 22)pVR->ReAlloc(22);
    }
    else 
    {
        pVR = new VR( 0x0002, 0x0010, 22, TRUE);
        pVR->TypeCode = 'IU';
        pDDO->Push(pVR);
    }
    memcpy((char *)pVR->Data, "1.2.840.10008.1.2.4.", 20);
    switch(comp)// here was a big bug, used e.g. '2' instead of 2
    {
        case 2: memcpy(&((char *)pVR->Data)[20], "57", 2);//Lossless J2, SV6
            break;
        case 3: memcpy(&((char *)pVR->Data)[20], "50", 2);//Lossy baseline (8 bits)
            break;
        case 4: memcpy(&((char *)pVR->Data)[20], "51", 2);//Lossy extended (12 bits)
            break;
        case 5: memcpy(&((char *)pVR->Data)[20], "53", 2);// Obsolete! Progressive (8 bits)
            break;
        case 6: memcpy(&((char *)pVR->Data)[20], "55", 2);// Obsolete! Progressive (12 bits)
            break;
        default:  memcpy(&((char *)pVR->Data)[20], "70", 2);//Lossless J1, SV1
    }
    //Change the Photometric Interpretation if needed.
    if(colorType >= CLRSPC_FAM_YCBCR_422)//Color
    {
        // Reset the plane's VR, if there.
        pVR = pDDO->GetVR(0x0028, 0x0006);
        if(pVR && pVR->Length && *(char *)(pVR->Data) == 1) *(char *)(pVR->Data) = 0;
        // Set the color profile
        pDDO->ChangeVR( 0x0028, 0x0004, "YBR_FULL\0", 'CS');// Jpeg standard
    }
    if (DebugLevel > 0)
		SystemDebug.printf("JPEG compress time %u seconds.\n", (unsigned int)time(NULL) - t);
    return (TRUE);
}

/* This is a replacement for the get more data for the buffer routine.
 *  Because it is passed the whole image, it should never ask for more. */
METHODDEF(boolean)
jfill_input_buffer (j_decompress_ptr cinfo)
{
    /* Passed libjpeg the whole image, there is no more data, error exit */
    OperatorConsole.printf("***[LIBJPEG]: Ran out of data in image.\n");
    ERREXIT(cinfo, JERR_INPUT_EOF);
    
    return TRUE;
}

/* This was taken directly from the library with the error message changed. */
METHODDEF(void)
jskip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    jpeg_source_mgr *src = cinfo->src;
    
    /* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
     * any trouble anyway --- large skips are infrequent.
     */
    if (num_bytes > 0)
        if (num_bytes > (long) src->bytes_in_buffer)
        {
            OperatorConsole.printf("***[LIBJPEG]: Tried to skip past the end of the image.\n");
            ERREXIT(cinfo, JERR_INPUT_EOF);
        }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
}

/* This routine will take in a jpeg image and convert it to little endian, uncompressed,
 *  RGB or YBR_FULL format.  RGB and YBR_FULL is the IJG standard output for the Jpeg-6b
 *  library used everywhere.  As stated before, I did not use the Jpeg-6b version because
 *  it had to be compiled for 8, 12 or 16 only.  So I wrote Jpeg-6c that can change bit
 *  width on the fly.  You can get it here:
 *  http://www.bitsltd.net/Software/Software-Repository/index.php
 *  If I have made some mistakes (most likely) you can contact me bruce.barton
 *  (the mail symbol goes here) bitsltd.net.  Let me know where I can find a sample of
 *  the image that didn't work. */
BOOL DecompressJPEGL(DICOMDataObject* pDDO)
{
    Array < DICOMDataObject	*>	*ArrayPtr;
    DICOMDataObject             *DDO;
    VR                          *pVR, *vrImage;
    char                        *uncompImagPtr, *uncompImageBptr;
    char                        *outImageBptr;
    UINT16                      frames;
    unsigned int                outBytes, currFrame, currSQObject;
    size_t                      imageLen;
    BOOL                        color, leExplict;
    int                         t;
    // libjpeg stuff
    JSAMPARRAY16                    outBuffer;
    JDIMENSION                      num_scanlines, row, rowWidth, pixcnt;
    struct jpeg_decompress_struct   cinfo;
    struct jerror_mgr               jerr;
	
    // If debug > 0, get start time.
    t = 0;
    if (DebugLevel > 0)t = (unsigned int)time(NULL);
    if (DebugLevel > 1) SystemDebug.printf("JPEG decompress started.\n");
    leExplict = TRUE; // Only changed by FUJI_FIX
    uncompImagPtr = NULL;//Output data memory.
    color = FALSE;
    // Are there frames?
    if(!(frames = pDDO->Getatoi(0x0028, 0x0008))) frames = 1;
    currFrame = 0;
    // Get the encapsulated data, unless Fuji.
    pVR = pDDO->GetVR(0x7fe0, 0x0010); // Get the Image VR.
    if(!pVR)
    {
        OperatorConsole.printf("***[DecompressJPEGL]: No image VR\n");
        return (FALSE);
    }
    currSQObject = 0;// Init now for no warning later.
    vrImage = pVR;// Init now for no warning later, will use if Fuji fix.
    ArrayPtr = (Array<DICOMDataObject *> *) pVR->SQObjectArray;
    if(ArrayPtr)
    {
        while(TRUE)
        {
            if( currSQObject >= ArrayPtr->GetSize())
            {
                OperatorConsole.printf("***[DecompressJPEGL]: No jpeg data found\n");
                return (FALSE);
            }
            DDO = ArrayPtr->Get(currSQObject );//Get the array.
            vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
            //Look for size and jpeg SOI marker 
            if((vrImage->Length) && ((unsigned char *)vrImage->Data)[0] == 0xFF &&
			   ((unsigned char *)vrImage->Data)[1] == 0xD8)break;
            currSQObject++;
        }
    }
    else
    {
#ifdef FUJI_FIX
        // Look for a Jfif header. (LittleEndianImplicit, jpeg compressed,not encapsulated, how evil)
        if (((unsigned char *)pVR->Data)[0] == 0xFF && ((unsigned char *)pVR->Data)[1] == 0xD8 && ((unsigned char *)pVR->Data)[2] == 0xFF
            && ((unsigned char *)pVR->Data)[3] == 0xE0 && ((char *)pVR->Data)[4] == 0x00 && ((char *)pVR->Data)[5] == 0x10
            && strncmp(&(((char *)pVR->Data)[6]),"JFIF", 4) == 0)
        {
            SystemDebug.printf("Warn[DecompressJPEGL]: Applying Fuji Fix\n");
            leExplict = FALSE;
        }
        else
        {
#else
            OperatorConsole.printf("***[DecompressJPEGL]: No image VR array\n");
            return (FALSE);
#endif
#ifdef FUJI_FIX
        }
#endif
    }
    // Init the default handler
    cinfo.err = jpeg_std_error(&jerr.pub);
    // change the error exit so libjpeg can't kill us
    jerr.pub.error_exit = jerror_exit;
    // Use our methode for outputting messages.
    jerr.pub.output_message = joutput_message;
    
    if (setjmp(jerr.setjmp_buffer))
    {
        // If we get here, the JPEG code has signaled an error.
        // We need to clean up the JPEG object and return.
        jpeg_destroy_decompress(&cinfo);
        if(uncompImagPtr != NULL) free(uncompImagPtr);
        return (FALSE);
    }
    
    // Look for multi-byte version 6c (63) from www.bitsltd.net
    jpeg_CreateDecompress(&cinfo, 63,
                          (size_t) sizeof(struct jpeg_decompress_struct));
    // Set the source
    cinfo.src = (struct jpeg_source_mgr *)
    (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
                               sizeof(struct jpeg_source_mgr));
    cinfo.src->init_source = jdoes_nothing;
    cinfo.src->skip_input_data = jskip_input_data;
    cinfo.src->resync_to_restart = jpeg_resync_to_restart; // use default method
    cinfo.src->term_source = jdoes_nothing; /* term_source does nothing
                                             // Pass the whole image, so this is an error that should not happen. */
    cinfo.src->fill_input_buffer = jfill_input_buffer;
    // Get data for the first or only frame
    cinfo.src->bytes_in_buffer = vrImage->Length; // forces fill_input_buffer on first read
    cinfo.src->next_input_byte = (JOCTET *)vrImage->Data; // Image buffer
    // find out what we have.
    jpeg_read_header(&cinfo, TRUE);
    // Set all of the cinfo information.
    jpeg_calc_output_dimensions(&cinfo);
    // The default is 8 bits, set it to the image size.
    cinfo.data_precision_other =cinfo.data_precision;
    if (cinfo.out_color_components != 1)
    {
        if(cinfo.out_color_components!=3)
        {
            OperatorConsole.printf(
                                   "***[DecompressJPEGL]: Not grayscale, RGB or YCbCr. Number of components = %d\n",
                                   cinfo.out_color_components);
            jpeg_destroy_decompress(&cinfo);
            return (FALSE);
        }
        color = TRUE;
    }
    // Time to make an output image buffer.
    if(cinfo.data_precision_other > 8) outBytes = 2;
    else outBytes = 1;
    rowWidth =  cinfo.output_width * cinfo.output_components * outBytes;
    imageLen = rowWidth * cinfo.output_height * frames;
    if ( imageLen & 1 ) imageLen++;//Odd length, make it even.
    if(!(uncompImagPtr = (char*)malloc(imageLen)))
    {
        OperatorConsole.printf(
                               "***[DecompressJPEGL]: Could not allocate decompressed image memory.\n");
        jpeg_destroy_decompress(&cinfo);
        return (FALSE);
    }
    uncompImagPtr[imageLen - 1] = 0; // In the case of an odd length.
    outImageBptr = uncompImagPtr;
    /*  The size of a pixel is JSAMPLE16, 16 bits, no matter what the size of the original data.
     This is done because the size of the data in the jpeg image is not known to the library until 
     jpeg_read_header(). So if the buffer is 8 bit, the library will create an internal 16 bit
     buffer to hold the image data and copy it to the smaller buffer wasting time. 
     So just make a 16 bit buffer array now the height of DCTSIZE (jpeg standard = 8). */
    outBuffer = (JSAMPARRAY16)(*cinfo.mem->alloc_sarray)
	((j_common_ptr) &cinfo,  JPOOL_PERMANENT,//JPOOL_IMAGE,
     (JDIMENSION) cinfo.output_width * cinfo.output_components
     * sizeof(JSAMPLE16), (JDIMENSION) cinfo.rec_outbuf_height);
    if(!outBuffer)
    {
        OperatorConsole.printf(
                               "***[DecompressJPEGL]: Libjpeg could not allocate image buffer memory.\n");
        jpeg_destroy_decompress(&cinfo);
        return (FALSE);
    }
    // Start the frames loop.
    while(TRUE)
    {
        // The library will convert the internal YCbCr to RGB.
        if (color) cinfo.out_color_space = JCS_RGB;
        else cinfo.out_color_space = JCS_GRAYSCALE;
        // Ready to decompress 
        jpeg_start_decompress(&cinfo);
        // Tell libjpeg we will use a 16 bit buffer
        cinfo.buffer_size_char = FALSE;
        // Never scale.
        cinfo.shft = 0;
        // read the Image loop
        while (cinfo.output_scanline < cinfo.output_height)
        {
            num_scanlines = jpeg_read_scanlines(&cinfo,(JSAMPARRAY) outBuffer, 
                                                (JDIMENSION) cinfo.rec_outbuf_height);
            for(row = 0; row < num_scanlines; row++)
            {
                uncompImageBptr = (char *)outBuffer[row];
                if (outBytes == 1)// char
                {
                    for( pixcnt = 0; pixcnt < rowWidth;  pixcnt++)
                    {
#if NATIVE_ENDIAN == BIG_ENDIAN //Big Endian like PPC
                        uncompImageBptr++;
#endif
                        *outImageBptr++ = *uncompImageBptr++;
#if NATIVE_ENDIAN == LITTLE_ENDIAN //Little Endian
                        uncompImageBptr++;
#endif
                    }
                }
                else //words
                { /* Row with is 2 x, MSB, LSB */
                    for( pixcnt = 0; pixcnt < rowWidth;  pixcnt++)
                    {
#if NATIVE_ENDIAN == LITTLE_ENDIAN //Little Endian
                        *outImageBptr++ = *uncompImageBptr++;
#else //Big Endian like PPC
                        *outImageBptr++ = uncompImageBptr[1];
                        *outImageBptr++ = *uncompImageBptr++;
                        uncompImageBptr++;
                        pixcnt++;
#endif
                    }
                }
            }
        }
        jpeg_finish_decompress(&cinfo);
        // check for the end
        if( ++currFrame >= frames )break;
        // More images to read
        while(++currSQObject <= ArrayPtr->GetSize())
        {
            DDO = ArrayPtr->Get(currSQObject);//Get the array.
            vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
            //Look for size and jpeg SOI marker 
            if(vrImage->Length && ((unsigned char *)vrImage->Data)[0] == 0xFF &&
               ((unsigned char *)vrImage->Data)[1] == 0xD8)break;
        }
        if( currSQObject >= ArrayPtr->GetSize() )break;//Should not happen!
        // OK, have another image now, reset decompressor
        // Get data for next frame
        cinfo.src->bytes_in_buffer = vrImage->Length; // forces fill_input_buffer on first read
        cinfo.src->next_input_byte = (JOCTET *)vrImage->Data; // Image buffer
        // Read the new header, nothing should change (should we check?)
        jpeg_read_header(&cinfo, TRUE);
        // The default is 8 bits, set it back to the image size.
        cinfo.data_precision_other =cinfo.data_precision;
    }// Loop back to jpeg_start_decompress (while(TRUE))
    if(currFrame < frames)OperatorConsole.printf(
                                                 "Warn[DecompressJPEGL]: Found %d of %d frames.\n",currFrame, frames);
    // Should have image(s) here, time to save it.
    pVR->Reset();// Remove old data.
    if(outBytes == 2) pVR->TypeCode ='OW';
    else pVR->TypeCode ='OB';
    // Set it to the image data.
    pVR->Data = uncompImagPtr;
    // Tell it how long.
    pVR->Length = (UINT32)(imageLen & UINT32_MAX);
    // Give it responsible for it.
    pVR->ReleaseMemory = TRUE;
    //  pDDO->DeleteVR(pVR);// replace the pixel data: corrupts LastVRG
    //  pDDO->Push(vrs);
#ifdef GYROSCAN_FIX
    // Set the number of bits allocated.
    // Need to fix Philips Gyroscan that stores 8 in 16 (now 8 in 8 from decompression).
    // 20120624: do not change highbit and bitsstored
    if(cinfo.data_precision <= 8) pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)8, 'US');
    else pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)16, 'US');
    
    // Should not be needed, but never hurts.
    // mvh, 20110502: DecompressJPEGL either not write 0x0101 (bitstored), or write both 0x0101 and 0x0102 (bitsstored/highbit)
    // mvh, 20110502: need to write highbit for consistency if writing bitstored
    pDDO->ChangeVR( 0x0028, 0x0101, (UINT8)cinfo.data_precision, 'US');
    pDDO->ChangeVR( 0x0028, 0x0102, (UINT8)(cinfo.data_precision-1), 'US');
#endif
    
    // Done with libjpeg
    jpeg_destroy_decompress(&cinfo);
    //Change the Photometric Interpretation.
    if(color)
    {
        //Set Planes value if needed, do  not create if not there.
        pVR = pDDO->GetVR(0x0028, 0x0006);
        if(pVR && pVR->Length && *(char *)pVR->Data == 1)
            *(char *)pVR->Data = 0;
        //Change the Photometric Interpretation.
        pDDO->ChangeVR( 0x0028, 0x0004, "RGB\0", 'CS');
    }
    //Change the transfer syntax to LittleEndianExplict!
    if(leExplict)// Only false for FUJI_FIX w/ Fuji Image
        pDDO->ChangeVR( 0x0002, 0x0010, "1.2.840.10008.1.2.1\0", 'IU');
    // If debug > 0, print decompress time.
    if (DebugLevel > 0)
        SystemDebug.printf("JPEG decompress time %u seconds.\n", (unsigned int)time(NULL) - t);
    // Done!          
    return(TRUE);
}
#endif//End HAVE_LIBJPEG
#ifdef HAVE_LIBJASPER
/* This routine will take all of the listed color spaces in 08_03 of the standard
 *  in little endian, uncompressed format and  compress it to jpeg2000 in whatever
 *  format it came in.  I don't know if dicom viewers can support this.  It uses the
 *  Jasper library by Michael D. Adams from the Department of Electrical and
 *  Computer Engineering at the University of Victoria.  You can get it here:
 *  http://www.ece.uvic.ca/~mdadams/jasper/
 *  You can also get it here with a few minor changes to use the my Jpeg-6c
 *  library, but none of the changes are used by dgate: 
 *  http://www.bitsltd.net/Software/Software-Repository/index.php
 *  Jasper can compress anything, with any size for each plane, but the standard only
 *  allows YBR_RCT lossless and YBR_ICT lossy.  So if a YBR come in, it is lossy, and
 *  RGB is lossless.  If this is a problem I can  add a forced change in colorspace.
 *  If I have made some mistakes (most likely), you can contact me bruce.barton
 *  (the mail symbol goes here) bitsltd.net.  Let me know where I can find a sample of
 *  the image that didn't work. */
BOOL CompressJPEG2K(DICOMDataObject* pDDO, int j2kQuality)
{
    Array < DICOMDataObject	*>	*ArrayPtr, *ArrayImage;
    DICOMDataObject             *DDO;
    VR                          *pVR, *vrImage, *vrs;
    jas_stream_t                *out;
    jas_image_t                 *image;
    jas_image_cmpt_t            *cmpt;
    jas_image_coord_t           byteCnt, rowCnt, size;
    FILE                        *fp;
    UINT8                       bitwa, prec, colorTypeIn, sgnd;
    UINT16                      cmptno;
    char                        *colorASC, name[256], *brcrBuffer, *bufferPtr;
    char                        *colorBuffer_ptr[3], *colorBuffer[3];
    char                        *buffer_ptr, option[20];
    long                        fileLength, lengthRead;
    int                         tempInt, err;
    unsigned int				  currFrame, frames, t, inputArrayCnt, byteWidthIn, byteWidth;
    BOOL                        planes, buffers;
    
    // If debug > 0, get start time and set the level.
    t = 0;
    if (DebugLevel > 0)
    {
        t = (unsigned int)time(NULL);
        jas_setdbglevel(DebugLevel);
    }
    if (DebugLevel > 1) SystemDebug.printf("Jasper compress started.\n");
    buffers = FALSE;
    inputArrayCnt = 0;
    // Uninitialized warnings.
    ArrayImage = NULL;
    brcrBuffer = NULL;
    cmpt = NULL;
    // Check and set the quality for lossy.
    if(j2kQuality < MIN_QUALITY)// Set to 0 to use dicom.ini value. 
    {
        j2kQuality = IniValue::GetInstance()->sscscpPtr->LossyQuality;//Use the default or dicom.ini value.
    }
    if(j2kQuality > 100) j2kQuality = 100;
    // Init the jasper library.
    if(jas_init())
    {
        OperatorConsole.printf("***[CompressJPEG2K]: Cannot init the jasper library\n");
        return (FALSE);
    }
	/* Created from jas_image_create and jas_image_create0.
     *  It was done so the buffers are not allocated if not needed. */
    if (!(image = (jas_image_t *)malloc(sizeof(jas_image_t)))) 
    {
        OperatorConsole.printf("***[CompressJPEG2K]: Could not allocate an image structure.\n");
        return (FALSE);
    }
    // Set the some defaults for the outer box.
    image->tlx_ = 0;
    image->tly_ = 0;
    image->clrspc_ = JAS_CLRSPC_UNKNOWN;
    image->numcmpts_ = 0;//Set later
    // Get the number of samples per pixel VR.
    if(!(image->maxcmpts_ = pDDO->GetBYTE(0x0028, 0x0002))) image->maxcmpts_ = 1;
    // Are there frames?
    if(!(frames = pDDO->Getatoi(0x0028, 0x0008))) frames = 1;
    currFrame = 0;
    // Get the Rows VR and check size
    if(!(image->bry_ = pDDO->GetUINT(0x0028, 0x0010)))
    {
        SystemDebug.printf("***[CompressJPEG2K]: Failed to get image height.\n");
        free(image);
        return(FALSE);
    }
    // Get the Columns VR and check size.
    if(!(image->brx_ = pDDO->GetUINT(0x0028, 0x0011)))
    {
        OperatorConsole.printf("***[CompressJPEG2K]: Failed to get image width.\n");
        free(image);
        return(FALSE);
    }
    byteWidth = 1;
    byteWidthIn = 1;
    if(!(bitwa = pDDO->GetBYTE(0x0028, 0x0100)))bitwa = 8; // 8 bit default.
    if(!(prec = pDDO->GetBYTE(0x0028, 0x0101)))prec = bitwa; // the default.
    if(prec > bitwa)bitwa = prec;// the bigger one.
    if(prec > 8)byteWidth = 2;// Will fix 8 in 16 with a buffer.
    if(bitwa > 8)byteWidthIn = 2;
    // Check if we can do it
    if(bitwa != 8 && bitwa != 12  && bitwa != 16)
    {
        OperatorConsole.printf("***[CompressJPEG2K]: Unsuported allocated bit width: %d.\n", bitwa);
        free(image);
        return(FALSE);
    }
    // Checked if the data is signed,
    if(!(sgnd = pDDO->GetBYTE(0x0028, 0x0103)))sgnd = 0;
    // Set the image or buffer size
    size = image->brx_ * image->bry_ * byteWidth;
    // Planar configuration default.
    planes = FALSE;
    // Sort colors.
    colorTypeIn = image->clrspc_ = JAS_CLRSPC_FAM_GRAY;// The default
    colorASC = NULL;
    if(image->maxcmpts_ == 1)
    {
        planes = TRUE;//Just 1 plane.
    }
    else if(image->maxcmpts_ == 3)
    {
        pVR = pDDO->GetVR(0x0028, 0x0004); // Get the color profile
        if(pVR && pVR->Length > 2) colorASC = (char *)pVR->Data;
        // Look for the color type
        if(colorASC == NULL || strncmp(colorASC, "RGB",3)==0)// RGB
        {// Only RGB can be in planes or R,G,B format, check.
            if(pDDO->GetBYTE(0x0028, 0x0006)) planes = TRUE;; // Check planes.
            /*	  pVR = pDDO->GetVR(0x0028, 0x0006); // Get the plane's VR.
             if(pVR && pVR->Length && (*(char *)(pVR->Data) == 1)) planes = TRUE;*/
            colorTypeIn = image->clrspc_ = JAS_CLRSPC_FAM_RGB;
        }
        else if (pVR->Length > 6 && strncmp(colorASC, "YBR_",4)==0)
        {
            image->clrspc_ = JAS_CLRSPC_FAM_YCBCR;
            if ((strncmp(&colorASC[4], "ICT",3)==0)  || (strncmp(&colorASC[4], "RCT",3)==0))
            {
                OperatorConsole.printf
                ("Warn[CompressJPEG2K]: Uncompressed colorspace can not be YBR_ICT or YBR_RCT. Trying RGB\n");
                colorTypeIn = image->clrspc_ = JAS_CLRSPC_FAM_RGB;
                planes = FALSE;
            }
            if (pVR->Length > 7 && strncmp(&colorASC[4], "FULL",4)==0)// YBR_FULL(_422)
            {
                if (pVR->Length > 11 && strncmp(&colorASC[8], "_422",4)==0)// YBR_FULL_422
                {
                    colorTypeIn = CLRSPC_FAM_YCBCR_F422;
                    planes = FALSE;
                }
                else// YBR_FULL
                {
                    colorTypeIn = JAS_CLRSPC_FAM_YCBCR;
                    planes = TRUE;
                }
            }
            if (pVR->Length > 14 && strncmp(&colorASC[4], "PARTIAL_42",10)==0)// YBR_PARTIAL_42x
            {
                planes = FALSE;
                if(colorASC[14] == '0')colorTypeIn = CLRSPC_FAM_YCBCR_420;
                else colorTypeIn = CLRSPC_FAM_YCBCR_422;
            }
        }// End of YBR_
        // Add more colors here.
        else
        {
            if((colorASC = pDDO->GetCString(0x0028, 0x0004)))
            {
                OperatorConsole.printf(
                                       "***[CompressJPEGL]: Unknown or unsuported color space %s.\n",colorASC);
                free(colorASC);
            }
            else OperatorConsole.printf(
                                        "***[CompressJPEGL]: Unknown or unsuported color space record.\n");
            free(image);
            return (FALSE);
        }
    }//image->maxcmpts_ == 3
    else
    {
        OperatorConsole.printf("***[CompressJPEG2K]: Unsupported number of components %d.\n",image->maxcmpts_);
        free(image);
        return (FALSE);
    }
    // Get the data.
    pVR = pDDO->GetVR(0x7fe0, 0x0010); // Get the Image VR.
    vrImage = pVR;
    if(pVR && pVR->Length)
    {
        if(pVR->SQObjectArray)
        {//This should not be for uncompressed.
            ArrayImage = (Array<DICOMDataObject *> *) pVR->SQObjectArray;
            while (inputArrayCnt < ArrayImage->GetSize())
            {
                DDO = ArrayImage->Get(inputArrayCnt++);//Get the array.
                vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data.
                if(vrImage && vrImage->Length >= 
                   (unsigned int)(image->brx_ * image->bry_ * byteWidthIn * image->maxcmpts_)) break;
            }
        }
    }
    // Check for a least one frame.
    if(!vrImage || vrImage->Length < (unsigned int)(size * image->maxcmpts_))
    {
        OperatorConsole.printf("***[CompressJPEG2K]: Could not find the image data.\n");
        free(image);
        return (FALSE);
    }
    buffer_ptr = (char *)vrImage->Data;
    /*#if NATIVE_ENDIAN == LITTLE_ENDIAN // Little Endian
     if((planes || (image->maxcmpts_ == 1)) && ((bitwa == 8) || (bitwa == 16)))
     #else // Big Endian like Apple power pc*/
    if((planes || (image->maxcmpts_ == 1)) && (bitwa == 8))
        //#endif
    {// No need for buffers, just pointers
        for (cmptno = 0; cmptno < image->maxcmpts_; ++cmptno)
        {
            colorBuffer[cmptno] = buffer_ptr + (size * cmptno) ;
            colorBuffer_ptr[cmptno] = colorBuffer[cmptno];
        }
    }
    else// Buffers are needed.
    {
        for (cmptno = 0; cmptno < image->maxcmpts_; ++cmptno)
        {
            if(!(colorBuffer[cmptno] = (char *)malloc(size)))
            {
                OperatorConsole.printf(
                                       "***[CompressJPEG2K]: Could not allocate a %d byte image buffer #%d.\n",
                                       size, cmptno);
                while(cmptno > 0) free(colorBuffer[--cmptno]);
                free(image);
                return (FALSE);
            }
            colorBuffer_ptr[cmptno] = colorBuffer[cmptno];
        }
        if(colorTypeIn == CLRSPC_FAM_YCBCR_420)
        {
            if(!(brcrBuffer = (char *)malloc(image->brx_)))
            {
                OperatorConsole.printf(
                                       "***[CompressJPEG2K]: Could not allocate a %d byte image 420 buffer.\n",
                                       image->brx_);
                while(cmptno > 0) free(colorBuffer[--cmptno]);
                free(image);
                return (FALSE);
            }
        }
        buffers = TRUE;
    }
    // Allocate memory for the per-component pointer table.
    if (!(image->cmpts_ = ((jas_image_cmpt_t **)jas_malloc(image->maxcmpts_ *
                                                           sizeof(jas_image_cmpt_t *)))))
    {
        OperatorConsole.printf("***[CompressJPEG2K]: Could not create component pointers.\n");
        if(buffers)
        {
            for (cmptno = 0; cmptno < image->maxcmpts_; ++cmptno)
            {
                if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                colorBuffer[cmptno] =NULL;
            }
            if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
        }
        free(image);
        return (FALSE);
    }
    // Initialize in case of failure so jas_image_destroy can be used.
    for (cmptno = 0; cmptno < image->maxcmpts_; ++cmptno)
        image->cmpts_[cmptno] = 0;
    image->inmem_ = TRUE;
    image->cmprof_ = 0;
    // Allocate memory for the per-component information and init.
    for (cmptno = 0; cmptno < image->maxcmpts_; ++cmptno)
    {
        if (!(cmpt = (jas_image_cmpt_t *)jas_malloc(sizeof(jas_image_cmpt_t))))
        {
            jas_image_destroy(image);
            OperatorConsole.printf("***[CompressJPEG2K]: Could not create empty components.\n");
            if(buffers)
            {
                for (cmptno = 0; cmptno < image->maxcmpts_; ++cmptno)
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            return (FALSE);
        }
        image->cmpts_[cmptno] =cmpt;
        cmpt->type_ = JAS_IMAGE_CT_UNKNOWN;//Set next.
        cmpt->tlx_ = 0;
        cmpt->tly_ = 0;
        cmpt->hstep_ = 1;
        cmpt->vstep_ = 1;
        cmpt->width_ = image->brx_;
        cmpt->height_ = image->bry_;
        cmpt->prec_ = prec;
        cmpt->sgnd_ = sgnd;
        cmpt->stream_ = 0;
        cmpt->cps_ = byteWidth;
        if(!(cmpt->stream_ = jas_stream_memopen( colorBuffer[cmptno], size)))
        {
            jas_free(cmpt);
            jas_image_destroy(image);
            OperatorConsole.printf("***[CompressJPEG2K]: Jasper could not open the memory as stream.\n");
            if(buffers)
            {
                for (; cmptno < image->maxcmpts_; ++cmptno)// From curr # forward
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            return (FALSE);
        }
        ++(image->numcmpts_);
    }
    // Now set the color type
    switch (image->clrspc_)
    {
        case JAS_CLRSPC_FAM_GRAY:
            jas_image_setclrspc(image, JAS_CLRSPC_SGRAY);
            jas_image_setcmpttype(image, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
            break;
        case JAS_CLRSPC_FAM_RGB:
            jas_image_setclrspc(image, JAS_CLRSPC_SRGB);
            jas_image_setcmpttype(image, 0,
                                  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R));
            jas_image_setcmpttype(image, 1,
                                  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G));
            jas_image_setcmpttype(image, 2,
								  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B));
            break;
        default:
            jas_image_setclrspc(image, JAS_CLRSPC_SYCBCR);
            jas_image_setcmpttype(image, 0,
                                  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_Y));
            jas_image_setcmpttype(image, 1,
                                  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CB));
            image->cmpts_[1]->sgnd_ = TRUE;
            jas_image_setcmpttype(image, 2,
                                  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_YCBCR_CR));
            image->cmpts_[2]->sgnd_ = TRUE;
    }
    // Lossy (ICT) or lossless (RCT)?
    if((j2kQuality < 100) || (image->clrspc_ == JAS_CLRSPC_FAM_YCBCR))
    {
        if(j2kQuality >= 100)j2kQuality = 95;
        memcpy(option, "mode=real rate=0.", 17);
        option[17] = (j2kQuality / 10) + '0';//ICT j2kQuality msb
        option[18] = (j2kQuality % 10) + '0';//ICT j2kQuality lsb
        option[19] = 0;//end the string.
        if (DebugLevel > 2)SystemDebug.printf("JPEG2K Lossy Quality = %d", j2kQuality);
    }
    else
    {
        memcpy(option, "mode=int\0", 9);//RCT
        if (DebugLevel > 2)SystemDebug.printf("JPEG2K Lossless");
    }
    // Use a temp file to hold the data, easier to get the compressed size.
    NewTempFileWExt(name, ".j2k");
    // Print out some more info for debug.
    if (DebugLevel > 2)
    {
        if (DebugLevel > 3)
        {
            if(!buffers)SystemDebug.printf(", unbuffered data");
            else SystemDebug.printf(", buffered data");
            if(planes)SystemDebug.printf(", planes");
        }
        SystemDebug.printf(
                           "\n, H = %d, W = %d, Bits = %d in %d, Frames = %d, ",
                           image->brx_, image->brx_, prec, bitwa, frames);
        if((colorASC = pDDO->GetCString(0x0028, 0x0004)))
        {
            SystemDebug.printf("color = %s\n", colorASC);
            free(colorASC);
        }
        else SystemDebug.printf("Unknown color space record.\n");
    }
    // Create the encapsulation array.
    ArrayPtr = new Array < DICOMDataObject * >;
    // The first blank object.
    DDO = new DICOMDataObject;
    vrs = new VR(0xfffe, 0xe000, 0, (void *)NULL, FALSE);
    DDO->Push(vrs);
    ArrayPtr->Add(DDO);	
    // Frames loop.
    while(TRUE)
    {
        if(buffers)
        {
            // Now to fill the buffers.
            switch(colorTypeIn)
            {
                case JAS_CLRSPC_FAM_GRAY://12 or 12,16 bits here
                    /* For sone reason jasper wants the data in big endian on both a LE and BE system.
                     *  There may be an option switch for this, but I just swpped endian. */
                    if(bitwa == 16) // 16 Bits, byte swap time.
                    {
                        if(prec > 8) //Just swap.
                        {
                            for(byteCnt = 0; byteCnt < size; byteCnt++)
                            {
                                byteCnt++;// 2 bytes per pixel.
                                *colorBuffer_ptr[0]++ = buffer_ptr[1];
                                *colorBuffer_ptr[0]++ = *buffer_ptr++;
                                buffer_ptr++;
                            }
                            break;
                        }
                        for(byteCnt = 0; byteCnt < size; byteCnt++)//Only need 8.
                        {
                            *colorBuffer_ptr[0]++ = *buffer_ptr++;
                            buffer_ptr++;// Skip the zero byte.
                        }
                        break;
                    }
                    else
                        for(byteCnt = 0; byteCnt < size; byteCnt++)//12 bit
                        {
                            colorBuffer_ptr[0][1] = (*buffer_ptr & 0xF0) >> 4;//8-11
                            *colorBuffer_ptr[0] = (*buffer_ptr++ & 0x0F) << 4;//4-7
                            *colorBuffer_ptr[0]++ |= (*buffer_ptr & 0xF0) >> 4;//0-3
                            colorBuffer_ptr[0]++;
                            byteCnt++;
                            if(byteCnt >= size)break;
                            colorBuffer_ptr[0][1] = *buffer_ptr++ & 0x0F;//8-11
                            byteCnt++; 
                            *colorBuffer_ptr[0]++ = *buffer_ptr++;//0-7
                            colorBuffer_ptr[0]++;
                            byteCnt++;
                        }
                    break;
                case JAS_CLRSPC_FAM_RGB:
                    for(byteCnt = 0; byteCnt < size; byteCnt++)
                    {
                        *(colorBuffer_ptr[0])++ = *buffer_ptr++;
                        *(colorBuffer_ptr[1])++ = *buffer_ptr++;
                        *(colorBuffer_ptr[2])++ = *buffer_ptr++;
                    }
                    break;
                case CLRSPC_FAM_YCBCR_F422://Y1Y2CbCr
                    for(byteCnt = 0; byteCnt < size; byteCnt++)
                    {
                        byteCnt++;// 2 Ys per pixel.
                        *colorBuffer_ptr[0]++ = *buffer_ptr++;
                        *colorBuffer_ptr[0]++ = *buffer_ptr++;
                        *colorBuffer_ptr[1]++ = *buffer_ptr;
                        *colorBuffer_ptr[1]++ = *buffer_ptr++;
                        *colorBuffer_ptr[2]++ = *buffer_ptr;
                        *colorBuffer_ptr[2]++ = *buffer_ptr++;
                    }
                    break;
                case CLRSPC_FAM_YCBCR_422://Y1Y2CbCr
                    for(byteCnt = 0; byteCnt < size; byteCnt++)
                    {
                        byteCnt++;// 2 Ys per pass.
                        /* For Y the ratio is 256 / 220 (1.1636) and  for color 256 / 225 (1.1378).
                         *  Note: All values are multiplied by 1024 or 1 << 7.
                         *  Yo = 1.1636(Yi - 16)  == Yo = 1.1636Yi - 18.204 ==
                         *  Yo = [149Yi - 2330]/128 */
                        tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1
                        buffer_ptr++;
                        if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
                        else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
                        else *colorBuffer_ptr[0]++ = (char)((tempInt >> 7) & 0xFF);
                        tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y2
                        buffer_ptr++;
                        if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
                        else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
                        else *colorBuffer_ptr[0]++ = (char)((tempInt >> 7) & 0xFF);
                        /*  Cxo = 1.1378(Cxi - 16)  == Cxo = 1.1378Cxi - 18.205 ==
                         *  Cxo = [73Cxi - 1152]/64 */
                        tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
                        buffer_ptr++;
                        if(tempInt & 0x8000)
                        {
                            *colorBuffer_ptr[1]++ = 0;//Negitive
                            *colorBuffer_ptr[1]++ = 0;//Negitive
                        }
                        else if(tempInt & 0x4000)
                        {
                            *colorBuffer_ptr[1]++ = 0xFF;//Over
                            *colorBuffer_ptr[1]++ = 0xFF;//Over
                        }
                        else
                        {
                            *colorBuffer_ptr[1] = (char)((tempInt >> 6) & 0xFF);
                            *colorBuffer_ptr[1]++ = *colorBuffer_ptr[1];
                            colorBuffer_ptr[1]++;
                        }
                        tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
                        buffer_ptr++;
                        if(tempInt & 0x8000)
                        {
                            *colorBuffer_ptr[2]++ = 0;//Negitive
                            *colorBuffer_ptr[2]++ = 0;//Negitive
                        }
                        else if(tempInt & 0x4000)
                        {
                            *colorBuffer_ptr[2]++ = 0xFF;//Over
                            *colorBuffer_ptr[2]++ = 0xFF;//Over
                        }
                        else
                        {
                            *colorBuffer_ptr[2] = (char)((tempInt >> 6) & 0xFF);
                            *colorBuffer_ptr[2]++ = *colorBuffer_ptr[1];
                            colorBuffer_ptr[2]++;
                        }
                    }
                    break;
                    // What a strange standard. See page 323 of 08_03pu.pdf.
                case CLRSPC_FAM_YCBCR_420://Y1CrCbY2Y3CrCbY4-Y1Y2Y3Y4?
                    for(rowCnt = 0; rowCnt < image->bry_; rowCnt++)// 8 bit only.
                    {
                        rowCnt++;// 2 Ys per vertical pass.
                        bufferPtr = brcrBuffer;
                        for(byteCnt = 0; byteCnt < image->brx_; byteCnt++)
                        {
                            byteCnt++;// 2 Ys per horizantal pass.
                            tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1, Y3, ...
                            buffer_ptr++;
                            if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
                            else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
                            else *colorBuffer_ptr[0]++ = (char)((tempInt >> 7) & 0xFF);
                            tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
                            buffer_ptr++;
                            if(tempInt & 0x8000)
                            {
                                *bufferPtr++ = 0;//Next time negitive = 0
                                *colorBuffer_ptr[1]++ = 0;//Negitive
                                *colorBuffer_ptr[1]++ = 0;//Negitive
                            }
                            else if(tempInt & 0x4000)
                            {
                                *bufferPtr++ = 0xFF;//Next time over = 255
                                *colorBuffer_ptr[1]++ = 0xFF;//Over
                                *colorBuffer_ptr[1]++ = 0xFF;//Over
                            }
                            else
                            {
                                *bufferPtr = (char)((tempInt >> 6) & 0xFF);
                                *colorBuffer_ptr[1]++ = *bufferPtr;
                                *colorBuffer_ptr[1]++ = *bufferPtr++;
                            }
                            tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
                            buffer_ptr++;
                            if(tempInt & 0x8000)
                            {
                                *bufferPtr++ = 0;//Next time negitive = 0
                                *colorBuffer_ptr[2]++ = 0;//Negitive
                                *colorBuffer_ptr[2]++ = 0;//Negitive
                            }
                            else if(tempInt & 0x4000)
                            {
                                *bufferPtr++ = 0xFF;//Next time over = 255
                                *colorBuffer_ptr[2]++ = 0xFF;//Over
                                *colorBuffer_ptr[2]++ = 0xFF;//Over
                            }
                            else
                            {
                                *bufferPtr = (char)((tempInt >> 6) & 0xFF);
                                *colorBuffer_ptr[2]++ = *bufferPtr;
                                *colorBuffer_ptr[2]++ = *bufferPtr++;
                            }
                        }
                        bufferPtr = brcrBuffer;
                        for(byteCnt = 0; byteCnt < image->brx_; byteCnt++)
                        {
                            byteCnt++;// 2 Ys per horizantal pass.
                            tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1, Y3, ...
                            buffer_ptr++;
                            if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
                            else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
                            else *colorBuffer_ptr[0]++ = (char)((tempInt >> 7) & 0xFF);
                            tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1, Y3, ...
                            buffer_ptr++;
                            if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
                            else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
                            else *colorBuffer_ptr[0]++ = (char)((tempInt >> 7) & 0xFF);
                            *colorBuffer_ptr[1]++ = *bufferPtr;
                            *colorBuffer_ptr[1]++ = *bufferPtr++;
                            *colorBuffer_ptr[2]++ = *bufferPtr;
                            *colorBuffer_ptr[2]++ = *bufferPtr++;
                        }
                    }
                    break;
            }// End of the switch
        }// End of buffers.
        // Open the temp file to hold put data.
        if (!(out = jas_stream_fopen(name, "w+b")))
        {
            jas_image_destroy(image);
            OperatorConsole.printf("***[CompressJPEG2K]: Jasper could not open the file: %s.\n", name);
            if(buffers)
            {
                for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            while(ArrayPtr->GetSize())
            {
                delete ArrayPtr->Get(0);
                ArrayPtr->RemoveAt(0);
            }
            delete ArrayPtr;
            return (FALSE);
        }
        // Time to compress.
        if(jpc_encode(image, out, option))
        {
            jas_stream_close(out);
            unlink(name);
            jas_image_destroy(image);
            OperatorConsole.printf("***[CompressJPEG2K]: Jasper could not encode the image.\n");
            if(buffers)
            {
                for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            while(ArrayPtr->GetSize())
            {
                delete ArrayPtr->Get(0);
                ArrayPtr->RemoveAt(0);
            }
            delete ArrayPtr;
            return (FALSE);
        }
        jas_stream_close(out);// Jasper is done with the file.
        // Lets get the data.
        fp = fopen(name, "rb");
        if(!fp)
        {
            jas_image_destroy(image);
            OperatorConsole.printf("***[CompressJPEG2K]: Could open the Jasper output file %s.\n",name);
            if(buffers)
            {
                for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            while(ArrayPtr->GetSize())
            {
                delete ArrayPtr->Get(0);
                ArrayPtr->RemoveAt(0);
            }
            delete ArrayPtr;
            return ( FALSE );
        }
        fseek(fp, 0, SEEK_END);
        fileLength = ftell(fp);
        if(fileLength == -1 || fileLength == 0)
        {
            if(fileLength == 0)
                OperatorConsole.printf("***[CompressJPEG2K]: File %s has a zero length\n", name);
            else
                OperatorConsole.printf("***[CompressJPEG2K]: Could not get file size for %s\n", name);
            jas_image_destroy(image);
            fclose(fp);
            unlink(name);
            if(buffers)
            {
                for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            while(ArrayPtr->GetSize())
            {
                delete ArrayPtr->Get(0);
                ArrayPtr->RemoveAt(0);
            }
            delete ArrayPtr;
            return ( FALSE );
        }
        rewind(fp);
        // Jpeg2k is encapsulated, make a new vr to encapsulate.
        vrs = new VR(0xfffe, 0xe000, (UINT32)(fileLength & UINT32_MAX), TRUE);
        if(!vrs)
        {
            OperatorConsole.printf("***[CompressJPEG2K]: Failed to allocate memory.\n");
            fclose(fp);
            unlink(name);
            jas_image_destroy(image);
            if(buffers)
            {
                for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            while(ArrayPtr->GetSize())
            {
                delete ArrayPtr->Get(0);
                ArrayPtr->RemoveAt(0);
            }
            delete ArrayPtr;
            return ( FALSE );
        }
        //If odd data length, zero the added byte.
        if ( fileLength & 1  ) ((BYTE *)vrs->Data)[fileLength] = 0;
        //Change the Image data.
        lengthRead = fread(vrs->Data,1,fileLength,fp);
        if(lengthRead <= 0)
        {
            err = ferror(fp);
            if(err) OperatorConsole.printf("***[CompressJPEG2K]: File read error %d on %s.\n",err,name);
            else OperatorConsole.printf("***[CompressJPEG2K]: No compressed image data (0 length read).\n");
            fclose(fp);
            unlink(name);
            jas_image_destroy(image);
            if(buffers)
            {
                for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
                {
                    if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
                    colorBuffer[cmptno] = NULL;
                }
                if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
            }
            while(ArrayPtr->GetSize())
            {
                delete ArrayPtr->Get(0);
                ArrayPtr->RemoveAt(0);
            }
            delete ArrayPtr;
            delete vrs;
            return ( FALSE );
        }
        if(  lengthRead != fileLength)// Already checked for - 1.
        {
            OperatorConsole.printf("warn[CompressJPEG2K]: Only read %d bytes of %d of the jpeg2k file:%s, will try to save\n",
                                   vrs->Length, fileLength, name);
        }
        fclose(fp);
        unlink(name);
        // Save the image object.
        DDO = new DICOMDataObject;        
        DDO->Push(vrs);
        ArrayPtr->Add(DDO);
        if(++currFrame >= frames)break;//Done with all of the frames.
        //Out of data?
        // Deal with silly input arrays
        if(inputArrayCnt > 0  && (size * image->numcmpts_ * currFrame) < vrImage->Length)
        {// Look for the next array.
            while (inputArrayCnt < ArrayImage->GetSize())
            {
                DDO = ArrayImage->Get(inputArrayCnt++);//Get the array.
                vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
                // If found next one, break inner loop.
                if(vrImage && vrImage->Length >= (unsigned int)(size * image->numcmpts_)) break;
            }
            if(!vrImage || vrImage->Length < (unsigned int)(size * image->numcmpts_)) break;// Not enough data for at least 1 frame.
            buffer_ptr = (char *)vrImage->Data; // Piont at the new buffer.
        }
        // No silly arrays ( or a least one big file in the array).
        else
        {
            if(!buffers) buffer_ptr += (size * image->numcmpts_);//next plane or gray.
            if((unsigned int)(buffer_ptr - (char *)vrImage->Data + (size * image->numcmpts_))
               > vrImage->Length)break;
        }
        for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
        {
            if(buffers)
            {// Reset the buffers and streams.
                jas_stream_rewind( image->cmpts_[cmptno]->stream_ );
                colorBuffer_ptr[cmptno] = colorBuffer[cmptno];// Reset the buffers.
            }
            else
            {// Close and reopen the streams at the new locations.
                jas_stream_close( image->cmpts_[cmptno]->stream_);
                image->cmpts_[cmptno]->stream_ = NULL;
                if(!(cmpt->stream_ = jas_stream_memopen( buffer_ptr + (size * cmptno),size )))
                {
                    jas_image_destroy(image);
                    OperatorConsole.printf("***[CompressJPEG2K]: Jasper could not open the memory as stream.\n");
                    if(buffers) if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
                    return (FALSE);
                }
            }
        }
    }//Back for the next frame, end of while(TRUE)
    // Should we kill it and keep the uncompressed data?
    if(currFrame < frames) OperatorConsole.printf(
                                                  "Warn[CompressJPEG2K]: Only %d of %d frames saved.\n",currFrame, frames);
    // If buffers were made, free them.
    if(buffers)
    {
        for (cmptno = 0; cmptno < image->numcmpts_; ++cmptno)
        {
            if(colorBuffer[cmptno]) free(colorBuffer[cmptno]);
            colorBuffer[cmptno] = NULL;
        }
        if(colorTypeIn == CLRSPC_FAM_YCBCR_420)if(brcrBuffer)free(brcrBuffer);
    }
    // The end object.
    DDO = new DICOMDataObject;
    vrs = new VR(0xfffe, 0xe0dd, 0, (void *)NULL, FALSE);
    DDO->Push(vrs);
    ArrayPtr->Add(DDO);
    //  vrs = new VR(0x7fe0, 0x0010, 0, (void *)NULL, FALSE);
    pVR->Reset();//Clear the old image data including arrays.
    pVR->SQObjectArray = ArrayPtr;// Replace the data
    pVR->TypeCode ='OW';
    //  pVR->Length = 0xFFFFFFFF;
    // Change the dicom parameters
    if(image->numcmpts_ > 1)
    {
        // Reset the plane's VR, if there.
        pVR = pDDO->GetVR(0x0028, 0x0006);
        if(pVR && pVR->Length && *(char *)pVR->Data == 1) *(char *)pVR->Data = 0;
        // Set the color profile
        if(j2kQuality < 100)
            pDDO->ChangeVR( 0x0028, 0x0004, "YBR_ICT\0", 'CS');
        else pDDO->ChangeVR( 0x0028, 0x0004, "YBR_RCT\0", 'CS');
    }
    // Fix the bits allocated.
    // 20120624: do not change highbit and bitsstored
    //if(byteWidth == 1)pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)8, 'US');
    //else pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)16, 'US');
    
    //Change the transfer syntax to JPEG2K!
    if(j2kQuality < 100)
        pDDO->ChangeVR( 0x0002, 0x0010, "1.2.840.10008.1.2.4.91\0", 'IU');
    else pDDO->ChangeVR( 0x0002, 0x0010, "1.2.840.10008.1.2.4.90\0", 'IU');
    // Done with Jasper.
    jas_image_destroy(image);
    // If debug > 0, print when finished
    if (DebugLevel > 0)
        SystemDebug.printf("Jasper compress time %u seconds.\n", (unsigned int)time(NULL) - t);
    return (TRUE);
}

/* This routine will take in a jpeg2000 image and convert it to little endian, uncompressed,
 *  RGB or grayscale.  It uses the Jasper library by Michael D. Adams from the
 *  Department of Electrical and Computer Engineering at the University of Victoria.
 *  You can get it here:
 *  http://www.ece.uvic.ca/~mdadams/jasper/
 *  You can also get it here with a few minor changes to use the my Jpeg-6c
 *  library, but none of the changes are used by dgate: 
 *  http://www.bitsltd.net/Software/Software-Repository/index.php
 *  Jasper can compress anything, with any size for each plane.  So that means any
 *  of the color spaces and formats can be in it.  The dicom standard removes the header
 *  and color box infromation and sends just the data stream.  The standard states only
 *  MONO1,2 PALETTE_COLOR YBR_RCT and YBR_ICT can be use.  For the color space and
 *  image format, I have to trust jasper.
 *  If I have made some mistakes (most likely), you can contact me bruce.barton
 *  (the mail symbol goes here) bitsltd.net.  Let me know where I can find a sample of
 *  the image that didn't work. */
BOOL DecompressJPEG2K(DICOMDataObject* pDDO)
{
    Array < DICOMDataObject	*>	*ArrayPtr;
    DICOMDataObject				*DDO;
    VR							*pVR,p, *vrImage;
    unsigned char					*streamData;
#ifdef NOVARAD_FIX
    void							*fixPtr;
    unsigned char					*fix1Ptr, *fix2Ptr;
#endif
    char							*outData;
    unsigned int					frames;
    unsigned int					currFrame, currSQObject;
    jas_stream_t					*jas_in, *jas_out[4];
    jas_image_t					*decompImage;
    //  jas_image_cmpt_t            *imageComp;
    char							*out_ptr;
    unsigned int					i, cps_jas, t;
    UINT8							numcmpts, prec;
    //  UINT16                      bitwa, bitws,;
    UINT32						stream_len, instream_len, total_len;
    
    // If debug > 1, get start time and set the level.
    t=0;
    if (DebugLevel > 0)
    {
        t = (unsigned int)time(NULL);
        jas_setdbglevel(DebugLevel);
    }
    if (DebugLevel > 3) SystemDebug.printf("Jasper decompress started.\n");
    // Init the jasper library.
    if(jas_init())
    {
        OperatorConsole.printf("***[DecompressJPEG2K]: cannot init the jasper library\n");
        return (FALSE);
    }
    if (DebugLevel > 3) SystemDebug.printf("Jasper library init completed.\n");
    // Uninitialized warnings.
    prec = 8;
    outData = NULL;
    out_ptr = NULL;
    total_len = 0;
    cps_jas = 1;
    stream_len = 0;
    // Get the number of samples per pixel VR.
    if(!(numcmpts = pDDO->GetBYTE(0x0028, 0x0002))) numcmpts = 1;// Gray default.
    // Are there frames?
    currFrame = 0;
    if(!(frames = pDDO->Getatoi(0x0028, 0x0008))) frames = 1;
    // Get the encapsulated data.
    pVR = pDDO->GetVR(0x7fe0, 0x0010); // Get the Image VR.
    if(!pVR)
    {
        OperatorConsole.printf("***[DecompressJPEG2K]: No image VR\n");
        return (FALSE);
    }
    currSQObject = 0;// Init now for no warning later.
    if(pVR->SQObjectArray)
    {
        ArrayPtr = (Array<DICOMDataObject *> *) pVR->SQObjectArray;
        while(TRUE)
        {
            if( currSQObject >= ArrayPtr->GetSize())
            {
                OperatorConsole.printf("***[DecompressJPEG2K]: No j2k data found\n");
                return (FALSE);
            }
            DDO = ArrayPtr->Get(currSQObject);//Get the last array.
            vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
            //Look for size and j2k marker or jp2 file header.
            if(vrImage->Length)
            {
                streamData = (unsigned char *)vrImage->Data;
                instream_len = vrImage->Length;
                if( *streamData == 0xFF) break; //Jpeg stream.
                if( *streamData == 0x00 && streamData[1] == 0x00 && streamData[2] == 0x00
                   && streamData[3] == 0x0C && streamData[4] == 0x6A && streamData[5] == 0x50)
                { //Wow, a j2k file header, dicom is normally just the stream.
                    streamData += 6;
                    instream_len -= 6;
                    for(i = 0; i < 512; i++)// I just picked 512.
                    { // Look for stream header.
                        if(*streamData == 0xFF && streamData[1] == 0x4F)break;
                        streamData++;
                        instream_len--;
                    }
                    if( i < 512 )break;// Found the stream.
                }
            }
            currSQObject++;
        }
    }
    else
    {
        OperatorConsole.printf("***[DecompressJPEG2K]: No image in encapsulation arrray\n");
        return (FALSE);
    }
    // Start the frames loop.
    while(TRUE)
    {
        // Fix the stream length.
        if(streamData[instream_len - 1] == 0) instream_len--;
#ifdef NOVARAD_FIX
        // Put the end of file back if not present.
        if ((streamData[instream_len - 2] != 0xFF)
            || (streamData[instream_len - 1] != 0xD9))
        {
            if(!(fixPtr = malloc(vrImage->Length + 2)))
            {
                OperatorConsole.printf("***[DecompressJPEG2K]: Can not create memory fix buffer.\n");
                return (FALSE);
            }
            fix1Ptr = (unsigned char*)fixPtr;
            fix2Ptr = (unsigned char*)vrImage->Data;
            for( i = instream_len; i > 0; --i ) *fix1Ptr++ = *fix2Ptr++;
            *fix1Ptr++ = 0xFF;
            *fix1Ptr++ = 0xD9;
            if(instream_len & 1 ) *fix1Ptr = 0;
            free(vrImage->Data);
            vrImage->Data = fixPtr;
            vrImage->Length += 2;
            streamData = (unsigned char*)vrImage->Data;
            instream_len = vrImage->Length;
            if(DebugLevel > 0)SystemDebug.printf("Warn[DecompressJPEG2K]: Novarad fix, EOC marker added.\n");
        }
#endif
        // Open the memory as a stream.
        if(!(jas_in = jas_stream_memopen((char*)streamData, instream_len)))
        {
            OperatorConsole.printf("***[DecompressJPEG2K]: cannot open jpeg 2K data\n");
            return (FALSE);
        }
        // Decopress the stream.
        if (!(decompImage = jpc_decode(jas_in, NULL)))
        {
            OperatorConsole.printf("***[DecompressJPEG2K]: cannot decode the jpeg 2K code stream\n");
            jas_stream_close(jas_in);
            return ( FALSE );
        }
        jas_stream_close(jas_in);
        // Do this the first time only.
        if(!currFrame)
        {
            // Check for color.
            if(numcmpts > 1)
            {
                if((numcmpts != 3) || (decompImage->numcmpts_ != 3))
                {
                    OperatorConsole.printf(
                                           "***[DecompressJPEG2K]: Should be 3 colors, DICOM: %d ,J2K: %d \n",
                                           numcmpts , decompImage->numcmpts_);
                    jas_image_destroy(decompImage);
                    return ( FALSE );
                }
            }
            // Get the total uncompressed length and rewind all streams and get pointers.
            stream_len = 0;
            for ( i = 0; i < numcmpts ; i++ )
            {
                jas_out[i] = decompImage->cmpts_[i]->stream_;
                jas_stream_rewind(jas_out[i]);
                stream_len += jas_stream_length(jas_out[i]);
            }
            total_len = stream_len * frames;
            prec = decompImage->cmpts_[0]->prec_;//Save for outside loop
            //    bits_jas = jas_image_cmptprec(decompImage, 0);
            //    signed_jas = jas_image_cmptsgnd(decompImage, 0);
            cps_jas = decompImage->cmpts_[0]->cps_;// Bytes or words.
            // Allocate the bigger uncompressed and unencapsulated image.
            if (( total_len & 1) != 0 ) total_len++;//Odd length, make it even.
            if (DebugLevel > 3)SystemDebug.printf(
                                                  "Info[DecompressJPEG2K] stream_len = %d, frames = %d, total_len = %d, prec = %d,cps_jas = %d.\n",
                                                  stream_len,frames,total_len,prec,cps_jas);
            if(!(outData = (char *)malloc(total_len)))
            {
                OperatorConsole.printf(
                                       "***[DecompressJPEG2K]: Failed to allocate %d bytes of memory.\n", total_len);
                jas_image_destroy(decompImage);
                return ( FALSE );
            }
            outData[total_len -1] = 0;// Dosen't hurt.        
            out_ptr = outData;
        }// end currFrame = 0.
        else
        {//Not first, just rewind streams
            for ( i = 0; i < numcmpts ; i++ )
            {
                jas_out[i] = decompImage->cmpts_[i]->stream_;
                jas_stream_rewind(jas_out[i]);
            }
        }
        //Image copy loops.
        if( numcmpts == 1)
        {
            // Again, for some reason, Jasper outputs big endian.  There may be a switch.
            //#if NATIVE_ENDIAN == BIG_ENDIAN //Big Endian like Apple power pc
            if(cps_jas == 2)
            {
                for(i = 0; i < stream_len; i++)
                {
                    out_ptr[1] = jas_stream_getc(jas_out[0]);
                    *out_ptr++ = jas_stream_getc(jas_out[0]);
                    out_ptr++;
                    i++;
                }
            }
            else
                //#endif //Big Endian
                for(i = 0; i < stream_len; i++)
                    *out_ptr++ = jas_stream_getc(jas_out[0]);
        }
        else //RGB
        {
            stream_len = (UINT32)(jas_stream_length(jas_out[0]) &UINT32_MAX);
            for(i = 0; i < stream_len; i++)
            {
                *out_ptr++ = jas_stream_getc(jas_out[0]);
                *out_ptr++ = jas_stream_getc(jas_out[1]);
                *out_ptr++ = jas_stream_getc(jas_out[2]);
            }
        }
        jas_image_destroy(decompImage);//Done with the image.
        // check for the end
        if(++currFrame >= frames)break;
        // More images to read
        while(++currSQObject < ArrayPtr->GetSize())
        {
            DDO = ArrayPtr->Get(currSQObject);//Get the array.
            vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
            //Look for size and j2k marker or jp2 file header.
            if(vrImage->Length)
            {
                streamData = (unsigned char *)vrImage->Data;
                instream_len = vrImage->Length;
                if( *streamData == 0xFF) break; //Jpeg stream.
                if( *streamData == 0x00 && streamData[1] == 0x00 && streamData[2] == 0x00
                   && streamData[3] == 0x0C && streamData[4] == 0x6A && streamData[5] == 0x50)
                { //Wow, a j2k file header again, dicom is normally just the stream.
                    streamData += 6;
                    instream_len -= 6;
                    for(i = 0; i < 512; i++)// I just picked 512.
                    { // Look for stream header.
                        if(*streamData == 0xFF && streamData[1] == 0x4F)break;
                        streamData++;
                        instream_len--;
                    }
                    if( i < 512 )break;// Found the stream.
                }
            }
        }
        if( currSQObject >= ArrayPtr->GetSize() )break;//Should not happen!
        // Loop back to open the memory as a stream.    
    }//End of the frames loop
    if(currFrame < frames)OperatorConsole.printf(
                                                 "Warn[DecompressJPEG2K]: Found %d of %d frames.\n",currFrame, frames);
    // Change the image vr to the bigger uncompressed and unencapsulated image.
    pVR->Reset();// Delete the old data including the array.
    pVR->Length = total_len;
    pVR->Data = outData;// The new uncompressed data.
    pVR->ReleaseMemory = TRUE;//Give the memory to the vr.
    // Set the image type.
    if(cps_jas == 1)// 8 bits
    {
        pVR->TypeCode ='OB';
    }
    else
    {
        pVR->TypeCode ='OW';
    }
    //  pDDO->DeleteVR(pVR);// replace the pixel data
    //  pDDO->Push(vrs);
    // Set color stuff if needed.
    if( numcmpts > 1)//color.
    {
        pDDO->ChangeVR( 0x0028, 0x0004, "RGB\0", 'CS');
        /*	pVR = pDDO->GetVR( 0x0028, 0x0004 );
         if(pVR)
         {
         if (pVR->Length != 4) pVR->ReAlloc(4);
         }
         else
         {
         pVR = new VR(0x0028, 0x0004, 4, TRUE);
         pDDO->Push(pVR);
         }
         memcpy((char *)pVR->Data, "RGB\0",4);*/
        // Optional planar configuration.
        pVR = pDDO->GetVR(0x0028, 0x0006); // Fix the planes VR if there.
        if(pVR && pVR->Length && *(char *)(pVR->Data) == 1)
        {
            *(UINT16 *)pVR->Data = 0;
        }
    }
    
    // Set the number of bits allocated.
    // 20120624: do not change highbit and bitsstored
    //if(cps_jas == 1) pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)8, 'US');
    //else pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)16, 'US');
    
    // DecompressJPEG2K: Set the number of bits stored.
    // mvh 20110502: write highbit for consistency
    // 20120624: do not change highbit and bitsstored
    // pDDO->ChangeVR( 0x0028, 0x0101, (UINT8)prec,     'US');
    // pDDO->ChangeVR( 0x0028, 0x0102, (UINT8)(prec-1), 'US');
    
    //Change the transfer syntax to LittleEndianExplict!
    pDDO->ChangeVR( 0x0002, 0x0010, "1.2.840.10008.1.2.1\0", 'IU');
    // If debug > 1, print when finished
    if (DebugLevel > 0)
        SystemDebug.printf("Jasper decompress time %u seconds.\n", (unsigned int)time(NULL) - t);
    return (TRUE);
}
#endif //End for LIBJASPER

#ifdef HAVE_LIBOPENJPEG
/* Error and message callback does not use the FILE* client object. */
void error_callback(const char *msg, void *client_data)
{
	UNUSED_ARGUMENT(client_data);
	OperatorConsole.printf("***[Libopenjpeg(J2k)]: %s\n", msg);
}

/* Warning callback expecting a FILE* client object. */
void warning_callback(const char *msg, void *client_data)
{
	UNUSED_ARGUMENT(client_data);
	OperatorConsole.printf("Warn[Libopenjpeg(J2k)]: %s\n", msg);
}

/* Debug callback expecting no client object. */
void info_callback(const char *msg, void *client_data)
{
	UNUSED_ARGUMENT(client_data);
	if(DebugLevel > 1)SystemDebug.printf("Info[Libopenjpeg(J2k)]: %s\n", msg);
}

/* This routine will take all of the listed color spaces in 08_03 of the standard
 *  in little endian, uncompressed format and  compress it to jpeg2000 in whatever
 *  format it came in.  I don't know if dicom viewers can support this. It uses the 
 *  openjpeg library from Communications and Remote Sensing Lab (TELE) in the
 *  Universitait Catholique de Louvain (UCL), and CNES with the support of the CS company,
 *  version 1.3.  You can get it here:
 *  http://code.google.com/p/openjpeg/downloads/list
 *  The library was built with USE_JPWL defined.
 *  OpenJPEG can compress anything, with any size for each plane, but the standard only
 *  allows YBR_RCT lossless and YBR_ICT lossy.  So if a YBR come in, it is lossy, and
 *  RGB is lossless.  If this is a problem I can  add a forced change in colorspace.
 *  If I have made some mistakes (most likely), you can contact me bruce.barton
 *  (the mail symbol goes here) bitsltd.net.  Let me know where I can find a sample of
 *  the image that didn't work. */
BOOL CompressJPEG2Ko(DICOMDataObject* pDDO, int j2kQuality)
{
	Array < DICOMDataObject	*>	*ArrayPtr, *ArrayImage;
	DICOMDataObject			*DDO;
	VR				*pVR, *vrImage, *vrs;
	int				colorTypeIn, codestream_length;
	UINT8				bitwa, prec, sgnd;
	INT32				rowCnt, byteCnt, imgSize;
	char				*colorASC;
	register char			*brcrBuffer, *buffer_ptr, *bufferPtr, *bufferg_ptr, *bufferb_ptr;
	register unsigned int		mask;
	register int			*colorBuffer_ptr[3];
	int				*colorBuffer[3], tempInt;
	unsigned int			currFrame, frames, i, t, inputArrayCnt, byteWidthIn, byteWidth;
	BOOL				planes;
    // OpenJPEG Stuff
	opj_cparameters_t		parameters;	// compression parameters.
	opj_event_mgr_t			event_mgr;	// event manager.
	opj_cio_t*			cio;
	opj_cinfo_t*			cinfo;
	opj_image_t			*image = NULL;
	opj_image_comp_t		*comp;
	const char			comment[] = "Created by OpenJPEG with JPWL version ";
	const size_t			clen =	strlen(comment);
	const char			*version = opj_version();
    
    // If debug > 0, get start time and set the level.
	t=0;
	if (DebugLevel > 0) t = (unsigned int)time(NULL);
	if (DebugLevel > 1) SystemDebug.printf("openJPEG compress started.\n");
    // Uninitialized warnings.
	ArrayImage = NULL;
	i = 0;
    // Check and set the quality for lossy.
	if(j2kQuality < MIN_QUALITY)// Set to 0 to use dicom.ini value. 
	{
                    j2kQuality = IniValue::GetInstance()->sscscpPtr->LossyQuality;//Use the default or dicom.ini value.
	}
	if(j2kQuality > 100) j2kQuality = 100;
    // Create an image    
	if(!(image = (opj_image_t*)calloc(1, sizeof(opj_image_t))))
	{
        OperatorConsole.printf("***[CompressJPEG2K]: Could not allocate an image structure.\n");
        return (FALSE);
	}
    // Set image offset
	image->x0 = 0;
	image->y0 = 0;
    // How many colors
	if(!(image->numcomps = pDDO->GetBYTE(0x0028, 0x0002))) image->numcomps = 1;// Gray default.
    // Decide if MCT should be used.
	parameters.tcp_mct = image->numcomps == 3 ? 1 : 0;
    // Are there frames?
    if(!(frames = pDDO->Getatoi(0x0028, 0x0008))) frames = 1;
	currFrame = 0;
    // Get the Rows VR and check size
	if(!(image->y1 = pDDO->GetUINT(0x0028, 0x0010)))
	{
		free(image);
		SystemDebug.printf("***[CompressJPEG2K]: Failed to get image height.\n");
		return(FALSE);
	}
	if(!(image->x1 = pDDO->GetUINT(0x0028, 0x0011)))
	{
		free(image);
		OperatorConsole.printf("***[CompressJPEG2K]: Failed to get image width.\n");
		return(FALSE);
	}
	imgSize = image->x1 * image->y1;
	
	if(!(bitwa = pDDO->GetBYTE(0x0028, 0x0100)))bitwa = 8; // 8 bit default.
	if(!(prec = pDDO->GetBYTE(0x0028, 0x0101)))prec = bitwa; //the default.
	if(prec > bitwa)bitwa = prec; // the bigger one.
	byteWidthIn = 1;
	byteWidth = 1;
	if(bitwa > 8)byteWidthIn = 2;
	if(prec > 8)byteWidth = 2;
	if(bitwa != 8 && bitwa != 12  && bitwa != 16)
	{
		free(image);
		OperatorConsole.printf("***[CompressJPEG2K]: Unsuported allocated bit width: %d.\n", bitwa);
		return(FALSE);
	}    
    // Checked if the data is signed,
	if(!(sgnd = pDDO->GetBYTE(0x0028, 0x0103)))sgnd = 0;
    // Planar configuration default.
	planes = FALSE;
    //Sort colors.
	colorTypeIn = image->color_space = CLRSPC_GRAY;// Set the default.
    colorASC = NULL;
	if(image->numcomps == 3)
	{
        pVR = pDDO->GetVR(0x0028, 0x0004); // Get the color profile
		if(pVR && pVR->Length > 2)colorASC = (char *)pVR->Data;
        // Look for the color type
		if(colorASC == NULL || strncmp(colorASC, "RGB",3)==0)// RGB
		{// Only RGB can be in planes or R,G,B format, check.
			if(pDDO->GetBYTE(0x0028, 0x0006)) planes = TRUE; // Check planes.
            /*			pVR = pDDO->GetVR(0x0028, 0x0006); // Get the plane's VR.
             if(pVR && pVR->Length && (*(char *)(pVR->Data) == 1)) planes = TRUE;*/
			colorTypeIn = image->color_space = CLRSPC_SRGB;
		}
		else if ( pVR->Length > 6 && strncmp(colorASC, "YBR_",4) == 0 )
		{
			image->color_space = CLRSPC_SYCC;
			if ((strncmp(colorASC+4, "ICT",3)==0)  || (strncmp(colorASC+4, "RCT",3)==0))
			{//Some decompressor forgot to change it.
				OperatorConsole.printf
				("Warn[CompressJPEG2K]: Uncompressed colorspace can not be YBR_ICT or YBR_RCT. Trying RGB\n");
				colorTypeIn = image->color_space = CLRSPC_SRGB;
			}
			else if (pVR->Length > 7 && strncmp(colorASC+4, "FULL",4)==0)// YBR_FULL(_422)
			{
				if (pVR->Length > 11 && strncmp(colorASC+8, "_422",4)==0)// YBR_FULL_422
				{
					colorTypeIn = CLRSPC_FAM_YCBCR_F422;
				}
				else// YBR_FULL
				{//Change color in to use planes, the compressor knows what it is.
					colorTypeIn = CLRSPC_SRGB;
					planes = TRUE;
				}
			}
			else if (pVR->Length > 14 && strncmp(&colorASC[4], "PARTIAL_42",10)==0)// YBR_PARTIAL_42x
			{
				if(colorASC[14] == '2')colorTypeIn = CLRSPC_FAM_YCBCR_422;
				else colorTypeIn = CLRSPC_FAM_YCBCR_420;
			}
		}// End of YBR_
        // Add more colors here.
		else
        {
            if((colorASC = pDDO->GetCString(0x0028, 0x0004)))
            {
                OperatorConsole.printf(
                                       "***[CompressJPEGL]: Unknown or unsuported color space %s.\n",colorASC);
                free(colorASC);
            }
            else OperatorConsole.printf(
                                        "***[CompressJPEGL]: Unknown or unsuported color space record.\n");
			free(image);
			return (FALSE);
		}//End of while color profile
	}//image->numcomps == 3
	else if(image->numcomps != 1 )// Must be gray or error.
	{
		OperatorConsole.printf("***[CompressJPEG2K]: Unsupported number of components %d.\n",
                               image->numcomps);
		free(image);
		return (FALSE);
	}
    // Get the data.
	inputArrayCnt = 0;
	pVR = pDDO->GetVR(0x7fe0, 0x0010); // Get the Image VR.
	vrImage = pVR;//Should be done here.
	if(pVR && pVR->Length)
	{
		if(pVR->SQObjectArray)
		{// Should not be here for uncompressed.
			ArrayImage = (Array<DICOMDataObject *> *) pVR->SQObjectArray;
			while (inputArrayCnt < ArrayImage->GetSize())
			{
				DDO = ArrayImage->Get(inputArrayCnt++);//Get the array.
				vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
				if(vrImage && vrImage->Length >= (unsigned int)(imgSize *image->numcomps * byteWidthIn)) break;
			}
			if(inputArrayCnt == ArrayImage->GetSize())
			{
				free(image);
				OperatorConsole.printf("***[CompressJPEG2K]: Could not find the image data.\n");
				return (FALSE);
			}
		}
	}
    // Check for a least one frame.
	if(!vrImage || vrImage->Length < (unsigned int)(imgSize *image->numcomps * byteWidthIn))
	{
		free(image);
		OperatorConsole.printf("***[CompressJPEG2K]: Could not find the image data.\n");
		return (FALSE);
	}
    //Create comment for codestream.
	if(!(parameters.cp_comment = (char*)malloc(clen+strlen(version)+1)))
	{
		free(image);
		OperatorConsole.printf("***[CompressJPEG2K]: Could allocate the coment buffer.\n");
		return (FALSE);
	}
	memcpy(parameters.cp_comment, comment, clen);
	memcpy(&parameters.cp_comment[clen], version, strlen(version));
	parameters.cp_comment[+strlen(version)] = 0;
    // Buffers are always needed for ints.
	for (tempInt = 0; tempInt < image->numcomps; tempInt++)
	{
		if(!(colorBuffer[tempInt] = (int *)malloc(imgSize  * sizeof(int))))
		{
			free(image);
			free(parameters.cp_comment);
			parameters.cp_comment = NULL;
			OperatorConsole.printf(
                                   "***[CompressJPEG2K]: Could not allocate a %d int image buffer #%d.\n",
                                   imgSize, tempInt);
			while(i > 0) free(colorBuffer[--tempInt]);
			return (FALSE);
		}
		colorBuffer_ptr[tempInt] = (int *)colorBuffer[tempInt];
	}
	buffer_ptr = (char *)vrImage->Data;
    //Almost never used (planes)
	bufferg_ptr = buffer_ptr + imgSize;
	bufferb_ptr = bufferg_ptr + imgSize;
    // Now for some more openJPEG.
    // Created from image_to_j2k.c and convert.c.
    // configure the event callbacks.
	memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
	event_mgr.error_handler = error_callback;
	event_mgr.warning_handler = warning_callback;
	event_mgr.info_handler = info_callback;
    // set encoding parameters values.
	opj_set_default_encoder_parameters(&parameters);
	if(j2kQuality < 100)parameters.tcp_rates[0] = 100/j2kQuality;
	else parameters.tcp_rates[0] = 0;// MOD antonin : losslessbug
	parameters.tcp_numlayers++;
	parameters.cp_disto_alloc = 1;
    // Lossy (ICT) or lossless (RCT)?
    if((j2kQuality < 100) || (image->color_space == CLRSPC_SYCC)) parameters.irreversible = 1;//ICT
    // allocate memory for the per-component information.
	if(!(image->comps = (opj_image_comp_t*)calloc(image->numcomps,sizeof(opj_image_comp_t))))
	{
		for(tempInt = 0; tempInt < image->numcomps; tempInt++)
		{
			if(colorBuffer[tempInt])
			{
				free(colorBuffer[tempInt]);
				colorBuffer[tempInt] = NULL;
			}
		}
		free(image);
		free(parameters.cp_comment);
		parameters.cp_comment = NULL;
		OperatorConsole.printf("***[CompressJPEG2K]: Could not create empty components.\n");
		return FALSE;
	}
    // Create the individual image components.
	for(tempInt = 0; tempInt < image->numcomps; tempInt++)
	{
		comp = &image->comps[tempInt];
		comp->dx = parameters.subsampling_dx;
		comp->dy = parameters.subsampling_dy;
		comp->w = image->x1;
		comp->h = image->y1;
		comp->x0 = 0;
		comp->y0 = 0;
		comp->prec = prec;
		comp->bpp = prec;
		comp->sgnd = sgnd;
        // From now, forward, opj_image_destroy(image) will free the buffers.
		comp->data = (int*)colorBuffer[tempInt];
	}
	// Print out some more info for debug.
	if (DebugLevel > 2)
	{
		if(parameters.irreversible == 0) SystemDebug.printf("JPEG2K Lossless");
		else SystemDebug.printf("JPEG2K Lossy Quality = %d", j2kQuality);
		SystemDebug.printf("\n, H = %d, W = %d, Bits = %d in %d, Frames = %d\n",
                           image->x1, image->y1, prec, bitwa, frames);
        if((colorASC = pDDO->GetCString(0x0028, 0x0004)))
        {
            SystemDebug.printf("color = %s\n", colorASC);
            free(colorASC);
        }
        else SystemDebug.printf("Unknown color space record.\n");
	}
    // Create the encapsulation array.
	ArrayPtr = new Array < DICOMDataObject * >;
    // The first blank object.
	DDO = new DICOMDataObject;
	vrs = new VR(0xfffe, 0xe000, 0, (void *)NULL, FALSE);
	DDO->Push(vrs);
	ArrayPtr->Add(DDO);
    // Now to fill the buffers.
	mask = ((1 << prec) - 1);
    // Frames loop.
	while(TRUE)
	{
        // Get a J2K compressor handle.
		cinfo = opj_create_compress(CODEC_J2K);
        //Catch events using our callbacks and give a local context.
		opj_set_event_mgr((opj_common_ptr)cinfo, &event_mgr, stderr);
        // Setup the encoder parameters using the current image and user parameters.
		opj_setup_encoder(cinfo, &parameters, image);
        // Open a byte stream for writing.
		if(!(cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0)))
		{
			OperatorConsole.printf("***[CompressJPEG2K]: Failed to allocate output stream memory.\n");
			opj_destroy_compress(cinfo);
			opj_image_destroy(image);
			free(parameters.cp_comment);
			parameters.cp_comment = NULL;
			return ( FALSE );
		}
		switch(colorTypeIn)
		{
			case CLRSPC_GRAY://12 or 12,16 bits here
				if(bitwa == 8) // 8 Bits, char to int.
				{
					for(byteCnt = 0; byteCnt < imgSize; byteCnt++)
					{
						*colorBuffer_ptr[0]++ = ((int)*buffer_ptr & mask);
						buffer_ptr++;
					}
					break;
				}
				if(bitwa == 16)
				{
					if(prec > 8)//Byte swap
					{
						for(byteCnt = 0; byteCnt < imgSize; byteCnt++)
						{
							*colorBuffer_ptr[0] = ((int)*buffer_ptr & 0xFF);
							buffer_ptr++;
							*colorBuffer_ptr[0]++ |= (((int)*buffer_ptr & 0xFF) << 8) & mask;
							buffer_ptr++;
						}
						break;
					}
					for(byteCnt = 0; byteCnt < imgSize; byteCnt++)// 8 in 16.
					{
						*colorBuffer_ptr[0] = ((int)*buffer_ptr  & mask);
						buffer_ptr++;
						buffer_ptr++;
					}
					break;
				}
				for(byteCnt = 0; byteCnt < imgSize; byteCnt++)// 12 bits allocated
				{
					*colorBuffer_ptr[0] = (((int)*buffer_ptr & 0xFF) << 4) & mask;//4-11
					buffer_ptr++;
					*colorBuffer_ptr[0]++ |= (((int)*buffer_ptr & 0x0F) << 4);// 0-3
					byteCnt++;
					if(byteCnt >= imgSize)break;
					*colorBuffer_ptr[0] = (((int)*buffer_ptr & 0x0F) << 8) & mask;//8-11
					buffer_ptr++;
					*colorBuffer_ptr[0]++ = (int)*buffer_ptr & 0xFF;//0-7
					buffer_ptr++;
				}
				break;
			case CLRSPC_SRGB:// or YBR_FULL
				if(planes)
				{
					for(byteCnt = 0; byteCnt < imgSize; byteCnt++)
					{
						*(colorBuffer_ptr[0])++ = (int)*buffer_ptr & mask;
						buffer_ptr++;
						*(colorBuffer_ptr[1])++ = (int)*bufferg_ptr & mask;
						bufferg_ptr++;
						*(colorBuffer_ptr[2])++ = (int)*bufferb_ptr & mask;
						bufferb_ptr++;
					}
					break;
				}
				for(byteCnt = 0; byteCnt < imgSize; byteCnt++)
				{
					*(colorBuffer_ptr[0])++ = (int)*buffer_ptr & mask;
					buffer_ptr++;
					*(colorBuffer_ptr[1])++ = (int)*buffer_ptr & mask;
					buffer_ptr++;
					*(colorBuffer_ptr[2])++ = (int)*buffer_ptr & mask;
					buffer_ptr++;
				}
				break;
			case CLRSPC_FAM_YCBCR_F422://Y1Y2CbCr
				for(byteCnt = 0; byteCnt < imgSize; byteCnt++)
				{
					byteCnt++;// 2 Ys per pass.
					*colorBuffer_ptr[0]++ = (int)*buffer_ptr & mask;
					buffer_ptr++;
					*colorBuffer_ptr[0]++ = (int)*buffer_ptr & mask;
					buffer_ptr++;
					*colorBuffer_ptr[1]++ = (int)*buffer_ptr & mask;
					*colorBuffer_ptr[1]++ = (int)*buffer_ptr & mask;
					buffer_ptr++;
					*colorBuffer_ptr[2]++ = (int)*buffer_ptr & mask;
					*colorBuffer_ptr[2]++ = (int)*buffer_ptr & mask;
					buffer_ptr++;
				}
				break;
			case CLRSPC_FAM_YCBCR_422://Y1Y2CbCr
				for(byteCnt = 0; byteCnt < imgSize; byteCnt++)
				{
					byteCnt++;// 2 Ys per pass.
					/* For Y the ratio is 256 / 220 (1.1636) and  for color 256 / 225 (1.1378).
					 *  Note: All values are multiplied by 1024 or 1 << 7.
					 *  Yo = 1.1636(Yi - 16)  == Yo = 1.1636Yi - 18.204 ==
					 *  Yo = [149Yi - 2330]/128 */
					tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1
					buffer_ptr++;
					if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
					else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
					else *colorBuffer_ptr[0]++ = ((tempInt >> 7) & 0xFF);
					tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y2
					buffer_ptr++;
					if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
					else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
					else *colorBuffer_ptr[0]++ = ((tempInt >> 7) & 0xFF);
					/*  Cxo = 1.1378(Cxi - 16)  == Cxo = 1.1378Cxi - 18.205 ==
					 *  Cxo = [73Cxi - 1152]/64 */
					tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
					buffer_ptr++;
					if(tempInt & 0x8000)
					{
						*colorBuffer_ptr[1]++ = 0;//Negitive
						*colorBuffer_ptr[1]++ = 0;//Negitive
					}
					else if(tempInt & 0x4000)
					{
						*colorBuffer_ptr[1]++ = 0xFF;//Over
						*colorBuffer_ptr[1]++ = 0xFF;//Over
					}
					else
					{
						*colorBuffer_ptr[1]++ = ((tempInt >> 6) & 0xFF);
						*colorBuffer_ptr[1]++ = ((tempInt >> 6) & 0xFF);
					}
					tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
					buffer_ptr++;
					if(tempInt & 0x8000)
					{
						*colorBuffer_ptr[2]++ = 0;//Negitive
						*colorBuffer_ptr[2]++ = 0;//Negitive
					}
					else if(tempInt & 0x4000)
					{
						*colorBuffer_ptr[2]++ = 0xFF;//Over
						*colorBuffer_ptr[2]++ = 0xFF;//Over
					}
					else
					{
						*colorBuffer_ptr[2]++ = ((tempInt >> 6) & 0xFF);
						*colorBuffer_ptr[2]++ = ((tempInt >> 6) & 0xFF);
						colorBuffer_ptr[2]++;
					}
				}
				break;
				/* What a strange standard. See page 323 of 08_03pu.pdf. */
			case CLRSPC_FAM_YCBCR_420://Y1CrCbY2Y3CrCbY4-Y1Y2Y3Y4?
				brcrBuffer = (char *)malloc(image->x1);
				for(rowCnt = 0; rowCnt < image->y1; rowCnt++)// 8 bit only.
				{
					rowCnt++;
					bufferPtr = brcrBuffer;
					for(byteCnt = 0; byteCnt < image->x1; byteCnt++)
					{
						byteCnt++;// 2 Ys per horizantal pass.
						tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1, Y3, ...
						buffer_ptr++;
						if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
						else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
						else *colorBuffer_ptr[0]++ = ((tempInt >> 7) & 0xFF);
						tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
						buffer_ptr++;
						if(tempInt & 0x8000)
						{
							*bufferPtr++ = 0;//Next time negitive = 0
							*colorBuffer_ptr[1]++ = 0;//Negitive
							*colorBuffer_ptr[1]++ = 0;//Negitive
						}
						else if(tempInt & 0x4000)
						{
							*bufferPtr++ = 0xFF;//Next time over = 255
							*colorBuffer_ptr[1]++ = 0xFF;//Over
							*colorBuffer_ptr[1]++ = 0xFF;//Over
						}
						else
						{
							*bufferPtr = (char)((tempInt >> 6) & 0xFF);
							*colorBuffer_ptr[1]++ = (int)*bufferPtr & 0xFF;
							*colorBuffer_ptr[1]++ = (int)*bufferPtr & 0xFF;
							bufferPtr++;
						}
						tempInt = (((int)*buffer_ptr) * 73) - 1152;//Cb
						buffer_ptr++;
						if(tempInt & 0x8000)
						{
							*bufferPtr++ = 0;//Next time negitive = 0
							*colorBuffer_ptr[2]++ = 0;//Negitive
							*colorBuffer_ptr[2]++ = 0;//Negitive
						}
						else if(tempInt & 0x4000)
						{
							*bufferPtr++ = 0xFF;//Next time over = 255
							*colorBuffer_ptr[2]++ = 0xFF;//Over
							*colorBuffer_ptr[2]++ = 0xFF;//Over
						}
						else
						{
							*bufferPtr = (char)((tempInt >> 6) & 0xFF);
							*colorBuffer_ptr[2]++ = (int)*bufferPtr & 0xFF;
							*colorBuffer_ptr[2]++ = (int)*bufferPtr & 0xFF;
							bufferPtr++;
						}
					}
					bufferPtr = brcrBuffer;
					for(byteCnt = 0; byteCnt < image->x1; byteCnt++)
					{
						byteCnt++;// 2 Ys per horizantal pass.
						tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1, Y3, ...
						buffer_ptr++;
						if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
						else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
						else *colorBuffer_ptr[0]++ = ((tempInt >> 7) & 0xFF);
						tempInt = (((int)*buffer_ptr) * 149) - 2330;//Y1, Y3, ...
						buffer_ptr++;
						if(tempInt & 0x10000)*colorBuffer_ptr[0]++ = 0;// Neg not allowed here.
						else if(tempInt & 0x8000)*colorBuffer_ptr[0]++ = 0xFF;// Over flow.
						else *colorBuffer_ptr[0]++ = (char)((tempInt >> 7) & 0xFF);
						*colorBuffer_ptr[1]++ = (int)*bufferPtr & 0xFF;
						*colorBuffer_ptr[1]++ = (int)*bufferPtr & 0xFF;
						bufferPtr++;
						*colorBuffer_ptr[2]++ = (int)*bufferPtr & 0xFF;
						*colorBuffer_ptr[2]++ = (int)*bufferPtr & 0xFF;
						bufferPtr++;
					}
				}
				free(brcrBuffer);
				break;
		}// End of the colorType switch.
        // encode the image.
		if(!(opj_encode(cinfo, cio, image, NULL)))
		{
			OperatorConsole.printf("***[CompressJPEG2K]: OpenJpeg could not encode the image.\n");
			while(ArrayPtr->GetSize())
			{
				delete ArrayPtr->Get(0);
				ArrayPtr->RemoveAt(0);
			}
			delete ArrayPtr;
			opj_destroy_compress(cinfo);
			opj_image_destroy(image);
			opj_cio_close(cio);
			free(parameters.cp_comment);
			parameters.cp_comment = NULL;
			return FALSE;
		}
		opj_destroy_compress(cinfo);//Done with cinfo.
		codestream_length = cio_tell(cio);
        // Jpeg2k is encapsulated, make a new vr to encapsulate.
		vrs = new VR(0xfffe, 0xe000, (UINT32)codestream_length, TRUE);
		if(!vrs)
		{
			OperatorConsole.printf("***[CompressJPEG2K]: Failed to allocate memory.\n");
			while(ArrayPtr->GetSize())
			{
				delete ArrayPtr->Get(0);
				ArrayPtr->RemoveAt(0);
			}
			delete ArrayPtr;
			opj_image_destroy(image);
			opj_cio_close(cio);
			free(parameters.cp_comment);
			parameters.cp_comment = NULL;
			return ( FALSE );
		}
        //If odd data length, zero the added byte.
		if ( codestream_length & 1  ) ((BYTE *)vrs->Data)[codestream_length] = 0;
        // Copy the Image data.
		brcrBuffer = (char *)vrs->Data;
		bufferPtr = (char *)cio->buffer;
		for(i = codestream_length; i > 0; --i) *brcrBuffer++ = *bufferPtr++;
        // Done with the code stream and cinfo.
		opj_cio_close(cio);
        // Save the image object. */        
		DDO = new DICOMDataObject;        
		DDO->Push(vrs);
		ArrayPtr->Add(DDO);
        //Out of data?
		if(++currFrame >= frames)break;//Done with all of the frames.
        // Deal with silly input arrays
		if(inputArrayCnt > 0  && (unsigned int)(imgSize *image->numcomps * byteWidthIn) < vrImage->Length)
		{// Look for the next array.
			while (inputArrayCnt < ArrayImage->GetSize())
			{
				DDO = ArrayImage->Get(inputArrayCnt++);//Get the array.
				vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
                // If found next one, break inner loop.
				if(vrImage && vrImage->Length >= (unsigned int)(imgSize *image->numcomps * byteWidthIn)) break;
			}
            // Not enough data for at least 1 frame, end frames loop.
			if(!vrImage || vrImage->Length < (unsigned int)(imgSize *image->numcomps * byteWidthIn)) break;
			buffer_ptr = (char *)vrImage->Data; // Point at the new buffer.
		}
        // No silly arrays ( or a least one big file in the array).
		else
		{
			if((unsigned int)(buffer_ptr - (char *)vrImage->Data + imgSize)
			   > vrImage->Length)break; //Out of data?
		}
        // Reset the buffers.
		for( tempInt = 0; tempInt < image->numcomps; tempInt++)
			colorBuffer_ptr[tempInt] = (int *)colorBuffer[tempInt];
		if(planes)//Jump the next 2 planes.
		{
			buffer_ptr = bufferb_ptr;
			bufferg_ptr = buffer_ptr + imgSize;
			bufferb_ptr = bufferg_ptr + imgSize;
		}
	}//Back for the next frame, end of while(TRUE)
    //Done with the comments.
	free(parameters.cp_comment);
	parameters.cp_comment = NULL;
    // Should we kill it and keep the uncompressed data?
	if(currFrame < frames) OperatorConsole.printf(
                                                  "Warn[CompressJPEG2K]: Only %d of %d frames saved.\n",currFrame, frames);
    // Finish encapsulating.
    DDO = new DICOMDataObject;
    vrs = new VR(0xfffe, 0xe0dd, 0, (void *)NULL, FALSE);
    DDO->Push(vrs);
    ArrayPtr->Add(DDO);
    // Attach the array to the vr.
    pVR->Reset(); // Deletes the pixel data
    pVR->SQObjectArray = ArrayPtr;
    pVR->TypeCode ='OW';
    // mvh: should not be needed - pDDO->Length = 0xFFFFFFFF;
    // Change the dicom parameters.
	if(image->numcomps == 3)
	{
        // Reset the plane's VR, if there.
        pVR = pDDO->GetVR(0x0028, 0x0006);
        if(pVR && pVR->Length && *(char *)(pVR->Data) == 1) *(char *)(pVR->Data) = 0;
        // Set the color profile
        if((j2kQuality < 100) || (image->color_space == CLRSPC_SYCC))
            pDDO->ChangeVR( 0x0028, 0x0004, "YBR_ICT\0", 'CS');
        else pDDO->ChangeVR( 0x0028, 0x0004, "YBR_RCT\0", 'CS');
        /*		pVR = pDDO->GetVR(0x0028, 0x0004);		if(pVR)
         {
         pVR->Length = 8;
         if(pVR->ReAlloc(pVR->Length))
         {
         if((j2kQuality < 100) || (image->color_space == CLRSPC_SYCC))
         memcpy(pVR->Data, "YBR_ICT\0",8);
         else memcpy(pVR->Data, "YBR_RCT\0",8);
         }
         }*/
	}
    // Fix the bits allocated.
    // 20120624: do not change highbit and bitsstored
    //if(byteWidth == 1)pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)8, 'US');
    //else pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)16, 'US');
    
    //Change the transfer syntax to JPEG2K!
    if((j2kQuality < 100) || (image->color_space == CLRSPC_SYCC))
        pDDO->ChangeVR( 0x0002, 0x0010, "1.2.840.10008.1.2.4.91\0", 'IU');
    else pDDO->ChangeVR( 0x0002, 0x0010, "1.2.840.10008.1.2.4.90\0", 'IU');
    // At last, done with the image.
	opj_image_destroy(image);
    // If debug > 0, print when finished
	if (DebugLevel > 0)
		SystemDebug.printf("OpenJPEG compress time %u seconds.\n", (unsigned int)time(NULL) - t);
    return (TRUE);
}

/* This routine will take in a jpeg2000 image and convert it to little endian, uncompressed,
 *  RGB or grayscale.  It uses the openjpeg library from Communications and Remote Sensing Lab (TELE)
 *  in the Universitait Catholique de Louvain (UCL), and CNES with the support of the CS company.
 *  You can get it here:
 *  http://code.google.com/p/openjpeg/downloads/list
 *  The library was built with USE_JPWL defined.
 *  JPEG200 can compress almost anything, with any size for each plane.  So that means any
 *  of the color spaces and formats can be in it.  The dicom standard removes the header
 *  and color box infromation and sends just the data stream.  The standard states only
 *  MONO1,2 PALETTE_COLOR YBR_RCT and YBR_ICT can be use.  For the color space and
 *  image format, I have to trust libopenjpeg.
 *  If I have made some mistakes (most likely) you can contact me bruce.barton
 *  (the mail symbol goes here) bitsltd.net.  Let me know where I can find a sample of
 *  the image that didn't work. */
BOOL DecompressJPEG2Ko(DICOMDataObject* pDDO)
{
    Array < DICOMDataObject	*>	*ArrayPtr;
    DICOMDataObject             *DDO;
    VR                          *pVR, *vrImage;
    register unsigned char      *outc_ptr;
    register UINT16				*out16_ptr;
	unsigned char				*streamData;
#ifdef NOVARAD_FIX
    void			*fixPtr;
    unsigned char		*fix1Ptr, *fix2Ptr;
#endif
    unsigned char		*outData;
    UINT16			frames;
    unsigned int                i, t;
    register int		*jpc_out[3], mask;
    int				bytes_jpc, prec_jpc;
    unsigned int		currFrame, currSQObject;
    UINT8			numcmpts;
    //    UINT16		bitwa, bitws,;
    UINT32			stream_len, instream_len, total_len;
    //	bool						bResult;
#if NATIVE_ENDIAN == BIG_ENDIAN //Big Endian like Apple power pc
    register int		masked;
#endif //Big Endian
    // openJPEG stuff.
    opj_dparameters_t		parameters;	// decompression parameters.
    opj_image_t			*decompImage = NULL;
    opj_event_mgr_t		event_mgr;		// event manager.
    opj_dinfo_t			*dinfo = NULL;	// handle to a decompressor.
    opj_cio_t			*cio = NULL;
	
    // If debug > 0, get start time.
	t = 0;
	if (DebugLevel > 0)t = (unsigned int)time(NULL);
	if (DebugLevel > 1) SystemDebug.printf("openJPEG decompress started.\n");
    // Init some variables	
	outData = NULL;
    // Uninitialized warnings.
	outc_ptr = NULL;
	out16_ptr = NULL;
	mask = 0;
	bytes_jpc = 1;
	prec_jpc = 8;
	total_len = 0;
	stream_len = 0;
    // Get the number of samples per pixel VR.
    if(!(numcmpts = pDDO->GetBYTE(0x0028, 0x0002))) numcmpts = 1;// Gray default.
    // Are there frames?
    if(!(frames = pDDO->Getatoi(0x0028, 0x0008))) frames = 1;
    currFrame = 0;
    // Get the encapsulated data.
    pVR = pDDO->GetVR(0x7fe0, 0x0010); // Get the Image VR.
	if(!pVR)
	{
        OperatorConsole.printf("***[DecompressJPEG2K]: No image VR\n");
        return (FALSE);
	}
	currSQObject = 0;// Init now for no warning later.
	if(pVR->SQObjectArray)
	{
		ArrayPtr = (Array<DICOMDataObject *> *) pVR->SQObjectArray;
		while(TRUE)
		{
			if( currSQObject >= ArrayPtr->GetSize())
			{
				OperatorConsole.printf("***[DecompressJPEG2K]: No j2k data found\n");
				return (FALSE);
			}
			DDO = ArrayPtr->Get(currSQObject );//Get the array.
			vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
			//Look for size and j2k marker or jp2 file header.
			if(vrImage->Length)
			{
				streamData = (unsigned char *)vrImage->Data;
				instream_len = vrImage->Length;
				if( *streamData == 0xFF) break; //Jpeg stream.
				if( *streamData == 0x00 && streamData[1] == 0x00 && streamData[2] == 0x00
				   && streamData[3] == 0x0C && streamData[4] == 0x6A && streamData[5] == 0x50)
				{ //Wow, a j2k file header, dicom is normally just the stream.
					streamData += 6;
					instream_len -= 6;
					for(i = 0; i < 512; i++)// I just picked 512.
					{ // Look for stream header.
						if(*streamData == 0xFF && streamData[1] == 0x4F)break;
						streamData++;
						instream_len--;
					}
					if( i < 512 )break;// Found the stream.
				}
			}
			currSQObject++;
		}
	}
	else
	{
		OperatorConsole.printf("***[DecompressJPEG2K]: No image in encapsulation arrray\n");
		return (FALSE);
	}
    // configure the event callbacks.
	memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
	event_mgr.error_handler = error_callback;
	event_mgr.warning_handler = warning_callback;
	event_mgr.info_handler = info_callback;
    // set decoding parameters to default values.
	opj_set_default_decoder_parameters(&parameters);
    // set decoding parameter format to stream.
	parameters.decod_format = CODEC_J2K;
    // Start the frames loop.
	while(TRUE)
	{
		// Fix the stream length.
		if(streamData[instream_len - 1] == 0) instream_len--;
#ifdef NOVARAD_FIX
		// Put the end of file back if not present.
		if ((streamData[instream_len - 2] != 0xFF)
			|| (streamData[instream_len - 1] != 0xD9))
		{
			if(!(fixPtr = malloc(vrImage->Length + 2)))
			{
				OperatorConsole.printf("***[DecompressJPEG2K]: Can not create memory fix buffer.\n");
				return (FALSE);
			}
			fix1Ptr = (unsigned char*)fixPtr;
			fix2Ptr = (unsigned char*)vrImage->Data;
			for( i = instream_len; i > 0; --i ) *fix1Ptr++ = *fix2Ptr++;
			*fix1Ptr++ = 0xFF;
			*fix1Ptr++ = 0xD9;
			if(instream_len & 1 ) *fix1Ptr = 0;
			free(vrImage->Data);
			vrImage->Data = fixPtr;
			vrImage->Length += 2;
			streamData = (unsigned char*)vrImage->Data;
			instream_len = vrImage->Length;
			if(DebugLevel > 0)SystemDebug.printf("Warn[DecompressJPEG2K]: Novarad fix, EOC marker added.\n");
		}
#endif
		// get a decoder handle.
		dinfo = opj_create_decompress(CODEC_J2K);//Bad openjpeg, can't be reused!
        // catch events using our callbacks and give a local context.
		opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, stderr);
        // setup the decoder decoding parameters using user parameters.
		opj_setup_decoder(dinfo, &parameters);
        // Open the memory as a stream.    
		if(!(cio = opj_cio_open((opj_common_ptr)dinfo, streamData, instream_len)))
		{
			opj_destroy_decompress(dinfo);
			OperatorConsole.printf("***[DecompressJPEG2K]: cannot open jpeg 2K data\n");
			return (FALSE);
		}
        // decode the stream and fill the image structure
		decompImage = opj_decode(dinfo, cio);
        // Check the image
		if (!decompImage)
		{
			OperatorConsole.printf("***[DecompressJPEG2K]: Jpeg 2K code stream decode did not create an image\n");
			opj_destroy_decompress(dinfo);
			opj_cio_close(cio);
			return FALSE;
		}
        // close the byte stream
		opj_cio_close(cio);	   
        // Do this the first time only
		if(!currFrame)
		{//Make the buffer.
            // Check for color.
			if(numcmpts > 1)
			{
				if((numcmpts != 3) || (decompImage->numcomps != 3))
				{
					OperatorConsole.printf(
                                           "***[DecompressJPEG2K]: Should be 3 colors, DICOM: %d ,J2K: %d \n",
                                           numcmpts , decompImage->numcomps);
					opj_image_destroy(decompImage);
					return ( FALSE );
				}
			}
            // Get the total uncompressed length.
			prec_jpc =decompImage->comps[0].prec;//Need it at the end
			bytes_jpc = ((prec_jpc -1) / 8) + 1;// Bytes or words.
			total_len = decompImage->comps[0].w * decompImage->comps[0].h
            * bytes_jpc * numcmpts * frames;
			if (( total_len & 1) != 0 ) total_len++;
			if(!(outData = (unsigned char *)malloc(total_len)))
			{
				OperatorConsole.printf(
                                       "***[DecompressJPEG2K]: Failed to allocate %d bytes of memory.\n", total_len);
				opj_image_destroy(decompImage);
				return ( FALSE );
			}
			outData[total_len -1] = 0;// Dosen't hurt.
			outc_ptr = outData;
			out16_ptr = (UINT16 *)outData;
            // The same for all images
			mask = (1 << prec_jpc) - 1;
			stream_len = (decompImage->comps[0].w * decompImage->comps[0].h);
		}//End of make the data buffer
        //Get the data pointer(s)
		for ( i = 0; i < numcmpts ; i++ )
		{
			jpc_out[i] = decompImage->comps[i].data;
		}
        // Image copy loops, open JPEG outputs ints.
		if( numcmpts == 1)
		{
			if(bytes_jpc == 2)
			{
				for(i = 0; i < stream_len; i++)
				{
#if NATIVE_ENDIAN == BIG_ENDIAN //Big Endian like Apple power pc
					masked = (UINT16)(*jpc_out[0]++ & mask);
					*outc_ptr++ = (char)(masked & 0xFF);
					*outc_ptr++ = (char)((masked >> 8) & 0xFF);
#else //Little Endian
					*out16_ptr++ = (UINT16)(*jpc_out[0]++ & mask);
#endif //Big Endian
				}
				
			}
			else
				for(i = 0; i < stream_len; i++)
					*outc_ptr++ = (unsigned char)(*jpc_out[0]++ & mask);
		}
		else //RGB
		{
			for(i = 0; i < stream_len; i++)
			{
				*outc_ptr++ = (unsigned char)(*jpc_out[0]++ & mask);
				*outc_ptr++ = (unsigned char)(*jpc_out[1]++ & mask);
				*outc_ptr++ = (unsigned char)(*jpc_out[2]++ & mask);
                
			}
        }
        // Done with libopenjpeg for this loop.
        opj_destroy_decompress(dinfo);
        opj_image_destroy(decompImage);
        // check for the end
        if(++currFrame >= frames)break;
        // More images to read
        while(++currSQObject < ArrayPtr->GetSize())
        {
            DDO = ArrayPtr->Get(currSQObject);//Get the array.
            vrImage = DDO->GetVR(0xfffe, 0xe000);//Get the data
            //Look for size and j2k marker or jp2 file header.
            if(vrImage->Length)
            {
                streamData = (unsigned char *)vrImage->Data;
                instream_len = vrImage->Length;
                if( *streamData == 0xFF) break; //Jpeg stream.
                if( *streamData == 0x00 && streamData[1] == 0x00 && streamData[2] == 0x00
                   && streamData[3] == 0x0C && streamData[4] == 0x6A && streamData[5] == 0x50)
                { //Wow, a j2k file header again, dicom is normally just the stream.
                    streamData += 6;
                    instream_len -= 6;
                    for(i = 0; i < 512; i++)// I just picked 512.
                    { // Look for stream header.
                        if(*streamData == 0xFF && streamData[1] == 0x4F)break;
                        streamData++;
                        instream_len--;
                    }
                    if( i < 512 )break;// Found the stream.
                }
            }
        }
        if( currSQObject >= ArrayPtr->GetSize() )break;//Should not happen!
        // Loop back to open the memory as a stream.    
    }//End of the frames loop
    // Done with libjpeg.
    //	if(dinfo) opj_destroy_decompress(dinfo);
    if(currFrame < frames)OperatorConsole.printf(
                                                 "Warn[DecompressJPEG2K]: Found %d of %d frames.\n",currFrame, frames);
    // Change the image vr to the bigger uncompressed and unencapsulated image.
    pVR->Reset(); // Deletes the pixel data
    pVR->Length = total_len;
    pVR->Data = outData;
    pVR->ReleaseMemory = TRUE;//Give the memory to the vr.
    // Set the image type.
    if(bytes_jpc == 1)// 8 bits
    {
        pVR->TypeCode ='OB';
    }
    else
    {
        pVR->TypeCode ='OW';
    }
	
    // The color stuff
    if(numcmpts    > 1)
    {
        //Set Planes value if needed, do  not create if not there.
        pVR = pDDO->GetVR(0x0028, 0x0006);
        if(pVR && pVR->Length && *(char *)pVR->Data == 1)
            *(char *)pVR->Data = 0;
        //Change the Photometric Interpretation.
        pDDO->ChangeVR( 0x0028, 0x0004, "RGB\0", 'CS');
    }
    // Set the number of bits allocated.
    // 20120624: do not change highbit and bitsstored
    //  if(bytes_jpc == 1) pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)8, 'US');
    //  else pDDO->ChangeVR( 0x0028, 0x0100, (UINT8)16, 'US');
    
    // mvh 20110502: DecompressJPEG2Ko: write highbit for consistency
    // 20120624: do not change highbit and bitsstored
    //pDDO->ChangeVR( 0x0028, 0x0101, (UINT8)prec_jpc,     'US');// Set the number of bits stored.
    //pDDO->ChangeVR( 0x0028, 0x0102, (UINT8)(prec_jpc-1), 'US');// Set high bit.
    
    //Change the transfer syntax to LittleEndianExplict!
    pDDO->ChangeVR( 0x0002, 0x0010, "1.2.840.10008.1.2.1\0", 'IU');
    // If debug > 0, print decompress time.
    if (DebugLevel > 0)
        SystemDebug.printf("OpenJPEG decompress time %u seconds.\n", (unsigned int)time(NULL) - t);
    return (TRUE);
}
#endif //End for libopenjpeg

