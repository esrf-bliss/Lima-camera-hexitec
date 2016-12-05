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
//#include "HexitecReconstruction.h"

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
	int nframes = 600000;

	try {
		m_camera = new Camera("00:11:1c:01:eb:24", "/home/grm84/software/git/Lima/camera/hexitec/sdk/HexitecApi.ini", bufferCount, 500);
//		m_camera = new Camera("169.254.63.243", "/home/grm84/software/git/Lima/camera/hexitec/sdk/HexitecApi.ini", bufferCount, 500);
		m_interface = new Interface(*m_camera);
		m_control = new CtControl(m_interface);

		string type;
		m_camera->getDetectorType(type);
		cout << "Detector type : " << type << endl;
		string model;
		m_camera->getDetectorModel(model);
		std::cout << "Detector model: " << model << std::endl;

//		m_camera->collectOffsetValues();
//		std::cout << "Collected offset values" << std::endl;

//		m_camera->setType(HexitecReconstruction::CSA_NF);
		m_camera->setHighThreshold(8000);
		m_camera->setSaveOpt(Camera::SaveRaw|Camera::SaveProcessed);

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
		saving->setFramesPerFile(nframes, stream);
		saving->setNextNumber(0, stream);
// for processed data stream 1
		stream = 1;
		saving->setStreamActive(stream, true);
		saving->setDirectory("/home/grm84/data/processed", stream);
		saving->setFormat(CtSaving::HDF5, stream);
		saving->setSuffix(".hdf", stream);
		saving->setPrefix("hexitec_", stream);
		saving->setOverwritePolicy(CtSaving::Overwrite, stream);
		saving->setFramesPerFile(nframes, stream);
		saving->setNextNumber(0, stream);
		std::cout << "Saving setup is OK: " << std::endl;

		// do acquisition
		m_control->acquisition()->setAcqNbFrames(nframes);
		m_control->prepareAcq();
		m_control->startAcq();
		while (1) {
			sleep(1);
			m_control->getStatus(status);
//			std::cout << "Status: " << status.AcquisitionStatus << std::endl;
			if (status.AcquisitionStatus != AcqRunning)
				break;
		}
		sleep(2);

//		m_control->ReadImage(data, 9999);

//		uint16_t *bptr = (uint16_t*) data.data();
//		for (auto i = 0; i < 80; i++) {
//			for (auto j = 0; j < 80; j++) {
//				std::cout << *bptr++ << " ";
//			}
//			std::cout << "|" << std::endl;
//		}
	} catch (Exception &e) {
		std::cout << "Exception!!!!!!" << std::endl;
	}
}
