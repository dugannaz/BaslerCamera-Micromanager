///////////////////////////////////////////////////////////////////////////////
// FILE:          Basler.cpp
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Implementation of Basler camera.
//                Device adapter for 
//		  Basler ace acA2000-340km camera 
//		  is implemented by minimal modification of Demo Camera example. 
//                Enables full speed image capture (340 fps) at resolution 2048x1088 
//                
// AUTHOR:        Nazim Dugan, dugannaz@gmail.com, 2014
//
// COPYRIGHT:     University of California, San Francisco, 2006-2015
//                100X Imaging Inc, 2008

#include "Basler.h"
#include <cstdio>
#include <string>
#include <math.h>
#include "../../MMDevice/ModuleInterface.h"
#include "../../MMCore/Error.h"
#include <sstream>
#include <algorithm>
#include "WriteCompactTiffRGB.h"
#include <iostream>

#include "cudaheader.h"



using namespace std;
const double CBaslerCamera::nominalPixelSizeUm_ = 1.0;
double g_IntensityFactor_ = 1.0;

// External names used used by the rest of the system
// to load particular device from the "DemoCamera.dll" library
const char* g_CameraDeviceName = "DCam";
const char* g_WheelDeviceName = "DWheel";
const char* g_StateDeviceName = "DStateDevice";
const char* g_LightPathDeviceName = "DLightPath";
const char* g_ObjectiveDeviceName = "DObjective";
const char* g_StageDeviceName = "DStage";
const char* g_XYStageDeviceName = "DXYStage";
const char* g_AutoFocusDeviceName = "DAutoFocus";
const char* g_ShutterDeviceName = "DShutter";
const char* g_DADeviceName = "D-DA";
const char* g_MagnifierDeviceName = "DOptovar";
const char* g_HubDeviceName = "DHub";

// constants for naming pixel types (allowed values of the "PixelType" property)
const char* g_PixelType_8bit = "8bit";
const char* g_PixelType_16bit = "16bit";
const char* g_PixelType_32bitRGB = "32bitRGB";
const char* g_PixelType_64bitRGB = "64bitRGB";
const char* g_PixelType_32bit = "32bit";  // floating point greyscale

// TODO: linux entry code

void WINAPI GlobalCallback(PMCSIGNALINFO SigInfo)
{
	

    if (SigInfo && SigInfo->Context)
    {
		switch(SigInfo->Signal)
		{
			case MC_SIG_SURFACE_PROCESSING:

				MCSTATUS st;
				
				st =  McGetParamInt(SigInfo->SignalInfo, MC_SurfaceAddr, (PINT32) &m_pCurrent1);

			case MC_SIG_ACQUISITION_FAILURE:
				break;
			default:
				break;

		}       
    }
}


///////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
///////////////////////////////////////////////////////////////////////////////

/**
 * List all suppoerted hardware devices here
 * Do not discover devices at runtime.  To avoid warnings about missing DLLs, Micro-Manager
 * maintains a list of supported device (MMDeviceList.txt).  This list is generated using 
 * information supplied by this function, so runtime discovery will create problems.
 */
MODULE_API void InitializeModuleData()
{
   AddAvailableDeviceName(g_CameraDeviceName, "Basler cuda reconstruction camera");
   AddAvailableDeviceName(g_WheelDeviceName, "Demo filter wheel");
   AddAvailableDeviceName(g_StateDeviceName, "Demo State Device");
   AddAvailableDeviceName(g_ObjectiveDeviceName, "Demo objective turret");
   AddAvailableDeviceName(g_StageDeviceName, "Demo stage");
   AddAvailableDeviceName(g_XYStageDeviceName, "Demo XY stage");
   AddAvailableDeviceName(g_LightPathDeviceName, "Demo light path");
   AddAvailableDeviceName(g_AutoFocusDeviceName, "Demo auto focus");
   AddAvailableDeviceName(g_ShutterDeviceName, "Demo shutter");
   AddAvailableDeviceName(g_DADeviceName, "Demo DA");
   AddAvailableDeviceName(g_MagnifierDeviceName, "Demo Optovar");
   AddAvailableDeviceName("TransposeProcessor", "TransposeProcessor");
   AddAvailableDeviceName("ImageFlipX", "ImageFlipX");
   AddAvailableDeviceName("ImageFlipY", "ImageFlipY");
   AddAvailableDeviceName("MedianFilter", "MedianFilter");
   AddAvailableDeviceName(g_HubDeviceName, "DHub");
}

MODULE_API MM::Device* CreateDevice(const char* deviceName)
{
   if (deviceName == 0)
      return 0;

   // decide which device class to create based on the deviceName parameter
   if (strcmp(deviceName, g_CameraDeviceName) == 0)
   {
      // create camera
      return new CBaslerCamera();
   }
   else if (strcmp(deviceName, g_WheelDeviceName) == 0)
   {
      // create filter wheel
      return new CDemoFilterWheel();
   }
   else if (strcmp(deviceName, g_ObjectiveDeviceName) == 0)
   {
      // create objective turret
      return new CDemoObjectiveTurret();
   }
   else if (strcmp(deviceName, g_StateDeviceName) == 0)
   {
      // create state device
      return new CDemoStateDevice();
   }
   else if (strcmp(deviceName, g_StageDeviceName) == 0)
   {
      // create stage
      return new CDemoStage();
   }
   else if (strcmp(deviceName, g_XYStageDeviceName) == 0)
   {
      // create stage
      return new CDemoXYStage();
   }
   else if (strcmp(deviceName, g_LightPathDeviceName) == 0)
   {
      // create light path
      return new CDemoLightPath();
   }
   else if (strcmp(deviceName, g_ShutterDeviceName) == 0)
   {
      // create shutter
      return new DemoShutter();
   }
   else if (strcmp(deviceName, g_DADeviceName) == 0)
   {
      // create DA
      return new DemoDA();
   }
   else if (strcmp(deviceName, g_AutoFocusDeviceName) == 0)
   {
      // create autoFocus
      return new DemoAutoFocus();
   }
   else if (strcmp(deviceName, g_MagnifierDeviceName) == 0)
   {
      // create Optovar 
      return new DemoMagnifier();
   }

   else if(strcmp(deviceName, "TransposeProcessor") == 0)
   {
      return new TransposeProcessor();
   }
   else if(strcmp(deviceName, "ImageFlipX") == 0)
   {
      return new ImageFlipX();
   }
   else if(strcmp(deviceName, "ImageFlipY") == 0)
   {
      return new ImageFlipY();
   }
   else if(strcmp(deviceName, "MedianFilter") == 0)
   {
      return new MedianFilter();
   }
   else if (strcmp(deviceName, g_HubDeviceName) == 0)
   {
	  return new DemoHub();
   }

   // ...supplied name not recognized
   return 0;
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
   delete pDevice;
}

///////////////////////////////////////////////////////////////////////////////
// CBaslerCamera implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
* CBaslerCamera constructor.
* Setup default all variables and create device properties required to exist
* before intialization. In this case, no such properties were required. All
* properties will be created in the Initialize() method.
*
* As a general guideline Micro-Manager devices do not access hardware in the
* the constructor. We should do as little as possible in the constructor and
* perform most of the initialization in the Initialize() method.
*/
CBaslerCamera::CBaslerCamera() :
   CCameraBase<CBaslerCamera> (),
   dPhase_(0),
   initialized_(false),
   readoutUs_(0.0),
   scanMode_(1),
   bitDepth_(8),
   roiX_(0),
   roiY_(0),
   sequenceStartTime_(0),
   isSequenceable_(false),
   sequenceMaxLength_(100),
   sequenceRunning_(false),
   sequenceIndex_(0),
	binSize_(1),
	cameraCCDXSize_(2040),
	cameraCCDYSize_(1088),
   ccdT_ (0.0),
   triggerDevice_(""),
   stopOnOverflow_(false),
	dropPixels_(false),
   fastImage_(false),
   saturatePixels_(false),
	fractionOfPixelsToDropOrSaturate_(0.002),
   pDemoResourceLock_(0),
   nComponents_(1)
{
   memset(testProperty_,0,sizeof(testProperty_));

   // call the base class method to set-up default error codes/messages
   InitializeDefaultErrorMessages();
   readoutStartTime_ = GetCurrentMMTime();
   pDemoResourceLock_ = new MMThreadLock();
   thd_ = new MySequenceThread(this);

   // parent ID display
   CreateHubIDProperty();

   // Initialize driver and error handling
   McOpenDriver(NULL);

   // Activate message box error handling and generate an error log file
   McSetParamInt (MC_CONFIGURATION, MC_ErrorHandling, MC_ErrorHandling_MSGBOX);
   McSetParamStr (MC_CONFIGURATION, MC_ErrorLog, "error.log");

   AfxEnableControlContainer();

   //SetRegistryKey(_T("Local AppWizard-Generated Applications"));

   //LoadStdProfileSettings();  // Load standard INI file options (including MRU)

   int nRet = CreateProperty("BLOCKx", "4", MM::Integer, false);
   assert(nRet == DEVICE_OK);
   nRet = CreateProperty("BLOCKy", "4", MM::Integer, false);
   assert(nRet == DEVICE_OK);
}

/**
* CBaslerCamera destructor.
* If this device used as intended within the Micro-Manager system,
* Shutdown() will be always called before the destructor. But in any case
* we need to make sure that all resources are properly released even if
* Shutdown() was not called.
*/
CBaslerCamera::~CBaslerCamera()
{
   StopSequenceAcquisition();
   delete thd_;
   delete pDemoResourceLock_;
}

/**
* Obtains device name.
* Required by the MM::Device API.
*/
void CBaslerCamera::GetName(char* name) const
{
   // Return the name used to referr to this device adapte
   CDeviceUtils::CopyLimitedString(name, g_CameraDeviceName);
}

