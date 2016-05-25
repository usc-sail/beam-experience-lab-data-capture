#!/usr/bin/env python

import pdb
import rospy
import rosbag
from std_msgs.msg import Float32

def doStitchBags():
   rospy.init_node('bag_stitcher')

   with rosbag.Bag('/home/elab/Desktop/outbag.bag', 'w') as outbag:
      bag = rosbag.Bag('/home/elab/Desktop/mooc_pilot_study_data/original_bags/11_Participant_2016-05-03-17-39-07.bag', 'r')
      stitchBag = rosbag.Bag('/home/elab/Desktop/mooc_pilot_study_data/11_session_coded.bag', 'r')

      # Obtain a sorted list of new topics to stitch in
      bagStartTime = rospy.Time(bag.get_start_time())
      stitchList = []
      lastTime = bagStartTime
      for topic, msg, t in stitchBag.read_messages():
         if t < lastTime and len(stitchList) > 0:
            clipIndex = next(x[0] for x in enumerate(stitchList) if x[1][2] >= t)
            stitchList = stitchList[0:clipIndex]
         lastTime = t
         stitchList.append((topic, msg, t))

      # Write original topics
      for topic, msg, t in bag.read_messages():
         outbag.write(topic, msg, t)
         
      # Stitch in new topics
      for (topic, msg, t) in stitchList:
         outbag.write(topic, msg, t)

if __name__=='__main__':
   doStitchBags()