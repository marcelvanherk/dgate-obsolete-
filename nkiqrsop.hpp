#ifndef	_NKIQRSOP_H_
#	define	_NKIQRSOP_H_

/*
19990317	ljz	NKI-specific code
20001128	ljz	Fix: Crashes happened when more than one 'ServerChildThread' was
			active. m_pDCO was wrongly shared by all threads !!
20010429        mvh     Added GETADDO to allow optional read ahead withing calling program
20010502        mvh     Added extrabytes pointer to Read and RetrieveOn
20020415        mvh     ProcessDDO now returns status (to allow compression error check)
20020613	ljz	Added prototypes for DecompressNKI and CompressNKI
20021115	mvh	Added Generic retrieve classes
20030522	ljz	Added prototype of ComputeCRC
20030701        mvh     QualifyOn now also has compression parameter
20030702        mvh     added ExtendedPDU_Service
20030704        mvh     Changed ProcessDDO parameter to **DDO (for recompress)
20050118	mvh	replaced thread local storage under linux with variables in object
20050121	mvh	Changed filename to lower case
20050211	mvh	Removed need for thread local storage
20090209	mvh	Added QueryMoveScript callback
20091231	bcb	Added HAVE_LIBJPEG (version 6c!) and HAVE_JASPER for external library support (beta for now)
			Changed char* to const char* and cast time_t as int for gcc4.2 warnings
20100111	mvh	Merged
20100703	mvh	Merged some bcb OpenJPG changes
20100706	bcb	Added support for J2K and Jasper
20100721	mvh	Merged
20110118	mvh	Derived ExtendedPDU_Service from CheckedPDU_Service
20110118	mvh	Added lua_State to ExtendedPDU_Service
20110119	mvh	Added *VariableVRs and ThreadNum to ExtendedPDU_Service
20110122	mvh	Added ExtendedPDU_Service destructor
20120703	bcb	Clean up prototypes
20120710	bcb	Split up nkiqrsop into nkiqrsop, jpegconv.
20130321	bcb	Merged and size warnings.
*/

#include "dicom.hpp"
#include "lua.hpp"
/*
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
*/
typedef struct
{
	char	szModality[16];
	char	szSeriesInstanceUID[256];
	float	fSpacingX;
	float	fSpacingY;
	int	iDimX;
	int	iDimY;
	int	iNbFrames;
	int	iNbTimeSequences;
	char	szPatientPosition[16];
	int	iDataSize;
	BOOL	bNkiCompressed;
	int	iPixelRepresentation;
	int	iBitsStored;
	float	fRescaleIntercept;
	float	fRescaleSlope;
	float	fDoseGridScaling;
	BOOL	bRtImagePosOK;
	float	fRtImagePosX;
	float	fRtImagePosY;
	char	szPhotometricInterpretation[20];
} SLICE_INFO;


class	ExtendedPDU_Service	:
	public		CheckedPDU_Service
	{
		BOOL	AddTransferSyntaxs(PresentationContext &);	// override original function
		char	RequestedCompressionType[64];
		char	AcceptedCompressionType[64];
	public:
		BOOL	SetRequestedCompressionType(const char *type);
		char*	GetAcceptedCompressionType(UID uid);
		ExtendedPDU_Service (char *filename = NULL) : CheckedPDU_Service(filename) 
		{ L = NULL;
		  memset(VariableVRs, 0, sizeof(VariableVRs));
		  ThreadNum = 0;
		};
		~ExtendedPDU_Service () 
		{ if (L) lua_close(L);
		  if (VariableVRs[0]) delete VariableVRs[0];
		  if (VariableVRs[1]) delete VariableVRs[1];
		  if (VariableVRs[2]) delete VariableVRs[2];
		};

		// scripting state per association
		lua_State *L;
		VR *VariableVRs[3];
		int ThreadNum;
	};

