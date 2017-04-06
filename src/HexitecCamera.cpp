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
#include <cfloat>

#include "lima/Debug.h"
#include "lima/Constants.h"
#include "lima/Exceptions.h"
#include "lima/CtBuffer.h"
#include "processlib/PoolThreadMgr.h"
#include "processlib/TaskMgr.h"
#include "processlib/TaskEventCallback.h"
#include "HexitecCamera.h"
#include "HexitecSavingTask.h"
#include "HexitecProcessingTask.h"

using namespace lima;
using namespace lima::Hexitec;
using namespace std;

typedef std::chrono::high_resolution_clock Clock;

class Camera::TaskEventCb: public TaskEventCallback {
DEB_CLASS_NAMESPC(DebModCamera, "Camera", "EventCb");
public:
	TaskEventCb(Camera& cam);
	virtual ~TaskEventCb();
	void finished(Data& d);
private:
	Camera& m_cam;
};

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
	TaskEventCb* m_eventCb;
	HexitecSavingTask* m_savingTask0;
	HexitecSavingTask* m_savingTask1;
	HexitecSavingTask* m_savingTask2;
	Data m_lastFrame;
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
Camera::Camera(const std::string& ipAddress, const std::string& configFilename, int bufferCount, int timeout, int asicPitch) :
		m_ipAddress(ipAddress), m_configFilename(configFilename), m_bufferCount(bufferCount), m_timeout(timeout),
		m_asicPitch(asicPitch),
		m_detectorImageType(Bpp16), m_detector_type("Hexitec"), m_detector_model("V1.0.0"), m_maxImageWidth(80),
		m_maxImageHeight(80), m_x_pixelsize(1), m_y_pixelsize(1), m_offset_x(0), m_offset_y(0),
		m_collectDcTimeout(100), m_acq_started(false), m_quit(false), m_processType(ProcessType::CSA),
		m_saveOpt(Camera::SaveRaw), m_binWidth(10), m_speclen(8000), m_lowThreshold(0), m_highThreshold(10000),
		m_biasVoltageRefreshInterval(10000), m_biasVoltageRefreshTime(5000), m_biasVoltageSettleTime(2000) {

	DEB_CONSTRUCTOR();

//	DebParams::setModuleFlags(DebParams::AllFlags);
//	DebParams::setTypeFlags(DebParams::AllFlags);
//	DebParams::setFormatFlags(DebParams::AllFlags);

	m_savingCtrlObj = new SavingCtrlObj(*this, 3);
	m_bufferCtrlObj = new SoftBufferCtrlObj();

	setStatus(Camera::Initialising);
	m_hexitec = std::unique_ptr < HexitecAPI::HexitecApi > (new HexitecAPI::HexitecApi(ipAddress, m_timeout));
	initialise();

	// Acquisition Thread
	m_acq_thread = std::unique_ptr < AcqThread > (new AcqThread(*this));
	m_acq_started = false;
	m_acq_thread->start();

	// Timer thread for cycling the bias voltage
	m_timer_thread = std::unique_ptr < TimerThread > (new TimerThread(*this));
	m_timer_thread->start();

	setStatus(Camera::Ready);
	DEB_TRACE() << "Camera constructor complete";
}

