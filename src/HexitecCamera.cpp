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

#include <iostream>
#include <chrono>
#include <unistd.h>

#include "lima/Debug.h"
#include "lima/Constants.h"
#include "lima/Exceptions.h"
#include "lima/CtBuffer.h"
#include "processlib/PoolThreadMgr.h"
#include "processlib/TaskMgr.h"
#include "HexitecCamera.h"
#include "HexitecSavingTask.h"
//#include "HexitecProcessingTask.h"

using namespace lima;
using namespace lima::Hexitec;
using namespace std;

typedef std::chrono::high_resolution_clock Clock;

//-----------------------------------------------------
// AcqThread class
//-----------------------------------------------------
class Camera::AcqThread: public Thread {
DEB_CLASS_NAMESPC(DebModCamera, "Camera", "AcqThread");
public:
	AcqThread(Camera &aCam);
	virtual ~AcqThread();

protected:
	virtual void threadFunction();

private:
	Camera& m_cam;
};

//-----------------------------------------------------
// TimerThread class
//-----------------------------------------------------
class Camera::TimerThread: public Thread {
DEB_CLASS_NAMESPC(DebModCamera, "Camera", "TimerThread");
public:
	TimerThread(Camera &aCam);
	virtual ~TimerThread();

protected:
	virtual void threadFunction();

private:
	Camera& m_cam;
};

//-----------------------------------------------------
// @brief camera constructor
//-----------------------------------------------------
Camera::Camera(std::string ipAddress, std::string configFilename, int bufferCount, int timeout) :
		m_detectorImageType(Bpp16), m_detector_type("Hexitec"), m_detector_model("V1.0.0"), m_maxImageWidth(80), m_maxImageHeight(
				80), m_x_pixelsize(1), m_y_pixelsize(1), m_offset_x(0), m_offset_y(0), m_ipAddress(ipAddress), m_configFilename(
				configFilename), m_bufferCount(bufferCount), m_timeout(timeout), m_status(Camera::Initialising), m_collectDcTimeout(
				100), m_bufferCtrlObj(), m_savingCtrlObj(*this, 2),
//                m_type(HexitecReconstruction::SORT),
		m_asicPitch(250), m_binWidth(10), m_speclen(8000), m_lowThreshold(0), m_highThreshold(10000), m_biasVoltageRefreshInterval(
				10000), m_biasVoltageRefreshTime(5000), m_biasVoltageSettleTime(2000) {

	DEB_CONSTRUCTOR();

//	DebParams::setModuleFlags(DebParams::AllFlags);
//	DebParams::setTypeFlags(DebParams::AllFlags);
//	DebParams::setFormatFlags(DebParams::AllFlags);

	m_hexitec = std::unique_ptr < HexitecAPI::HexitecApi > (new HexitecAPI::HexitecApi(ipAddress, m_timeout));
	initialise();

	// Acquisition Thread
	m_acq_thread = std::unique_ptr < AcqThread > (new AcqThread(*this));
	m_acq_started = false;
	m_acq_thread->start();

// Timer thread for cycling the bias voltage
	m_timer_thread = std::unique_ptr < TimerThread > (new TimerThread(*this));
//	m_timer_wait_flag = true;
	m_timer_thread->start();

	m_status = Camera::Ready;
}

//-----------------------------------------------------
// @brief camera destructor
//-----------------------------------------------------
Camera::~Camera() {
	DEB_DESTRUCTOR();
	setHvBiasOff();
	m_hexitec->closePipeline();
	m_hexitec->closeStream();
}

