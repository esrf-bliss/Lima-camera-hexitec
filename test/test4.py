############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2017
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################

import PyTango
import numpy
import time

bufferCount = 100
#nframes = 953604; # maximum from test.dat
#nframes = 100000
nframes = 10000
#nframes = 10
binWidth = 10
speclen = 8000
lowThreshold = 100;
highThreshold = 8000
nbins = (speclen / binWidth)
asicPitch = 250
timeout = 15000
nextNumber = 0
loop = 1
dcTimeout = 1000

cam = PyTango.DeviceProxy('hexitec/tango/1')
lima = PyTango.DeviceProxy('limaccd/tango/6')
print "Detector type : ", lima.read_attribute("camera_type").value
print "Detector model: ", lima.read_attribute("camera_model").value

cam.write_attribute("frameTimeout", timeout)

cam.write_attribute("collectDcTimeout", dcTimeout)
#cam.command_inout("collectOffsetValues")
print "Collected offset values"

cam.write_attribute("type", 'CSD');
cam.write_attribute("highThreshold", highThreshold);
cam.write_attribute("lowThreshold", lowThreshold);
cam.write_attribute("binWidth", binWidth);
cam.write_attribute("specLen", speclen);
#cam.write_attribute("saveOpt", (Camera.SaveRaw );
#cam.write_attribute("saveOpt", (Camera.SaveRaw | Camera.SaveSummed);
#cam.write_attribute("saveOpt", (Camera.SaveRaw | Camera.SaveProcessed );
#cam.write_attribute("saveOpt", (Camera.SaveRaw | Camera.SaveHistogram));
cam.write_attribute("saveOpt", 15);

# for raw data stream 0
lima.command_inout("setSavingStream", 0)
lima.write_attribute("saving_stream_active", True);
lima.write_attribute("saving_mode","MANUAL")
lima.write_attribute("saving_managed_mode","HARDWARE")
lima.write_attribute("saving_directory","/home/grm84/data/raw")
lima.write_attribute("saving_format","HDf5")
lima.write_attribute("saving_overwrite_policy","Overwrite")
lima.write_attribute("saving_suffix", ".hdf")
lima.write_attribute("saving_prefix","hexitec_")
lima.write_attribute("saving_frame_per_file", nframes)
lima.write_attribute("saving_next_number", nextNumber)

# for processed data stream 1
lima.command_inout("setSavingStream", 1)
lima.write_attribute("saving_stream_active", True);
lima.write_attribute("saving_directory","/home/grm84/data/processed")
lima.write_attribute("saving_format","HDf5")
lima.write_attribute("saving_overwrite_policy","Overwrite")
lima.write_attribute("saving_suffix", ".hdf")
lima.write_attribute("saving_prefix","hexitec_")
lima.write_attribute("saving_frame_per_file", nframes)
lima.write_attribute("saving_next_number", nextNumber)
		
# for histogram data stream 2
lima.command_inout("setSavingStream", 2)
lima.write_attribute("saving_stream_active", True);
lima.write_attribute("saving_directory","/home/grm84/data/histogram")
lima.write_attribute("saving_format","HDf5")
lima.write_attribute("saving_overwrite_policy","Overwrite")
lima.write_attribute("saving_suffix", ".hdf")
lima.write_attribute("saving_prefix","hexitec_")
lima.write_attribute("saving_frame_per_file", nframes)
lima.write_attribute("saving_next_number", nextNumber)
		
# for summed data stream 3
lima.command_inout("setSavingStream", 3)
lima.write_attribute("saving_stream_active", True);
lima.write_attribute("saving_directory","/home/grm84/data/summed")
lima.write_attribute("saving_format","HDf5")
lima.write_attribute("saving_overwrite_policy","Overwrite")
lima.write_attribute("saving_suffix", ".hdf")
lima.write_attribute("saving_prefix","hexitec_")
lima.write_attribute("saving_frame_per_file", nframes)
lima.write_attribute("saving_next_number", nextNumber)

print "Saving setup is OK: "
success = 0
for i in range(loop):
	# do acquisition
#	lima.write_attribute("acq_trigger_mode", "INTERNAL_TRIGGER")
#	lima.write_attribute("acq_trigger_mode", "EXTERNAL_GATE")
	lima.write_attribute("acq_trigger_mode", "EXTERNAL_TRIGGER")
#	lima.write_attribute("acq_trigger_mode", "EXTERNAL_TRIGGER_MULTI")
	lima.write_attribute("acq_nb_frames", nframes)
	lima.command_inout("prepareAcq")
	lima.command_inout("startAcq")

	while True:		
		time.sleep(15)
		status = lima.read_attribute("acq_status").value
		if status == 'Ready':
			success += 1
			break
		elif status == 'Fault':
			break

	nextNumber += 1
	lima.command_inout("setSavingStream", 0)
	lima.write_attribute("saving_next_number", nextNumber)
	lima.command_inout("setSavingStream", 1)
	lima.write_attribute("saving_next_number", nextNumber)
	lima.command_inout("setSavingStream", 2)
	lima.write_attribute("saving_next_number", nextNumber)
	lima.command_inout("setSavingStream", 3)
	lima.write_attribute("saving_next_number", nextNumber)
	print"loop ",i," complete"
time.sleep(2)
print "success "
