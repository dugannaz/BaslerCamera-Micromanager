///////////////////////////////////////////////////////////////////////////////
// FILE:          Basler.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Implementation of Basler camera.
//                Device adapter for 
//                Basler ace acA2000-340km camera 
//                is implemented by minimal modification of Demo Camera example. 
//                Enables full speed image capture (340 fps) at resolution 2048x1088 
//                
// AUTHOR:        Nazim Dugan, dugannaz@gmail.com, 2014
//

#ifndef _BASLERCAMERA_H_
#define _BASLERCAMERA_H_

#define EURESYS_SURFACE_COUNT 3

#define _AFXDLL

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

#include "../../MMDevice/DeviceBase.h"
#include "../../MMDevice/ImgBuffer.h"
#include "../../MMDevice/DeviceThreads.h"
#include <string>
#include <map>
#include <algorithm>

#include "multicam.h"

//////////////////////////////////////////////////////////////////////////////
// Error codes
//
#define ERR_UNKNOWN_MODE         102
#define ERR_UNKNOWN_POSITION     103
#define ERR_IN_SEQUENCE          104
#define ERR_SEQUENCE_INACTIVE    105
#define ERR_STAGE_MOVING         106
#define SIMULATED_ERROR          200
#define HUB_NOT_AVAILABLE        107

const char* NoHubError = "Parent Hub not defined.";

class DemoHub : public HubBase<DemoHub>
{
public:
   DemoHub():initialized_(false), busy_(false), errorRate_(0.0), divideOneByMe_(1) {} ;
   ~DemoHub() {};

   // Device API
   // ---------
   int Initialize();
   int Shutdown() {return DEVICE_OK;};
   void GetName(char* pName) const; 
   bool Busy() { return busy_;} ;
   bool GenerateRandomError();

   // HUB api
   int DetectInstalledDevices();
   MM::Device* CreatePeripheralDevice(const char* adapterName);

