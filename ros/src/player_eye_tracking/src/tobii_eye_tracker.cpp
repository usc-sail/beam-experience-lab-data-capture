#include "tobii_eye_tracker.h"
#include "tobiigaze_discovery.h"
#include "tobiigaze_error_codes.h"
#include "tobiigaze.h"
#include <boost/lockfree/spsc_queue.hpp>

TobiiEyeTracker* TobiiEyeTracker::instance = NULL;

TobiiEyeTracker* TobiiEyeTracker::GetInstance()
{
   if (!instance)
   {
      instance = new TobiiEyeTracker();
   }

   return instance;
}

TobiiEyeTracker::TobiiEyeTracker()
{
}

TobiiEyeTracker::~TobiiEyeTracker()
{
   Shutdown();
   instance = NULL;
}

bool TobiiEyeTracker::Initialize()
{
   // Check for a connected Tobii eye tracker device
   tobiigaze_error_code errorCode;
   char urlBuffer[2048];
   tobiigaze_get_connected_eye_tracker(urlBuffer, sizeof(urlBuffer), &errorCode);
   if (errorCode)
   {
      errorString = "No eye tracker device found connected to the system\nError: " + GetErrorCodeString(errorCode);
      return false;
   }

   sdkVersionString = tobiigaze_get_version();

   // Create an eye tracker instance
   eyeTracker = tobiigaze_create(urlBuffer, &errorCode);
   if (errorCode)
   {
      errorString = "Could not create eye tracker object\nError: " + GetErrorCodeString(errorCode);
      return false;
   }

   // Spawn a thread to process eye tracker events
   threadHandle = xthread_create(&EyeTrackerEventLoop, eyeTracker);

   // Connect to the eye tracker device
   tobiigaze_connect(eyeTracker, &errorCode);
   if (errorCode)
   {
      Shutdown();
      errorString = "Could not connect to eye tracker\nError: " + GetErrorCodeString(errorCode);
      return false;
   }

   // Get the device information
   tobiigaze_device_info deviceInfo;
   tobiigaze_get_device_info(eyeTracker, &deviceInfo, &errorCode);
   if (errorCode)
   {
      Shutdown();
      errorString = "Unable to retrieve device info\nError: " + GetErrorCodeString(errorCode);
      return false;
   }
   deviceSerialNumberString = deviceInfo.serial_number;

   // Start eye tracking
   tobiigaze_start_tracking(eyeTracker, &TobiiEyeTracker::GazeDataCallback, &errorCode, 0);
   if (errorCode)
   {
      Shutdown();
      errorString = "Unable to start eye tracking\nError: " + GetErrorCodeString(errorCode);
      return false;
   }
}

bool TobiiEyeTracker::Shutdown()
{
   // Stop eye tracking
   tobiigaze_error_code errorCode;
   tobiigaze_stop_tracking(eyeTracker, &errorCode);
   if (errorCode)
   {
      errorString = "Unable to stop eye tracking\nError: " + GetErrorCodeString(errorCode);
   }

   // Tell the event loop thread to stop running and wait for it to finish
   tobiigaze_break_event_loop(eyeTracker);
   xthread_join(threadHandle);

   // Clean up
   tobiigaze_destroy(eyeTracker);
}

xthread_retval TobiiEyeTracker::EyeTrackerEventLoop(void* eyeTracker)
{
   tobiigaze_error_code errorCode;
   tobiigaze_run_event_loop((tobiigaze_eye_tracker*)eyeTracker, &errorCode);
   if (errorCode)
   {
      TobiiEyeTracker::GetInstance()->errorString = "Event loop returned an error\nError: " + TobiiEyeTracker::GetInstance()->GetErrorCodeString(errorCode);
   }
  
   THREADFUNC_RETURN(errorCode);
}

void TobiiEyeTracker::GazeDataCallback(const tobiigaze_gaze_data* tobiiGazeData, const tobiigaze_gaze_data_extensions* extensions, void* userData)
{
   GazeData gazeData;
   if (tobiiGazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_BOTH_EYES_TRACKED ||
       tobiiGazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONLY_LEFT_EYE_TRACKED ||
       tobiiGazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONE_EYE_TRACKED_PROBABLY_LEFT)
   {
      gazeData.leftEyeX = tobiiGazeData->left.gaze_point_on_display_normalized.x;
      gazeData.leftEyeY = tobiiGazeData->left.gaze_point_on_display_normalized.y;
   }
   else
   {
      gazeData.leftEyeX = 0;
      gazeData.leftEyeY = 0;
   }

   if (tobiiGazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_BOTH_EYES_TRACKED ||
       tobiiGazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONLY_RIGHT_EYE_TRACKED ||
       tobiiGazeData->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONE_EYE_TRACKED_PROBABLY_RIGHT)
   {
      gazeData.rightEyeX = tobiiGazeData->right.gaze_point_on_display_normalized.x;
      gazeData.rightEyeY = tobiiGazeData->right.gaze_point_on_display_normalized.y;
   }
   else
   {
      gazeData.rightEyeX = 0;
      gazeData.rightEyeY = 0;
   }

   TobiiEyeTracker::GetInstance()->gazeDataQueue.push(gazeData);
}

bool TobiiEyeTracker::HasGazeData()
{
   return !gazeDataQueue.empty();
}

bool TobiiEyeTracker::GetGazeData(GazeData& gazeData)
{
   return gazeDataQueue.pop(gazeData);
}

std::string TobiiEyeTracker::GetErrorCodeString(tobiigaze_error_code errorCode)
{
   return std::string(tobiigaze_get_error_message(errorCode));
}