/**
* Intializes the hardware.
* Required by the MM::Device API.
* Typically we access and initialize hardware at this point.
* Device properties are typically created here as well, except
* the ones we need to use for defining initialization parameters.
* Such pre-initialization properties are created in the constructor.
* (This device does not have any pre-initialization properties)
*/
int CBaslerCamera::Initialize()
{
    McSetParamInt(MC_BOARD + 0, MC_BoardTopology, MC_BoardTopology_MONO_DECA);

    // Create a channel and associate it with the first connector on the first board
    McCreate(MC_CHANNEL, &m_Channel);
    McSetParamInt(m_Channel, MC_DriverIndex, 0);

	// In order to use single camera on connector A
	// MC_Connector need to be set to A for Grablink Expert 2 and Grablink DualBase
	// For all the other Grablink boards the parameter has to be set to M  
	
	// For all GrabLink boards but Grablink Expert 2 and Dualbase
	McSetParamStr(m_Channel, MC_Connector, "M");
	// For Grablink Expert 2 and Dualbase
	//McSetParamStr(m_Channel, MC_Connector, "A");

	// Choose the video standard
    McSetParamStr(m_Channel, MC_CamFile, "acA2000-340km_P340RG");
    // Choose the camera expose duration
    McSetParamInt(m_Channel, MC_Expose_us, 2500);
    // Choose the pixel color format
    McSetParamInt(m_Channel, MC_ColorFormat, MC_ColorFormat_Y8);

    // For HFR //

     // Set the acquisition mode to High Frame Rate
    McSetParamInt(m_Channel, MC_AcquisitionMode, MC_AcquisitionMode_HFR);

    // Configure the height of a slice (107 lines)
    McSetParamInt(m_Channel, MC_Vactive_Ln, 1088);

    // Choose the number of Frames in a phase
	McSetParamInt(m_Channel, MC_PhaseLength_Fr, 1);

    // For HFR //


    // Configure triggering mode
    McSetParamInt(m_Channel, MC_TrigMode, MC_TrigMode_IMMEDIATE);
    McSetParamInt(m_Channel, MC_NextTrigMode, MC_NextTrigMode_REPEAT);

    // Configure triggering line
    // A rising edge on the triggering line generates a trigger.
    // See the TrigLine Parameter and the board documentation for more details.
    McSetParamInt(m_Channel, MC_TrigLine, MC_TrigLine_NOM);
    McSetParamInt(m_Channel, MC_TrigEdge, MC_TrigEdge_GOHIGH);
    McSetParamInt(m_Channel, MC_TrigFilter, MC_TrigFilter_ON);
	
    // Parameter valid for all Grablink but Full, DualBase, Base
    //McSetParamInt(m_Channel, MC_TrigCtl, MC_TrigCtl_ITTL);
    // Parameter valid only for Grablink Full, DualBase, Base
    McSetParamInt(m_Channel, MC_TrigCtl, MC_TrigCtl_ISO);
    //McSetParamInt(m_Channel, MC_TrigCtl, 10);

    // Choose the number of images to acquire
    //McSetParamInt(m_Channel, MC_SeqLength_Fr, 1);
    McSetParamInt(m_Channel, MC_SeqLength_Fr, MC_INDETERMINATE); // For HFR

    // Retrieve image dimensions
    McGetParamInt(m_Channel, MC_ImageSizeX, &m_SizeX);
    McGetParamInt(m_Channel, MC_ImageSizeY, &m_SizeY);
    McGetParamInt(m_Channel, MC_BufferPitch, &m_BufferPitch);

    // The memory allocation for the images is automatically done by Multicam when activating the channel.
    // We only set the number of surfaces to be created by MultiCam.
    McSetParamInt(m_Channel, MC_SurfaceCount, EURESYS_SURFACE_COUNT);

    // Enable MultiCam signals
    McSetParamInt(m_Channel, MC_SignalEnable + MC_SIG_SURFACE_PROCESSING, MC_SignalEnable_ON);
    McSetParamInt(m_Channel, MC_SignalEnable + MC_SIG_ACQUISITION_FAILURE, MC_SignalEnable_ON);

    //McSetParamInt(m_Channel, MC__NominalGain, 33);
    //McSetParamInt(m_Channel, MC_Gain, 33);
    //McSetParamInt(m_Channel, EC_PARAM_VideoGain, 0);


    // Register the callback function
    McRegisterCallback(m_Channel, GlobalCallback, this);
		
    if (initialized_)
      return DEVICE_OK;

    DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
    if (pHub)
    {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
    }
    else
      LogMessage(NoHubError);

   // set property list
   // -----------------

   // Name
   int nRet = CreateProperty(MM::g_Keyword_Name, g_CameraDeviceName, MM::String, true);
   if (DEVICE_OK != nRet)
      return nRet;

   // Description
   nRet = CreateProperty(MM::g_Keyword_Description, "Basler Camera Device Adapter", MM::String, true);
   if (DEVICE_OK != nRet)
      return nRet;

   // CameraName
   nRet = CreateProperty(MM::g_Keyword_CameraName, "BaslerCamera-MultiMode", MM::String, true);
   assert(nRet == DEVICE_OK);

   // CameraID
   nRet = CreateProperty(MM::g_Keyword_CameraID, "V1.0", MM::String, true);
   assert(nRet == DEVICE_OK);

   // binning
   CPropertyAction *pAct = new CPropertyAction (this, &CBaslerCamera::OnBinning);
   nRet = CreateProperty(MM::g_Keyword_Binning, "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   nRet = SetAllowedBinning();
   if (nRet != DEVICE_OK)
      return nRet;

   // pixel type
   pAct = new CPropertyAction (this, &CBaslerCamera::OnPixelType);
   nRet = CreateProperty(MM::g_Keyword_PixelType, g_PixelType_8bit, MM::String, false, pAct);
   assert(nRet == DEVICE_OK);

   vector<string> pixelTypeValues;
   pixelTypeValues.push_back(g_PixelType_8bit);
   pixelTypeValues.push_back(g_PixelType_16bit); 
   pixelTypeValues.push_back(g_PixelType_32bitRGB);
   pixelTypeValues.push_back(g_PixelType_64bitRGB);
   pixelTypeValues.push_back(::g_PixelType_32bit);

   nRet = SetAllowedValues(MM::g_Keyword_PixelType, pixelTypeValues);
   if (nRet != DEVICE_OK)
      return nRet;

   // Bit depth
   pAct = new CPropertyAction (this, &CBaslerCamera::OnBitDepth);
   nRet = CreateProperty("BitDepth", "8", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);

   vector<string> bitDepths;
   bitDepths.push_back("8");
   bitDepths.push_back("10");
   bitDepths.push_back("12");
   bitDepths.push_back("14");
   bitDepths.push_back("16");
   bitDepths.push_back("32");
   nRet = SetAllowedValues("BitDepth", bitDepths);
   if (nRet != DEVICE_OK)
      return nRet;

   // exposure
   nRet = CreateProperty(MM::g_Keyword_Exposure, "2.5", MM::Float, false);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits(MM::g_Keyword_Exposure, 0, 2500);

   CPropertyActionEx *pActX = 0;
   // create an extended (i.e. array) properties 1 through 4
	
   for(int ij = 1; ij < 7;++ij)
   {
      std::ostringstream os;
      os<<ij;
      std::string propName = "TestProperty" + os.str();
		pActX = new CPropertyActionEx(this, &CBaslerCamera::OnTestProperty, ij);
		nRet = CreateProperty(propName.c_str(), "0.", MM::Float, false, pActX);
      if(0!=(ij%5))
      {
         // try several different limit ranges
         double upperLimit = (double)ij*pow(10.,(double)(((ij%2)?-1:1)*ij));
         double lowerLimit = (ij%3)?-upperLimit:0.;
         SetPropertyLimits(propName.c_str(), lowerLimit, upperLimit);
      }
   }

   //pAct = new CPropertyAction(this, &CBaslerCamera::OnSwitch);
   //nRet = CreateProperty("Switch", "0", MM::Integer, false, pAct);
   //SetPropertyLimits("Switch", 8, 1004);
	
	
	// scan mode
   pAct = new CPropertyAction (this, &CBaslerCamera::OnScanMode);
   nRet = CreateProperty("ScanMode", "1", MM::Integer, false, pAct);
   assert(nRet == DEVICE_OK);
   AddAllowedValue("ScanMode","1");
   AddAllowedValue("ScanMode","2");
   AddAllowedValue("ScanMode","3");

   // camera gain
   nRet = CreateProperty(MM::g_Keyword_Gain, "-5", MM::Integer, false);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits(MM::g_Keyword_Gain, -5, 8);

   // camera offset
   nRet = CreateProperty(MM::g_Keyword_Offset, "0", MM::Integer, false);
   assert(nRet == DEVICE_OK);

   // camera temperature
   pAct = new CPropertyAction (this, &CBaslerCamera::OnCCDTemp);
   nRet = CreateProperty(MM::g_Keyword_CCDTemperature, "-100", MM::Float, false, pAct);
   assert(nRet == DEVICE_OK);
   SetPropertyLimits(MM::g_Keyword_CCDTemperature, -100, 10);

   // camera temperature RO
   pAct = new CPropertyAction (this, &CBaslerCamera::OnCCDTemp);
   nRet = CreateProperty("CCDTemperature RO", "0", MM::Float, true, pAct);
   assert(nRet == DEVICE_OK);

   // readout time
   pAct = new CPropertyAction (this, &CBaslerCamera::OnReadoutTime);
   nRet = CreateProperty(MM::g_Keyword_ReadoutTime, "0", MM::Float, false, pAct);
   assert(nRet == DEVICE_OK);

   // CCD size of the camera we are modeling
   pAct = new CPropertyAction (this, &CBaslerCamera::OnCameraCCDXSize);
   CreateProperty("OnCameraCCDXSize", "2040", MM::Integer, false, pAct);
   pAct = new CPropertyAction (this, &CBaslerCamera::OnCameraCCDYSize);
   CreateProperty("OnCameraCCDYSize", "1088", MM::Integer, false, pAct);

   // Trigger device
   pAct = new CPropertyAction (this, &CBaslerCamera::OnTriggerDevice);
   CreateProperty("TriggerDevice","", MM::String, false, pAct);

   pAct = new CPropertyAction (this, &CBaslerCamera::OnDropPixels);
   CreateProperty("DropPixels", "0", MM::Integer, false, pAct);
   AddAllowedValue("DropPixels", "0");
   AddAllowedValue("DropPixels", "1");

   pAct = new CPropertyAction (this, &CBaslerCamera::OnSaturatePixels);
   CreateProperty("SaturatePixels", "0", MM::Integer, false, pAct);
   AddAllowedValue("SaturatePixels", "0");
   AddAllowedValue("SaturatePixels", "1");

   pAct = new CPropertyAction (this, &CBaslerCamera::OnFastImage);
   CreateProperty("FastImage", "0", MM::Integer, false, pAct);
   AddAllowedValue("FastImage", "0");
   AddAllowedValue("FastImage", "1");

   pAct = new CPropertyAction (this, &CBaslerCamera::OnFractionOfPixelsToDropOrSaturate);
   CreateProperty("FractionOfPixelsToDropOrSaturate", "0.002", MM::Float, false, pAct);
   SetPropertyLimits("FractionOfPixelsToDropOrSaturate", 0., 0.1);

   // Whether or not to use exposure time sequencing
   pAct = new CPropertyAction (this, &CBaslerCamera::OnIsSequenceable);
   std::string propName = "UseExposureSequences";
   CreateProperty(propName.c_str(), "No", MM::String, false, pAct);
   AddAllowedValue(propName.c_str(), "Yes");
   AddAllowedValue(propName.c_str(), "No");

   // synchronize all properties
   // --------------------------
   nRet = UpdateStatus();
   if (nRet != DEVICE_OK)
      return nRet;


   // setup the buffer
   // ----------------
   nRet = ResizeImageBuffer();
   if (nRet != DEVICE_OK)
      return nRet;

#ifdef TESTRESOURCELOCKING
   TestResourceLocking(true);
   LogMessage("TestResourceLocking OK",true);
#endif


   initialized_ = true;

   // initialize image buffer
   GenerateEmptyImage(img_);

   char buf[MM::MaxStrLength];

   int bx, by;
   GetProperty("BLOCKx", buf);
   bx = atoi(buf);
   GetProperty("BLOCKy", buf);
   by = atoi(buf);
   method = initReconstruction(img_.Width(), img_.Height(), &bx, &by );

   char paddedx[10];
   char paddedy[10];
   sprintf(paddedx, "%d", bx);
   sprintf(paddedy, "%d", by);

   paddedX = bx;
   paddedY = by;

   nRet = CreateProperty("PaddedSizeX", paddedx, MM::Integer, true);
   assert(nRet == DEVICE_OK);
   nRet = CreateProperty("PaddedSizeY", paddedy, MM::Integer, true);
   assert(nRet == DEVICE_OK);

   return DEVICE_OK;

}

/**
* Shuts down (unloads) the device.
* Required by the MM::Device API.
* Ideally this method will completely unload the device and release all resources.
* Shutdown() may be called multiple times in a row.
* After Shutdown() we should be allowed to call Initialize() again to load the device
* without causing problems.
*/
int CBaslerCamera::Shutdown()
{
   // Set the channel to IDLE before deleting it
   McSetParamInt(m_Channel, MC_ChannelState, MC_ChannelState_IDLE);

   // Delete the channel
   McDelete(m_Channel);

   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   initialized_ = false;
   return DEVICE_OK;
}

/**
* Performs exposure and grabs a single image.
* This function should block during the actual exposure and return immediately afterwards 
* (i.e., before readout).  This behavior is needed for proper synchronization with the shutter.
* Required by the MM::Camera API.
*/
int CBaslerCamera::SnapImage()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

	static int callCounter = 0;
	++callCounter;

   MM::MMTime startTime = GetCurrentMMTime();
   double exp = GetExposure();
   if (sequenceRunning_ && IsCapturing()) 
   {
      exp = GetSequenceExposure();
   }

   // Start an acquisition sequence by activating the channel
   McSetParamInt(m_Channel, MC_ChannelState, MC_ChannelState_ACTIVE);

   // Generate a soft trigger event (STRG)
   McSetParamInt(m_Channel, MC_ForceTrig, MC_ForceTrig_TRIG);

   GetCameraImage(img_);
   //GenerateEmptyImage(img_);
   //GenerateSyntheticImage(img_,exp);

   MM::MMTime s0(0,0);
   if( s0 < startTime )
   {
      while (exp > (GetCurrentMMTime() - startTime).getMsec())
      {
         CDeviceUtils::SleepMs(1);
      }		
   }
   else
   {
      std::cerr << "You are operating this device adapter without setting the core callback, timing functions aren't yet available" << std::endl;
      // called without the core callback probably in off line test program
      // need way to build the core in the test program

   }
   readoutStartTime_ = GetCurrentMMTime();

   return DEVICE_OK;
}


