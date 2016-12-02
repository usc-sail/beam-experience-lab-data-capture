#!/usr/bin/python

import os
import sys
import pdb
import time
import platform
import subprocess

def doAnnotationLaunch(data_folder, out_data_folder, do_pov=True):
   data_subfolders = os.listdir(data_folder)
   found_subject_subfolder = False
   for data_subfolder in data_subfolders:
      if data_subfolder.startswith('subject_'):
         found_subject_subfolder = True
         break
   if not found_subject_subfolder:
      print 'Please provide a valid data folder containing subfolders with names like "subject_01"'
      return

   if not os.path.isdir(out_data_folder):
      os.mkdir(out_data_folder)

   proc = subprocess.Popen(['pgrep', 'roscore'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
   proc_out, proc_err = proc.communicate()
   if proc_out is None:
      os.system("roscore &")

   user_name = str(raw_input("Please enter your first name: ")).lower()
   subject_num = int(raw_input("Please enter your subject number: "))

   subject_data_folder = data_folder+"/subject_%02d"%subject_num
   subject_data_files = os.listdir(subject_data_folder)

   if do_pov:
      pov = str(raw_input("Please enter your point-of-view, either 'student' or 'lecture': ")).lower()
      if pov == 'lecture':
         subject_data_files = [x for x in subject_data_files if x.find('main') != -1]
   else:
      pov = ''

   for subject_data_file in subject_data_files:
      task = subject_data_file.split('_')[-1][:-4]
      out_bag_filename = str(subject_num)+'_'+task+'_'+pov+'_'+user_name+'.bag'
      launch_file = 'bag_annotate.launch' if (do_pov or pov == 'student') else 'bag_annotate_audio.launch'
      os.system('roslaunch experience_lab '+launch_file+' bag:='+subject_data_folder+'/'+subject_data_file+' outbag:='+out_data_folder+'/'+out_bag_filename)
      print 'Finished running process, sleeping for 5 seconds...'
      time.sleep(5)


if __name__=='__main__':
   if len(sys.argv) >= 3:
      if len(sys.argv) >= 4:
         do_pov = sys.argv[3].lower() != 'nopov'
      else:
         do_pov = True
      doAnnotationLaunch(sys.argv[1], sys.argv[2], do_pov)
   else:
      print 'Please provide the path to the folder containing subject data as a command line parameter'
