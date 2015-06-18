#include <boost/filesystem.hpp>

// work around the fact that dcmtk doesn't work in unicode mode, so all string operation needs to be converted from/to mbcs
#ifdef _UNICODE
#undef _UNICODE
#undef UNICODE
#define _UNDEFINEDUNICODE
#endif

#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/oflog/oflog.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmimage/diregist.h" // register support for color

#include "dcmtk/dcmdata/dcrledrg.h"	// rle decoder
#include "dcmtk/dcmjpeg/djdecode.h"	// jpeg decoder
#include "dcmtk/dcmjpeg/djencode.h" 
#include "dcmtk/dcmdata/dcrledrg.h" 
#include "dcmtk/dcmdata/dcrleerg.h" 
#include "fmjpeg2k/fmjpeg2kdrg.h"
#include "fmjpeg2k/fmjpeg2kccd.h"

// check DCMTK functionality
#if !defined(WIDE_CHAR_FILE_IO_FUNCTIONS) && defined(_WIN32)
#error "DCMTK and this program must be compiled with DCMTK_WIDE_CHAR_FILE_IO_FUNCTIONS"
#endif

#ifdef _UNDEFINEDUNICODE
#define _UNICODE 1
#define UNICODE 1
#endif

#include <wx/image.h>
#include <wx/rawbmp.h>

void dcm2img(boost::filesystem::path filename, int clientWidth, int clientHeight, wxImage &image)
{
	OFLog::configure(OFLogger::OFF_LOG_LEVEL);	
		
	unsigned char *source = NULL;

	try
	{
		DcmFileFormat dfile;
		if (dfile.loadFile(filename.c_str(), EXS_Unknown, EGL_withoutGL).bad())
			throw std::runtime_error("");
		if(dfile.loadAllDataIntoMemory().bad())
			throw std::runtime_error("");

		E_TransferSyntax xfer = EXS_Unknown; //dfile.getDataset()->getOriginalXfer();
		DicomImage di(&dfile, xfer, CIF_MayDetachPixelData, 0, 1);
		
		if(di.getStatus() != EIS_Normal)
			throw std::runtime_error("");

		di.getFirstFrame();
		di.showAllOverlays();			
		if (di.isMonochrome())
		{
			if(di.getWindowCount())
				di.setWindow(0);
			else
				di.setHistogramWindow();
		}
		
		// don't use di.createScaledImage, it is broken!!

		unsigned int width = di.getWidth();
		unsigned int height = di.getHeight();

		if(!image.Create(width, height, true))
			throw std::runtime_error("");
		
		source = new unsigned char[width * height * 4];		

		di.createWindowsDIB((void * &)source, width * height * 4, 0, 32);			
		
		wxImagePixelData data(image);
		wxImagePixelData::Iterator p(data);
		for(int j = 0; j < height; j++)
		{			
			for(int i = 0; i < width; i++)
			{
				p.MoveTo(data, i, j);
				p.Red() = source[(width * j + i) * 4 + 2];
				p.Green() = source[(width * j + i) * 4 + 1];
				p.Blue() = source[(width * j + i) * 4];
			}
		}	
		
		// scale
		float widthFactor = (float) clientWidth / (float) width;
        float heightFactor = (float) clientHeight / (float) height;

		float scaleFactor = 0;
        if (widthFactor < heightFactor) 
            scaleFactor = widthFactor;
        else
            scaleFactor = heightFactor;

        int scaledWidth  = width * scaleFactor;
        int scaledHeight = height * scaleFactor;

		image.Rescale(scaledWidth, scaledHeight);
	}
	catch(...)
	{

	}

	if(source != NULL)
		delete source;
	
}

void RegisterCodecs()
{
	DJDecoderRegistration::registerCodecs();
	DJEncoderRegistration::registerCodecs();    
	DcmRLEEncoderRegistration::registerCodecs();    
	DcmRLEDecoderRegistration::registerCodecs();
	DcmJPEG2000DecoderRegistration::registerCodecs();
	DiRegister::Pointer = new DiRegister();
}


void DeregisterCodecs()
{
	DJDecoderRegistration::cleanup();
	DJEncoderRegistration::cleanup();    
	DcmRLEEncoderRegistration::cleanup();    
	DcmRLEDecoderRegistration::cleanup();	
	DcmJPEG2000DecoderRegistration::cleanup();
}