/**
* Returns pixel data.
* Required by the MM::Camera API.
* The calling program will assume the size of the buffer based on the values
* obtained from GetImageBufferSize(), which in turn should be consistent with
* values returned by GetImageWidth(), GetImageHight() and GetImageBytesPerPixel().
* The calling program allso assumes that camera never changes the size of
* the pixel buffer on its own. In other words, the buffer can change only if
* appropriate properties are set (such as binning, pixel type, etc.)
*/
const unsigned char* CBaslerCamera::GetImageBuffer()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return 0;

   MMThreadGuard g(imgPixelsLock_);
   MM::MMTime readoutTime(readoutUs_);
   while (readoutTime > (GetCurrentMMTime() - readoutStartTime_)) {}		
   unsigned char *pB = (unsigned char*)(img_.GetPixels());
   return pB;
}

/**
* Returns image buffer X-size in pixels.
* Required by the MM::Camera API.
*/
unsigned CBaslerCamera::GetImageWidth() const
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return 0;

   return img_.Width();
}

/**
* Returns image buffer Y-size in pixels.
* Required by the MM::Camera API.
*/
unsigned CBaslerCamera::GetImageHeight() const
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return 0;

   return img_.Height();
}

/**
* Returns image buffer pixel depth in bytes.
* Required by the MM::Camera API.
*/
unsigned CBaslerCamera::GetImageBytesPerPixel() const
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return 0;

   return img_.Depth();
} 

/**
* Returns the bit depth (dynamic range) of the pixel.
* This does not affect the buffer size, it just gives the client application
* a guideline on how to interpret pixel values.
* Required by the MM::Camera API.
*/
unsigned CBaslerCamera::GetBitDepth() const
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return 0;

   return bitDepth_;
}

/**
* Returns the size in bytes of the image buffer.
* Required by the MM::Camera API.
*/
long CBaslerCamera::GetImageBufferSize() const
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return 0;

   return img_.Width() * img_.Height() * GetImageBytesPerPixel();
}

/**
* Sets the camera Region Of Interest.
* Required by the MM::Camera API.
* This command will change the dimensions of the image.
* Depending on the hardware capabilities the camera may not be able to configure the
* exact dimensions requested - but should try do as close as possible.
* If the hardware does not have this capability the software should simulate the ROI by
* appropriately cropping each frame.
* This demo implementation ignores the position coordinates and just crops the buffer.
* @param x - top-left corner coordinate
* @param y - top-left corner coordinate
* @param xSize - width
* @param ySize - height
*/
int CBaslerCamera::SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (xSize == 0 && ySize == 0)
   {
      // effectively clear ROI
      ResizeImageBuffer();
      roiX_ = 0;
      roiY_ = 0;
   }
   else
   {
      // apply ROI
      img_.Resize(xSize, ySize);
      roiX_ = x;
      roiY_ = y;
   }
   return DEVICE_OK;
}

/**
* Returns the actual dimensions of the current ROI.
* Required by the MM::Camera API.
*/
int CBaslerCamera::GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   x = roiX_;
   y = roiY_;

   xSize = img_.Width();
   ySize = img_.Height();

   return DEVICE_OK;
}

/**
* Resets the Region of Interest to full frame.
* Required by the MM::Camera API.
*/
int CBaslerCamera::ClearROI()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   ResizeImageBuffer();
   roiX_ = 0;
   roiY_ = 0;
      
   return DEVICE_OK;
}

/**
* Returns the current exposure setting in milliseconds.
* Required by the MM::Camera API.
*/
double CBaslerCamera::GetExposure() const
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   char buf[MM::MaxStrLength];
   int ret = GetProperty(MM::g_Keyword_Exposure, buf);
   if (ret != DEVICE_OK)
      return 0.0;
   return atof(buf);
}

/**
 * Returns the current exposure from a sequence and increases the sequence counter
 * Used for exposure sequences
 */
double CBaslerCamera::GetSequenceExposure() 
{
   if (exposureSequence_.size() == 0) 
      return this->GetExposure();

   double exposure = exposureSequence_[sequenceIndex_];

   sequenceIndex_++;
   if (sequenceIndex_ >= exposureSequence_.size())
      sequenceIndex_ = 0;

   return exposure;
}

/**
* Sets exposure in milliseconds.
* Required by the MM::Camera API.
*/
void CBaslerCamera::SetExposure(double exp)
{
   SetProperty(MM::g_Keyword_Exposure, CDeviceUtils::ConvertToString(exp));
   GetCoreCallback()->OnExposureChanged(this, exp);;
}

/**
* Returns the current binning factor.
* Required by the MM::Camera API.
*/
int CBaslerCamera::GetBinning() const
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   char buf[MM::MaxStrLength];
   int ret = GetProperty(MM::g_Keyword_Binning, buf);
   if (ret != DEVICE_OK)
      return 1;
   return atoi(buf);
}

/**
* Sets binning factor.
* Required by the MM::Camera API.
*/
int CBaslerCamera::SetBinning(int binF)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   return SetProperty(MM::g_Keyword_Binning, CDeviceUtils::ConvertToString(binF));
}

/**
 * Clears the list of exposures used in sequences
 */
int CBaslerCamera::ClearExposureSequence()
{
   exposureSequence_.clear();
   return DEVICE_OK;
}

/**
 * Adds an exposure to a list of exposures used in sequences
 */
int CBaslerCamera::AddToExposureSequence(double exposureTime_ms) 
{
   exposureSequence_.push_back(exposureTime_ms);
   return DEVICE_OK;
}

int CBaslerCamera::SetAllowedBinning() 
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   vector<string> binValues;
   binValues.push_back("1");
   binValues.push_back("2");
   if (scanMode_ < 3)
      binValues.push_back("4");
   if (scanMode_ < 2)
      binValues.push_back("8");
   if (binSize_ == 8 && scanMode_ == 3) {
      SetProperty(MM::g_Keyword_Binning, "2");
   } else if (binSize_ == 8 && scanMode_ == 2) {
      SetProperty(MM::g_Keyword_Binning, "4");
   } else if (binSize_ == 4 && scanMode_ == 3) {
      SetProperty(MM::g_Keyword_Binning, "2");
   }
      
   LogMessage("Setting Allowed Binning settings", true);
   return SetAllowedValues(MM::g_Keyword_Binning, binValues);
}


/**
 * Required by the MM::Camera API
 * Please implement this yourself and do not rely on the base class implementation
 * The Base class implementation is deprecated and will be removed shortly
 */
int CBaslerCamera::StartSequenceAcquisition(double interval) {
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   return StartSequenceAcquisition(LONG_MAX, interval, false);            
}

/**                                                                       
* Stop and wait for the Sequence thread finished                                   
*/                                                                        
int CBaslerCamera::StopSequenceAcquisition()                                     
{
   if (IsCallbackRegistered())
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;
   }

   if (!thd_->IsStopped()) {
      thd_->Stop();                                                       
      thd_->wait();                                                       
   }                                                                      
                                                                          
   return DEVICE_OK;                                                      
} 

/**
* Simple implementation of Sequence Acquisition
* A sequence acquisition should run on its own thread and transport new images
* coming of the camera into the MMCore circular buffer.
*/
int CBaslerCamera::StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (IsCapturing())
      return DEVICE_CAMERA_BUSY_ACQUIRING;

   int ret = GetCoreCallback()->PrepareForAcq(this);
   if (ret != DEVICE_OK)
      return ret;
   sequenceStartTime_ = GetCurrentMMTime();
   imageCounter_ = 0;
   thd_->Start(numImages,interval_ms);
   stopOnOverflow_ = stopOnOverflow;
   return DEVICE_OK;
}

/*
 * Inserts Image and MetaData into MMCore circular Buffer
 */
int CBaslerCamera::InsertImage()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   MM::MMTime timeStamp = this->GetCurrentMMTime();
   char label[MM::MaxStrLength];
   this->GetLabel(label);
 
   // Important:  metadata about the image are generated here:
   Metadata md;
   md.put("Camera", label);
   md.put(MM::g_Keyword_Metadata_StartTime, CDeviceUtils::ConvertToString(sequenceStartTime_.getMsec()));
   md.put(MM::g_Keyword_Elapsed_Time_ms, CDeviceUtils::ConvertToString((timeStamp - sequenceStartTime_).getMsec()));
   md.put(MM::g_Keyword_Metadata_ROI_X, CDeviceUtils::ConvertToString( (long) roiX_)); 
   md.put(MM::g_Keyword_Metadata_ROI_Y, CDeviceUtils::ConvertToString( (long) roiY_)); 

   imageCounter_++;

   char buf[MM::MaxStrLength];
   GetProperty(MM::g_Keyword_Binning, buf);
   md.put(MM::g_Keyword_Binning, buf);

   MMThreadGuard g(imgPixelsLock_);

   const unsigned char* pI;
   pI = GetImageBuffer();

   unsigned int w = GetImageWidth();
   unsigned int h = GetImageHeight();
   unsigned int b = GetImageBytesPerPixel();

   int ret = GetCoreCallback()->InsertImage(this, pI, w, h, b, md.Serialize().c_str());
   if (!stopOnOverflow_ && ret == DEVICE_BUFFER_OVERFLOW)
   {
      // do not stop on overflow - just reset the buffer
      GetCoreCallback()->ClearImageBuffer(this);
      // don't process this same image again...
      return GetCoreCallback()->InsertImage(this, pI, w, h, b, md.Serialize().c_str(), false);
   } else
      return ret;
}

/*
 * Do actual capturing
 * Called from inside the thread  
 */
int CBaslerCamera::ThreadRun (MM::MMTime startTime)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   int ret=DEVICE_ERR;
   
   // Trigger
   if (triggerDevice_.length() > 0) {
      MM::Device* triggerDev = GetDevice(triggerDevice_.c_str());
      if (triggerDev != 0) {
      	LogMessage("trigger requested");
      	triggerDev->SetProperty("Trigger","+");
      }
   }
   
   if (!fastImage_)
   {
      //GenerateSyntheticImage(img_, GetSequenceExposure());
	  GetCameraImage(img_);
   }

   ret = InsertImage();
     

   while (((double) (this->GetCurrentMMTime() - startTime).getMsec() / imageCounter_) < this->GetSequenceExposure())
   {
      CDeviceUtils::SleepMs(1);
   }

   if (ret != DEVICE_OK)
   {
      return ret;
   }
   return ret;
};

bool CBaslerCamera::IsCapturing() {
   return !thd_->IsStopped();
}

/*
 * called from the thread function before exit 
 */