   // action interface
   int OnErrorRate(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnDivideOneByMe(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   void GetPeripheralInventory();

   std::vector<std::string> peripherals_;
   bool initialized_;
   bool busy_;
   double errorRate_;
   long divideOneByMe_;
};


//////////////////////////////////////////////////////////////////////////////
// CBaslerCamera class
//////////////////////////////////////////////////////////////////////////////

class MySequenceThread;

class CBaslerCamera : public CCameraBase<CBaslerCamera>,public CDocument
{
public:
   CBaslerCamera();
   ~CBaslerCamera();
  
   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();
  
   void GetName(char* name) const;      
   
   // MMCamera API
   // ------------
   int SnapImage();
   const unsigned char* GetImageBuffer();
   unsigned GetImageWidth() const;
   unsigned GetImageHeight() const;
   unsigned GetImageBytesPerPixel() const;
   unsigned GetBitDepth() const;
   long GetImageBufferSize() const;
   double GetExposure() const;
   void SetExposure(double exp);
   int SetROI(unsigned x, unsigned y, unsigned xSize, unsigned ySize); 
   int GetROI(unsigned& x, unsigned& y, unsigned& xSize, unsigned& ySize); 
   int ClearROI();
   int PrepareSequenceAcqusition()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      return DEVICE_OK;
   }
   int StartSequenceAcquisition(double interval);
   int StartSequenceAcquisition(long numImages, double interval_ms, bool stopOnOverflow);
   int StopSequenceAcquisition();
   int InsertImage();
   int ThreadRun(MM::MMTime startTime);
   bool IsCapturing();
   void OnThreadExiting() throw(); 
   double GetNominalPixelSizeUm() const {return nominalPixelSizeUm_;}
   double GetPixelSizeUm() const {return nominalPixelSizeUm_ * GetBinning();}
   int GetBinning() const;
   int SetBinning(int bS);
   int IsExposureSequenceable(bool& isSequenceable) const 
   {
      isSequenceable = isSequenceable_;
      return DEVICE_OK;
   }
   int GetExposureSequenceMaxLength(long& nrEvents) 
   {
      nrEvents = sequenceMaxLength_;
      return DEVICE_OK;
   }
   int StartExposureSequence() 
   {
      // may need thread lock
      sequenceRunning_ = true;
      return DEVICE_OK;
   }
   int StopExposureSequence() 
   {
      // may need thread lock
      sequenceRunning_ = false; 
      sequenceIndex_ = 0;
      return DEVICE_OK;
   }
   // Remove all values in the sequence                                   
   int ClearExposureSequence();
   // Add one value to the sequence                                       
   int AddToExposureSequence(double exposureTime_ms);
   // Signal that we are done sending sequence values so that the adapter can send the whole sequence to the device
   int SendExposureSequence() const {
      return DEVICE_OK;
   }

   unsigned  GetNumberOfComponents() const { return nComponents_;};

   // action interface
   // ----------------
	// floating point read-only properties for testing
	int OnTestProperty(MM::PropertyBase* pProp, MM::ActionType eAct, long);
	//int OnSwitch(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnBinning(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnPixelType(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnBitDepth(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnReadoutTime(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnScanMode(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnErrorSimulation(MM::PropertyBase* , MM::ActionType );
   int OnCameraCCDXSize(MM::PropertyBase* , MM::ActionType );
   int OnCameraCCDYSize(MM::PropertyBase* , MM::ActionType );
   int OnTriggerDevice(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnDropPixels(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnFastImage(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnSaturatePixels(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnFractionOfPixelsToDropOrSaturate(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnCCDTemp(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnIsSequenceable(MM::PropertyBase* pProp, MM::ActionType eAct);

   //static PVOID m_pCurrent;
   MCHANDLE m_Channel;
   int m_SizeX;
   int m_SizeY;
   int m_BufferPitch;

private:
   int SetAllowedBinning();
   void TestResourceLocking(const bool);
   void GenerateEmptyImage(ImgBuffer& img);
   void GetCameraImage(ImgBuffer& img);
   void GenerateSyntheticImage(ImgBuffer& img, double exp);
   int ResizeImageBuffer();

   static const double nominalPixelSizeUm_;

   double dPhase_;
   ImgBuffer img_;
   bool busy_;
   bool stopOnOverFlow_;
   bool initialized_;
   double readoutUs_;
   MM::MMTime readoutStartTime_;
   long scanMode_;
   int bitDepth_;
   unsigned roiX_;
   unsigned roiY_;
   MM::MMTime sequenceStartTime_;
   bool isSequenceable_;
   long sequenceMaxLength_;
   bool sequenceRunning_;
   unsigned long sequenceIndex_;
   double GetSequenceExposure();
   std::vector<double> exposureSequence_;
   long imageCounter_;
	long binSize_;
	long cameraCCDXSize_;
	long cameraCCDYSize_;
   double ccdT_;
	std::string triggerDevice_;

   bool stopOnOverflow_;

	bool dropPixels_;
   bool fastImage_;
	bool saturatePixels_;
	double fractionOfPixelsToDropOrSaturate_;

	double testProperty_[10];
   MMThreadLock* pDemoResourceLock_;
   MMThreadLock imgPixelsLock_;
   friend class MySequenceThread;
   int nComponents_;
   MySequenceThread * thd_;
   void *method;
   int paddedX;
   int paddedY;
};
PVOID m_pCurrent;
unsigned char *m_pCurrent1;

class MySequenceThread : public MMDeviceThreadBase
{
   friend class CBaslerCamera;
   enum { default_numImages=1, default_intervalMS = 100 };
   public:
      MySequenceThread(CBaslerCamera* pCam);
      ~MySequenceThread();
      void Stop();
      void Start(long numImages, double intervalMs);
      bool IsStopped();
      void Suspend();
      bool IsSuspended();
      void Resume();
      double GetIntervalMs(){return intervalMs_;}                               
      void SetLength(long images) {numImages_ = images;}                        
      long GetLength() const {return numImages_;}
      long GetImageCounter(){return imageCounter_;}                             
      MM::MMTime GetStartTime(){return startTime_;}                             
      MM::MMTime GetActualDuration(){return actualDuration_;}
   private:                                                                     
      int svc(void) throw();
      double intervalMs_;                                                       
      long numImages_;                                                          
      long imageCounter_;                                                       
      bool stop_;                                                               
      bool suspend_;                                                            
      CBaslerCamera* camera_;                                                     
      MM::MMTime startTime_;                                                    
      MM::MMTime actualDuration_;                                               
      MM::MMTime lastFrameTime_;                                                
      MMThreadLock stopLock_;                                                   
      MMThreadLock suspendLock_;                                                
}; 

//////////////////////////////////////////////////////////////////////////////
// CDemoFilterWheel class
// Simulation of the filter changer (state device)
//////////////////////////////////////////////////////////////////////////////

class CDemoFilterWheel : public CStateDeviceBase<CDemoFilterWheel>
{
public:
   CDemoFilterWheel();
   ~CDemoFilterWheel();
  
   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy();
   unsigned long GetNumberOfPositions()const {return numPos_;}

   // action interface
   // ----------------
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   long numPos_;
   bool busy_;
   bool initialized_;
   MM::MMTime changedTime_;
   long position_;
};

//////////////////////////////////////////////////////////////////////////////
// CDemoLightPath class
// Simulation of the microscope light path switch (state device)
//////////////////////////////////////////////////////////////////////////////
class CDemoLightPath : public CStateDeviceBase<CDemoLightPath>
{
public:
   CDemoLightPath();
   ~CDemoLightPath();
  
   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy() {return busy_;}
   unsigned long GetNumberOfPositions()const {return numPos_;}

   // action interface
   // ----------------
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   long numPos_;
   bool busy_;
   bool initialized_;
   long position_;
};

//////////////////////////////////////////////////////////////////////////////
// CDemoObjectiveTurret class
// Simulation of the objective changer (state device)
//////////////////////////////////////////////////////////////////////////////

class CDemoObjectiveTurret : public CStateDeviceBase<CDemoObjectiveTurret>
{
public:
   CDemoObjectiveTurret();
   ~CDemoObjectiveTurret();
  
   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const ;
   bool Busy() {return busy_;}
   unsigned long GetNumberOfPositions()const {return numPos_;}

   // action interface
   // ----------------
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnTrigger(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   long numPos_;
   bool busy_;
   bool initialized_;
   bool sequenceRunning_;
   unsigned long sequenceMaxSize_;
   unsigned long sequenceIndex_;
   std::vector<std::string> sequence_;
   long position_;
};

//////////////////////////////////////////////////////////////////////////////
// CDemoStateDevice class
// Simulation of a state device in which the number of states can be specified (state device)
//////////////////////////////////////////////////////////////////////////////

class CDemoStateDevice : public CStateDeviceBase<CDemoStateDevice>
{
public:
   CDemoStateDevice();
   ~CDemoStateDevice();
  
   // MMDevice API
   // ------------
   int Initialize();
   int Shutdown();
  
   void GetName(char* pszName) const;
   bool Busy();
   unsigned long GetNumberOfPositions()const {return numPos_;}

   // action interface
   // ----------------
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnNumberOfStates(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   long numPos_;
   bool busy_;
   bool initialized_;
   MM::MMTime changedTime_;
   long position_;
};

//////////////////////////////////////////////////////////////////////////////
// CDemoStage class
// Simulation of the single axis stage
//////////////////////////////////////////////////////////////////////////////

class CDemoStage : public CStageBase<CDemoStage>
{
public:
   CDemoStage();
   ~CDemoStage();

   bool Busy() {return busy_;}
   void GetName(char* pszName) const;

   int Initialize();
   int Shutdown();
     
   // Stage API
   int SetPositionUm(double pos);
   int GetPositionUm(double& pos) {pos = pos_um_; LogMessage("Reporting position", true); return DEVICE_OK;}
   double GetStepSize() {return stepSize_um_;}
   int SetPositionSteps(long steps) 
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      pos_um_ = steps * stepSize_um_; 
      return  OnStagePositionChanged(pos_um_);
   }
   int GetPositionSteps(long& steps)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      steps = (long)(pos_um_ / stepSize_um_);
      return DEVICE_OK;
   }
   int SetOrigin()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      return DEVICE_OK;
   }
   int GetLimits(double& lower, double& upper)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      lower = lowerLimit_;
      upper = upperLimit_;
      return DEVICE_OK;
   }
   int Move(double /*v*/) {return DEVICE_OK;}

   bool IsContinuousFocusDrive() const {return false;}

   // action interface
   // ----------------
   int OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct);

   // Sequence functions
   int IsStageSequenceable(bool& isSequenceable) const {
      isSequenceable = false;
      return DEVICE_OK;
   }
   int GetStageSequenceMaxLength(long& nrEvents) const 
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      nrEvents = 0; return DEVICE_OK;
   }
   int StartStageSequence() const
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      return DEVICE_OK;
   }
   int StopStageSequence() const
   {  
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;
      return DEVICE_OK;
   }
   int ClearStageSequence() {return DEVICE_OK;}
   int AddToStageSequence(double /* position */) {return DEVICE_OK;}
   int SendStageSequence() const {return DEVICE_OK;}

private:
   void SetIntensityFactor(double pos);
   double stepSize_um_;
   double pos_um_;
   bool busy_;
   bool initialized_;
   double lowerLimit_;
   double upperLimit_;
};

//////////////////////////////////////////////////////////////////////////////
// CDemoStage class
// Simulation of the single axis stage
//////////////////////////////////////////////////////////////////////////////

class CDemoXYStage : public CXYStageBase<CDemoXYStage>
{
public:
   CDemoXYStage();
   ~CDemoXYStage();

   bool Busy();
   void GetName(char* pszName) const;

   int Initialize();
   int Shutdown();
     
   // XYStage API
   /* Note that only the Set/Get PositionStep functions are implemented in the adapter
    * It is best not to override the Set/Get PositionUm functions in DeviceBase.h, since
    * those implement corrections based on whether or not X and Y directionality should be 
    * mirrored and based on a user defined origin
    */

   // This must be correct or the conversions between steps and Um will go wrong
   virtual double GetStepSize() {return stepSize_um_;}
   virtual int SetPositionSteps(long x, long y)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      if (timeOutTimer_ != 0)
      {
         if (!timeOutTimer_->expired(GetCurrentMMTime()))
               return ERR_STAGE_MOVING;
         delete (timeOutTimer_);
      }
      double newPosX = x * stepSize_um_;
      double newPosY = y * stepSize_um_;
      double difX = newPosX - posX_um_;
      double difY = newPosY - posY_um_;
      double distance = sqrt( (difX * difX) + (difY * difY) );
      long timeOut = (long) (distance / velocity_);
      timeOutTimer_ = new MM::TimeoutMs(GetCurrentMMTime(),  timeOut);
      posX_um_ = x * stepSize_um_;
      posY_um_ = y * stepSize_um_;
      int ret = OnXYStagePositionChanged(posX_um_, posY_um_);
      if (ret != DEVICE_OK)
         return ret;

      return DEVICE_OK;
   }
   virtual int GetPositionSteps(long& x, long& y)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      x = (long)(posX_um_ / stepSize_um_);
      y = (long)(posY_um_ / stepSize_um_);
      return DEVICE_OK;
   }
   int SetRelativePositionSteps(long x, long y)                                                           
   {                                                                                                      
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      long xSteps, ySteps;                                                                                
      GetPositionSteps(xSteps, ySteps);                                                   

      return this->SetPositionSteps(xSteps+x, ySteps+y);                                                  
   } 
   virtual int Home()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }
   virtual int Stop()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }

   /* This sets the 0,0 position of the adapter to the current position.  
    * If possible, the stage controller itself should also be set to 0,0
    * Note that this differs form the function SetAdapterOrigin(), which 
    * sets the coordinate system used by the adapter
    * to values different from the system used by the stage controller
    */
   virtual int SetOrigin()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }
   virtual int GetLimits(double& lower, double& upper)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      lower = lowerLimit_;
      upper = upperLimit_;
      return DEVICE_OK;
   }
   virtual int GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      xMin = lowerLimit_; xMax = upperLimit_;
      yMin = lowerLimit_; yMax = upperLimit_;
      return DEVICE_OK;
   }

   virtual int GetStepLimits(long& /*xMin*/, long& /*xMax*/, long& /*yMin*/, long& /*yMax*/)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_UNSUPPORTED_COMMAND;
   }
   double GetStepSizeXUm()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return stepSize_um_;
   }
   double GetStepSizeYUm()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return stepSize_um_;
   }
   int Move(double /*vx*/, double /*vy*/) {return DEVICE_OK;}

   int IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}


   // action interface
   // ----------------
   int OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   double stepSize_um_;
   double posX_um_;
   double posY_um_;
   bool busy_;
   MM::TimeoutMs* timeOutTimer_;
   double velocity_;
   bool initialized_;
   double lowerLimit_;
   double upperLimit_;
};