//----------------------------------------------------------------------------
// initialize detector
//----------------------------------------------------------------------------
void Camera::initialise() {
	DEB_MEMBER_FUNCT();
	int32_t rc;
	uint32_t errorCode;
	std::string errorCodeString;
	std::string errorDescription;

	if (m_hexitec->readConfiguration(m_configFilename) != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to read the configuration file " << DEB_VAR1(m_configFilename);
	}
	m_hexitec->initDevice(errorCode, errorCodeString, errorDescription);
	if (errorCode != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << errorDescription << " " << DEB_VAR1(errorCode);
	}
	std::cout << "Error code :" << errorCode << std::endl;
	std::cout << "Error      :" << errorCodeString << std::endl;
	std::cout << "Description:" << errorDescription << std::endl;
	uint8_t useTermChar = true;
	rc = m_hexitec->openSerialPort(PvDeviceSerialBulk0, (2 << 16), useTermChar, 0x0d);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to open serial port " << DEB_VAR1(rc);
	}
	uint8_t customerId;
	uint8_t projectId;
	uint8_t version;
	uint8_t forceEqualVersion = false;
	rc = m_hexitec->checkFirmware(customerId, projectId, version, forceEqualVersion);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to read firmware version information " << DEB_VAR1(rc);
	}
	DEB_TRACE() << "customerId :" << int(customerId);
	DEB_TRACE() << "projectId  :" << int(projectId);
	DEB_TRACE() << "version    :" << int(version);

	uint8_t width;
	uint8_t height;
	uint32_t collectDcTime;
	rc = m_hexitec->configureDetector(width, height, m_frameTime, collectDcTime);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to configure the detector " << DEB_VAR1(rc);
	}
	DEB_TRACE() << "width         :" << int(width);
	DEB_TRACE() << "height        :" << int(height);
	std::cout << "frameTime     :" << m_frameTime << std::endl;
	DEB_TRACE() << "collectDcTime :" << collectDcTime;

	double humidity;
	double ambientTemperature;
	double asicTemperature;
	double adcTemperature;
	double ntcTemperature;
	rc = m_hexitec->readEnvironmentValues(humidity, ambientTemperature, asicTemperature, adcTemperature, ntcTemperature);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to read environmental values " << DEB_VAR1(rc);
	}
	std::cout << "humidity           :" << humidity << std::endl;
	std::cout << "ambientTemperature :" << ambientTemperature << std::endl;
	std::cout << "asicTemperature    :" << asicTemperature << std::endl;
	std::cout << "adcTemperature     :" << adcTemperature << std::endl;
	std::cout << "ntcTemperature     :" << ntcTemperature << std::endl;
	double v3_3;
	double hvMon;
	double hvOut;
	double v1_2;
	double v1_8;
	double v3;
	double v2_5;
	double v3_3ln;
	double v1_65ln;
	double v1_8ana;
	double v3_8ana;
	double peltierCurrent;

	rc = m_hexitec->readOperatingValues(v3_3, hvMon, hvOut, v1_2, v1_8, v3, v2_5, v3_3ln, v1_65ln, v1_8ana, v3_8ana, peltierCurrent,
			ntcTemperature);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to read operating values" << DEB_VAR1(rc);
	}
	std::cout << "v3_3           :" << v3_3 << std::endl;
	std::cout << "hvMon          :" << hvMon << std::endl;
	std::cout << "hvOut          :" << hvOut << std::endl;
	std::cout << "v1_2           :" << v1_2 << std::endl;
	std::cout << "v1_8           :" << v1_8 << std::endl;
	std::cout << "v3             :" << v3 << std::endl;
	std::cout << "v2_5           :" << v2_5 << std::endl;
	std::cout << "v3_3ln         :" << v3_3ln << std::endl;
	std::cout << "v1_65ln        :" << v1_65ln << std::endl;
	std::cout << "v1_8ana        :" << v1_8ana << std::endl;
	std::cout << "v3_8ana        :" << v3_8ana << std::endl;
	std::cout << "peltierCurrent :" << peltierCurrent << std::endl;
	std::cout << "ntcTemperature :" << ntcTemperature << std::endl;

	rc = m_hexitec->setFrameFormatControl("Mono16", m_maxImageWidth, m_maxImageHeight, m_offset_x, m_offset_y, "One", "Off"); //IPEngineTestPattern");
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to set frame format control " << DEB_VAR1(rc);
	}

	//openStream needs to be called before createPipeline
	rc = m_hexitec->openStream();
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to open stream" << DEB_VAR1(rc);
	}

	std::cout << "setting buffer count to " << m_bufferCount << std::endl;
	rc = m_hexitec->createPipelineOnly(m_bufferCount);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to create pipeline" << DEB_VAR1(rc);
	}
}