void CBaslerCamera::OnThreadExiting() throw()
{
   try
   {
      LogMessage(g_Msg_SEQUENCE_ACQUISITION_THREAD_EXITING);
      GetCoreCallback()?GetCoreCallback()->AcqFinished(this,0):DEVICE_OK;
   }

   catch( CMMError& e){
      std::ostringstream oss;
      oss << g_Msg_EXCEPTION_IN_ON_THREAD_EXITING << " " << e.getMsg() << " " << e.getCode();
      LogMessage(oss.str().c_str(), false);
   }
   catch(...)
   {
      LogMessage(g_Msg_EXCEPTION_IN_ON_THREAD_EXITING, false);
   }
}


MySequenceThread::MySequenceThread(CBaslerCamera* pCam)
   :intervalMs_(default_intervalMS)
   ,numImages_(default_numImages)
   ,imageCounter_(0)
   ,stop_(true)
   ,suspend_(false)
   ,camera_(pCam)
   ,startTime_(0)
   ,actualDuration_(0)
   ,lastFrameTime_(0)
{};

MySequenceThread::~MySequenceThread() {};

void MySequenceThread::Stop() {
   MMThreadGuard(this->stopLock_);
   stop_=true;
}

void MySequenceThread::Start(long numImages, double intervalMs)
{
   MMThreadGuard(this->stopLock_);
   MMThreadGuard(this->suspendLock_);
   numImages_=numImages;
   intervalMs_=intervalMs;
   imageCounter_=0;
   stop_ = false;
   suspend_=false;
   activate();
   actualDuration_ = 0;
   startTime_= camera_->GetCurrentMMTime();
   lastFrameTime_ = 0;
}

bool MySequenceThread::IsStopped(){
   MMThreadGuard(this->stopLock_);
   return stop_;
}

void MySequenceThread::Suspend() {
   MMThreadGuard(this->suspendLock_);
   suspend_ = true;
}

bool MySequenceThread::IsSuspended() {
   MMThreadGuard(this->suspendLock_);
   return suspend_;
}

void MySequenceThread::Resume() {
   MMThreadGuard(this->suspendLock_);
   suspend_ = false;
}

int MySequenceThread::svc(void) throw()
{
   int ret=DEVICE_ERR;
   try 
   {
      do
      {  
         ret=camera_->ThreadRun(startTime_);
      } while (DEVICE_OK == ret && !IsStopped() && imageCounter_++ < numImages_-1);
      if (IsStopped())
         camera_->LogMessage("SeqAcquisition interrupted by the user\n");

   }catch( CMMError& e){
      camera_->LogMessage(e.getMsg(), false);
      ret = e.getCode();
   }catch(...){
      camera_->LogMessage(g_Msg_EXCEPTION_IN_THREAD, false);
   }
   stop_=true;
   actualDuration_ = camera_->GetCurrentMMTime() - startTime_;
   camera_->OnThreadExiting();
   return ret;
}


///////////////////////////////////////////////////////////////////////////////
// CBaslerCamera Action handlers
///////////////////////////////////////////////////////////////////////////////

/*
* this Read Only property will update whenever any property is modified
*/

int CBaslerCamera::OnTestProperty(MM::PropertyBase* pProp, MM::ActionType eAct, long indexx)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(testProperty_[indexx]);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(testProperty_[indexx]);
   }
	return DEVICE_OK;

}

//int CBaslerCamera::OnSwitch(MM::PropertyBase* pProp, MM::ActionType eAct)
//{
   // use cached values
//   return DEVICE_OK;
//}

/**
* Handles "Binning" property.
*/
int CBaslerCamera::OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   int ret = DEVICE_ERR;
   switch(eAct)
   {
   case MM::AfterSet:
      {
         if(IsCapturing())
            return DEVICE_CAMERA_BUSY_ACQUIRING;

         // the user just set the new value for the property, so we have to
         // apply this value to the 'hardware'.
         long binFactor;
         pProp->Get(binFactor);
			if(binFactor > 0 && binFactor < 10)
			{
				img_.Resize(cameraCCDXSize_/binFactor, cameraCCDYSize_/binFactor);
				binSize_ = binFactor;
            std::ostringstream os;
            os << binSize_;
            OnPropertyChanged("Binning", os.str().c_str());
				ret=DEVICE_OK;
			}
      }break;
   case MM::BeforeGet:
      {
         ret=DEVICE_OK;
			pProp->Set(binSize_);
      }break;
   default:
      break;
   }
   return ret; 
}

/**
* Handles "PixelType" property.
*/
int CBaslerCamera::OnPixelType(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   int ret = DEVICE_ERR;
   switch(eAct)
   {
   case MM::AfterSet:
      {
         if(IsCapturing())
            return DEVICE_CAMERA_BUSY_ACQUIRING;

         string pixelType;
         pProp->Get(pixelType);

         if (pixelType.compare(g_PixelType_8bit) == 0)
         {
            nComponents_ = 1;
            img_.Resize(img_.Width(), img_.Height(), 1);
            bitDepth_ = 8;
            ret=DEVICE_OK;
         }
         else if (pixelType.compare(g_PixelType_16bit) == 0)
         {
            nComponents_ = 1;
            img_.Resize(img_.Width(), img_.Height(), 2);
            ret=DEVICE_OK;
         }
			else if ( pixelType.compare(g_PixelType_32bitRGB) == 0)
			{
            nComponents_ = 4;
            img_.Resize(img_.Width(), img_.Height(), 4);
            ret=DEVICE_OK;
			}
			else if ( pixelType.compare(g_PixelType_64bitRGB) == 0)
			{
            nComponents_ = 4;
            img_.Resize(img_.Width(), img_.Height(), 8);
            ret=DEVICE_OK;
			}
         else if ( pixelType.compare(g_PixelType_32bit) == 0)
			{
            nComponents_ = 1;
            img_.Resize(img_.Width(), img_.Height(), 4);
            ret=DEVICE_OK;
			}
         else
         {
            // on error switch to default pixel type
            nComponents_ = 1;
            img_.Resize(img_.Width(), img_.Height(), 1);
            pProp->Set(g_PixelType_8bit);
            ret = ERR_UNKNOWN_MODE;
         }
      } break;
   case MM::BeforeGet:
      {
         long bytesPerPixel = GetImageBytesPerPixel();
         if (bytesPerPixel == 1)
         	pProp->Set(g_PixelType_8bit);
         else if (bytesPerPixel == 2)
         	pProp->Set(g_PixelType_8bit); //nazim
         else if (bytesPerPixel == 4)
         {
            if(4 == this->nComponents_) // todo SEPARATE bitdepth from #components
				   pProp->Set(g_PixelType_32bitRGB);
            else if( 1 == nComponents_)
               pProp->Set(::g_PixelType_32bit);
         }
         else if (bytesPerPixel == 8) // todo SEPARATE bitdepth from #components
				pProp->Set(g_PixelType_64bitRGB);
			else
				pProp->Set(g_PixelType_8bit);
         ret=DEVICE_OK;
      } break;
   default:
      break;
   }
   return ret; 
}

/**
* Handles "BitDepth" property.
*/
int CBaslerCamera::OnBitDepth(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   int ret = DEVICE_ERR;
   switch(eAct)
   {
   case MM::AfterSet:
      {
         if(IsCapturing())
            return DEVICE_CAMERA_BUSY_ACQUIRING;

         long bitDepth;
         pProp->Get(bitDepth);

			unsigned int bytesPerComponent;

         switch (bitDepth) {
            case 8:
					bytesPerComponent = 1;
               bitDepth_ = 8;
               ret=DEVICE_OK;
            break;
            case 10:
					bytesPerComponent = 2;
               bitDepth_ = 10;
               ret=DEVICE_OK;
            break;
            case 12:
					bytesPerComponent = 2;
               bitDepth_ = 12;
               ret=DEVICE_OK;
            break;
            case 14:
					bytesPerComponent = 2;
               bitDepth_ = 14;
               ret=DEVICE_OK;
            break;
            case 16:
					bytesPerComponent = 1; //nazim 2
               bitDepth_ = 8; //nazim 16
               ret=DEVICE_OK;
            break;
            case 32:
               bytesPerComponent = 4;
               bitDepth_ = 32; 
               ret=DEVICE_OK;
            break;
            default: 
               // on error switch to default pixel type
					bytesPerComponent = 1;

               pProp->Set((long)8);
               bitDepth_ = 8;
               ret = ERR_UNKNOWN_MODE;
            break;
         }
			char buf[MM::MaxStrLength];
			GetProperty(MM::g_Keyword_PixelType, buf);
			std::string pixelType(buf);
			unsigned int bytesPerPixel = 1;
			

         // automagickally change pixel type when bit depth exceeds possible value
         if (pixelType.compare(g_PixelType_8bit) == 0)
         {
				if( 2 == bytesPerComponent)
				{
					SetProperty(MM::g_Keyword_PixelType, g_PixelType_16bit);
					bytesPerPixel = 2;
				}
				else if ( 4 == bytesPerComponent)
            {
					SetProperty(MM::g_Keyword_PixelType, g_PixelType_32bit);
					bytesPerPixel = 4;

            }else
				{
				   bytesPerPixel = 1;
				}
         }
         else if (pixelType.compare(g_PixelType_16bit) == 0)
         {
				bytesPerPixel = 2;
         }
			else if ( pixelType.compare(g_PixelType_32bitRGB) == 0)
			{
				bytesPerPixel = 4;
			}
			else if ( pixelType.compare(g_PixelType_32bit) == 0)
			{
				bytesPerPixel = 4;
			}
			else if ( pixelType.compare(g_PixelType_64bitRGB) == 0)
			{
				bytesPerPixel = 8;
			}
			img_.Resize(img_.Width(), img_.Height(), bytesPerPixel);

      } break;
   case MM::BeforeGet:
      {
         pProp->Set((long)bitDepth_);
         ret=DEVICE_OK;
      } break;
   default:
      break;
   }
   return ret; 
}
/**
* Handles "ReadoutTime" property.
*/
int CBaslerCamera::OnReadoutTime(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::AfterSet)
   {
      double readoutMs;
      pProp->Get(readoutMs);

      readoutUs_ = readoutMs * 1000.0;
   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(readoutUs_ / 1000.0);
   }

   return DEVICE_OK;
}

int CBaslerCamera::OnDropPixels(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::AfterSet)
   {
      long tvalue = 0;
      pProp->Get(tvalue);
		dropPixels_ = (0==tvalue)?false:true;
   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(dropPixels_?1L:0L);
   }

   return DEVICE_OK;
}

int CBaslerCamera::OnFastImage(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::AfterSet)
   {
      long tvalue = 0;
      pProp->Get(tvalue);
		fastImage_ = (0==tvalue)?false:true;
   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(fastImage_?1L:0L);
   }

   return DEVICE_OK;
}

int CBaslerCamera::OnSaturatePixels(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::AfterSet)
   {
      long tvalue = 0;
      pProp->Get(tvalue);
		saturatePixels_ = (0==tvalue)?false:true;
   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(saturatePixels_?1L:0L);
   }

   return DEVICE_OK;
}

int CBaslerCamera::OnFractionOfPixelsToDropOrSaturate(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::AfterSet)
   {
      double tvalue = 0;
      pProp->Get(tvalue);
		fractionOfPixelsToDropOrSaturate_ = tvalue;
   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(fractionOfPixelsToDropOrSaturate_);
   }

   return DEVICE_OK;
}

/*
* Handles "ScanMode" property.
* Changes allowed Binning values to test whether the UI updates properly
*/
int CBaslerCamera::OnScanMode(MM::PropertyBase* pProp, MM::ActionType eAct)
{ 
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::AfterSet) {
      pProp->Get(scanMode_);
      SetAllowedBinning();
      if (initialized_) {
         int ret = OnPropertiesChanged();
         if (ret != DEVICE_OK)
            return ret;
      }
   } else if (eAct == MM::BeforeGet) {
      LogMessage("Reading property ScanMode", true);
      pProp->Set(scanMode_);
   }
   return DEVICE_OK;
}