//////////////////////////////////////////////////////////////////////////////
// DemoShutter class
// Simulation of shutter device
//////////////////////////////////////////////////////////////////////////////
class DemoShutter : public CShutterBase<DemoShutter>
{
public:
   DemoShutter() : state_(false), initialized_(false), changedTime_(0.0)
   {
      EnableDelay(); // signals that the dealy setting will be used
      
      // parent ID display
      CreateHubIDProperty();
   }
   ~DemoShutter() {}

   int Initialize();
   int Shutdown() {initialized_ = false; return DEVICE_OK;}

   void GetName (char* pszName) const;
   bool Busy();

   // Shutter API
   int SetOpen (bool open = true)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      state_ = open;
      changedTime_ = GetCurrentMMTime();
      return DEVICE_OK;
   }
   int GetOpen(bool& open)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      open = state_;
      return DEVICE_OK;
   }
   int Fire(double /*deltaT*/)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_UNSUPPORTED_COMMAND;
   }

   // action interface
   int OnState(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   bool state_;
   bool initialized_;
   MM::MMTime changedTime_;
};

//////////////////////////////////////////////////////////////////////////////
// DemoShutter class
// Simulation of shutter device
//////////////////////////////////////////////////////////////////////////////
class DemoDA : public CSignalIOBase<DemoDA>
{
public:
   DemoDA ();
   ~DemoDA ();