//-----------------------------------------------------------------------------
// @brief Prepare the detector for acquisition
//-----------------------------------------------------------------------------
void Camera::prepareAcq() {
	DEB_MEMBER_FUNCT();
	m_image_number = 0;
	setHvBiasOn();
	// wait asynchronously for the HV Bias to settle
	// check the result before start acquisition in Acq thread.
	m_future_result = std::async(std::launch::async, [=] {std::chrono::milliseconds(m_biasVoltageRefreshTime);});
//	std::this_thread::sleep_for(std::chrono::milliseconds(m_biasVoltageRefreshTime));
	m_bufferCtrlObj.setNbBuffers(m_bufferCount);
}

//-----------------------------------------------------------------------------
// @brief start the acquisition
//-----------------------------------------------------------------------------
void Camera::startAcq() {
	DEB_MEMBER_FUNCT();
	AutoMutex lock(m_cond.mutex());
	m_acq_started = true;
//	m_acq_paused = false;
//	m_timer_wait_flag = false;
	m_saved_frame_nb = 0;
	m_cond.broadcast();
}

//-----------------------------------------------------------------------------
// @brief stop the acquisition
//-----------------------------------------------------------------------------
void Camera::stopAcq() {
	DEB_MEMBER_FUNCT();
	AutoMutex lock(m_cond.mutex());
	if (m_acq_started)
		m_acq_started = false;
}

//-----------------------------------------------------------------------------
// @brief return the detector Max image size
//-----------------------------------------------------------------------------
void Camera::getDetectorMaxImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
	size = Size(m_maxImageWidth, m_maxImageHeight);
}

//-----------------------------------------------------------------------------
// @brief return the detector image size
//-----------------------------------------------------------------------------
void Camera::getDetectorImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
	getDetectorMaxImageSize(size);
}

//-----------------------------------------------------------------------------
// @brief Get the image type
//-----------------------------------------------------------------------------
void Camera::getImageType(ImageType& type) {
	DEB_MEMBER_FUNCT();
	type = m_detectorImageType;
}

//-----------------------------------------------------------------------------
// @brief set Image type
//-----------------------------------------------------------------------------
void Camera::setImageType(ImageType type) {
	DEB_MEMBER_FUNCT();
	if (type != Bpp16)
		THROW_HW_ERROR(NotSupported) << DEB_VAR1(type) << " Only Bpp16 supported";
}

//-----------------------------------------------------------------------------
// @brief return the detector type
//-----------------------------------------------------------------------------
void Camera::getDetectorType(string& type) {
	DEB_MEMBER_FUNCT();
	type = m_detector_type;
}

//-----------------------------------------------------------------------------
// @brief return the detector model
//-----------------------------------------------------------------------------
void Camera::getDetectorModel(string& model) {
	DEB_MEMBER_FUNCT();
	model = m_detector_model;
}

//-----------------------------------------------------------------------------
// @brief Checks trigger mode
//-----------------------------------------------------------------------------
bool Camera::checkTrigMode(TrigMode trig_mode) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(trig_mode);
	switch (trig_mode) {
	case IntTrig:
		return true;
	case IntTrigMult:
	case ExtTrigSingle:
	case ExtTrigMult:
	case ExtGate:
	default:
		return false;
	}
}

//-----------------------------------------------------------------------------
// @brief Set the new trigger mode
//-----------------------------------------------------------------------------
void Camera::setTrigMode(TrigMode trig_mode) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(trig_mode);

	switch (trig_mode) {
	case IntTrig:
		break;
	case IntTrigMult:
	case ExtTrigSingle:
	case ExtTrigMult:
	case ExtGate:
	default:
		THROW_HW_ERROR(NotSupported) << DEB_VAR1(trig_mode);
	}