int CBaslerCamera::OnCameraCCDXSize(MM::PropertyBase* pProp , MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
		pProp->Set(cameraCCDXSize_);
   }
   else if (eAct == MM::AfterSet)
   {
      long value;
      pProp->Get(value);
		if ( (value < 16) || (33000 < value))
			return DEVICE_ERR;  // invalid image size
		if( value != cameraCCDXSize_)
		{
			cameraCCDXSize_ = value;
			img_.Resize(cameraCCDXSize_/binSize_, cameraCCDYSize_/binSize_);
		}
   }
	return DEVICE_OK;

}

int CBaslerCamera::OnCameraCCDYSize(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
		pProp->Set(cameraCCDYSize_);
   }
   else if (eAct == MM::AfterSet)
   {
      long value;
      pProp->Get(value);
		if ( (value < 16) || (33000 < value))
			return DEVICE_ERR;  // invalid image size
		if( value != cameraCCDYSize_)
		{
			cameraCCDYSize_ = value;
			img_.Resize(cameraCCDXSize_/binSize_, cameraCCDYSize_/binSize_);
		}
   }
	return DEVICE_OK;

}

int CBaslerCamera::OnTriggerDevice(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(triggerDevice_.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(triggerDevice_);
   }
   return DEVICE_OK;
}


int CBaslerCamera::OnCCDTemp(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(ccdT_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(ccdT_);
   }
   return DEVICE_OK;
}

int CBaslerCamera::OnIsSequenceable(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   std::string val = "Yes";
   if (eAct == MM::BeforeGet)
   {
      if (!isSequenceable_) 
      {
         val = "No";
      }
      pProp->Set(val.c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      isSequenceable_ = false;
      pProp->Get(val);
      if (val == "Yes") 
      {
         isSequenceable_ = true;
      }
   }

   return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Private CBaslerCamera methods
///////////////////////////////////////////////////////////////////////////////

/**
* Sync internal image buffer size to the chosen property values.
*/
int CBaslerCamera::ResizeImageBuffer()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   char buf[MM::MaxStrLength];
   //int ret = GetProperty(MM::g_Keyword_Binning, buf);
   //if (ret != DEVICE_OK)
   //   return ret;
   //binSize_ = atol(buf);

   int ret = GetProperty(MM::g_Keyword_PixelType, buf);
   if (ret != DEVICE_OK)
      return ret;

	std::string pixelType(buf);
	int byteDepth = 0;

   if (pixelType.compare(g_PixelType_8bit) == 0)
   {
      byteDepth = 1;
   }
   else if (pixelType.compare(g_PixelType_16bit) == 0)
   {
      byteDepth = 2;
   }
	else if ( pixelType.compare(g_PixelType_32bitRGB) == 0)
	{
      byteDepth = 4;
	}
	else if ( pixelType.compare(g_PixelType_32bit) == 0)
	{
      byteDepth = 4;
	}
	else if ( pixelType.compare(g_PixelType_64bitRGB) == 0)
	{
      byteDepth = 8;
	}

   img_.Resize(cameraCCDXSize_/binSize_, cameraCCDYSize_/binSize_, byteDepth);
   return DEVICE_OK;
}

void CBaslerCamera::GenerateEmptyImage(ImgBuffer& img)
{
   MMThreadGuard g(imgPixelsLock_);

   char buf[MM::MaxStrLength];
   GetProperty(MM::g_Keyword_PixelType, buf);
   std::string pixelType(buf);

   if (img.Height() == 0 || img.Width() == 0 || img.Depth() == 0)
      return;
   unsigned char* pBuf = const_cast<unsigned char*>(img.GetPixels());
   memset(pBuf, 0, img.Height()*img.Width()*img.Depth());

}



void CBaslerCamera::GetCameraImage(ImgBuffer& img) 
{

   MMThreadGuard g(imgPixelsLock_);
   if (img.Height() == 0 || img.Width() == 0 || img.Depth() == 0)
      return;  

   unsigned char* pBuf = (unsigned char*)const_cast<unsigned char*>(img.GetPixels());

   unsigned char *rec = reconstruct(method, m_pCurrent1);

   memcpy (pBuf, rec, paddedX*paddedY);

}


/**
* Generate a spatial sine wave.
*/
void CBaslerCamera::GenerateSyntheticImage(ImgBuffer& img, double exp)
{ 

   MMThreadGuard g(imgPixelsLock_);

	//std::string pixelType;
	char buf[MM::MaxStrLength];
   GetProperty(MM::g_Keyword_PixelType, buf);
	std::string pixelType(buf);

	if (img.Height() == 0 || img.Width() == 0 || img.Depth() == 0)
      return;

   const double cPi = 3.14159265358979;
   long lPeriod = img.Width()/2;
   double dLinePhase = 0.0;
   const double dAmp = exp;
   const double cLinePhaseInc = 2.0 * cPi / 4.0 / img.Height();

   static bool debugRGB = false;
#ifdef TIFFDEMO
	debugRGB = true;
#endif
   static  unsigned char* pDebug  = NULL;
   static unsigned long dbgBufferSize = 0;
   static long iseq = 1;

 

	// for integer images: bitDepth_ is 8, 10, 12, 16 i.e. it is depth per component
   long maxValue = (1L << bitDepth_)-1;

	long pixelsToDrop = 0;
	if( dropPixels_)
		pixelsToDrop = (long)(0.5 + fractionOfPixelsToDropOrSaturate_*img.Height()*img.Width());
	long pixelsToSaturate = 0;
	if( saturatePixels_)
		pixelsToSaturate = (long)(0.5 + fractionOfPixelsToDropOrSaturate_*img.Height()*img.Width());

   unsigned j, k;
   if (pixelType.compare(g_PixelType_8bit) == 0)
   {
      double pedestal = 127 * exp / 100.0 * GetBinning() * GetBinning();
      unsigned char* pBuf = const_cast<unsigned char*>(img.GetPixels());
      for (j=0; j<img.Height(); j++)
      {
         for (k=0; k<img.Width(); k++)
         {
            long lIndex = img.Width()*j + k;
            *(pBuf + lIndex) = (unsigned char) (g_IntensityFactor_ * min(255.0, (pedestal + dAmp * sin(dPhase_ + dLinePhase + (2.0 * cPi * k) / lPeriod))));
         }
         dLinePhase += cLinePhaseInc;
      }
	   for(int snoise = 0; snoise < pixelsToSaturate; ++snoise)
		{
			j = (unsigned)( (double)(img.Height()-1)*(double)rand()/(double)RAND_MAX);
			k = (unsigned)( (double)(img.Width()-1)*(double)rand()/(double)RAND_MAX);
			*(pBuf + img.Width()*j + k) = (unsigned char)maxValue;
		}
		int pnoise;
		for(pnoise = 0; pnoise < pixelsToDrop; ++pnoise)
		{
			j = (unsigned)( (double)(img.Height()-1)*(double)rand()/(double)RAND_MAX);
			k = (unsigned)( (double)(img.Width()-1)*(double)rand()/(double)RAND_MAX);
			*(pBuf + img.Width()*j + k) = 0;
		}

   }
   else if (pixelType.compare(g_PixelType_16bit) == 0)
   {
      double pedestal = maxValue/2 * exp / 100.0 * GetBinning() * GetBinning();
      double dAmp16 = dAmp * maxValue/255.0; // scale to behave like 8-bit
      unsigned short* pBuf = (unsigned short*) const_cast<unsigned char*>(img.GetPixels());
	  for (int i=0;i<2040*1088;i++)
		  *(pBuf+i)= *(m_pCurrent1+i);
      for (j=0; j<img.Height(); j++)
      {
         for (k=0; k<img.Width(); k++)
         {
            long lIndex = img.Width()*j + k;
            *(pBuf + lIndex) = (unsigned short) (g_IntensityFactor_ * min((double)maxValue, pedestal + dAmp16 * sin(dPhase_ + dLinePhase + (2.0 * cPi * k) / lPeriod)));
         }
         dLinePhase += cLinePhaseInc;
      }         
	   for(int snoise = 0; snoise < pixelsToSaturate; ++snoise)
		{
			j = (unsigned)(0.5 + (double)img.Height()*(double)rand()/(double)RAND_MAX);
			k = (unsigned)(0.5 + (double)img.Width()*(double)rand()/(double)RAND_MAX);
			*(pBuf + img.Width()*j + k) = (unsigned short)maxValue;
		}
		int pnoise;
		for(pnoise = 0; pnoise < pixelsToDrop; ++pnoise)
		{
			j = (unsigned)(0.5 + (double)img.Height()*(double)rand()/(double)RAND_MAX);
			k = (unsigned)(0.5 + (double)img.Width()*(double)rand()/(double)RAND_MAX);
			*(pBuf + img.Width()*j + k) = 0;
		}
	
	}
   else if (pixelType.compare(g_PixelType_32bit) == 0)
   {
      double pedestal = 127 * exp / 100.0 * GetBinning() * GetBinning();
      float* pBuf = (float*) const_cast<unsigned char*>(img.GetPixels());
      float saturatedValue = 255.;
      memset(pBuf, 0, img.Height()*img.Width()*4);
      // static unsigned int j2;
      for (j=0; j<img.Height(); j++)
      {
         for (k=0; k<img.Width(); k++)
         {
            long lIndex = img.Width()*j + k;
            double value =  (g_IntensityFactor_ * min(255.0, (pedestal + dAmp * sin(dPhase_ + dLinePhase + (2.0 * cPi * k) / lPeriod))));
            *(pBuf + lIndex) = (float) value;
            if( 0 == lIndex)
            {
               std::ostringstream os;
               os << " first pixel is " << (float)value;
               LogMessage(os.str().c_str(), true);

            }
         }
         dLinePhase += cLinePhaseInc;
      }

	   for(int snoise = 0; snoise < pixelsToSaturate; ++snoise)
		{
			j = (unsigned)(0.5 + (double)img.Height()*(double)rand()/(double)RAND_MAX);
			k = (unsigned)(0.5 + (double)img.Width()*(double)rand()/(double)RAND_MAX);
			*(pBuf + img.Width()*j + k) = saturatedValue;
		}
		int pnoise;
		for(pnoise = 0; pnoise < pixelsToDrop; ++pnoise)
		{
			j = (unsigned)(0.5 + (double)img.Height()*(double)rand()/(double)RAND_MAX);
			k = (unsigned)(0.5 + (double)img.Width()*(double)rand()/(double)RAND_MAX);
			*(pBuf + img.Width()*j + k) = 0;
      }
	
	}
	else if (pixelType.compare(g_PixelType_32bitRGB) == 0)
	{
      double pedestal = 127 * exp / 100.0;
      unsigned int * pBuf = (unsigned int*) img.GetPixelsRW();

      unsigned char* pTmpBuffer = NULL;

      if(debugRGB)
      {
         const unsigned long bfsize = img.Height() * img.Width() * 3;
         if(  bfsize != dbgBufferSize)
         {
            if (NULL != pDebug)
            {
               free(pDebug);
               pDebug = NULL;
            }
            pDebug = (unsigned char*)malloc( bfsize);
            if( NULL != pDebug)
            {
               dbgBufferSize = bfsize;
            }
         }
      }

		// only perform the debug operations if pTmpbuffer is not 0
      pTmpBuffer = pDebug;
      unsigned char* pTmp2 = pTmpBuffer;
      if( NULL!= pTmpBuffer)
			memset( pTmpBuffer, 0, img.Height() * img.Width() * 3);

      for (j=0; j<img.Height(); j++)
      {
         unsigned char theBytes[4];
         for (k=0; k<img.Width(); k++)
         {
            long lIndex = img.Width()*j + k;
            unsigned char value0 =   (unsigned char) min(255.0, (pedestal + dAmp * sin(dPhase_ + dLinePhase + (2.0 * cPi * k) / lPeriod)));
            theBytes[0] = value0;
            if( NULL != pTmpBuffer)
               pTmp2[2] = value0;
            unsigned char value1 =   (unsigned char) min(255.0, (pedestal + dAmp * sin(dPhase_ + dLinePhase*2 + (2.0 * cPi * k) / lPeriod)));
            theBytes[1] = value1;
            if( NULL != pTmpBuffer)
               pTmp2[1] = value1;
            unsigned char value2 = (unsigned char) min(255.0, (pedestal + dAmp * sin(dPhase_ + dLinePhase*4 + (2.0 * cPi * k) / lPeriod)));
            theBytes[2] = value2;

            if( NULL != pTmpBuffer){
               pTmp2[0] = value2;
               pTmp2+=3;
            }
            theBytes[3] = 0;
            unsigned long tvalue = *(unsigned long*)(&theBytes[0]);
            *(pBuf + lIndex) =  tvalue ;  //value0+(value1<<8)+(value2<<16);
         }
         dLinePhase += cLinePhaseInc;
      }


      // ImageJ's AWT images are loaded with a Direct Color processor which expects BGRA, that's why we swapped the Blue and Red components in the generator above.
      if(NULL != pTmpBuffer)
      {
         // write the compact debug image...
         char ctmp[12];
         snprintf(ctmp,12,"%ld",iseq++);
         int status = writeCompactTiffRGB( img.Width(), img.Height(), pTmpBuffer, ("democamera"+std::string(ctmp)).c_str()
            );
			status = status;
      }

	}

	// generate an RGB image with bitDepth_ bits in each color
	else if (pixelType.compare(g_PixelType_64bitRGB) == 0)
	{
      double pedestal = maxValue/2 * exp / 100.0 * GetBinning() * GetBinning();
      double dAmp16 = dAmp * maxValue/255.0; // scale to behave like 8-bit
      
		double maxPixelValue = (1<<(bitDepth_))-1;
      unsigned long long * pBuf = (unsigned long long*) img.GetPixelsRW();
      for (j=0; j<img.Height(); j++)
      {
         for (k=0; k<img.Width(); k++)
         {
            long lIndex = img.Width()*j + k;
            unsigned long long value0 = (unsigned short) min(maxPixelValue, (pedestal + dAmp16 * sin(dPhase_ + dLinePhase + (2.0 * cPi * k) / lPeriod)));
            unsigned long long value1 = (unsigned short) min(maxPixelValue, (pedestal + dAmp16 * sin(dPhase_ + dLinePhase*2 + (2.0 * cPi * k) / lPeriod)));
            unsigned long long value2 = (unsigned short) min(maxPixelValue, (pedestal + dAmp16 * sin(dPhase_ + dLinePhase*4 + (2.0 * cPi * k) / lPeriod)));
            unsigned long long tval = value0+(value1<<16)+(value2<<32);
         *(pBuf + lIndex) = tval;
			}
         dLinePhase += cLinePhaseInc;
      }
	}

   dPhase_ += cPi / 4.;
}


void CBaslerCamera::TestResourceLocking(const bool recurse)
{
   MMThreadGuard g(*pDemoResourceLock_);
   if(recurse)
      TestResourceLocking(false);
}

///////////////////////////////////////////////////////////////////////////////
// CDemoFilterWheel implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CDemoFilterWheel::CDemoFilterWheel() : 
numPos_(10), 
busy_(false), 
initialized_(false), 
changedTime_(0.0),
position_(0)
{
   InitializeDefaultErrorMessages();
   SetErrorText(ERR_UNKNOWN_POSITION, "Requested position not available in this device");
   EnableDelay(); // signals that the dealy setting will be used
   // parent ID display
   CreateHubIDProperty();
}

CDemoFilterWheel::~CDemoFilterWheel()
{
   Shutdown();
}

void CDemoFilterWheel::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_WheelDeviceName);
}


