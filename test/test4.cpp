//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2013
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
#include <iostream>
#include <unistd.h>

#include "lima/HwInterface.h"
#include "lima/CtControl.h"
#include "lima/CtAccumulation.h"
#include "lima/CtAcquisition.h"
#include "lima/CtSaving.h"
#include "lima/CtShutter.h"
#include "lima/Constants.h"
#include "lima/Debug.h"

#include "HexitecCamera.h"
#include "HexitecInterface.h"

using namespace std;
using namespace lima;
using namespace lima::Hexitec;

DEB_GLOBAL(DebModTest);

int main() {
	DEB_GLOBAL_FUNCT();

	Camera *m_camera;
	Interface *m_interface;
	CtControl* m_control;
	Data data;
	CtControl::Status status;


	int stream;
	int bufferCount = 1000;
//	int nframes = 953604; // maximum from test.dat
	int nframes = 100000;
//    int nframes = 50000;
//	int nframes = 10000;
//	int nframes = 1000;
//	int nframes = 100;
	int binWidth = 10;
	int speclen = 8000;
	int nbins = (speclen / binWidth);
	int asicPitch = 250;
	int timeout = 30000;
	int nextNumber = 0;
	int loop = 50;
	int framesPerFile = nframes;
	int highThreshold = 8000;
	int lowThreshold = 100;

	try {
#ifdef COMPILE_HEXITEC_DUMMY
		m_camera = new Camera("hexitec-test.dat", "/home/grm84/software/Lima/camera/hexitec/sdk/HexitecApi.ini", bufferCount, timeout, asicPitch);
#else
//		m_camera = new Camera("00:11:1c:01:eb:24", "/home/grm84/software/Lima/camera/hexitec/sdk/HexitecApi.ini", bufferCount, timeout, asicPitch);
		m_camera = new Camera("192.168.0.10", "/home/grm84/software/Lima/camera/hexitec/sdk/HexitecApi.ini", bufferCount, timeout, asicPitch);
#endif
		m_interface = new Interface(*m_camera);
		m_control = new CtControl(m_interface);

		string type;
		m_camera->getDetectorType(type);
		cout << "Detector type : " << type << endl;
		string model;
		m_camera->getDetectorModel(model);
		std::cout << "Detector model: " << model << std::endl;
		double rate;
		m_camera->getFrameRate(rate);
        std::cout << "Frame rate: " << rate << "kHz" << std::endl;

        std::cout << "Collecting offset values" << std::endl;
//		m_camera->collectOffsetValues();
		std::cout << "Collected offset values" << std::endl;

//        m_camera->setFramesPerTrigger(10);
		m_camera->setType(Camera::SORT);
		m_camera->setHighThreshold(highThreshold);
		m_camera->setLowThreshold(lowThreshold);
		m_camera->setBinWidth(binWidth);
		m_camera->setSpecLen(speclen);
		m_camera->setBiasVoltageRefreshInterval(60000);
//		m_camera->setSaveOpt(Camera::SaveNothing);
//        m_camera->setSaveOpt(Camera::SaveRaw);
//		m_camera->setSaveOpt(Camera::SaveRaw | Camera::SaveProcessed);
//		m_camera->setSaveOpt(Camera::SaveRaw | Camera::SaveHistogram);
//        m_camera->setSaveOpt(Camera::SaveRaw | Camera::SaveSummed);
		m_camera->setSaveOpt(Camera::SaveRaw | Camera::SaveProcessed | Camera::SaveHistogram | Camera::SaveSummed);
//      m_camera->setSaveOpt(Camera::SaveRaw | Camera::SaveProcessed | Camera::SaveSummed);

		CtSaving* saving = m_control->saving();
		// for raw data stream 0
		stream = 0;
		saving->setSavingMode(CtSaving::Manual);
		saving->setManagedMode(CtSaving::Hardware);
		saving->setDirectory("/home/grm84/data/raw", stream);
		saving->setFormat(CtSaving::HDF5, stream);
		saving->setSuffix(".hdf", stream);
		saving->setPrefix("hexitec_", stream);
		saving->setOverwritePolicy(CtSaving::Overwrite, stream);
		saving->setFramesPerFile(framesPerFile, stream);
		saving->setNextNumber(nextNumber, stream);
		// for processed data stream 1
		stream = 1;
		saving->setStreamActive(stream, true);
		saving->setDirectory("/home/grm84/data/processed", stream);
		saving->setFormat(CtSaving::HDF5, stream);
		saving->setSuffix(".hdf", stream);
		saving->setPrefix("hexitec_", stream);
		saving->setOverwritePolicy(CtSaving::Overwrite, stream);
		saving->setFramesPerFile(framesPerFile, stream);
		saving->setNextNumber(nextNumber, stream);
		// for histogram data stream 2
		stream = 2;
		saving->setStreamActive(stream, true);
		saving->setDirectory("/home/grm84/data/histogram", stream);
		saving->setFormat(CtSaving::HDF5, stream);
		saving->setSuffix(".hdf", stream);
		saving->setPrefix("hexitec_", stream);
		saving->setOverwritePolicy(CtSaving::Overwrite, stream);
		saving->setFramesPerFile(nbins, stream);
		saving->setNextNumber(nextNumber, stream);
        // for summed data stream 3
        stream = 3;
        saving->setStreamActive(stream, true);
        saving->setDirectory("/home/grm84/data/summed", stream);
        saving->setFormat(CtSaving::HDF5, stream);
        saving->setSuffix(".hdf", stream);
        saving->setPrefix("hexitec_", stream);
        saving->setOverwritePolicy(CtSaving::Overwrite, stream);
        saving->setFramesPerFile(1, stream);
        saving->setNextNumber(nextNumber, stream);
		std::cout << "Saving setup is OK: " << std::endl;
		int success = 0;
		for (auto i = 0; i < loop; i++) {
			// do acquisition
	          m_control->acquisition()->setTriggerMode(IntTrig);
//            m_control->acquisition()->setTriggerMode(ExtGate);
//            m_control->acquisition()->setTriggerMode(ExtTrigSingle);
//            m_control->acquisition()->setTriggerMode(ExtTrigMult);
            m_control->acquisition()->setAcqNbFrames(nframes);
			m_control->prepareAcq();
			m_control->startAcq();
            while (1) {
                sleep(15);
                m_control->getStatus(status);
                std::cout << "Test4 Status: " << status.AcquisitionStatus << std::endl;
                if (status.AcquisitionStatus == AcqReady) {
                    success++;
                    break;
                } else if (status.AcquisitionStatus == AcqFault) {
                    break;
                }
            }
			nextNumber++;
			saving->setNextNumber(nextNumber, 0);
			saving->setNextNumber(nextNumber, 1);
			saving->setNextNumber(nextNumber, 2);
            saving->setNextNumber(nextNumber, 3);
			std::cout << "loop " << i << " complete" << std::endl;
		}
		sleep(2);
		std::cout << "success " << success <<std::endl;
	} catch (Exception &e) {
		std::cout << "Exception!!!!!!" << std::endl;
	}
	delete m_control;
	delete m_interface;
	delete m_camera;
	return 0;
}