//	m_hexitec.setTrigger();
	m_trig_mode = trig_mode;
}

//-----------------------------------------------------------------------------
// @brief Get the current trigger mode
//-----------------------------------------------------------------------------
void Camera::getTrigMode(TrigMode& mode) {
	DEB_MEMBER_FUNCT();
	mode = m_trig_mode;
	DEB_RETURN() << DEB_VAR1(mode);
}

//-----------------------------------------------------------------------------
/// @brief Set the new exposure time
//-----------------------------------------------------------------------------
void Camera::setExpTime(double exp_time) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(exp_time);

	std::cout << "setExpTime " << DEB_VAR3(m_exp_time, m_nb_frames, m_frameTime) << std::endl;
	m_exp_time = exp_time;
	if (m_nb_frames == 0) {
		m_nb_frames = int(m_exp_time / m_frameTime);
	}
	std::cout << "setExpTime " << DEB_VAR3(m_exp_time, m_nb_frames, m_frameTime) << std::endl;
}

//-----------------------------------------------------------------------------
// @brief Get the current exposure time
//-----------------------------------------------------------------------------
void Camera::getExpTime(double& exp_time) {
	DEB_MEMBER_FUNCT();
	exp_time = m_exp_time;
	DEB_RETURN() << DEB_VAR1(exp_time);
}

//-----------------------------------------------------------------------------
// @brief Set the new latency time between images
//-----------------------------------------------------------------------------
void Camera::setLatTime(double lat_time) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(lat_time);
	m_latency_time = lat_time;
}

//-----------------------------------------------------------------------------
// @brief Get the current latency time
//-----------------------------------------------------------------------------
void Camera::getLatTime(double& lat_time) {
	DEB_MEMBER_FUNCT();
	lat_time = m_latency_time;
	DEB_RETURN() << DEB_VAR1(lat_time);
}

//-----------------------------------------------------------------------------
// @brief Get the exposure time range
//-----------------------------------------------------------------------------
void Camera::getExposureTimeRange(double& min_expo, double& max_expo) const {
	DEB_MEMBER_FUNCT();

//	min_expo = min_val.data.double_val;
//	max_expo = max_val.data.double_val;
//	DEB_RETURN() << DEB_VAR2(min_expo, max_expo);
}

//-----------------------------------------------------------------------------
// @brief Get the latency time range
//-----------------------------------------------------------------------------
void Camera::getLatTimeRange(double& min_lat, double& max_lat) const {
	DEB_MEMBER_FUNCT();
	// --- no info on min latency
}

//-----------------------------------------------------------------------------
// @brief Set the number of frames to be taken
//-----------------------------------------------------------------------------
void Camera::setNbFrames(int nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(nb_frames);
	m_nb_frames = nb_frames;
	m_exp_time = m_frameTime * m_nb_frames;
	std::cout << "setNbFrames " << DEB_VAR3(m_exp_time, m_nb_frames, m_frameTime) << std::endl;
}

//-----------------------------------------------------------------------------
// @brief Get the number of frames to be taken
//-----------------------------------------------------------------------------
void Camera::getNbFrames(int& nb_frames) {
	DEB_MEMBER_FUNCT();
	nb_frames = m_nb_frames;
	DEB_RETURN() << DEB_VAR1(nb_frames);
}

//-----------------------------------------------------------------------------
// @brief Get the number of acquired frames
//-----------------------------------------------------------------------------
void Camera::getNbHwAcquiredFrames(int &nb_acq_frames) {
	DEB_MEMBER_FUNCT();
	nb_acq_frames = m_image_number;
}

//-----------------------------------------------------------------------------
// @brief Get the camera status
//-----------------------------------------------------------------------------
Camera::Status Camera::getStatus() {
	DEB_MEMBER_FUNCT();
	return m_status;
}

//-----------------------------------------------------------------------------
// @brief check if binning is available
//-----------------------------------------------------------------------------
bool Camera::isBinningAvailable() {
	DEB_MEMBER_FUNCT();
	return false;
}

