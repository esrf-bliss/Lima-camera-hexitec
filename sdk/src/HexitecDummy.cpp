#include <HexitecApi.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cmath>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <fstream>

using namespace HexitecAPI;

HexitecApi::HexitecApi(const std::string deviceDescriptor, uint32_t timeout) : m_deviceDescriptor(deviceDescriptor), m_timeout(timeout),
	m_sensorConfig(), m_operationMode(), m_systemConfig(), m_biasConfig() {
	mutexLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
}

HexitecApi::~HexitecApi() {
}

int32_t HexitecApi::readConfiguration(std::string fname) {
	return NO_ERROR;
}

int32_t HexitecApi::createPipelineOnly(uint32_t bufferCount) {
	return NO_ERROR;
}

std::string HexitecApi::getErrorDescription() {
	return "error";
}

int32_t HexitecApi::getFramesAcquired() {
	return 1.0;
}

int32_t HexitecApi::startAcq() {
	m_file.open(m_deviceDescriptor, std::fstream::in|std::ifstream::binary);
	return NO_ERROR;
}

int32_t HexitecApi::stopAcq() {
	return NO_ERROR;
}

int32_t HexitecApi::setHvBiasOn(bool onOff) {
	return NO_ERROR;
}

/**
 * @param [IN] frametimeout time in milliseconds to wait for frame to complete
 */
int32_t HexitecApi::retrieveBuffer(uint8_t* buffer, uint32_t frametimeout) {
	uint16_t* dptr = (uint16_t*)buffer;
	if (m_file.is_open()) {
		m_file.read ((char*)dptr, 80*80*2);
//		std::this_thread::sleep_for(std::chrono::microseconds(10));
	} else {
//		std::this_thread::sleep_for(std::chrono::microseconds(100));
		int k = 0;
		for (auto i = 0; i < 80; i++) {
			for (auto j = 0; j < 80; j++, dptr++) {
				*dptr = k++;
			}
		}
	}
	return NO_ERROR;
}

int32_t HexitecApi::acquireFrame(uint32_t& frameCount, uint8_t* buffer, uint32_t frametimeout) {
	return NO_ERROR;
}

int32_t HexitecApi::acquireFrames(uint32_t frameCount, uint64_t& framesAcquired, uint32_t frametimeout) {
	return NO_ERROR;
}

int32_t HexitecApi::checkFirmware(uint8_t& customerId, uint8_t& projectId, uint8_t& version, uint8_t forceEqualVersion) {
	customerId = 100;
	projectId = 101;
	version = 102;
	return NO_ERROR;
}

int32_t HexitecApi::closePipeline() {
	return NO_ERROR;
}

int32_t HexitecApi::closeSerialPort() {
	return NO_ERROR;
}

int32_t HexitecApi::closeStream() {
	return NO_ERROR;
}

int32_t HexitecApi::collectOffsetValues(uint32_t collectDctimeout) {
	return NO_ERROR;
}

int32_t HexitecApi::configureDetector(uint8_t& width, uint8_t& height, double& frameTime, uint32_t& collectDcTime) {
	width = 80;
	height = 80;
	frameTime = 0.000165;
	collectDcTime = 1000;
	return NO_ERROR;
}

void HexitecApi::copyBuffer(uint8_t* sourceBuffer, uint8_t* destBuffer, uint32_t byteCount) {
//	memcpy(destBuffer, sourceBuffer, byteCount);
}

int32_t HexitecApi::createPipeline(uint32_t bufferCount, uint32_t transferBufferCount, uint32_t transferBufferFrameCount) {
	return NO_ERROR;
}

double HexitecApi::getFrameTime(uint8_t width, uint8_t height) {
	return 3.142;
}

int32_t HexitecApi::getLastResult(uint32_t& internalErrorCode, std::string errorCodeString, std::string errorDescription) {
	return NO_ERROR;
}

int32_t HexitecApi::getSensorConfig(HexitecSensorConfig& sensorConfig) {
	return NO_ERROR;
}


int32_t HexitecApi::getTriggerState(uint8_t& trigger1, uint8_t& trigger2, uint8_t& trigger3) {
	trigger1 = 0;
	trigger2 = 0;
	trigger3 = 0;
	return NO_ERROR;
}

int32_t HexitecApi::initDevice(uint32_t& internalErrorCode, std::string& errorCodeString, std::string& errorDescription) {
	internalErrorCode = NO_ERROR;
	return NO_ERROR;
}

int32_t HexitecApi::initFwDefaults(uint8_t setHv, double& hvSetPoint, uint8_t width, uint8_t height,
		HexitecSensorConfig& sensorConfig, HexitecOperationMode& operationMode, double& frameTime, uint32_t& collectDcTime) {
	return NO_ERROR;
}

int32_t HexitecApi::openStream() {
	return NO_ERROR;
}

int32_t HexitecApi::openSerialPortBulk0(uint32_t rxBufferSize, uint8_t useTermChar, uint8_t termChar) {
	return NO_ERROR;
}

int32_t HexitecApi::readEnvironmentValues(double& humidity, double& ambientTemperature, double& asicTemperature,
		double& adcTemperature, double& ntcTemperature) {
	ambientTemperature = 22.0;
	humidity = 3.4;
	asicTemperature = 32.0;
	adcTemperature = 40.0;
	ntcTemperature = 25.0;
	return NO_ERROR;
}

int32_t HexitecApi::readOperatingValues(double& v3_3, double& hvMon, double& hvOut, double& v1_2, double& v1_8, double& v3,
		double& v2_5, double& v3_3ln, double& v1_65ln, double& v1_8ana, double& v3_8ana, double& peltierCurrent,
		double& ntcTemperature) {

	v3_3 = 3.0;
	hvMon = 3.1;
	hvOut = 3.2;
	v1_2 = 3.3;
	v1_8 = 3.4;
	v3 = 3.5;
	v2_5 = 3.6;
	v3_3ln = 3.7;
	v1_65ln = 3.8;
	v1_8ana = 3.9;
	v3_8ana = 4.0;
	peltierCurrent = 4.4;
	ntcTemperature = 23.0;
	return NO_ERROR;
}

int32_t HexitecApi::setDAC(double& vCal, double& uMid, double& hvSetPoint, double& detCtrl, double& targetTemperature) {
	vCal = 2.3;
	uMid = 4.5;
	hvSetPoint = 6.7;
	detCtrl = 4.6;
	targetTemperature = 23.0;
	return NO_ERROR;
}

int32_t HexitecApi::setFrameFormatControl(const std::string& pixelFormat, uint64_t width, uint64_t height, uint64_t offsetX,
		uint64_t offsetY, const std::string& sensorTaps, const std::string& testPattern) {
	return NO_ERROR;
}

int32_t HexitecApi::setTriggeredFrameCount(uint32_t frameCount) {
	return NO_ERROR;
}


int32_t HexitecApi::uploadOffsetValues(Reg2Byte* offsetValues, uint32_t offsetValuesLength) {
	return NO_ERROR;
}

int32_t HexitecApi::setSensorConfig(HexitecSensorConfig sensorConfig) {
	return NO_ERROR;
}