class	StandardRetrieveNKI	:
	public	CMoveRQ,
	public	CMoveRSP
	{
	public:
		StandardRetrieveNKI();
		virtual	BOOL	GetUID (UID	&uid) { return (uGetUID(uid)); };
		virtual	BOOL	uGetUID ( UID &) = 0;
		virtual	BOOL	QueryMoveScript (PDU_Service *PDU, DICOMCommandObject *DCO, DICOMDataObject *DDO) = 0;
		virtual	BOOL	SearchOn ( DICOMDataObject	*,
					Array < DICOMDataObject *> *) = 0;
		virtual	BOOL	RetrieveOn (	DICOMDataObject *,
						DICOMDataObject **,
						StandardStorage	**,
						DICOMCommandObject	   *,
					        Array < DICOMDataObject *> *,
						void *) = 0;
		virtual	BOOL	QualifyOn ( BYTE *, BYTE *, BYTE *, BYTE *, BYTE * ) = 0;
		virtual	BOOL	CallBack (	DICOMCommandObject	*,
						DICOMDataObject	* ) = 0;
		BOOL	Read (	ExtendedPDU_Service *, DICOMCommandObject *, void *ExtraBytes );
		BOOL	Write ( PDU_Service	*, DICOMDataObject	*, BYTE	*);
	};


class	PatientRootRetrieveNKI	:
			public	StandardRetrieveNKI
	{
	public:
		BOOL	GetUID ( UID & );
		BOOL	uGetUID ( UID &uid ) { return ( GetUID(uid) ); };

		inline	BOOL	Read (	ExtendedPDU_Service	*PDU,
					DICOMCommandObject	*DCO,
					void 			*ExtraBytes )
			{ return ( StandardRetrieveNKI :: Read ( PDU, DCO, ExtraBytes ) ); };

		BOOL	Write (	PDU_Service	*PDU,
						DICOMDataObject	*DDO,
						BYTE	*ACRNema)
			{ return ( StandardRetrieveNKI :: Write ( PDU,
						DDO, ACRNema )); };
	};


class	StudyRootRetrieveNKI	:
			public	StandardRetrieveNKI
	{
	public:
		BOOL	GetUID ( UID & );
		BOOL	uGetUID ( UID &uid ) { return ( GetUID(uid) ); };
		
		inline	BOOL	Read (	ExtendedPDU_Service	*PDU,
					DICOMCommandObject	*DCO,
					void 			*ExtraBytes )
			{ return ( StandardRetrieveNKI :: Read ( PDU, DCO, ExtraBytes ) ); };

		BOOL	Write (	PDU_Service	*PDU,
						DICOMDataObject	*DDO,
						BYTE	*ACRNema)
			{ return ( StandardRetrieveNKI :: Write ( PDU,
						DDO, ACRNema )); };
	};


class	PatientStudyOnlyRetrieveNKI	:
			public	StandardRetrieveNKI
	{
	public:
		BOOL	GetUID ( UID & );
		BOOL	uGetUID ( UID &uid ) { return ( GetUID(uid) ); };
		
		inline	BOOL	Read (	ExtendedPDU_Service	*PDU,
					DICOMCommandObject	*DCO,
					void 			*ExtraBytes )
			{ return ( StandardRetrieveNKI :: Read ( PDU, DCO, ExtraBytes ) ); };

		BOOL	Write (	PDU_Service	*PDU,
						DICOMDataObject	*DDO,
						BYTE	*ACRNema)
			{ return ( StandardRetrieveNKI :: Write ( PDU,
						DDO, ACRNema )); };
	};
						

class	PatientRootRetrieveGeneric	:
			public	StandardRetrieveNKI
	{
	public:
		BOOL	GetUID ( UID & );
		BOOL	uGetUID ( UID &uid ) { return ( GetUID(uid) ); };

		inline	BOOL	Read (	ExtendedPDU_Service	*PDU,
					DICOMCommandObject	*DCO,
					void 			*ExtraBytes )
			{ return ( StandardRetrieveNKI :: Read ( PDU, DCO, ExtraBytes ) ); };

		BOOL	Write (	PDU_Service	*PDU,
						DICOMDataObject	*DDO,
						BYTE	*ACRNema)
			{ return ( StandardRetrieveNKI :: Write ( PDU,
						DDO, ACRNema )); };
	};