//-----------------------------------------------------------------------------
// @brief return the detector pixel size
//-----------------------------------------------------------------------------
void Camera::getPixelSize(double& sizex, double& sizey) {
	DEB_MEMBER_FUNCT();
	sizex = m_x_pixelsize;
	sizey = m_y_pixelsize;
	DEB_RETURN() << DEB_VAR2(sizex, sizey);
}

//-----------------------------------------------------------------------------
// @brief reset the camera
//-----------------------------------------------------------------------------
void Camera::reset() {
	DEB_MEMBER_FUNCT();
	return;
}

HwBufferCtrlObj* Camera::getBufferCtrlObj() {
	return &m_bufferCtrlObj;
}

SavingCtrlObj* Camera::getSavingCtrlObj() {
	return &m_savingCtrlObj;
}

//-----------------------------------------------------
// acquisition thread
//-----------------------------------------------------
Camera::AcqThread::AcqThread(Camera &cam) :
		m_cam(cam) {
	pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

Camera::AcqThread::~AcqThread() {
	AutoMutex lock(m_cam.m_cond.mutex());
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	lock.unlock();
	join();
}

void Camera::AcqThread::threadFunction() {
	DEB_MEMBER_FUNCT();
	int32_t rc;
	uint8_t* bptr;
	AutoMutex lock(m_cam.m_cond.mutex());
	StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj.getBuffer();
	buffer_mgr.setStartTimestamp(Timestamp::now());
	PoolThreadMgr::get().setNumberOfThread(8);

	while (true) {
		while (!m_cam.m_acq_started && !m_cam.m_quit) {
			std::cout << "Acq thread Waiting " << std::endl;
			m_cam.m_thread_running = false;
			m_cam.m_cond.wait();
		}
		auto t1 = Clock::now();
		if (m_cam.m_quit)
			return;

		m_cam.m_thread_running = true;
		m_cam.m_status = Camera::Exposure;
		lock.unlock();

		bool continue_acq = true;
		try {
			m_cam.m_future_result.get();
			std::cout << "Starting acquisition" << std::endl;
			rc = m_cam.m_hexitec->startAcquisition();
			if (rc != HexitecAPI::NO_ERROR) {
				DEB_ERROR() << "Failed to start acquisition " << DEB_VAR1(rc);
				m_cam.setHvBiasOff();
//				m_cam.m_timer_wait_flag = true;
				bool continue_acq = false;
			}
		} catch (Exception& e) {
			DEB_ERROR() << "Failed to start acquisition " << DEB_VAR1(rc);
			bool continue_acq = false;
		}
		int nbf;
		buffer_mgr.getNbBuffers(nbf);
		std::cout << "Nb buffer " << nbf << std::endl;

		int saveOpt;
		m_cam.getSaveOpt(saveOpt);
		std::vector<int> dimensions;
		dimensions.push_back(80);
		dimensions.push_back(80);
		HexitecSavingTask* savingTask = new HexitecSavingTask(*m_cam.getSavingCtrlObj(), 0);
//		HexitecProcessingTask* processingTask = new HexitecProcessingTask(*m_cam.getSavingCtrlObj(), 1,
//				m_cam.m_processType, m_cam.m_asicPitch,
//				m_cam.m_binWidth, m_cam.m_speclen, m_cam.m_lowThreshold, m_cam.m_highThreshold);

		while (continue_acq && m_cam.m_acq_started && (!m_cam.m_nb_frames || m_cam.m_image_number < m_cam.m_nb_frames)) {

			bptr = (uint8_t*) buffer_mgr.getFrameBufferPtr(m_cam.m_image_number);
//			std::cout << "Retrieving image# " << m_cam.m_image_number << " into buffer address " << (void*) bptr << std::endl;
			rc = m_cam.m_hexitec->retrieveBuffer(bptr, m_cam.m_timeout);
			if (rc != HexitecAPI::NO_ERROR) {
				DEB_ERROR() << m_cam.m_hexitec->getErrorDescription() << " " << DEB_VAR1(rc);
				break;
			}
			if (m_cam.m_status != Camera::Paused) {
//				std::cout << "saving frame " << m_cam.m_image_number << std::endl;
				int taskNb = 0;

				Data srcData;
				srcData.type = Data::UINT16;
				srcData.dimensions = dimensions;
				srcData.frameNumber = m_cam.m_image_number;
				Buffer *fbuf = new Buffer();
				fbuf->data = bptr;
				srcData.setBuffer(fbuf);
				if (saveOpt & Camera::SaveRaw) {
//					std::cout << "Saving raw srcData" << (void*) &srcData << std::endl;
					TaskMgr *taskMgr = new TaskMgr();
					taskMgr->setLinkTask(0, savingTask);
					taskMgr->setInputData(srcData);
///					std::cout << "Adding task to pool " << (void*) taskMgr << std::endl;
					PoolThreadMgr::get().addProcess(taskMgr);
				}
//				if (saveOpt & Camera::SaveProcessed || saveOpt & Camera::SaveHistogram) {
////					std::cout << "Processinging raw srcData" << (void*) &srcData << std::endl;
//					TaskMgr *taskMgr = new TaskMgr();
//					taskMgr->setLinkTask(1, processingTask);
//					taskMgr->setInputData(srcData);
/////					std::cout << "Adding task to pool " << (void*) taskMgr << std::endl;
//					PoolThreadMgr::get().addProcess(taskMgr);
//				}
//				std::cout << "Image# " << m_cam.m_image_number << " acquired" << std::endl;
				m_cam.m_image_number++;
			}
		}
		m_cam.m_timer_wait_flag = true;
		std::cout << m_cam.m_image_number << " images acquired" << std::endl;
		auto t2 = Clock::now();
		std::cout << "Delta t2-t1: " << std::chrono::duration_cast < std::chrono::nanoseconds
				> (t2 - t1).count() << " nanoseconds" << std::endl;

		std::cout << "Stop acquisition" << std::endl;
		auto rc2 = m_cam.m_hexitec->stopAcquisition();
		if (rc2 != HexitecAPI::NO_ERROR) {
			DEB_ERROR() << "Failed to stop acquisition " << DEB_VAR1(rc);
		}
		std::cout << "Setting bias off" << std::endl;
		m_cam.setHvBiasOff();
//		lock.lock();
// ToDo	Wait to check for saving complete
//		while (m_cam.m_saved_frame_nb != m_cam.m_image_number) {
//			std::cout << "Waiting for save to complete" << std::endl;
//			std::this_thread::sleep_for(std::chrono::milliseconds(100));
//		}
		std::cout << "Set status to ready" << std::endl;
		if (rc == HexitecAPI::NO_ERROR && rc2 == HexitecAPI::NO_ERROR) {
			m_cam.m_status = Camera::Ready;
		} else {
			m_cam.m_status = Camera::Fault;
		}
		lock.lock();
		m_cam.m_acq_started = false;
	}
}

//-----------------------------------------------------
// timer thread
//-----------------------------------------------------
Camera::TimerThread::TimerThread(Camera& cam) :
		m_cam(cam) {
	pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

Camera::TimerThread::~TimerThread() {
	AutoMutex lock(m_cam.m_cond.mutex());
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	lock.unlock();
	join();
}

void Camera::TimerThread::threadFunction() {
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cam.m_cond.mutex());

	while (!m_cam.m_quit) {
//		while (m_cam.m_timer_wait_flag && !m_cam.m_quit) {
		while (!m_cam.m_acq_started && !m_cam.m_quit) {
			std::cout << "Timer thread waiting" << std::endl;
			m_cam.m_cond.wait();
		}
		std::cout << "Timer thread Running" << std::endl;
		if (m_cam.m_quit)
			return;
		aLock.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(m_cam.m_biasVoltageRefreshInterval));
		aLock.lock();
		m_cam.m_status = Camera::Paused;
		aLock.unlock();
		if (m_cam.m_acq_started) {
//			auto rc2 = m_cam.m_hexitec->stopAcquisition();
			std::cout << "Paused at frame " << m_cam.m_image_number << " -------------------" << std::endl;
			m_cam.setHvBiasOff();
			std::this_thread::sleep_for(std::chrono::milliseconds(m_cam.m_biasVoltageRefreshTime));
			m_cam.setHvBiasOn();
			std::this_thread::sleep_for(std::chrono::milliseconds(m_cam.m_biasVoltageSettleTime));
			std::cout << "Acq Restarted --------------------------" << std::endl;
//			auto rc = m_cam.m_hexitec->startAcquisition();
		}
		aLock.lock();
		if (m_cam.m_acq_started) {
			m_cam.m_status = Camera::Exposure;
//			m_cam.m_cond.broadcast();
		}
	}
}

