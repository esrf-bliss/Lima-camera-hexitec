//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2016
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

#include "HexitecInterface.h"
#include "HexitecCamera.h"
#include "HexitecDetInfoCtrlObj.h"
#include "HexitecSyncCtrlObj.h"
#include "HexitecSavingCtrlObj.h"

using namespace lima;
using namespace lima::Hexitec;
using namespace std;


//-----------------------------------------------------
// @brief constructor
//-----------------------------------------------------
Interface::Interface(Camera& cam) : m_cam(cam) {
	DEB_CONSTRUCTOR();
	m_det_info = new DetInfoCtrlObj(cam);
	m_sync = new SyncCtrlObj(cam);

	m_cap_list.push_back(HwCap(m_det_info));
	m_cap_list.push_back(HwCap(m_sync));

	HwBufferCtrlObj *buffer = m_cam.getBufferCtrlObj();
	m_cap_list.push_back(HwCap(buffer));

	m_saving = m_cam.getSavingCtrlObj();
	HwSavingCtrlObj *saving = m_saving;
	m_cap_list.push_back(saving);
}

//-----------------------------------------------------
// @brief destructor
//-----------------------------------------------------
Interface::~Interface() {
	DEB_DESTRUCTOR();
	delete m_det_info;
	delete m_sync;
}

//-----------------------------------------------------
// @brief return the capability list
//-----------------------------------------------------
void Interface::getCapList(HwInterface::CapList &cap_list) const {
	DEB_MEMBER_FUNCT();
	cap_list = m_cap_list;
}

//-----------------------------------------------------
// @brief reset the interface, stop the acquisition
//-----------------------------------------------------
void Interface::reset(ResetLevel reset_level) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(reset_level);
	stopAcq();
}

//-----------------------------------------------------
// @brief prepare for acquisition
//-----------------------------------------------------
void Interface::prepareAcq() {
	DEB_MEMBER_FUNCT();
	m_cam.prepareAcq();
}

//-----------------------------------------------------
// @brief start camera acquisition
//-----------------------------------------------------
void Interface::startAcq() {
	DEB_MEMBER_FUNCT();
	m_cam.startAcq();
}

//-----------------------------------------------------
// @brief stop the camera acquisition
//-----------------------------------------------------
void Interface::stopAcq() {
	DEB_MEMBER_FUNCT();
	m_cam.stopAcq();
}

//-----------------------------------------------------
// @brief return the status of detector/acquisition
//-----------------------------------------------------
void Interface::getStatus(StatusType& status) {
	DEB_MEMBER_FUNCT();
	Camera::Status cam_status = Camera::Ready;
	cam_status = m_cam.getStatus();
	DEB_TRACE() << "Status is: "<< static_cast<int>(cam_status);
	switch (cam_status) {
	case Camera::Ready:
		status.det = DetIdle;
		status.acq = AcqReady;
		break;
	case Camera::Initialising:
		status.det = DetIdle;
		status.acq = AcqConfig;
		break;
	case Camera::Exposure:
		status.det = DetExposure;
		status.acq = AcqRunning;
		break;
	case Camera::Readout:
		status.det = DetReadout;
		status.acq = AcqRunning;
		break;
	case Camera::Paused:
		status.det = DetLatency;
		status.acq = AcqRunning;
		break;
	case Camera::Fault:
		status.det = DetFault;
		status.acq = AcqFault;
		break;
	}
	status.det_mask = DetExposure | DetReadout | DetLatency;
	DEB_TRACE() << DEB_VAR1(status);
}

//-----------------------------------------------------
// @brief return the hw number of acquired frames
//-----------------------------------------------------
int Interface::getNbHwAcquiredFrames() {
	DEB_MEMBER_FUNCT();
	int acq_frames;
	m_cam.getNbHwAcquiredFrames(acq_frames);
	return acq_frames;
}