int CDemoFilterWheel::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_WheelDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo filter wheel driver", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Set timer for the Busy signal, or we'll get a time-out the first time we check the state of the shutter, for good measure, go back 'delay' time into the past
   changedTime_ = GetCurrentMMTime();   

   // Gate Closed Position
   ret = CreateProperty(MM::g_Keyword_Closed_Position,"", MM::Integer, false);
   if (ret != DEVICE_OK)
      return ret;

   // create default positions and labels
   const int bufSize = 1024;
   char buf[bufSize];
   for (long i=0; i<numPos_; i++)
   {
      snprintf(buf, bufSize, "State-%ld", i);
      SetPositionLabel(i, buf);
      snprintf(buf, bufSize, "%ld", i);
      AddAllowedValue(MM::g_Keyword_Closed_Position, buf);
   }

   // State
   // -----
   CPropertyAction* pAct = new CPropertyAction (this, &CDemoFilterWheel::OnState);
   ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Label
   // -----
   pAct = new CPropertyAction (this, &CStateBase::OnLabel);
   ret = CreateProperty(MM::g_Keyword_Label, "", MM::String, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

bool CDemoFilterWheel::Busy()
{
   MM::MMTime interval = GetCurrentMMTime() - changedTime_;
   MM::MMTime delay(GetDelayMs()*1000.0);
   if (interval < delay)
      return true;
   else
      return false;
}


int CDemoFilterWheel::Shutdown()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int CDemoFilterWheel::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(position_);
      // nothing to do, let the caller to use cached property
   }
   else if (eAct == MM::AfterSet)
   {
      // Set timer for the Busy signal
      changedTime_ = GetCurrentMMTime();

      long pos;
      pProp->Get(pos);
      if (pos >= numPos_ || pos < 0)
      {
         pProp->Set(position_); // revert
         return ERR_UNKNOWN_POSITION;
      }

      position_ = pos;
   }

   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CDemoStateDevice implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CDemoStateDevice::CDemoStateDevice() : 
numPos_(10), 
busy_(false), 
initialized_(false), 
changedTime_(0.0),
position_(0)
{
   InitializeDefaultErrorMessages();
   SetErrorText(ERR_UNKNOWN_POSITION, "Requested position not available in this device");
   EnableDelay(); // signals that the dealy setting will be used

   // Number of positions
   // -----
   CPropertyAction* pAct = new CPropertyAction (this, &CDemoStateDevice::OnNumberOfStates);
   CreateProperty("Number of positions", "0", MM::Integer, false, pAct, true);

   // parent ID display
   CreateHubIDProperty();

}

CDemoStateDevice::~CDemoStateDevice()
{
   Shutdown();
}

void CDemoStateDevice::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_StateDeviceName);
}


int CDemoStateDevice::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_StateDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo state device driver", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Set timer for the Busy signal, or we'll get a time-out the first time we check the state of the shutter, for good measure, go back 'delay' time into the past
   changedTime_ = GetCurrentMMTime();   

   // Gate Closed Position
   ret = CreateProperty(MM::g_Keyword_Closed_Position,"", MM::String, false);

   // create default positions and labels
   const int bufSize = 1024;
   char buf[bufSize];
   for (long i=0; i<numPos_; i++)
   {
      snprintf(buf, bufSize, "State-%ld", i);
      SetPositionLabel(i, buf);
      AddAllowedValue(MM::g_Keyword_Closed_Position, buf);
   }

   // State
   // -----
   CPropertyAction* pAct = new CPropertyAction (this, &CDemoStateDevice::OnState);
   ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Label
   // -----
   pAct = new CPropertyAction (this, &CStateBase::OnLabel);
   ret = CreateProperty(MM::g_Keyword_Label, "", MM::String, false, pAct);
   if (ret != DEVICE_OK)
      return ret;



   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

bool CDemoStateDevice::Busy()
{
   MM::MMTime interval = GetCurrentMMTime() - changedTime_;
   MM::MMTime delay(GetDelayMs()*1000.0);
   if (interval < delay)
      return true;
   else
      return false;
}


int CDemoStateDevice::Shutdown()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int CDemoStateDevice::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(position_);
      // nothing to do, let the caller to use cached property
   }
   else if (eAct == MM::AfterSet)
   {
      // Set timer for the Busy signal
      changedTime_ = GetCurrentMMTime();

      long pos;
      pProp->Get(pos);
      if (pos >= numPos_ || pos < 0)
      {
         pProp->Set(position_); // revert
         return ERR_UNKNOWN_POSITION;
      }
      position_ = pos;
   }

   return DEVICE_OK;
}

int CDemoStateDevice::OnNumberOfStates(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(numPos_);
   }
   else if (eAct == MM::AfterSet)
   {
      if (!initialized_)
         pProp->Get(numPos_);
   }

   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CDemoLightPath implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CDemoLightPath::CDemoLightPath() : 
numPos_(3), 
busy_(false), 
initialized_(false)
{
   InitializeDefaultErrorMessages();
   // parent ID display
   CreateHubIDProperty();
}

CDemoLightPath::~CDemoLightPath()
{
   Shutdown();
}

void CDemoLightPath::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_LightPathDeviceName);
}


int CDemoLightPath::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_LightPathDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo light-path driver", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // create default positions and labels
   const int bufSize = 1024;
   char buf[bufSize];
   for (long i=0; i<numPos_; i++)
   {
      snprintf(buf, bufSize, "State-%ld", i);
      SetPositionLabel(i, buf);
   }

   // State
   // -----
   CPropertyAction* pAct = new CPropertyAction (this, &CDemoLightPath::OnState);
   ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Label
   // -----
   pAct = new CPropertyAction (this, &CStateBase::OnLabel);
   ret = CreateProperty(MM::g_Keyword_Label, "", MM::String, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

int CDemoLightPath::Shutdown()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int CDemoLightPath::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      // nothing to do, let the caller to use cached property
   }
   else if (eAct == MM::AfterSet)
   {
      long pos;
      pProp->Get(pos);
      if (pos >= numPos_ || pos < 0)
      {
         pProp->Set(position_); // revert
         return ERR_UNKNOWN_POSITION;
      }
      position_ = pos;
   }

   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CDemoObjectiveTurret implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CDemoObjectiveTurret::CDemoObjectiveTurret() : 
   numPos_(6), 
   busy_(false), 
   initialized_(false),
   sequenceRunning_(false),
   sequenceMaxSize_(10)
{
   SetErrorText(ERR_IN_SEQUENCE, "Error occurred while executing sequence");
   SetErrorText(ERR_SEQUENCE_INACTIVE, "Sequence triggered, but sequence is not running");
   InitializeDefaultErrorMessages();
   // parent ID display
   CreateHubIDProperty();
}

CDemoObjectiveTurret::~CDemoObjectiveTurret()
{
   Shutdown();
}

void CDemoObjectiveTurret::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_ObjectiveDeviceName);
}


int CDemoObjectiveTurret::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_ObjectiveDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo objective turret driver", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // create default positions and labels
   const int bufSize = 1024;
   char buf[bufSize];
   for (long i=0; i<numPos_; i++)
   {
      snprintf(buf, bufSize, "Objective-%c",'A'+ (char)i);
      SetPositionLabel(i, buf);
   }

   // State
   // -----
   CPropertyAction* pAct = new CPropertyAction (this, &CDemoObjectiveTurret::OnState);
   ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Label
   // -----
   pAct = new CPropertyAction (this, &CStateBase::OnLabel);
   ret = CreateProperty(MM::g_Keyword_Label, "", MM::String, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   // Triggers to test sequence capabilities
   pAct = new CPropertyAction (this, &CDemoObjectiveTurret::OnTrigger);
   ret = CreateProperty("Trigger", "-", MM::String, false, pAct);
   AddAllowedValue("Trigger", "-");
   AddAllowedValue("Trigger", "+");

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

int CDemoObjectiveTurret::Shutdown()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}