//-----------------------------------------------------
// Hexitec specific stuff
//-----------------------------------------------------

//-----------------------------------------------------------------------------
// @brief get environmental values
//-----------------------------------------------------------------------------
void Camera::getEnvironmentalValues(Environment& env) {
	DEB_MEMBER_FUNCT();
	auto rc = m_hexitec->readEnvironmentValues(env.humidity, env.ambientTemperature, env.asicTemperature, env.adcTemperature,
			env.ntcTemperature);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to read environmental values " << DEB_VAR1(rc);
	}
}

//-----------------------------------------------------------------------------
// @brief get operating values
//-----------------------------------------------------------------------------
void Camera::getOperatingValues(OperatingValues& opval) {
	DEB_MEMBER_FUNCT();
	auto rc = m_hexitec->readOperatingValues(opval.v3_3, opval.hvMon, opval.hvOut, opval.v1_2, opval.v1_8, opval.v3, opval.v2_5,
			opval.v3_3ln, opval.v1_65ln, opval.v1_8ana, opval.v3_8ana, opval.peltierCurrent, opval.ntcTemperature);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to read operating values " << DEB_VAR1(rc);
	}
}

/**
 * Set dark current collection timeout
 * @param[in] timeout time in milliseconds
 */
void Camera::setCollectDcTimeout(int timeout) {
	m_collectDcTimeout = timeout;
}

