#include "ros/ros.h"
#include "player_eye_tracking/Position2D.h"
#include "tobii_eye_tracker.h"

int main(int argc, char** argv)
{
   const double SmoothingFactor = 0.15; // Smaller means more smoothing
   const ros::Duration DurationUntilLostEyeTrack(0.5);

	ros::init(argc, argv, "playerEyeTracking");
	ros::NodeHandle nodeHandle;
	ros::Publisher eyeTrackingPublisher = nodeHandle.advertise<player_eye_tracking::Position2D>("eye_position", 1);

   ros::Time lastEyePositionTime = ros::Time::now();
   player_eye_tracking::Position2D smoothEyePosition;

   TobiiEyeTracker::GetInstance()->Initialize();
   const std::string& errorString = TobiiEyeTracker::GetInstance()->GetErrorString();
   if (!errorString.empty())
   {
      ROS_ERROR("%s", errorString.c_str());
      return EXIT_FAILURE;
   }
   ROS_INFO("Eye tracking started...");
   ROS_INFO("TobiiGazeCore version: %s", TobiiEyeTracker::GetInstance()->GetSDKVersionString().c_str());
   ROS_INFO("Eye tracking device serial number is: %s", TobiiEyeTracker::GetInstance()->GetDeviceSerialNumber().c_str());

   while (ros::ok())
   {
      while (TobiiEyeTracker::GetInstance()->HasGazeData())
      {
         GazeData gazeData;
         TobiiEyeTracker::GetInstance()->GetGazeData(gazeData);

         // Compute the average gaze location between both the left and right eyes
         bool hasValidEyePosition = false;
         player_eye_tracking::Position2D eyePosition;
         if (gazeData.leftEyeX != 0 && gazeData.leftEyeY != 0)
         {
            if (gazeData.rightEyeX != 0 && gazeData.rightEyeY != 0)
            {
               eyePosition.posX = (gazeData.leftEyeX + gazeData.rightEyeX)/2.0f;
               eyePosition.posY = (gazeData.leftEyeY + gazeData.rightEyeY)/2.0f;
            }
            else
            {
               eyePosition.posX = gazeData.leftEyeX;
               eyePosition.posY = gazeData.leftEyeY;
            }

            hasValidEyePosition = true;
         }
         else if (gazeData.rightEyeX != 0 && gazeData.rightEyeY != 0)
         {
            eyePosition.posX = gazeData.rightEyeX;
            eyePosition.posY = gazeData.rightEyeY;

            hasValidEyePosition = true;
         }

         if (hasValidEyePosition)
         {
            if (ros::Time::now() - lastEyePositionTime >= DurationUntilLostEyeTrack)
            {
               smoothEyePosition = eyePosition;
            }
            else
            {
               smoothEyePosition.posX = SmoothingFactor*eyePosition.posX + (1.00 - SmoothingFactor)*smoothEyePosition.posX;
               smoothEyePosition.posY = SmoothingFactor*eyePosition.posY + (1.00 - SmoothingFactor)*smoothEyePosition.posY;
            }

            lastEyePositionTime = ros::Time::now();
            eyeTrackingPublisher.publish(smoothEyePosition);
         }
         else
         {
            if (ros::Time::now() - lastEyePositionTime < DurationUntilLostEyeTrack)
            {
               eyeTrackingPublisher.publish(smoothEyePosition);
            }
         }
      }

      ros::spinOnce();
   }

   TobiiEyeTracker::GetInstance()->Shutdown();
   ROS_INFO("Eye tracking stopped...");
}