class	StudyRootRetrieveGeneric	:
			public	StandardRetrieveNKI
	{
	public:
		BOOL	GetUID ( UID & );
		BOOL	uGetUID ( UID &uid ) { return ( GetUID(uid) ); };
		
		inline	BOOL	Read (	ExtendedPDU_Service	*PDU,
					DICOMCommandObject	*DCO,
					void 			*ExtraBytes )
			{ return ( StandardRetrieveNKI :: Read ( PDU, DCO, ExtraBytes ) ); };

		BOOL	Write (	PDU_Service	*PDU,
						DICOMDataObject	*DDO,
						BYTE	*ACRNema)
			{ return ( StandardRetrieveNKI :: Write ( PDU,
						DDO, ACRNema )); };
	};


class	PatientStudyOnlyRetrieveGeneric	:
			public	StandardRetrieveNKI
	{
	public:
		BOOL	GetUID ( UID & );
		BOOL	uGetUID ( UID &uid ) { return ( GetUID(uid) ); };
		
		inline	BOOL	Read (	ExtendedPDU_Service	*PDU,
					DICOMCommandObject	*DCO,
					void 			*ExtraBytes )
			{ return ( StandardRetrieveNKI :: Read ( PDU, DCO, ExtraBytes ) ); };

		BOOL	Write (	PDU_Service	*PDU,
						DICOMDataObject	*DDO,
						BYTE	*ACRNema)
			{ return ( StandardRetrieveNKI :: Write ( PDU,
						DDO, ACRNema )); };
	};