   int Shutdown() {return DEVICE_OK;}
   void GetName(char* name) const;
   int SetGateOpen(bool open); 
   int GetGateOpen(bool& open);
   int SetSignal(double volts);
   int GetSignal(double& volts);
   int GetLimits(double& minVolts, double& maxVolts)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      minVolts=0.0; maxVolts= 10.0; return DEVICE_OK;
   }
   bool Busy() {return false;}
   int Initialize();

   // Sequence functions
   int IsDASequenceable(bool& isSequenceable) const
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      isSequenceable = true;
      return DEVICE_OK;
   }
   int GetDASequenceMaxLength(long& nrEvents) const 
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      nrEvents = 256;
      return DEVICE_OK;
   }
   int StartDASequence() const
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      (const_cast<DemoDA *>(this))->SetSequenceStateOn();
      return DEVICE_OK;
   }
   int StopDASequence() const
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;
   
      (const_cast<DemoDA *>(this))->SetSequenceStateOff();
      return DEVICE_OK;
   }
   int SendDASequence();
   int ClearDASequence();
   int AddToDASequence(double voltage);

   int OnTrigger(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnVoltage(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnRealVoltage(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   double volt_;
   double gatedVolts_;
   bool open_;
   bool sequenceRunning_;
   unsigned long sequenceIndex_;
   std::vector<double> sentSequence_;
   std::vector<double> nascentSequence_;

   void SetSequenceStateOn() { sequenceRunning_ = true; }
   void SetSequenceStateOff() { sequenceRunning_ = false; sequenceIndex_ = 0; }

   void SetSentSequence();
};


//////////////////////////////////////////////////////////////////////////////
// DemoMagnifier class
// Simulation of magnifier Device
//////////////////////////////////////////////////////////////////////////////
class DemoMagnifier : public CMagnifierBase<DemoMagnifier>
{
public:
   DemoMagnifier();

   ~DemoMagnifier () {};

   int Shutdown()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }

   void GetName(char* name) const;

   bool Busy() {return false;}
   int Initialize();

   double GetMagnification();

   // action interface
   // ----------------
   int OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct);
   int OnHighMag(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   std::string highMagString();

   int position_;
   double highMag_;
};

//////////////////////////////////////////////////////////////////////////////
// TransposeProcessor class
// transpose an image
// K.H.
//////////////////////////////////////////////////////////////////////////////
class TransposeProcessor : public CImageProcessorBase<TransposeProcessor>
{
public:
   TransposeProcessor () : inPlace_ (false), pTemp_(NULL), tempSize_(0), busy_(false)
   {
      // parent ID display
      CreateHubIDProperty();
   }
   ~TransposeProcessor () {if( NULL!= pTemp_) free(pTemp_); tempSize_=0;  }

   int Shutdown() {return DEVICE_OK;}
   void GetName(char* name) const {strcpy(name,"TransposeProcessor");}

   int Initialize();

   bool Busy(void) { return busy_;};

    // really primative image transpose algorithm which will work fine for non-square images... 
   template <typename PixelType> int TransposeRectangleOutOfPlace( PixelType* pI, unsigned int width, unsigned int height)
   {
      int ret = DEVICE_OK;
      unsigned long tsize = width*height*sizeof(PixelType);
      if( this->tempSize_ != tsize)
      {
         if( NULL != this->pTemp_)
         {
            free(pTemp_);
            pTemp_ = NULL;
         }
         pTemp_ = (PixelType *)malloc(tsize);
      }
      if( NULL != pTemp_)
      {
         PixelType* pTmpImage = (PixelType *) pTemp_;
         tempSize_ = tsize;
         for( unsigned long ix = 0; ix < width; ++ix)
         {
            for( unsigned long iy = 0; iy < height; ++iy)
            {
               pTmpImage[iy + ix*width] = pI[ ix + iy*height];
            }
         }
         memcpy( pI, pTmpImage, tsize);
      }
      else
      {
         ret = DEVICE_ERR;
      }

      return ret;
   }

   
   template <typename PixelType> void TransposeSquareInPlace( PixelType* pI, unsigned int dim) 
   { 
      PixelType tmp;
      for( unsigned long ix = 0; ix < dim; ++ix)
      {
         for( unsigned long iy = ix; iy < dim; ++iy)
         {
            tmp = pI[iy*dim + ix];
            pI[iy*dim +ix] = pI[ix*dim + iy];
            pI[ix*dim +iy] = tmp; 
         }
      }

      return;
   }

   int Process(unsigned char* buffer, unsigned width, unsigned height, unsigned byteDepth);

   // action interface
   // ----------------
   int OnInPlaceAlgorithm(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   bool inPlace_;
   void* pTemp_;
   unsigned long tempSize_;
   bool busy_;
};



//////////////////////////////////////////////////////////////////////////////
// ImageFlipX class
// flip an image
// K.H.
//////////////////////////////////////////////////////////////////////////////
class ImageFlipX : public CImageProcessorBase<ImageFlipX>
{
public:
   ImageFlipX () :  busy_(false) {}
   ~ImageFlipX () {  }

   int Shutdown() {return DEVICE_OK;}
   void GetName(char* name) const {strcpy(name,"ImageFlipX");}

   int Initialize();
   bool Busy(void) { return busy_;};

    // 
   template <typename PixelType> int Flip( PixelType* pI, unsigned int width, unsigned int height)
   {
      PixelType tmp;
      int ret = DEVICE_OK;
      for( unsigned long iy = 0; iy < height; ++iy)
      {
         for( unsigned long ix = 0; ix <  (width>>1) ; ++ix)
         {
            tmp = pI[ ix + iy*width];
            pI[ ix + iy*width] = pI[ width - 1 - ix + iy*width];
            pI[ width -1 - ix + iy*width] = tmp;
         }
      }
      return ret;
   }

   int Process(unsigned char* buffer, unsigned width, unsigned height, unsigned byteDepth);

   int OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   bool busy_;
   MM::MMTime performanceTiming_;
};


//////////////////////////////////////////////////////////////////////////////
// ImageFlipY class
// flip an image
// K.H.
//////////////////////////////////////////////////////////////////////////////
class ImageFlipY : public CImageProcessorBase<ImageFlipY>
{
public:
   ImageFlipY () : busy_(false), performanceTiming_(0.) {}
   ~ImageFlipY () {  }

   int Shutdown() {return DEVICE_OK;}
   void GetName(char* name) const {strcpy(name,"ImageFlipY");}

   int Initialize();
   bool Busy(void) { return busy_;};

   template <typename PixelType> int Flip( PixelType* pI, unsigned int width, unsigned int height)
   {
      PixelType tmp;
      int ret = DEVICE_OK;
      for( unsigned long ix = 0; ix < width ; ++ix)
      {
         for( unsigned long iy = 0; iy < (height>>1); ++iy)
         {
            tmp = pI[ ix + iy*width];
            pI[ ix + iy*width] = pI[ ix + (height - 1 - iy)*width];
            pI[ ix + (height - 1 - iy)*width] = tmp;
         }
      }
      return ret;
   }


   int Process(unsigned char* buffer, unsigned width, unsigned height, unsigned byteDepth);

   // action interface
   // ----------------
   int OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   bool busy_;
   MM::MMTime performanceTiming_;

};



//////////////////////////////////////////////////////////////////////////////
// MedianFilter class
// apply Median filter an image
// K.H.
//////////////////////////////////////////////////////////////////////////////
class MedianFilter : public CImageProcessorBase<MedianFilter>
{
public:
   MedianFilter () : busy_(false), performanceTiming_(0.),pSmoothedIm_(0), sizeOfSmoothedIm_(0)
   {
      // parent ID display
      CreateHubIDProperty();
   };
   ~MedianFilter () { if(0!=pSmoothedIm_) free(pSmoothedIm_); };

   int Shutdown() {return DEVICE_OK;}
   void GetName(char* name) const {strcpy(name,"MedianFilter");}

   int Initialize();
   bool Busy(void) { return busy_;};

   // NOTE: this utility MODIFIES the argument, make a copy yourself if you want the original data preserved
   template <class U> U FindMedian(std::vector<U>& values ) {
      std::sort(values.begin(), values.end());
      return values[(values.size())>>1];
   };


   template <typename PixelType> int Filter( PixelType* pI, unsigned int width, unsigned int height)
   {
      int ret = DEVICE_OK;
      int x[9];
      int y[9];

      const unsigned long thisSize = sizeof(*pI)*width*height;
      if( thisSize != sizeOfSmoothedIm_)
      {
         if(NULL!=pSmoothedIm_)
         {
            sizeOfSmoothedIm_ = 0;
            free(pSmoothedIm_);
         }
         // malloc is faster than new...
         pSmoothedIm_ = (PixelType*)malloc(thisSize);
         if(NULL!=pSmoothedIm_)
         {
            sizeOfSmoothedIm_ = thisSize;
         }
      }

      PixelType* pSmooth = (PixelType*) pSmoothedIm_;

      if(NULL != pSmooth)
      {
      /*Apply 3x3 median filter to reduce shot noise*/
      for (unsigned int i=0; i<width; i++) {
         for (unsigned int j=0; j<height; j++) {
            x[0]=i-1;
            y[0]=(j-1);
            x[1]=i;
            y[1]=(j-1);
            x[2]=i+1;
            y[2]=(j-1);
            x[3]=i-1;
            y[3]=(j);
            x[4]=i;
            y[4]=(j);
            x[5]=i+1;
            y[5]=(j);
            x[6]=i-1;
            y[6]=(j+1);
            x[7]=i;
            y[7]=(j+1);
            x[8]=i+1;
            y[8]=(j+1);
            // truncate the median filter window  -- duplicate edge points
            // this could be more efficient, we could fill in the interior image [1,w0-1]x[1,h0-1] then explicitly fill in the edge pixels.
            // also the temporary image could be as small as 2 rasters of the image
            for(int ij =0; ij < 9; ++ij)
            {
               if( x[ij] < 0)
                  x[ij] = 0;
               else if( int(width-1) < x[ij])
                  x[ij] = int(width-1);
               if( y[ij] < 0)
                  y[ij] = 0;
               else if( int(height-1) < y[ij])
                  y[ij] = (int)(height-1);
            }
            std::vector<PixelType> windo;
            for(int ij = 0; ij < 9; ++ij)
            {
               windo.push_back(pI[ x[ij] + width*y[ij]]);
            }
            pSmooth[i + j*width] = FindMedian(windo);
         }
      }

      memcpy( pI, pSmoothedIm_, thisSize);
      }
      else
         ret = DEVICE_ERR;

      return ret;
   }
   int Process(unsigned char* buffer, unsigned width, unsigned height, unsigned byteDepth);

   // action interface
   // ----------------
   int OnPerformanceTiming(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
   bool busy_;
   MM::MMTime performanceTiming_;
   void*  pSmoothedIm_;
   unsigned long sizeOfSmoothedIm_;
   


};




//////////////////////////////////////////////////////////////////////////////
// DemoAutoFocus class
// Simulation of the auto-focusing module
//////////////////////////////////////////////////////////////////////////////
class DemoAutoFocus : public CAutoFocusBase<DemoAutoFocus>
{
public:
   DemoAutoFocus() : 
      running_(false), 
      busy_(false), 
      initialized_(false)  
      {
         CreateHubIDProperty();
      }

   ~DemoAutoFocus() {}
      
   // MMDevice API
   bool Busy() {return busy_;}
   void GetName(char* pszName) const;

   int Initialize();
   int Shutdown(){initialized_ = false; return DEVICE_OK;}

   // AutoFocus API
   virtual int SetContinuousFocusing(bool state)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      running_ = state; return DEVICE_OK;
   }
   virtual int GetContinuousFocusing(bool& state)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      state = running_; return DEVICE_OK;
   }
   virtual bool IsContinuousFocusLocked()
   {
      return running_;
   }
   virtual int FullFocus()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }
   virtual int IncrementalFocus()
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }
   virtual int GetLastFocusScore(double& score)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      score = 0.0;
      return DEVICE_OK;
   }
   virtual int GetCurrentFocusScore(double& score)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      score = 1.0;
      return DEVICE_OK;
   }
   virtual int GetOffset(double& /*offset*/)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }
   virtual int SetOffset(double /*offset*/)
   {
      DemoHub* pHub = static_cast<DemoHub*>(GetParentHub());
      if (pHub && pHub->GenerateRandomError())
         return SIMULATED_ERROR;

      return DEVICE_OK;
   }

private:
   bool running_;
   bool busy_;
   bool initialized_;
};





#endif //_DEMOCAMERA_H_