///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int CDemoObjectiveTurret::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      // nothing to do, let the caller to use cached property
   }
   else if (eAct == MM::AfterSet)
   {
      long pos;
      pProp->Get(pos);
      if (pos >= numPos_ || pos < 0)
      {
         pProp->Set(position_); // revert
         return ERR_UNKNOWN_POSITION;
      }
      position_ = pos;
      std::ostringstream os;
      os << position_;
      OnPropertyChanged("State", os.str().c_str());
      char label[MM::MaxStrLength];
      GetPositionLabel(position_, label);
      OnPropertyChanged("Label", label);
   }
   else if (eAct == MM::IsSequenceable) 
   {
      pProp->SetSequenceable(sequenceMaxSize_);
   }
   else if (eAct == MM::AfterLoadSequence)
   {
      sequence_ = pProp->GetSequence();
      // DeviceBase.h checks that the vector is smaller than sequenceMaxSize_
   }
   else if (eAct == MM::StartSequence)
   {
      if (sequence_.size() > 0) {
         sequenceIndex_ = 0;
         sequenceRunning_ = true;
      }
   }
   else if (eAct  == MM::StopSequence)
   {
      sequenceRunning_ = false;
   }

   return DEVICE_OK;
}

int CDemoObjectiveTurret::OnTrigger(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set("-");
   } else if (eAct == MM::AfterSet) {
      if (!sequenceRunning_)
         return ERR_SEQUENCE_INACTIVE;
      std::string tr;
      pProp->Get(tr);
      if (tr == "+") {
         if (sequenceIndex_ < sequence_.size()) {
            std::string state = sequence_[sequenceIndex_];
            int ret = SetProperty("State", state.c_str());
            if (ret != DEVICE_OK)
               return ERR_IN_SEQUENCE;
            sequenceIndex_++;
            if (sequenceIndex_ >= sequence_.size()) {
               sequenceIndex_ = 0;
            }
         } else
         {
            return ERR_IN_SEQUENCE;
         }
      }
   }
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CDemoStage implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~

CDemoStage::CDemoStage() : 
stepSize_um_(0.025),
pos_um_(0.0),
busy_(false),
initialized_(false),
lowerLimit_(0.0),
upperLimit_(20000.0)
{
   InitializeDefaultErrorMessages();

   // parent ID display
   CreateHubIDProperty();
}

CDemoStage::~CDemoStage()
{
   Shutdown();
}

void CDemoStage::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_StageDeviceName);
}

int CDemoStage::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_StageDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo stage driver", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Position
   // --------
   CPropertyAction* pAct = new CPropertyAction (this, &CDemoStage::OnPosition);
   ret = CreateProperty(MM::g_Keyword_Position, "0", MM::Float, false, pAct);
   if (ret != DEVICE_OK)
      return ret;

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

int CDemoStage::Shutdown()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

int CDemoStage::SetPositionUm(double pos) 
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   pos_um_ = pos; 
   SetIntensityFactor(pos);
   return OnStagePositionChanged(pos_um_);
}

void CDemoStage::SetIntensityFactor(double pos)
{
   pos = fabs(pos);
   pos = 10.0 - pos;
   if (pos < 0)
      g_IntensityFactor_ = 1.0;
   else
      g_IntensityFactor_ = pos/10.0;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int CDemoStage::OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      std::stringstream s;
      s << pos_um_;
      pProp->Set(s.str().c_str());
   }
   else if (eAct == MM::AfterSet)
   {
      double pos;
      pProp->Get(pos);
      if (pos > upperLimit_ || lowerLimit_ > pos)
      {
         pProp->Set(pos_um_); // revert
         return ERR_UNKNOWN_POSITION;
      }
      pos_um_ = pos;
      SetIntensityFactor(pos);
   }

   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CDemoXYStage implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~

CDemoXYStage::CDemoXYStage() : 
CXYStageBase<CDemoXYStage>(),
stepSize_um_(0.015),
posX_um_(0.0),
posY_um_(0.0),
busy_(false),
timeOutTimer_(0),
velocity_(10.0), // in micron per second
initialized_(false),
lowerLimit_(0.0),
upperLimit_(20000.0)
{
   InitializeDefaultErrorMessages();

   // parent ID display
   CreateHubIDProperty();
}

CDemoXYStage::~CDemoXYStage()
{
   Shutdown();
}

void CDemoXYStage::GetName(char* Name) const
{
   CDeviceUtils::CopyLimitedString(Name, g_XYStageDeviceName);
}

int CDemoXYStage::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_XYStageDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo XY stage driver", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}

int CDemoXYStage::Shutdown()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (initialized_)
   {
      initialized_ = false;
   }
   return DEVICE_OK;
}

bool CDemoXYStage::Busy()
{
   if (timeOutTimer_ == 0)
      return false;
   if (timeOutTimer_->expired(GetCurrentMMTime()))
   {
      // delete(timeOutTimer_);
      return false;
   }
   return true;
}


///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////
// none implemented


///////////////////////////////////////////////////////////////////////////////
// CDemoShutter implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DemoShutter::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_ShutterDeviceName);
}

int DemoShutter::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_ShutterDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo shutter driver", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   changedTime_ = GetCurrentMMTime();

   // state
   CPropertyAction* pAct = new CPropertyAction (this, &DemoShutter::OnState);
   ret = CreateProperty(MM::g_Keyword_State, "0", MM::Integer, false, pAct); 
   if (ret != DEVICE_OK) 
      return ret; 

   AddAllowedValue(MM::g_Keyword_State, "0"); // Closed
   AddAllowedValue(MM::g_Keyword_State, "1"); // Open

   state_ = false;

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;

   return DEVICE_OK;
}


bool DemoShutter::Busy()
{
   MM::MMTime interval = GetCurrentMMTime() - changedTime_;

   if ( interval < MM::MMTime(1000.0 * GetDelayMs()))
      return true;
   else
      return false;
}

///////////////////////////////////////////////////////////////////////////////
// Action handlers
///////////////////////////////////////////////////////////////////////////////

int DemoShutter::OnState(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      if (state_)
         pProp->Set(1L);
      else
         pProp->Set(0L);
   }
   else if (eAct == MM::AfterSet)
   {
      // Set timer for the Busy signal
      changedTime_ = GetCurrentMMTime();

      long pos;
      pProp->Get(pos);

      // apply the value
      state_ = pos == 0 ? false : true;
   }

   return DEVICE_OK;
}


///////////////////////////////////////////////////////////////////////////////
// CDemoMagnifier implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
DemoMagnifier::DemoMagnifier () :
      position_ (0),
      highMag_ (1.6) 
{
   CPropertyAction* pAct = new CPropertyAction (this, &DemoMagnifier::OnHighMag);
   CreateProperty("High Position Magnification", "1.6", MM::Float, false, pAct, true);

   // parent ID display
   CreateHubIDProperty();
};

void DemoMagnifier::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_MagnifierDeviceName);
}

int DemoMagnifier::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   CPropertyAction* pAct = new CPropertyAction (this, &DemoMagnifier::OnPosition);
   int ret = CreateProperty("Position", "1x", MM::String, false, pAct); 
   if (ret != DEVICE_OK) 
      return ret; 

   position_ = 0;

   AddAllowedValue("Position", "1x"); 
   AddAllowedValue("Position", highMagString().c_str()); 

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   return DEVICE_OK;
}

std::string DemoMagnifier::highMagString() {
   std::ostringstream os;
   os << highMag_ << "x";
   return os.str();
}

double DemoMagnifier::GetMagnification() {
   if (position_ == 0)
      return 1.0;
   return highMag_;
}

int DemoMagnifier::OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct) 
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      // nothing to do, let the caller use cached property
   }
   else if (eAct == MM::AfterSet)
   {
      std::string pos;
      pProp->Get(pos);
      if (pos == "1x")
         position_ = 0;
      else
         position_ = 1;
   }

   return DEVICE_OK;
}

int DemoMagnifier::OnHighMag(MM::PropertyBase* pProp, MM::ActionType eAct) 
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(highMag_);
   }
   else if (eAct == MM::AfterSet)
   {
      pProp->Get(highMag_);
      ClearAllowedValues("Position");
      AddAllowedValue("Position", "1x"); 
      AddAllowedValue("Position", highMagString().c_str()); 
   }

   return DEVICE_OK;
}

/****
* Demo DA device
*/

DemoDA::DemoDA () : 
volt_(0), 
gatedVolts_(0), 
open_(true),
sequenceRunning_(false),
sequenceIndex_(0),
sentSequence_(vector<double>()),
nascentSequence_(vector<double>())
{
   SetErrorText(SIMULATED_ERROR, "Random, simluated error");
   SetErrorText(ERR_SEQUENCE_INACTIVE, "Sequence triggered, but sequence is not running");

   // parent ID display
   CreateHubIDProperty();
}

DemoDA::~DemoDA() {
}

void DemoDA::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_DADeviceName);
}

int DemoDA::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   // Triggers to test sequence capabilities
   CPropertyAction* pAct = new CPropertyAction (this, &DemoDA::OnTrigger);
   CreateProperty("Trigger", "-", MM::String, false, pAct);
   AddAllowedValue("Trigger", "-");
   AddAllowedValue("Trigger", "+");

   pAct = new CPropertyAction(this, &DemoDA::OnVoltage);
   CreateProperty("Voltage", "0", MM::Float, false, pAct);
   SetPropertyLimits("Voltage", 0.0, 10.0);

   pAct = new CPropertyAction(this, &DemoDA::OnRealVoltage);
   CreateProperty("Real Voltage", "0", MM::Float, true, pAct);

   return DEVICE_OK;
}

int DemoDA::SetGateOpen(bool open) 
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   open_ = open; 
   if (open_) 
      gatedVolts_ = volt_; 
   else 
      gatedVolts_ = 0;

   return DEVICE_OK;
}

int DemoDA::GetGateOpen(bool& open) 
{
   open = open_; 
   return DEVICE_OK;
}

int DemoDA::SetSignal(double volts) {
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   volt_ = volts; 
   if (open_)
      gatedVolts_ = volts;
   stringstream s;
   s << "Voltage set to " << volts;
   LogMessage(s.str(), false);
   return DEVICE_OK;
}

int DemoDA::GetSignal(double& volts) 
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   volts = volt_; 
   return DEVICE_OK;
}

int DemoDA::SendDASequence() 
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   (const_cast<DemoDA*> (this))->SetSentSequence();
   return DEVICE_OK;
}

// private
void DemoDA::SetSentSequence()
{
   sentSequence_ = nascentSequence_;
   nascentSequence_.clear();
}

int DemoDA::ClearDASequence()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   nascentSequence_.clear();
   return DEVICE_OK;
}

int DemoDA::AddToDASequence(double voltage) {
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   nascentSequence_.push_back(voltage);
   return DEVICE_OK;
}

int DemoDA::OnTrigger(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set("-");
   } else if (eAct == MM::AfterSet) {
      if (!sequenceRunning_)
         return ERR_SEQUENCE_INACTIVE;
      std::string tr;
      pProp->Get(tr);
      if (tr == "+") {
         if (sequenceIndex_ < sentSequence_.size()) {
            double voltage = sentSequence_[sequenceIndex_];
            int ret = SetSignal(voltage);
            if (ret != DEVICE_OK)
               return ERR_IN_SEQUENCE;
            sequenceIndex_++;
            if (sequenceIndex_ >= sentSequence_.size()) {
               sequenceIndex_ = 0;
            }
         } else
         {
            return ERR_IN_SEQUENCE;
         }
      }
   }
   return DEVICE_OK;
}