void Camera::getCollectDcTimeout(int& timeout) {
	timeout = m_collectDcTimeout;
}

void Camera::collectOffsetValues() {
	DEB_MEMBER_FUNCT();
	auto rc = m_hexitec->collectOffsetValues(m_collectDcTimeout);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to collect offset values! " << DEB_VAR1(rc);
	}
}

//void Camera::setType(HexitecReconstruction::Type type) {
//    m_type = type;
//}

void Camera::setBinWidth(int binWidth) {
	m_binWidth = binWidth;
}

void Camera::getBinWidth(int& binWidth) {
	binWidth = m_binWidth;
}

void Camera::setSpeclen(int speclen) {
	m_speclen = speclen;
}

void Camera::getSpecLen(int& speclen) {
	speclen = m_speclen;
}

void Camera::setLowThreshold(int threshold) {
	m_lowThreshold = threshold;
}

void Camera::getLowThreshold(int& threshold) {
	threshold = m_lowThreshold;
}

void Camera::setHighThreshold(int threshold) {
	m_highThreshold = threshold;
}

void Camera::getHighThreshold(int& threshold) {
	threshold = m_highThreshold;
}

void Camera::setHvBiasOn() {
	DEB_MEMBER_FUNCT();
	auto rc = m_hexitec->setHvBiasOn(true);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to set HV Bias on " << DEB_VAR1(rc);
	}
}
void Camera::setHvBiasOff() {
	DEB_MEMBER_FUNCT();
	auto rc = m_hexitec->setHvBiasOn(false);
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to turn HV Bias off" << DEB_VAR1(rc);
	}
}

void Camera::getFrameRate(double& rate) {
	rate = 0.001 / m_frameTime;
}

void Camera::setSaveOpt(int saveOpt) {
	m_saveOpt = saveOpt;
}

void Camera::getSaveOpt(int& saveOpt) {
	saveOpt = m_saveOpt;
}