unsigned int ComputeCRC(char* pcData, size_t iNbChars);//Was int, changed to use sizeof without warnings.
BOOL UseBuiltInDecompressor(BOOL *DecompressNon16BitsJpeg);
int nki_private_compress(signed char  *dest, short int  *src, int npixels, int iMode);
int get_nki_private_decompressed_length(signed char *src);
int get_nki_private_compress_mode(signed char *src);
int nki_private_decompress(short int *dest, signed char *src, int size);
BOOL DecompressNKI(DICOMDataObject* pDDO);
BOOL CompressNKI(DICOMDataObject* pDDO, int CompressMode);
BOOL ExtractFrame(DICOMDataObject* pDDO, unsigned int Frame);
int GetNumberOfFrames(DICOMDataObject* pDDO);
BOOL ExtractFrame(DICOMDataObject* pDDO, unsigned int Frame);
BOOL ExtractFrames(DICOMDataObject* pDDO, unsigned int FirstFrame, unsigned int LastFrame, int skip);
int MaybeDownsize(DICOMDataObject* pDDO, DICOMCommandObject* pDCO, int size);
BOOL ProcessDDO(DICOMDataObject** pDDO, DICOMCommandObject* pDCO, ExtendedPDU_Service *PDU);
int DcmConvertPixelData(DICOMDataObject*pDDO, bool bConvertToMono, bool	bCrop, int iStartX, int iEndX, int iStartY, int iEndY, float fPixelSizeX, float fPixelSizeY, float fPixelSizeZ);
void SaveDICOMDataObject(char *Filename, DICOMDataObject* pDDO);
SLICE_INFO*	getpSliceInfo(DICOMDataObject* pDDO);
char * DePlane(char* data, int length);
void DeYBRFULL(char* data, int length);
unsigned char * MakeSegTable(VR *pVR, unsigned int tableSize,unsigned int  *bitsWidth);
bool DecodePalette(DICOMDataObject* pDDO);
void NewTempFileWExt(char *name, const char *ext);
void NewTempFile(char *name);
//BOOL DecompressImageFile(char *file, int *Changed);
BOOL DecompressImage(DICOMDataObject **pDDO, int *Changed);
//BOOL CompressNKIImageFile(char *file, int lFileCompressMode, int *ActualMode);
//BOOL DownSizeImageFile(char *file, int lFileCompressMode, int *ActualMode);
//BOOL CompressJPEGImageFile(char *file, int lFileCompressMode, int *ActualMode);
BOOL CompressJPEGImage(DICOMDataObject **pDDO, int lFileCompressMode, int *ActualMode, int qual);
BOOL recompressFile(char *File, char *Compression, ExtendedPDU_Service *PDU);
static void Strip2(DICOMDataObject *pDDO);
char jpeg_type(VR *vr);
BOOL recompress(DICOMDataObject **pDDO, const char *Compression, const char *Filename, BOOL StripGroup2, ExtendedPDU_Service *PDU);
BOOL To8bitMonochromeOrRGB(DICOMDataObject* pDDO, int size, int *Dimx, int *Dimy, unsigned char **out, int RGBout=0, int level=0, int window=0, unsigned int frame=0);
BOOL ToGif(DICOMDataObject* pDDO, char *filename, int size, int append, int level, int window, unsigned int frame);
BOOL ToBMP(DICOMDataObject* pDDO, char *filename, int size, int append, int level, int window, unsigned int frame);
/*#ifdef HAVE_LIBJPEG
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
#endif //End LIBOPENJPEG*/
#ifdef UNIX // Statics
static signed char *recompress4bit(int n, signed char *dest);
static int TestDownsize(DICOMDataObject* pDDO, DICOMCommandObject* pDCO, int size);
static void RgbToMono(unsigned char* pSrc, unsigned char* pDest, int iNbPixels);
static bool CropImage(unsigned char* pSrc, unsigned char* pDest, int iSizeX, int iSizeY, int iPixelSize, int iStartX, int iEndX, int iStartY, int iEndY);
static BOOL ExecHidden(const char *ProcessBinary, const char *Arg1, const char *Arg2, const char *Arg3, const char *Env);
static int DecompressRLE(SLICE_INFO* pSliceInfo, VR* pSequence, void** ppResult, unsigned int * piResultSize);
#ifndef NOINTJPEG
#ifndef HAVE_LIBJPEG //Not needed for the jpeg library
static Uint16 readUint16(const Uint8* pData);
static Uint8 scanJpegDataForBitDepth(const Uint8 *data, const Uint32 fragmentLength);
static int DecompressJPEG(SLICE_INFO* pSliceInfo, VR* pSequence);
static int dcmdjpeg(DICOMDataObject* pDDO);
#endif // #ifndef HAVE_LIBJPEG
#endif // #ifndef NOINTJPEG
#else
signed char *recompress4bit(int n, signed char *dest);
int TestDownsize(DICOMDataObject* pDDO, DICOMCommandObject* pDCO, int size);
void RgbToMono(unsigned char* pSrc, unsigned char* pDest, int iNbPixels);
bool CropImage(unsigned char* pSrc, unsigned char* pDest, int iSizeX, int iSizeY, int iPixelSize, int iStartX, int iEndX, int iStartY, int iEndY);
BOOL ExecHidden(const char *ProcessBinary, const char *Arg1, const char *Arg2, const char *Arg3, const char *Arg4);
int DecompressRLE(SLICE_INFO* pSliceInfo, VR* pSequence, void** ppResult, unsigned int * piResultSize);
#ifndef NOINTJPEG
#ifndef HAVE_LIBJPEG //Not needed for the jpeg library
Uint16 readUint16(const Uint8* pData);
Uint8 scanJpegDataForBitDepth(const Uint8 *data, const Uint32 fragmentLength);
int DecompressJPEG(SLICE_INFO* pSliceInfo, VR* pSequence);
int dcmdjpeg(DICOMDataObject* pDDO);
#endif // #ifndef HAVE_LIBJPEG
#endif // #ifndef NOINTJPEG
#endif


/*********** xvgifwr prototypes used in nkiqrsop *********/
#ifndef hasColorStyleType
#define hasColorStyleType 1
enum ColorStyleType {csGlobalPalette, csLocalPalette};
#endif

int WriteGIF(FILE *fp, unsigned char *pic, int ptype, int w, int h, 
             unsigned char *rmap, unsigned char *gmap, 
             unsigned char *bmap, int numcols, int colorstyle, 
             const char *comment);
int WriteGIFHeader(FILE *fp, int w, int h, BYTE *rmap, BYTE *gmap, BYTE *bmap,
                   int numcols, ColorStyleType colorstyle, char *comment);
int WriteGIFFrame(FILE *fp, BYTE *p, int w, int h,
                  BYTE *rmap, BYTE *gmap, BYTE *bmap,
                  int numcols,
                  int frames, int time, ColorStyleType colorstyle);

#endif