int DemoDA::OnVoltage(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      double volts = 0.0;
      GetSignal(volts);
      pProp->Set(volts);
   }
   else if (eAct == MM::AfterSet)
   {
      double volts = 0.0;
      pProp->Get(volts);
      SetSignal(volts);
   }
   return DEVICE_OK;
}

int DemoDA::OnRealVoltage(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   if (eAct == MM::BeforeGet)
   {
      pProp->Set(gatedVolts_);
   }
   return DEVICE_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// CDemoAutoFocus implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DemoAutoFocus::GetName(char* name) const
{
   CDeviceUtils::CopyLimitedString(name, g_AutoFocusDeviceName);
}

int DemoAutoFocus::Initialize()
{
/*   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if (initialized_)
      return DEVICE_OK;

   // set property list
   // -----------------

   // Name
   int ret = CreateProperty(MM::g_Keyword_Name, g_AutoFocusDeviceName, MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   // Description
   ret = CreateProperty(MM::g_Keyword_Description, "Demo auto-focus adapter", MM::String, true);
   if (DEVICE_OK != ret)
      return ret;

   running_ = false;   

   ret = UpdateStatus();
   if (ret != DEVICE_OK)
      return ret;

   initialized_ = true;*/

   return DEVICE_OK;
}


int TransposeProcessor::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub)
   {
      if (pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      char hubLabel[MM::MaxStrLength];
      pHub->GetLabel(hubLabel);
      SetParentID(hubLabel); // for backward comp.
   }
   else
      LogMessage(NoHubError);

   if( NULL != this->pTemp_)
   {
      free(pTemp_);
      pTemp_ = NULL;
      this->tempSize_ = 0;
   }
    CPropertyAction* pAct = new CPropertyAction (this, &TransposeProcessor::OnInPlaceAlgorithm);
   (void)CreateProperty("InPlaceAlgorithm", "0", MM::Integer, false, pAct); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int TransposeProcessor::OnInPlaceAlgorithm(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   if (eAct == MM::BeforeGet)
   {
      pProp->Set(this->inPlace_?1L:0L);
   }
   else if (eAct == MM::AfterSet)
   {
      long ltmp;
      pProp->Get(ltmp);
      inPlace_ = (0==ltmp?false:true);
   }

   return DEVICE_OK;
}


int TransposeProcessor::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   int ret = DEVICE_OK;
   // 
   if( width != height)
      return DEVICE_NOT_SUPPORTED; // problem with tranposing non-square images is that the image buffer
   // will need to be modified by the image processor.
   if(busy_)
      return DEVICE_ERR;
 
   busy_ = true;

   if( inPlace_)
   {
      if(  sizeof(unsigned char) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned char*)pBuffer, width);
      }
      else if( sizeof(unsigned short) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned short*)pBuffer, width);
      }
      else if( sizeof(unsigned long) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned long*)pBuffer, width);
      }
      else if( sizeof(unsigned long long) == byteDepth)
      {
         TransposeSquareInPlace( (unsigned long long*)pBuffer, width);
      }
      else 
      {
         ret = DEVICE_NOT_SUPPORTED;
      }
   }
   else
   {
      if( sizeof(unsigned char) == byteDepth)
      {
         ret = TransposeRectangleOutOfPlace( (unsigned char*)pBuffer, width, height);
      }
      else if( sizeof(unsigned short) == byteDepth)
      {
         ret = TransposeRectangleOutOfPlace( (unsigned short*)pBuffer, width, height);
      }
      else if( sizeof(unsigned long) == byteDepth)
      {
         ret = TransposeRectangleOutOfPlace( (unsigned long*)pBuffer, width, height);
      }
      else if( sizeof(unsigned long long) == byteDepth)
      {
         ret =  TransposeRectangleOutOfPlace( (unsigned long long*)pBuffer, width, height);
      }
      else
      {
         ret =  DEVICE_NOT_SUPPORTED;
      }
   }
   busy_ = false;

   return ret;
}




int ImageFlipY::Initialize()
{
    CPropertyAction* pAct = new CPropertyAction (this, &ImageFlipY::OnPerformanceTiming);
    (void)CreateProperty("PeformanceTiming (microseconds)", "0", MM::Float, true, pAct); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int ImageFlipY::OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
      pProp->Set( performanceTiming_.getUsec());
   }
   else if (eAct == MM::AfterSet)
   {
      // -- it's ready only!
   }

   return DEVICE_OK;
}


int ImageFlipY::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   if(busy_)
      return DEVICE_ERR;

   int ret = DEVICE_OK;
 
   busy_ = true;
   performanceTiming_ = MM::MMTime(0.);
   MM::MMTime  s0 = GetCurrentMMTime();


   if( sizeof(unsigned char) == byteDepth)
   {
      ret = Flip( (unsigned char*)pBuffer, width, height);
   }
   else if( sizeof(unsigned short) == byteDepth)
   {
      ret = Flip( (unsigned short*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long) == byteDepth)
   {
      ret = Flip( (unsigned long*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long long) == byteDepth)
   {
      ret =  Flip( (unsigned long long*)pBuffer, width, height);
   }
   else
   {
      ret =  DEVICE_NOT_SUPPORTED;
   }

   performanceTiming_ = GetCurrentMMTime() - s0;
   busy_ = false;

   return ret;
}







///
int ImageFlipX::Initialize()
{
    CPropertyAction* pAct = new CPropertyAction (this, &ImageFlipX::OnPerformanceTiming);
    (void)CreateProperty("PeformanceTiming (microseconds)", "0", MM::Float, true, pAct); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int ImageFlipX::OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
      pProp->Set( performanceTiming_.getUsec());
   }
   else if (eAct == MM::AfterSet)
   {
      // -- it's ready only!
   }

   return DEVICE_OK;
}


int ImageFlipX::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   if(busy_)
      return DEVICE_ERR;

   int ret = DEVICE_OK;
 
   busy_ = true;
   performanceTiming_ = MM::MMTime(0.);
   MM::MMTime  s0 = GetCurrentMMTime();


   if( sizeof(unsigned char) == byteDepth)
   {
      ret = Flip( (unsigned char*)pBuffer, width, height);
   }
   else if( sizeof(unsigned short) == byteDepth)
   {
      ret = Flip( (unsigned short*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long) == byteDepth)
   {
      ret = Flip( (unsigned long*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long long) == byteDepth)
   {
      ret =  Flip( (unsigned long long*)pBuffer, width, height);
   }
   else
   {
      ret =  DEVICE_NOT_SUPPORTED;
   }

   performanceTiming_ = GetCurrentMMTime() - s0;
   busy_ = false;

   return ret;
}

///
int MedianFilter::Initialize()
{
    CPropertyAction* pAct = new CPropertyAction (this, &MedianFilter::OnPerformanceTiming);
    (void)CreateProperty("PeformanceTiming (microseconds)", "0", MM::Float, true, pAct); 
    (void)CreateProperty("BEWARE", "THIS FILTER MODIFIES DATA, EACH PIXEL IS REPLACED BY 3X3 NEIGHBORHOOD MEDIAN", MM::String, true); 
   return DEVICE_OK;
}

   // action interface
   // ----------------
int MedianFilter::OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct)
{

   if (eAct == MM::BeforeGet)
   {
      pProp->Set( performanceTiming_.getUsec());
   }
   else if (eAct == MM::AfterSet)
   {
      // -- it's ready only!
   }

   return DEVICE_OK;
}


int MedianFilter::Process(unsigned char *pBuffer, unsigned int width, unsigned int height, unsigned int byteDepth)
{
   if(busy_)
      return DEVICE_ERR;

   int ret = DEVICE_OK;
 
   busy_ = true;
   performanceTiming_ = MM::MMTime(0.);
   MM::MMTime  s0 = GetCurrentMMTime();


   if( sizeof(unsigned char) == byteDepth)
   {
      ret = Filter( (unsigned char*)pBuffer, width, height);
   }
   else if( sizeof(unsigned short) == byteDepth)
   {
      ret = Filter( (unsigned short*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long) == byteDepth)
   {
      ret = Filter( (unsigned long*)pBuffer, width, height);
   }
   else if( sizeof(unsigned long long) == byteDepth)
   {
      ret =  Filter( (unsigned long long*)pBuffer, width, height);
   }
   else
   {
      ret =  DEVICE_NOT_SUPPORTED;
   }

   performanceTiming_ = GetCurrentMMTime() - s0;
   busy_ = false;

   return ret;
}


int DemoHub::Initialize()
{
   DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
   if (pHub && pHub->GenerateRandomError())
      return SIMULATED_ERROR;

   SetErrorText(SIMULATED_ERROR, "Simulated Error");
	CPropertyAction *pAct = new CPropertyAction (this, &DemoHub::OnErrorRate);
	CreateProperty("SimulatedErrorRate", "0.0", MM::Float, false, pAct);
	AddAllowedValue("SimulatedErrorRate", "0.0000");
	AddAllowedValue("SimulatedErrorRate", "0.0001");
   AddAllowedValue("SimulatedErrorRate", "0.0010");
	AddAllowedValue("SimulatedErrorRate", "0.0100");
	AddAllowedValue("SimulatedErrorRate", "0.1000");
	AddAllowedValue("SimulatedErrorRate", "0.2000");
	AddAllowedValue("SimulatedErrorRate", "0.5000");
	AddAllowedValue("SimulatedErrorRate", "1.0000");

   pAct = new CPropertyAction (this, &DemoHub::OnDivideOneByMe);
   std::ostringstream os;
   os<<this->divideOneByMe_;
   CreateProperty("DivideOneByMe", os.str().c_str(), MM::Integer, false, pAct);

  	initialized_ = true;
 
	return DEVICE_OK;
}

int DemoHub::DetectInstalledDevices()
{  
   ClearInstalledDevices();

   // make sure this method is called before we look for available devices
   InitializeModuleData();

   char hubName[MM::MaxStrLength];
   GetName(hubName); // this device name
   for (unsigned i=0; i<GetNumberOfDevices(); i++)
   { 
      char deviceName[MM::MaxStrLength];
      bool success = GetDeviceName(i, deviceName, MM::MaxStrLength);
      if (success && (strcmp(hubName, deviceName) != 0))
      {
         MM::Device* pDev = CreateDevice(deviceName);
         AddInstalledDevice(pDev);
      }
   }
   return DEVICE_OK; 
}

MM::Device* DemoHub::CreatePeripheralDevice(const char* adapterName)
{
   for (unsigned i=0; i<GetNumberOfInstalledDevices(); i++)
   {
      MM::Device* d = GetInstalledDevice(i);
      char name[MM::MaxStrLength];
      d->GetName(name);
      if (strcmp(adapterName, name) == 0)
         return CreateDevice(adapterName);

   }
   return 0; // adapter name not found
}


void DemoHub::GetName(char* pName) const
{
   CDeviceUtils::CopyLimitedString(pName, g_HubDeviceName);
}

bool DemoHub::GenerateRandomError()
{
   if (errorRate_ == 0.0)
      return false;

	return (0 == (rand() % ((int) (1.0 / errorRate_))));
}

int DemoHub::OnErrorRate(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   // Don't simulate an error here!!!!

   if (eAct == MM::AfterSet)
   {
      pProp->Get(errorRate_);

   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(errorRate_);
   }
   return DEVICE_OK;
}

int DemoHub::OnDivideOneByMe(MM::PropertyBase* pProp, MM::ActionType eAct)
{
   // Don't simulate an error here!!!!

   if (eAct == MM::AfterSet)
   {
      pProp->Get(divideOneByMe_);
      static long result = 0;
      bool crashtest = CDeviceUtils::CheckEnvironment("MICROMANAGERCRASHTEST");
      if((0 != divideOneByMe_) || crashtest)
         result = 1/divideOneByMe_;
      result = result;

   }
   else if (eAct == MM::BeforeGet)
   {
      pProp->Set(divideOneByMe_);
   }
   return DEVICE_OK;
}