//-----------------------------------------------------
// @brief camera destructor
//-----------------------------------------------------
Camera::~Camera() {
	DEB_DESTRUCTOR();
	setHvBiasOff();
	m_hexitec->closePipeline();
	m_hexitec->closeStream();
	PoolThreadMgr::get().quit();
	delete m_bufferCtrlObj;
	delete m_savingCtrlObj;
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
		DEB_TRACE() << "Error      :" << errorCodeString;
		DEB_TRACE() << "Description:" << errorDescription;
		THROW_HW_ERROR(Error) << errorDescription << " " << DEB_VAR1(errorCode);
	}
	DEB_TRACE() << "Error code :" << errorCode;
	uint8_t useTermChar = true;
	rc = m_hexitec->openSerialPortBulk0((2 << 16), useTermChar, 0x0d);
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
	DEB_TRACE() << "frameTime     :" << m_frameTime;
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
	DEB_TRACE() << "humidity           :" << humidity;
	DEB_TRACE() << "ambientTemperature :" << ambientTemperature;
	DEB_TRACE() << "asicTemperature    :" << asicTemperature;
	DEB_TRACE() << "adcTemperature     :" << adcTemperature;
	DEB_TRACE() << "ntcTemperature     :" << ntcTemperature;
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
	DEB_TRACE() << "v3_3           :" << v3_3;
	DEB_TRACE() << "hvMon          :" << hvMon;
	DEB_TRACE() << "hvOut          :" << hvOut;
	DEB_TRACE() << "v1_2           :" << v1_2;
	DEB_TRACE() << "v1_8           :" << v1_8;
	DEB_TRACE() << "v3             :" << v3;
	DEB_TRACE() << "v2_5           :" << v2_5;
	DEB_TRACE() << "v3_3ln         :" << v3_3ln;
	DEB_TRACE() << "v1_65ln        :" << v1_65ln;
	DEB_TRACE() << "v1_8ana        :" << v1_8ana;
	DEB_TRACE() << "v3_8ana        :" << v3_8ana;
	DEB_TRACE() << "peltierCurrent :" << peltierCurrent;
	DEB_TRACE() << "ntcTemperature :" << ntcTemperature;

	rc = m_hexitec->setFrameFormatControl("Mono16", m_maxImageWidth, m_maxImageHeight, m_offset_x, m_offset_y, "One", "Off"); //IPEngineTestPattern");
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to set frame format control " << DEB_VAR1(rc);
	}

	//openStream needs to be called before createPipeline
	rc = m_hexitec->openStream();
	if (rc != HexitecAPI::NO_ERROR) {
		THROW_HW_ERROR(Error) << "Failed to open stream" << DEB_VAR1(rc);
	}

	DEB_TRACE() << "setting buffer count to " << m_bufferCount;
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

	Size image_size;
    ImageType image_type;

    getDetectorMaxImageSize(image_size);
    getImageType(image_type);

    FrameDim frame_dim(image_size, image_type);
    m_bufferCtrlObj->setFrameDim(frame_dim);
    m_bufferCtrlObj->setNbBuffers(m_bufferCount);


}

//-----------------------------------------------------------------------------
// @brief start the acquisition
//-----------------------------------------------------------------------------
void Camera::startAcq() {
	DEB_MEMBER_FUNCT();
	AutoMutex lock(m_cond.mutex());
	m_acq_started = true;
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
	case ExtTrigSingle:
	case ExtGate:
		return true;
	case IntTrigMult:
	case ExtTrigMult:
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
	case ExtTrigSingle:
	case ExtGate:
		break;
	case IntTrigMult:
	case ExtTrigMult:
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

	DEB_TRACE() << "setExpTime " << DEB_VAR3(m_exp_time, m_nb_frames, m_frameTime);
	m_exp_time = exp_time;
	if (m_nb_frames == 0) {
		m_nb_frames = int(m_exp_time / m_frameTime);
	}
	DEB_TRACE() << "setExpTime " << DEB_VAR3(m_exp_time, m_nb_frames, m_frameTime);
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
	// --- no info on min/max exposure
	min_expo = 0.0;
	max_expo = DBL_MAX;
}

//-----------------------------------------------------------------------------
// @brief Get the latency time range
//-----------------------------------------------------------------------------
void Camera::getLatTimeRange(double& min_lat, double& max_lat) const {
	DEB_MEMBER_FUNCT();
	// --- no info on min/max latency
	min_lat = 0.0;
	max_lat = DBL_MAX;
}

//-----------------------------------------------------------------------------
// @brief Set the number of frames to be taken
//-----------------------------------------------------------------------------
void Camera::setNbFrames(int nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(nb_frames);
	m_nb_frames = nb_frames;
	m_exp_time = m_frameTime * m_nb_frames;
	DEB_TRACE() << "setNbFrames " << DEB_VAR3(m_exp_time, m_nb_frames, m_frameTime);
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
	int status = m_status;
	return static_cast<Camera::Status>(status);
}

void Camera::setStatus(Camera::Status status) {
	AutoMutex lock(m_cond.mutex());
	m_status = static_cast<int>(status);
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
	return m_bufferCtrlObj;
}

SavingCtrlObj* Camera::getSavingCtrlObj() {
	return m_savingCtrlObj;
}

//-----------------------------------------------------
// acquisition thread
//-----------------------------------------------------
Camera::AcqThread::AcqThread(Camera &cam) : m_cam(cam) {
	m_eventCb = new TaskEventCb(cam);
	pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

Camera::AcqThread::~AcqThread() {
	DEB_DESTRUCTOR();
	AutoMutex lock(m_cam.m_cond.mutex());
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	lock.unlock();
	delete m_eventCb;
	DEB_TRACE()  << "Waiting for the acquisition thread to be done (joining the main thread).";
	join();
}

void Camera::AcqThread::threadFunction() {
	DEB_MEMBER_FUNCT();
	int32_t rc;
	uint16_t* bptr;
	StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj->getBuffer();
	buffer_mgr.setStartTimestamp(Timestamp::now());
	PoolThreadMgr::get().setNumberOfThread(12);
	m_savingTask0 = new HexitecSavingTask(*m_cam.getSavingCtrlObj(), 0);
	m_savingTask1 = new HexitecSavingTask(*m_cam.getSavingCtrlObj(), 1);
	m_savingTask2 = new HexitecSavingTask(*m_cam.getSavingCtrlObj(), 2);

	while (true) {
		while (!m_cam.m_acq_started && !m_cam.m_quit) {
			DEB_TRACE() << "AcqThread Waiting ";
			m_cam.m_thread_running = false;
			AutoMutex lock(m_cam.m_cond.mutex());
			m_cam.m_cond.wait();
		}
		auto t1 = Clock::now();
		if (m_cam.m_quit) {
			return;
		}

		m_cam.m_finished_saving = false;
		m_cam.m_thread_running = true;
		m_cam.setStatus(Camera::Exposure);

		bool continue_acq = true;
		try {
			m_cam.m_future_result.get();
			DEB_ALWAYS() << "Starting acquisition";
			rc = m_cam.m_hexitec->startAcq();
			if (rc != HexitecAPI::NO_ERROR) {
				DEB_ERROR() << "Failed to start acquisition " << DEB_VAR1(rc);
				m_cam.setHvBiasOff();
				bool continue_acq = false;
			}
		} catch (Exception& e) {
			DEB_ERROR() << "Failed to start acquisition " << DEB_VAR1(rc);
			bool continue_acq = false;
		}
		int nbf;
		m_cam.m_bufferCtrlObj->getNbBuffers(nbf);
		DEB_TRACE() << DEB_VAR1(nbf);

		int saveOpt;
		m_cam.getSaveOpt(saveOpt);
		Size size;
		m_cam.getDetectorImageSize(size);
		int height = size.getHeight();
		int width = size.getWidth();
		std::vector<int> dimensions;
		dimensions.push_back(width);
		dimensions.push_back(height);

		m_lastFrame.dimensions = dimensions;
		m_lastFrame.type = Data::UINT16;
		Buffer *newFrameBuffer = new Buffer(width*height*sizeof(uint16_t));
		m_lastFrame.setBuffer(newFrameBuffer);
		newFrameBuffer->unref();
		memset(m_lastFrame.data(), 0, width*height*sizeof(uint16_t));

		HexitecProcessingTask* processingTask;
		if (saveOpt & Camera::SaveProcessed || saveOpt & Camera::SaveHistogram) {
			processingTask = new HexitecProcessingTask(m_savingTask1,
				m_cam.m_processType, m_cam.m_asicPitch, m_cam.m_binWidth, m_cam.m_speclen,
				m_cam.m_lowThreshold, m_cam.m_highThreshold);
		}
		while (continue_acq && m_cam.m_acq_started && (!m_cam.m_nb_frames || m_cam.m_image_number < m_cam.m_nb_frames)) {

			bptr = (uint16_t*) buffer_mgr.getFrameBufferPtr(m_cam.m_image_number);
			DEB_TRACE() << "Retrieving image# " << m_cam.m_image_number << " in bptr " << (void*) bptr;
			rc = m_cam.m_hexitec->retrieveBuffer((uint8_t*)bptr, m_cam.m_timeout);
			if (rc == HexitecAPI::NO_ERROR) {
				if (m_cam.getStatus() == Camera::Exposure) {
					Data srcData;
					srcData.type = Data::UINT16;
					srcData.dimensions = dimensions;
					srcData.frameNumber = m_cam.m_image_number;

					Buffer *fbuf = new Buffer(width*height*sizeof(uint16_t));
					srcData.setBuffer(fbuf);
					fbuf->unref();
					memcpy(srcData.data(), bptr, width*height*sizeof(uint16_t));

					if (saveOpt & Camera::SaveRaw) {
						DEB_TRACE() << "Save raw frame " << m_cam.m_image_number;
						TaskMgr *taskMgr = new TaskMgr();
						taskMgr->setLinkTask(0, m_savingTask0);
						taskMgr->setInputData(srcData);
						DEB_TRACE() << "Adding frame " << m_cam.m_image_number << " to task to pool (saving) ";
						PoolThreadMgr::get().addProcess(taskMgr);
					}
					if (saveOpt & Camera::SaveProcessed || saveOpt & Camera::SaveHistogram) {
						// As only a single Data structure can be passed to the processing task we therefore
						// create a Data structure with the current frame and last frame appended so that
						// next frame correction can be achieved.
						Data concatData;
						std::vector<int> concatDimensions;
						uint16_t* bptr16;
						concatData.frameNumber = m_cam.m_image_number;
						concatDimensions.push_back(width);
						concatDimensions.push_back(height);
						concatDimensions.push_back(2);
						concatData.type = Data::UINT16;
						concatData.dimensions = concatDimensions;
						Buffer *newConcatBuffer = new Buffer(width*height*sizeof(uint16_t)*2);
						concatData.setBuffer(newConcatBuffer);
						newConcatBuffer->unref();
						bptr16 = (uint16_t*)concatData.data();
						memcpy(bptr16, bptr, width*height*sizeof(uint16_t));
						bptr16 += width*height;
						memcpy(bptr16, m_lastFrame.data(),width*height*sizeof(uint16_t));

						DEB_TRACE() << "Process raw frame " << m_cam.m_image_number << " and last frame " << m_lastFrame.frameNumber;
						TaskMgr *taskMgr = new TaskMgr();
						memcpy(m_lastFrame.data(), bptr,width*height*sizeof(uint16_t));
						taskMgr->setLinkTask(1, processingTask);
						taskMgr->setInputData(concatData);
						DEB_TRACE() << "Adding frame " << m_cam.m_image_number << " to task to pool (proc) ";
						PoolThreadMgr::get().addProcess(taskMgr);
					}
					DEB_TRACE() << "Image# " << m_cam.m_image_number << " acquired";
					m_cam.m_image_number++;
				} else {
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
			} else if (rc == 27) {
				DEB_WARNING() << "Skipping frame " << m_cam.m_hexitec->getErrorDescription() << " " << DEB_VAR1(rc);
			} else {
				DEB_ERROR() << "Retrieve error " << m_cam.m_hexitec->getErrorDescription() << " " << DEB_VAR1(rc);
				break;
			}
		}
		DEB_TRACE() << m_cam.m_image_number << " images acquired";
		auto t2 = Clock::now();
		DEB_TRACE() << "Delta t2-t1: " << std::chrono::duration_cast < std::chrono::nanoseconds
				> (t2 - t1).count() << " nanoseconds";

		DEB_ALWAYS() << "Stop acquisition";
		auto rc2 = m_cam.m_hexitec->stopAcq();
		if (rc2 != HexitecAPI::NO_ERROR) {
			DEB_ERROR() << "Failed to stop acquisition " << DEB_VAR1(rc);
		}
		m_cam.m_acq_started = false;
		m_cam.setStatus(Camera::Readout);
		DEB_TRACE() << "Setting bias off";
		m_cam.setHvBiasOff();
		DEB_ALWAYS() << "Check for outstanding processes";
		if (saveOpt & Camera::SaveProcessed || saveOpt & Camera::SaveHistogram) {
			while (processingTask->getNbProcessedFrames() < m_cam.m_image_number) {
				DEB_TRACE() << "still processing " << processingTask->getNbProcessedFrames();
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}
		if (saveOpt & Camera::SaveHistogram) {
			DEB_ALWAYS() << "Saving histogram ";
			TaskMgr *taskMgr = new TaskMgr();
			taskMgr->setLinkTask(0, m_savingTask2);
			m_savingTask2->setEventCallback(m_eventCb);
			Data histData = processingTask->getGlobalHistogram();
			taskMgr->setInputData(histData);
			PoolThreadMgr::get().addProcess(taskMgr);
			while (!m_cam.m_finished_saving) {
				DEB_TRACE() << "still saving";
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		}
		DEB_ALWAYS() << "Closing streams 0 ";
		m_cam.m_savingCtrlObj->close(0);
		DEB_ALWAYS() << "Closing streams 1 ";
		m_cam.m_savingCtrlObj->close(1);
		DEB_ALWAYS() << "Closing streams 2 ";
		m_cam.m_savingCtrlObj->close(2);
		delete processingTask;
		DEB_ALWAYS() << "Set status to ready";
		if (rc == HexitecAPI::NO_ERROR && rc2 == HexitecAPI::NO_ERROR) {
			m_cam.setStatus(Camera::Ready);
		} else {
			m_cam.setStatus(Camera::Fault);
		}
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
	DEB_DESTRUCTOR();
	AutoMutex lock(m_cam.m_cond.mutex());
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	lock.unlock();
	DEB_TRACE()  << "Waiting for the timer thread to be done (joining the main thread)";
	join();
}

void Camera::TimerThread::threadFunction() {
	DEB_MEMBER_FUNCT();

	while (!m_cam.m_quit) {
		while (!m_cam.m_acq_started && !m_cam.m_quit) {
			DEB_TRACE() << "Timer thread waiting";
			AutoMutex lock(m_cam.m_cond.mutex());
			m_cam.m_cond.wait();
		}
		DEB_TRACE() << "Timer thread Running";
		if (m_cam.m_quit)
			return;

		std::this_thread::sleep_for(std::chrono::milliseconds(m_cam.m_biasVoltageRefreshInterval));
		if (m_cam.m_acq_started) {
			m_cam.setStatus(Camera::Paused);
			DEB_TRACE() << "Paused at frame " << DEB_VAR1(m_cam.m_image_number);
			m_cam.setHvBiasOff();
			std::this_thread::sleep_for(std::chrono::milliseconds(m_cam.m_biasVoltageRefreshTime));
			m_cam.setHvBiasOn();
			std::this_thread::sleep_for(std::chrono::milliseconds(m_cam.m_biasVoltageSettleTime));
			m_cam.setStatus(Camera::Exposure);
			DEB_TRACE() << "Acq status in timer after restart " << DEB_VAR1(m_cam.m_status);
		}
	}
}

//-----------------------------------------------------
// task event callback
//-----------------------------------------------------
Camera::TaskEventCb::TaskEventCb(Camera& cam) : m_cam(cam) {}

Camera::TaskEventCb::~TaskEventCb() {}

void Camera::TaskEventCb::finished(Data& data) {
	AutoMutex lock(m_cam.m_cond.mutex());
	m_cam.m_finished_saving = true;
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

void Camera::setType(ProcessType type) {
	m_processType = type;
}

void Camera::getType(ProcessType& type) {
	type = m_processType;
}

void Camera::setBinWidth(int binWidth) {
	m_binWidth = binWidth;
}

void Camera::getBinWidth(int& binWidth) {
	binWidth = m_binWidth;
}

void Camera::setSpecLen(int speclen) {
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

void Camera::setBiasVoltageRefreshInterval(int millis) {
	m_biasVoltageRefreshInterval = millis;
}

void Camera::setBiasVoltageRefreshTime(int millis) {
	m_biasVoltageRefreshTime = millis;
}

void Camera::setBiasVoltageSettleTime(int millis) {
	m_biasVoltageSettleTime = millis;
}
void Camera::getBiasVoltageRefreshInterval(int& millis) {
	millis = m_biasVoltageRefreshInterval;
}

void Camera::getBiasVoltageRefreshTime(int& millis) {
	millis = m_biasVoltageRefreshTime;
}

void Camera::getBiasVoltageSettleTime(int& millis) {
	millis = m_biasVoltageSettleTime;
}
