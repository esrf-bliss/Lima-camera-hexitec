#include <HexitecApi.h>
#include "INIReader.h"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <cmath>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>

using namespace HexitecAPI;
using namespace GigE;

HexitecApi::HexitecApi(const std::string deviceDescriptor, uint32_t timeout) : m_deviceDescriptor(deviceDescriptor), m_timeout(timeout),
	m_sensorConfig(), m_operationMode(), m_systemConfig(), m_biasConfig() {
}

HexitecApi::~HexitecApi() {
	delete gigeDevice;
}

int pack(std::string str, int idx) {
	int val = 0;
	for (auto j=0; j<8; j++, idx++) {
		val += str.at(idx) - 48 << j;
	}
}

int32_t HexitecApi::readConfiguration(std::string fname) {
	std::string line;
	std::string section;
	INIReader reader(fname);
	if (reader.ParseError() < 0) {
		return OPENFILE_ERR;
	}
	section = "HexitecSystemConfig";
	m_systemConfig.AdcDelay = reader.GetInteger(section, "ADC1 Delay", -1);
	m_systemConfig.SyncSignalDelay = reader.GetInteger(section, "Delay sync signals", -1);
	m_systemConfig.AdcSample = static_cast<HexitecAdcSample>(reader.GetInteger(section, "ADC sample", -1));
	m_systemConfig.AdcClockPhase = reader.GetInteger(section, "ADC1 Clock Phase", -1);
	m_systemConfig.Umid = reader.GetReal(section, "PreAmp ref voltage", 1.0);
	m_systemConfig.DetCtrl = reader.GetReal(section, "Unused", 0.0);
	m_systemConfig.TargetTemperature = reader.GetReal(section, "Peltier setpoint", 20.0);

	section = "Bias_Voltage";
	m_biasConfig.BiasVoltage=reader.GetReal(section, "BiasVoltage", 0.0);
	m_biasConfig.RefreshVoltage=reader.GetReal(section, "RefreshVoltage", 0.0);

	section = "HexitecSensorConfig";
	m_sensorConfig.Gain = static_cast<HexitecGain>(reader.GetInteger(section, "Gain", 0));
	m_sensorConfig.Row_S1.size2 = reader.GetInteger(section, "Row -> S1", 0);
	m_sensorConfig.S1_Sph = reader.GetInteger(section, "S1 -> Sph", -1);
	m_sensorConfig.Sph_S2 = reader.GetInteger(section, "Sph -> S2", -1);
	m_sensorConfig.Vcal2_Vcal1.size2 = reader.GetInteger(section, "VCAL2 -> VCAL1", -1);
	m_sensorConfig.Vcal = reader.GetReal(section, "VCAL", 0.2);
	m_sensorConfig.WaitClockCol = reader.GetInteger(section, "Wait clock column", -1);
	m_sensorConfig.WaitClockRow = reader.GetInteger(section, "Wait clock row", -1);

	section = "HexitecSetupRegister";
	std::string colEnbStr;
	std::string colPwrStr;
	std::string colCalStr;
	colEnbStr = reader.Get(section, "ColumnEn_1stChannel", "");
	colPwrStr = reader.Get(section, "ColumnPwr1stChannel", "");
	colCalStr = reader.Get(section, "ColumnCal1stChannel", "");
	colEnbStr += reader.Get(section, "ColumnEn_2ndChannel", "");
	colPwrStr += reader.Get(section, "ColumnPwr2ndChannel", "");
	colCalStr += reader.Get(section, "ColumnCal2ndChannel", "");
	colEnbStr += reader.Get(section, "ColumnEn_3rdChannel", "");
	colPwrStr += reader.Get(section, "ColumnPwr3rdChannel", "");
	colCalStr += reader.Get(section, "ColumnCal3rdChannel", "");
	colEnbStr += reader.Get(section, "ColumnEn_4thChannel", "");
	colPwrStr += reader.Get(section, "ColumnPwr4thChannel", "");
	colCalStr += reader.Get(section, "ColumnCal4thChannel", "");

	std::string rowEnbStr;
	std::string rowPwrStr;
	std::string rowCalStr;
	rowEnbStr = reader.Get(section, "RowEn_1stBlock", "");
	rowPwrStr = reader.Get(section, "RowPwr1stBlock", "");
	rowCalStr = reader.Get(section, "RowCal1stBlock", "");
	rowEnbStr += reader.Get(section, "RowEn_2ndBlock", "");
	rowPwrStr += reader.Get(section, "RowPwr2ndBlock", "");
	rowCalStr += reader.Get(section, "RowCal2ndBlock", "");
	rowEnbStr += reader.Get(section, "RowEn_3rdBlock", "");
	rowPwrStr += reader.Get(section, "RowPwr3rdBlock", "");
	rowCalStr += reader.Get(section, "RowCal3rdBlock", "");
	rowEnbStr += reader.Get(section, "RowEn_4thBlock", "");
	rowPwrStr += reader.Get(section, "RowPwr4thBlock", "");
	rowCalStr += reader.Get(section, "RowCal4thBlock", "");

	int idx = 0;
	for (auto i=0; i<10; i++, idx+= 8) {
		m_sensorConfig.SetupCol.ReadEn[i] = pack(colEnbStr, idx);
		m_sensorConfig.SetupCol.PowerEn[i] = pack(colPwrStr, idx);
		m_sensorConfig.SetupCol.CalEn[i] = pack(colCalStr, idx);
		m_sensorConfig.SetupRow.ReadEn[i] = pack(rowEnbStr, idx);
		m_sensorConfig.SetupRow.PowerEn[i] = pack(rowPwrStr, idx);
		m_sensorConfig.SetupRow.CalEn[i] = pack(rowCalStr, idx);
	}

	m_operationMode.DcUploadDarkCorrectionValues = CONTROL_DISABLED;
	m_operationMode.DcCollectDarkCorrectionValues = CONTROL_DISABLED;
	m_operationMode.DcEnableDarkCorrectionCountingMode = CONTROL_DISABLED;
	m_operationMode.DcEnableDarkCorrectionSpectroscopicMode = CONTROL_ENABLED;
	m_operationMode.DcSendDarkCorrectionValues = CONTROL_DISABLED;
	m_operationMode.DcDisableVcalPulse = CONTROL_DISABLED;
	m_operationMode.DcTestMode = CONTROL_DISABLED;
	m_operationMode.DcEnableTriggeredCountingMode = CONTROL_DISABLED;
	m_operationMode.EdUploadThresholdValues = CONTROL_DISABLED;
	m_operationMode.EdDisableCountingMode = CONTROL_DISABLED;
	m_operationMode.EdTestMode = CONTROL_DISABLED;
	m_operationMode.EdCycles.size2 = 1;
	m_operationMode.enSyncMode = CONTROL_ENABLED;
//	section = "HexitecOperationMode";
//	m_operationMode.DcUploadDarkCorrectionValues = static_cast<Control>(reader.GetInteger(section, "DcUploadDarkCorrectionValues", 0));
//	m_operationMode.DcCollectDarkCorrectionValues = static_cast<Control>(reader.GetInteger(section, "DcCollectDarkCorrectionValues", 0));
//	m_operationMode.DcEnableDarkCorrectionCountingMode = static_cast<Control>(reader.GetInteger(section, "DcEnableDarkCorrectionCountingMode", 0));
//	m_operationMode.DcEnableDarkCorrectionSpectroscopicMode = static_cast<Control>(reader.GetInteger(section, "DcEnableDarkCorrectionSpectroscopicMode", 0));
//	m_operationMode.DcSendDarkCorrectionValues = static_cast<Control>(reader.GetInteger(section, "DcSendDarkCorrectionValues", 0));
//	m_operationMode.DcDisableVcalPulse = static_cast<Control>(reader.GetInteger(section, "DcDisableVcalPulse", 0));
//	m_operationMode.DcTestMode = static_cast<Control>(reader.GetInteger(section, "DcTestMode", 0));
//	m_operationMode.DcEnableTriggeredCountingMode = static_cast<Control>(reader.GetInteger(section, "DcEnableTriggeredCountingMode", 0));
//	m_operationMode.EdUploadThresholdValues = static_cast<Control>(reader.GetInteger(section, "EdUploadThresholdValues", 0));
//	m_operationMode.EdDisableCountingMode = static_cast<Control>(reader.GetInteger(section, "EdDisableCountingMode", 0));
//	m_operationMode.EdTestMode = static_cast<Control>(reader.GetInteger(section, "EdTestMode", 0));
//	m_operationMode.EdCycles.size2 = static_cast<Control>(reader.GetInteger(section, "EdCycles", 0));
//	m_operationMode.enSyncMode = static_cast<Control>(reader.GetInteger(section, "enSyncMode", 0));
//	m_operationMode.enTriggerMode = static_cast<Control>(reader.GetInteger(section, "enTriggerMode", 0));

	return NO_ERROR;
}

int32_t HexitecApi::setTriggerCountingMode(bool enable) {
    int32_t result = NO_ERROR;
    HexitecOperationMode currentMode;

    result = getOperationMode(currentMode);
    if (result == NO_ERROR) {
        if (enable) {
            currentMode.DcEnableTriggeredCountingMode = Control::CONTROL_ENABLED;
        } else {
            currentMode.DcEnableTriggeredCountingMode = Control::CONTROL_DISABLED;
        }
        result = setOperationMode(currentMode);
    }
    return result;
}

int32_t HexitecApi::createPipelineOnly(uint32_t bufferCount) {
	return gigeDevice->CreatePipeline(bufferCount);
}

std::string HexitecApi::getErrorDescription() {
	return static_cast<std::string>(gigeDevice->GetLastResult().GetCodeString().GetAscii());
}

int32_t HexitecApi::getFramesAcquired() {
	return gigeDevice->GetAcquiredImageCount();
}

int32_t HexitecApi::startAcq() {
	return gigeDevice->startAcq();
}

int32_t HexitecApi::stopAcq() {
	return gigeDevice->stopAcq();
}

int32_t HexitecApi::setHvBiasOn(bool onOff) {
	int32_t result;
	if (onOff) {
		result = setDAC(m_sensorConfig.Vcal, m_systemConfig.Umid, m_biasConfig.BiasVoltage, m_systemConfig.DetCtrl, m_systemConfig.TargetTemperature);
	} else {
		result = setDAC(m_sensorConfig.Vcal, m_systemConfig.Umid, m_biasConfig.RefreshVoltage, m_systemConfig.DetCtrl, m_systemConfig.TargetTemperature);
	}
}

void HexitecApi::setBiasVoltage(int volts) {
    m_biasConfig.BiasVoltage = volts;
}
void HexitecApi::getBiasVoltage(int& volts) {
    volts = m_biasConfig.BiasVoltage;
}

void HexitecApi::setRefreshVoltage(int volts) {
    m_biasConfig.RefreshVoltage = volts;
}

void HexitecApi::getRefreshVoltage(int& volts) {
    volts = m_biasConfig.RefreshVoltage;
}

/**
 * @param [IN] frametimeout time in milliseconds to wait for frame to complete
 */
int32_t HexitecApi::retrieveBuffer(uint8_t* buffer, uint32_t frametimeout) {
	int32_t rc = gigeDevice->retrieveBuffer(buffer, frametimeout);
	return rc;
}

int32_t HexitecApi::openSerialPortBulk0(uint32_t rxBufferSize, uint8_t useTermChar, uint8_t termChar) {
	return gigeDevice->OpenSerialPort(PvDeviceSerialBulk0, rxBufferSize, useTermChar, termChar);
}

int32_t HexitecApi::acquireFrame(uint32_t& frameCount, uint8_t* buffer, uint32_t frametimeout) {
	return gigeDevice->AcquireImage(&frameCount, buffer, frametimeout);
}

int32_t HexitecApi::acquireFrames(uint32_t frameCount, uint64_t& framesAcquired, uint32_t frametimeout) {
	int32_t result = gigeDevice->AcquireImageThread(frameCount, frametimeout);
	framesAcquired = gigeDevice->GetAcquiredImageCount();
	return result;
}

int32_t HexitecApi::checkFirmware(uint8_t& customerId, uint8_t& projectId, uint8_t& version, uint8_t forceEqualVersion) {
	int32_t result = NO_ERROR;
	uint8_t requiredCustomerId = customerId;
	uint8_t requiredProjectId = projectId;
	uint8_t requiredVersion = version;

	result = readRegister(FPGA_FW_CHECK_CUSTOMER_REG, customerId);
	if (result == NO_ERROR) {
		result = readRegister(FPGA_FW_CHECK_PROJECT_REG, projectId);
	}
	if (result == NO_ERROR) {
		result = readRegister(FPGA_FW_CHECK_VERSION_REG, version);
	}
	return result;
}

int32_t HexitecApi::closePipeline() {
	return gigeDevice->ClosePipeline();
}

int32_t HexitecApi::closeSerialPort() {
	return gigeDevice->CloseSerialPort();
}

int32_t HexitecApi::closeStream() {
	return gigeDevice->CloseStream();
}

int32_t HexitecApi::collectOffsetValues(uint32_t collectDctimeout) {
	int32_t result = NO_ERROR;
	uint8_t value = 0;
	HexitecOperationMode currentMode;
	HexitecOperationMode collectMode;

	result = getOperationMode(currentMode);
	if (result == NO_ERROR) {
		collectMode = currentMode;
		collectMode.DcCollectDarkCorrectionValues = Control::CONTROL_ENABLED;
		collectMode.DcUploadDarkCorrectionValues = Control::CONTROL_DISABLED;
		collectMode.DcDisableVcalPulse = Control::CONTROL_ENABLED;
		result = setOperationMode(collectMode);
	}
	if (result == NO_ERROR) {
		usleep(collectDctimeout*1000);
		result = readRegister(0x89, value);
	}
	if (result == NO_ERROR && !(value & 0x01)) {
		for (auto i=0; i<20; i++) {
			result = readRegister(0x89, value);
		}
		result = COLLECT_DC_NOT_READY;
	}
	if (result == NO_ERROR) {
		result = setOperationMode(currentMode);
	}
	return result;
}

int32_t HexitecApi::configureDetector(uint8_t& width, uint8_t& height, double& frameTime, uint32_t& collectDcTime) {
	int32_t result = NO_ERROR;
	uint8_t value = 0;
//
// TODO This method needs sorting out. Its a jumble
// for a start enableSM & disableSM should be removed
// syncMode should be in SetOPerationMode
// and m.systemConfig's should be in a seperate method.
//
	result = disableSM();
	if (result == NO_ERROR) {
		if (m_operationMode.enSyncMode == CONTROL_ENABLED) {
			result = enableSyncMode();
		} else {
			result = disableSyncMode();
		}
	}
	if (result == NO_ERROR) {
		value = 0x01;
		result = writeRegister(0x07, value);
	}
	if (result == NO_ERROR) {
		value = m_systemConfig.AdcDelay;
		result = writeRegister(0x09, value);
	}
	if (result == NO_ERROR) {
		value = m_systemConfig.SyncSignalDelay;
		result = writeRegister(0x0e, value);
	}
	if (result == NO_ERROR) {
		value = m_systemConfig.AdcSample;
		result = writeRegister(0x14, value);
	}
	if (result == NO_ERROR) {
		result = setSensorConfig(m_sensorConfig);
	}
	if (result == NO_ERROR) {
		result = setOperationMode(m_operationMode);
	}
    if (result == NO_ERROR) {
        result = disableSM();
    }
	if (result == NO_ERROR) {
		result = enableFunctionBlocks(Control::CONTROL_DISABLED, Control::CONTROL_ENABLED, Control::CONTROL_ENABLED);
	}
	if (result == NO_ERROR) {
		result = enableSM();
	}
	if (result == NO_ERROR) {
		result = enableFunctionBlocks(Control::CONTROL_ENABLED, Control::CONTROL_ENABLED, Control::CONTROL_ENABLED);
	}
	if (result == NO_ERROR) {
		value = m_systemConfig.AdcClockPhase;
		result = writeAdcRegister(0x16, value);
	}
	if (result == NO_ERROR) {
		result = readResolution(width, height);
	}
	if (result == NO_ERROR) {
		frameTime = getFrameTime(width, height);
		collectDcTime = getCollectDcTime(frameTime);
		gigeDevice->SetFrameTime(frameTime);
		gigeDevice->SetFrameTimeOut((u32)((frameTime*1000*HEXITEC_FRAME_TIMEOUT_MULTIPLIER)+1));
	}
	if ((result == NO_ERROR) && (m_operationMode.enSyncMode))
	{
		result = enableSyncMode();
	}
	return result;
}

void HexitecApi::copyBuffer(uint8_t* sourceBuffer, uint8_t* destBuffer, uint32_t byteCount) {
	memcpy(destBuffer, sourceBuffer, byteCount);
}

int32_t HexitecApi::createPipeline(uint32_t bufferCount, uint32_t transferBufferCount, uint32_t transferBufferFrameCount) {
	gigeDevice->SetTransferBuffer(transferBufferCount, transferBufferFrameCount);
	return gigeDevice->CreatePipeline(bufferCount);
}

int32_t HexitecApi::createPipelineOld(u32 bufferCount) {
	return  gigeDevice->CreatePipeline( bufferCount );
}

int32_t HexitecApi::disableSM() {
	uint8_t value = CONTROL_DISABLED;
	return writeRegister(0x01, value);
}

int32_t HexitecApi::disableSyncMode() {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x43;
	txBuffer[3] = 0x30;
	txBuffer[4] = 0x41;
	txBuffer[5] = 0x30;
	txBuffer[6] = 0x31;
	txBuffer[7] = 0x0d;
	return serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
}

int32_t HexitecApi::disableTriggerGate() {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
    int32_t result = NO_ERROR;
    HexitecOperationMode currentMode;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x43;
	txBuffer[3] = 0x30;
	txBuffer[4] = 0x41;
	txBuffer[5] = 0x30;
	txBuffer[6] = 0x34;
	txBuffer[7] = 0x0d;
	return serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
}

int32_t HexitecApi::disableTriggerMode() {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x43;
	txBuffer[3] = 0x30;
	txBuffer[4] = 0x41;
	txBuffer[5] = 0x30;
	txBuffer[6] = 0x32;
	txBuffer[7] = 0x0d;
	return serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
}

int32_t HexitecApi::enableFunctionBlocks(Control adcEnable, Control dacEnable, Control peltierEnable) {
	uint8_t txBuffer[6];
	uint8_t rxBuffer[5];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	uint8_t value = 0;

	value = adcEnable + (dacEnable * 0x02) + (peltierEnable * 0x04);
	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x55;
	hexToString(value, 2, &txBuffer[3]);
	txBuffer[5] = 0x0d;
	return serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
}

int32_t HexitecApi::enableSM() {
	uint8_t value = CONTROL_ENABLED;
	return writeRegister(0x01, value);
}

int32_t HexitecApi::enableSyncMode() {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x42;
	txBuffer[3] = 0x30;
	txBuffer[4] = 0x41;
	txBuffer[5] = 0x30;
	txBuffer[6] = 0x31;
	txBuffer[7] = 0x0d;
	return serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
}

int32_t HexitecApi::enableTriggerGate() {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
    int32_t result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x42;
	txBuffer[3] = 0x30;
	txBuffer[4] = 0x41;
	txBuffer[5] = 0x30;
	txBuffer[6] = 0x34;
	txBuffer[7] = 0x0d;
	result=  serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
    return result;
}

int32_t HexitecApi::enableTriggerMode() {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t	result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x42;
	txBuffer[3] = 0x30;
	txBuffer[4] = 0x41;
	txBuffer[5] = 0x30;
	txBuffer[6] = 0x32;
	txBuffer[7] = 0x0d;
	result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
	if (result == NO_ERROR) {
		std::shared_ptr<AcqArmedCallback> cbk = std::shared_ptr<AcqArmedCallback>(new HexitecApi::HexitecArmedCb(*this));
		gigeDevice->RegisterAcqArmedCallBack(cbk);
	}
	if (result == NO_ERROR) {
		std::shared_ptr<AcqFinishCallback> cbk = std::shared_ptr<AcqFinishCallback>(new HexitecFinishCb(*this));
		gigeDevice->RegisterAcqFinishCallBack(cbk);
	}
	return result;
}

void armed() {
    std::cout << "+++++++++++++ I'm armed" << std::endl;
}

int32_t HexitecApi::exitDevice() {
	delete gigeDevice;
	return NO_ERROR;
}

#ifndef __linux__
int32_t HexitecApi::getErrorMsg(int32_t asError, char* errorMsg, uint32_t length) {
	hdl ghResDll = NULL;
	int32_t charCnt = 0;
	int32_t lAsError = asError;
	int32_t* pArgs[] = { &lAsError };

	if (asErrorMsg != NULL) {
		ghResDll = LoadLibrary(MESSAGE_DLL);
		if (ghResDll != NULL) {
			charCnt = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, ghResDll, lAsError, 0, asErrorMsg, length, (va_list*) pArgs);
			FreeLibrary((HMODULE) ghResDll);
		}
		if ((ghResDll == NULL) || (charCnt == 0)) {
			return GetLastError();
		}
	}
	return NO_ERROR;
}
#endif

int32_t HexitecApi::getBufferHandlingThreadPriority(int32_t& priority )
{
	priority = gigeDevice->GetBufferHandlingThreadPriority();
	return NO_ERROR;
}

int32_t HexitecApi::getDeviceInformation(HexitecDeviceInfo& deviceInfo) {
	GigEDeviceInfoStr deviceInfoStr = gigeDevice->GetDeviceInfoStr();
	deviceInfo.Vendor = deviceInfoStr.Vendor;
	deviceInfo.Model = deviceInfoStr.Model;
	deviceInfo.ManufacturerInfo = deviceInfoStr.ManufacturerInfo;
	deviceInfo.SerialNumber = deviceInfoStr.SerialNumber;
	deviceInfo.UserId = deviceInfoStr.UserId;
	deviceInfo.MacAddress = deviceInfoStr.MacAddress;
	deviceInfo.IpAddress = deviceInfoStr.IpAddress;
	deviceInfo.NetMask = deviceInfoStr.NetMask;
	deviceInfo.GateWay = deviceInfoStr.GateWay;
	return NO_ERROR;
}

double HexitecApi::getFrameTime(uint8_t width, uint8_t height) {
	double clkPeriod = 1 / HEXITEC_CLOCK_SPEED;
	uint32_t rowClks = ((m_sensorConfig.Row_S1.size2 + m_sensorConfig.S1_Sph + m_sensorConfig.Sph_S2 + (width / 4)
			+ m_sensorConfig.WaitClockCol) * 2) + 10;
	uint32_t frameClks = (height * rowClks) + 4 + (m_sensorConfig.WaitClockRow * 2);
	uint32_t frameClks3 = (frameClks * 3) + 2;
	return ((double) frameClks3) * clkPeriod / 3;
}

int32_t HexitecApi::getIntegerValue(const std::string propertyName, int64_t& value) {
	return gigeDevice->GetIntegerValue(const_cast<char*>(propertyName.c_str()), value);
}

int32_t HexitecApi::getLastResult(uint32_t& internalErrorCode, std::string errorCodeString, std::string errorDescription) {
	int32_t result = NO_ERROR;
	PvResult pvResult = PvResult::Code::OK;
	char pleoraErrorCodeString[128];
	uint32_t* pleoraErrorCodeStringLen;
	char pleoraErrorDescription[128];
	uint32_t* pleoraErrorDescriptionLen;

	pvResult = gigeDevice->GetLastResult();
	internalErrorCode = pvResult.GetCode();
	GigEDevice::GetErrorDescription(result, pleoraErrorCodeString, pleoraErrorCodeStringLen, pleoraErrorDescription,
			pleoraErrorDescriptionLen);
	errorCodeString = std::string(pleoraErrorCodeString);
	errorDescription = std::string(pleoraErrorDescription);
	return result;
}

int32_t HexitecApi::getOperationMode(HexitecOperationMode& operationMode) {
	uint8_t value = 0;
	int32_t result = NO_ERROR;

	result = readRegister(0x24, value);

	if (result == NO_ERROR) {
		operationMode.DcUploadDarkCorrectionValues = (Control) (value & 0x01);
		operationMode.DcCollectDarkCorrectionValues = (Control) ((value & 0x02) >> 1);
		operationMode.DcEnableDarkCorrectionCountingMode = (Control) ((value & 0x04) >> 2);
		operationMode.DcEnableDarkCorrectionSpectroscopicMode = (Control) ((value & 0x08) >> 3);
		operationMode.DcSendDarkCorrectionValues = (Control) ((value & 0x10) >> 4);
		operationMode.DcDisableVcalPulse = (Control) ((value & 0x20) >> 5);
		operationMode.DcTestMode = (Control) ((value & 0x40) >> 6);
		operationMode.DcEnableTriggeredCountingMode = (Control) ((value & 0x80) >> 7);
		result = readRegister(0x27, value);
	}
	if (result == NO_ERROR) {
		operationMode.EdUploadThresholdValues = (Control) (value & 0x01);
		operationMode.EdDisableCountingMode = (Control) ((value & 0x02) >> 1);
		operationMode.EdTestMode = (Control) ((value & 0x04) >> 2);
		result = readRegister(0x28, value);
	}
	if (result == NO_ERROR) {
		operationMode.EdCycles.size1[0] = value;
		result = readRegister(0x29, value);
	}
	if (result == NO_ERROR) {
		operationMode.EdCycles.size1[1] = value;
	}
	return result;
}

int32_t HexitecApi::getSensorConfig(HexitecSensorConfig& sensorConfig) {
	uint8_t value = 0;
	int32_t result = NO_ERROR;
	uint8_t setupReg = HEXITEC_SETUP_REGISTER_START_ADDRESS;

	result = readRegister(0x06, value);
	if (result == NO_ERROR) {
		sensorConfig.Gain = (HexitecGain) value;
		result = readRegister(0x02, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.Row_S1.size1[0] = value;
		result = readRegister(0x03, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.Row_S1.size1[1] = value;
		result = readRegister(0x04, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.S1_Sph = value;
		result = readRegister(0x05, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.Sph_S2 = value;
		result = readRegister(0x18, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.Vcal2_Vcal1.size1[0] = value;
		result = readRegister(0x19, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.Vcal2_Vcal1.size1[1] = value;
		result = readRegister(0x1a, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.WaitClockCol = value;
		result = readRegister(0x1b, value);
	}
	if (result == NO_ERROR) {
		sensorConfig.WaitClockRow = value;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			result = readRegister(setupReg, value);
			sensorConfig.SetupRow.PowerEn[i] = value;
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			result = readRegister(setupReg, value);
			sensorConfig.SetupRow.CalEn[i] = value;
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			result = readRegister(setupReg, value);
			sensorConfig.SetupRow.ReadEn[i] = value;
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			result = readRegister(setupReg, value);
			sensorConfig.SetupCol.PowerEn[i] = value;
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			result = readRegister(setupReg, value);
			sensorConfig.SetupCol.CalEn[i] = value;
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			result = readRegister(setupReg, value);
			sensorConfig.SetupCol.ReadEn[i] = value;
		}
		setupReg++;
	}
	return result;
}

#ifndef __linux__
int32_t HexitecApi::getSystemErrorMsg(int32_t sysError, char* sysErrorMsg, uint32_t length) {
	int32_t charCnt = 0;

	if (sysErrorMsg != NULL) {
		charCnt = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, sysError, 0, sysErrorMsg, length, NULL);
		if (charCnt == 0) {
			return GetLastError();
		}
	}
	return NO_ERROR;
}
#endif

int32_t HexitecApi::getTriggerState(uint8_t& trigger1, uint8_t& trigger2, uint8_t& trigger3) {
	int32_t result = NO_ERROR;
	uint8_t value = 0;

	result = readRegister(0x85, value);
	if (result == NO_ERROR) {
		trigger1 = value & 0x01;
		trigger2 = (value & 0x02) >> 1;
		trigger3 = (value & 0x04) >> 2;
	} else {
		trigger1 = 0;
		trigger2 = 0;
		trigger3 = 0;
	}
	return result;
}

int32_t HexitecApi::initDevice(uint32_t& internalErrorCode, std::string& errorCodeString, std::string& errorDescription) {
	PvResult pvResult = PvResult::Code::OK;
	char pleoraErrorCodeString[255];
	uint32_t pleoraErrorCodeStringLen = 255;
	char pleoraErrorDescription[255];
	uint32_t pleoraErrorDescriptionLen = 255;

	gigeDevice = new GigEDevice(const_cast<char*>(m_deviceDescriptor.c_str()));
	pvResult = gigeDevice->GetLastResult();
	internalErrorCode = pvResult.GetCode();
	gigeDevice->GetErrorDescription(pvResult, pleoraErrorCodeString, &pleoraErrorCodeStringLen, pleoraErrorDescription,
			&pleoraErrorDescriptionLen);
	errorCodeString = std::string(pleoraErrorCodeString);
	errorDescription = std::string(pleoraErrorDescription);
	return pvResult.IsOK();
}

int32_t HexitecApi::initFwDefaults(uint8_t setHv, double& hvSetPoint, uint8_t width, uint8_t height,
		HexitecSensorConfig& sensorConfig, HexitecOperationMode& operationMode, double& frameTime, uint32_t& collectDcTime) {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	result = disableSyncMode();
	if (result == NO_ERROR) {
		result = disableTriggerMode();
	}
	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x56;

	if (setHv) {
		hexToString(hvDacValFromHv(hvSetPoint), 4, &txBuffer[3]);
		txBuffer[7] = 0x0d;
		result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
	} else {
		txBuffer[3] = 0x0d;
		result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
	}

	hvSetPoint = hvDacValToHv((uint16_t) stringToHex(&rxBuffer[2], 4));
	if (result == NO_ERROR) {
		result = readResolution(width, height);
	}
	if (result == NO_ERROR) {
		result = getSensorConfig(sensorConfig);
	}
	if (result == NO_ERROR) {
		result = getOperationMode(operationMode);
	}
	if (result == NO_ERROR) {
		frameTime = getFrameTime(width, height);
		collectDcTime = getCollectDcTime(frameTime);
	}
	return result;
}

int32_t HexitecApi::openStream() {
	return gigeDevice->OpenStream(true, true);
}

int32_t HexitecApi::readEnvironmentValues(double& humidity, double& ambientTemperature, double& asicTemperature,
		double& adcTemperature, double& ntcTemperature) {
	uint8_t txBuffer[4];
	uint8_t rxBuffer[27];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x52;
	txBuffer[3] = 0x0d;
	result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);

	ambientTemperature = getAmbientTemperature((uint16_t) stringToHex(&rxBuffer[2], 4));
	humidity = getHumidity((uint16_t) stringToHex(&rxBuffer[6], 4));
	asicTemperature = getAsicTemperature((uint16_t) stringToHex(&rxBuffer[10], 4));
	adcTemperature = getAdcTemperature((uint16_t) stringToHex(&rxBuffer[14], 4));
	ntcTemperature = temperatureDacValToTemperature((uint16_t) stringToHex(&rxBuffer[22], 4),
			getInternalReference((uint16_t) stringToHex(&rxBuffer[18], 4)));
	return result;
}

int32_t HexitecApi::readOperatingValues(double& v3_3, double& hvMon, double& hvOut, double& v1_2, double& v1_8, double& v3,
		double& v2_5, double& v3_3ln, double& v1_65ln, double& v1_8ana, double& v3_8ana, double& peltierCurrent,
		double& ntcTemperature) {
	uint8_t txBuffer[4];
	uint8_t rxBuffer[51];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x50;
	txBuffer[3] = 0x0d;
	result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);

	v3_3 = getInternalReference((uint16_t) stringToHex(&rxBuffer[38], 4));
	hvMon = getVoltage(stringToHex(&rxBuffer[2], 4), v3_3);
	hvOut = getHvOut(hvMon);
	v1_2 = getVoltage(stringToHex(&rxBuffer[6], 4), v3_3);
	v1_8 = getVoltage(stringToHex(&rxBuffer[10], 4), v3_3);
	v3 = getVoltage(stringToHex(&rxBuffer[14], 4), v3_3);
	v2_5 = getVoltage(stringToHex(&rxBuffer[18], 4), v3_3);
	v3_3ln = getVoltage(stringToHex(&rxBuffer[22], 4), v3_3);
	v1_65ln = getVoltage(stringToHex(&rxBuffer[26], 4), v3_3);
	v1_8ana = getVoltage(stringToHex(&rxBuffer[30], 4), v3_3);
	v3_8ana = getVoltage(stringToHex(&rxBuffer[34], 4), v3_3);
	peltierCurrent = getPeltierCurrent(getVoltage(stringToHex(&rxBuffer[42], 4), v3_3));
	ntcTemperature = temperatureDacValToTemperature((uint16_t) stringToHex(&rxBuffer[46], 4), v3_3);

	return result;
}

int32_t HexitecApi::readRegister(uint8_t registerAddress, uint8_t& value) {
	uint8_t txBuffer[6];
	uint8_t rxBuffer[128];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x41;
	hexToString(registerAddress, 2, &txBuffer[3]);
	txBuffer[5] = 0x0d;
	result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
	value = (uint8_t) stringToHex(&rxBuffer[2], 2);
	return result;
}

int32_t HexitecApi::readResolution(uint8_t& width, uint8_t& height) {
	int32_t result = NO_ERROR;

	result = readRegister(0x83, width);
	width = width * 4;
	if (result == NO_ERROR) {
		result = readRegister(0x84, height);
	}
	return result;
}

int32_t HexitecApi::returnBuffer(uint8_t* transferBuffer) {
	gigeDevice->ReturnBuffer(transferBuffer);
	return NO_ERROR;
}

int32_t HexitecApi::serialPortWriteRead(const uint8_t* txBuffer, uint32_t txBufferSize, uint32_t& bytesWritten, uint8_t* rxBuffer,
		uint32_t rxBufferSize, uint32_t& bytesRead) {
	int32_t result = NO_ERROR;

	mutexLock.lock();
	if (txBufferSize) {
		result = gigeDevice->FlushRxBuffer();
		if (result == NO_ERROR) {
			result = gigeDevice->WriteSerialPort(const_cast<uint8_t*>(txBuffer), txBufferSize, &bytesWritten);
		}
	}
	if ((result == NO_ERROR) && rxBufferSize) {
		result = gigeDevice->ReadSerialPort(rxBuffer, rxBufferSize, &bytesRead, m_timeout);
	}
	mutexLock.unlock();
	return result;
}

int32_t HexitecApi::setDAC(double& vCal, double& uMid, double& hvSetPoint, double& detCtrl, double& targetTemperature) {
	uint8_t txBuffer[24];
	uint8_t rxBuffer[23];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	result = checkTemperatureLimit(targetTemperature);
	if (result == NO_ERROR) {
		txBuffer[0] = 0x23;
		txBuffer[1] = MODULE_ADDRESS;
		txBuffer[2] = 0x54;
		hexToString((uint32_t) dacValFromVoltage(vCal), 4, &txBuffer[3]);
		hexToString((uint32_t) dacValFromVoltage(uMid), 4, &txBuffer[7]);
		hexToString((uint32_t) hvDacValFromHv(hvSetPoint), 4, &txBuffer[11]);
		hexToString((uint32_t) dacValFromVoltage(detCtrl), 4, &txBuffer[15]);
		hexToString((uint32_t) temperatureDacValFromTemperature(targetTemperature), 4, &txBuffer[19]);
		txBuffer[23] = 0x0d;
		result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);

		vCal = dacValToVoltage((uint16_t) stringToHex(&rxBuffer[2], 4));
		uMid = dacValToVoltage((uint16_t) stringToHex(&rxBuffer[6], 4));
		hvSetPoint = hvDacValToHv((uint16_t) stringToHex(&rxBuffer[10], 4));
		detCtrl = dacValToVoltage((uint16_t) stringToHex(&rxBuffer[14], 4));
		targetTemperature = temperatureDacValToTemperature((uint16_t) stringToHex(&rxBuffer[18], 4), DAC_REF_VOLTAGE);
	}
	return result;
}

int32_t HexitecApi::setFrameFormatControl(const std::string& pixelFormat, uint64_t width, uint64_t height, uint64_t offsetX,
		uint64_t offsetY, const std::string& sensorTaps, const std::string& testPattern) {
	return gigeDevice->SetImageFormatControl(const_cast<char*>(pixelFormat.c_str()), width, height, offsetX, offsetY,
			const_cast<char*>(sensorTaps.c_str()), const_cast<char*>(testPattern.c_str()));
}

int32_t HexitecApi::setFrameTimeOut(uint32_t frameTimeOut)
{
	gigeDevice->SetFrameTimeOut(frameTimeOut);
	return NO_ERROR;
}

int32_t HexitecApi::setTriggeredFrameCount(uint32_t frameCount) {
	uint8_t txBuffer[14];
	uint8_t rxBuffer[19];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x44;
	txBuffer[3] = 0x32;
	txBuffer[4] = 0x41;
	hexToStringLE( frameCount, 8, &txBuffer[5]);
	txBuffer[13] = 0x0d;

	return serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
}

int32_t HexitecApi::stopAcquisition() {
	gigeDevice->StopAcquisition();
	return NO_ERROR;
}

int32_t HexitecApi::uploadOffsetValues(Reg2Byte* offsetValues, uint32_t offsetValuesLength) {
	int32_t result = NO_ERROR;
	uint8_t value = 0;
	HexitecOperationMode currentMode;
	HexitecOperationMode uploadMode;
	uint32_t valuesToTransmit = offsetValuesLength * 2;
	FpgaRegisterVector fpgaRegisterStream;
	uint32_t i = 0;
	uint32_t regsWritten = 0;

	if (result == NO_ERROR) {
		result = getOperationMode(currentMode);
	}
	if (result == NO_ERROR) {
		uploadMode = currentMode;
		uploadMode.DcCollectDarkCorrectionValues = Control::CONTROL_DISABLED;
		uploadMode.DcUploadDarkCorrectionValues = Control::CONTROL_ENABLED;
		result = setOperationMode(uploadMode);
	}
	while ((result == NO_ERROR) && valuesToTransmit) {
		if (valuesToTransmit < HEXITEC_MAX_STREAM_REGISTER_COUNT) {
			fpgaRegisterStream.resize(valuesToTransmit);
		} else {
			fpgaRegisterStream.resize(HEXITEC_MAX_STREAM_REGISTER_COUNT);
		}
		for (i = 0; i < fpgaRegisterStream.size(); i++) {
			fpgaRegisterStream[i].address = (uint8_t)(regsWritten % 2) + 0x25;
			fpgaRegisterStream[i].value = offsetValues[regsWritten / 2].size1[regsWritten % 2];
			regsWritten++;
		}
		result = writeRegisterStream(fpgaRegisterStream);
		valuesToTransmit = valuesToTransmit - (uint32_t) fpgaRegisterStream.size();
	}
	if (result == NO_ERROR) {
		result = setOperationMode(currentMode);
	}
	return result;
}

int32_t HexitecApi::writeAdcRegister(uint8_t registerAddress, uint8_t& value) {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[7];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x53;
	hexToString(registerAddress, 2, &txBuffer[3]);
	hexToString(value, 2, &txBuffer[5]);
	txBuffer[7] = 0x0d;
	result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
	value = (uint8_t) stringToHex(&rxBuffer[4], 2);
	return result;
}

int32_t HexitecApi::writeRegister(uint8_t registerAddress, uint8_t& value) {
	uint8_t txBuffer[8];
	uint8_t rxBuffer[128];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x40;
	hexToString(registerAddress, 2, &txBuffer[3]);
	hexToString(value, 2, &txBuffer[5]);
	txBuffer[7] = 0x0d;
	result = serialPortWriteRead(txBuffer,sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);
	value = (uint8_t) stringToHex(&rxBuffer[4], 2);
	return result;
}

int32_t HexitecApi::writeRegisterStream(FpgaRegisterVector &registerStream) {
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;
	uint32_t value = 0;
	uint32_t index = 5;

	int rsz = (registerStream.size() * 4) + 3;
	uint8_t rxBuffer[rsz];
	int tsz = (registerStream.size() * 4 ) + 6;
	uint8_t txBuffer[tsz];

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x46;
	hexToString(registerStream.size(), 2, &txBuffer[3]);
	for (auto i = 0; i < registerStream.size(); i++) {
		hexToString(registerStream[i].address, 2, &txBuffer[index]);
		index += 2;
		hexToString(registerStream[i].value, 2, &txBuffer[index]);
		index += 2;
	}
	txBuffer[index] = 0x0d;
	result = serialPortWriteRead(txBuffer, tsz, bytesWritten, rxBuffer, rsz, bytesRead);
	if (bytesRead >= rsz) {
		int j = 4;
		for (auto i = 0; i < registerStream.size(); i++, j+=4) {
			registerStream[i].value = (uint8_t) stringToHex(&rxBuffer[j], 2);
		}
	}
	return result;
}

int32_t HexitecApi::checkTemperatureLimit(double& temperature) {
	uint8_t txBuffer[4];
	uint8_t rxBuffer[83];
	uint32_t bytesWritten = 0;
	uint32_t bytesRead = 0;
	int32_t result = NO_ERROR;
	uint32_t detectorType = 0xff;

	txBuffer[0] = 0x23;
	txBuffer[1] = MODULE_ADDRESS;
	txBuffer[2] = 0x70;
	txBuffer[3] = 0x0d;
	result = serialPortWriteRead(txBuffer, sizeof(txBuffer), bytesWritten, rxBuffer, sizeof(rxBuffer), bytesRead);

	detectorType = stringToHex(&rxBuffer[2], 4);
	switch (detectorType)
	{
		case 0x00:
			if( temperature < HEXITEC_TARGET_TEMPERATURE_LL_0x00) {
				temperature = HEXITEC_TARGET_TEMPERATURE_LL_0x00;
			}
			break;
		default:
			if (temperature < HEXITEC_TARGET_TEMPERATURE_LL) {
				temperature = HEXITEC_TARGET_TEMPERATURE_LL;
			}
			break;
	}
	if (temperature > HEXITEC_TARGET_TEMPERATURE_UL) {
		temperature = HEXITEC_TARGET_TEMPERATURE_UL;
	}
	return result;
}

int32_t HexitecApi::setOperationMode(HexitecOperationMode operationMode) {
	uint8_t value = 0;
	int32_t result = NO_ERROR;

	result = disableSM();
	if (result == NO_ERROR) {
        value = operationMode.DcUploadDarkCorrectionValues;
        value = value + (operationMode.DcCollectDarkCorrectionValues * 0x02);
        value = value + (operationMode.DcEnableDarkCorrectionCountingMode * 0x04);
        value = value + (operationMode.DcEnableDarkCorrectionSpectroscopicMode * 0x08);
        value = value + (operationMode.DcSendDarkCorrectionValues * 0x10);
        value = value + (operationMode.DcDisableVcalPulse * 0x20);
        value = value + (operationMode.DcTestMode * 0x40);
        value = value + (operationMode.DcEnableTriggeredCountingMode * 0x80);
        result = writeRegister(0x24, value);
	}
	if (result == NO_ERROR) {
		value = operationMode.EdUploadThresholdValues;
		value = value + (operationMode.EdDisableCountingMode * 0x02);
		value = value + (operationMode.EdTestMode * 0x04);
		result = writeRegister(0x27, value);
	}
	if (result == NO_ERROR) {
		value = operationMode.EdCycles.size1[0];
		result = writeRegister(0x28, value);
	}
	if (result == NO_ERROR) {
		value = operationMode.EdCycles.size1[1];
		result = writeRegister(0x29, value);
	}
    if (result == NO_ERROR) {
        result = enableSM();
    }
	return result;
}

int32_t HexitecApi::setSensorConfig(HexitecSensorConfig sensorConfig) {
	uint8_t value = 0;
	int32_t result = NO_ERROR;
//	uint8_t i = 0;
	uint8_t setupReg = HEXITEC_SETUP_REGISTER_START_ADDRESS;

	value = sensorConfig.Gain;
	result = writeRegister(0x06, value);
	if (result == NO_ERROR) {
		value = sensorConfig.Row_S1.size1[0];
		result = writeRegister(0x02, value);
	}
	if (result == NO_ERROR) {
		value = sensorConfig.Row_S1.size1[1];
		result = writeRegister(0x03, value);
	}
	if (result == NO_ERROR) {
		value = sensorConfig.S1_Sph;
		result = writeRegister(0x04, value);
	}
	if (result == NO_ERROR) {
		value = sensorConfig.Sph_S2;
		result = writeRegister(0x05, value);
	}
	if (result == NO_ERROR) {
		value = sensorConfig.Vcal2_Vcal1.size1[0];
		result = writeRegister(0x18, value);
	}
	if (result == NO_ERROR) {
		value = sensorConfig.Vcal2_Vcal1.size1[1];
		result = writeRegister(0x19, value);
	}
	if (result == NO_ERROR) {
		value = 0x00;
		result = writeRegister(0x20, value);
	}
	if (result == NO_ERROR) {
		value = 0x00;
		result = writeRegister(0x21, value);
	}
	if (result == NO_ERROR) {
		value = 0x00;
		result = writeRegister(0x22, value);
	}
	if (result == NO_ERROR) {
		value = 0x00;
		result = writeRegister(0x23, value);
	}
	if (result == NO_ERROR) {
		value = sensorConfig.WaitClockCol;
		result = writeRegister(0x1a, value);
	}
	if (result == NO_ERROR) {
		value = sensorConfig.WaitClockRow;
		result = writeRegister(0x1b, value);
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			value = sensorConfig.SetupRow.PowerEn[i];
			result = writeRegister(setupReg, value);
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			value = sensorConfig.SetupRow.CalEn[i];
			result = writeRegister(setupReg, value);
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			value = sensorConfig.SetupRow.ReadEn[i];
			result = writeRegister(setupReg, value);
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			value = sensorConfig.SetupCol.PowerEn[i];
			result = writeRegister(setupReg, value);
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			value = sensorConfig.SetupCol.CalEn[i];
			result = writeRegister(setupReg, value);
		}
		setupReg++;
	}
	for (auto i = 0; i < HEXITEC_SETUP_REGISTER_SIZE; i++) {
		if (result == NO_ERROR) {
			value = sensorConfig.SetupCol.ReadEn[i];
			result = writeRegister(setupReg, value);
		}
		setupReg++;
	}
	return result;
}

//------------------------------------------------------------------------------
// private helper methods
//------------------------------------------------------------------------------
uint16_t HexitecApi::dacValFromVoltage(double voltage) {
	double lVoltage = voltage;
	double dacStepVoltage = DAC_REF_VOLTAGE / 4095;

	if (lVoltage < 0) {
		lVoltage = 0;
	} else if (lVoltage > DAC_REF_VOLTAGE) {
		lVoltage = DAC_REF_VOLTAGE;
	}
	return (uint16_t)((lVoltage / dacStepVoltage) + 0.5);
}

double HexitecApi::dacValToVoltage(uint16_t number) {
	double dacStepVoltage = DAC_REF_VOLTAGE / 4095;
	return dacStepVoltage * (double) number;
}

double HexitecApi::getAdcTemperature(uint16_t number) {
	return (double) number * 0.0625;
}

double HexitecApi::getAmbientTemperature(uint16_t number) {
	return (((double) number * 175.72) / 65535) - 46.85;
}

double HexitecApi::getAsicTemperature(uint16_t number) {
	return (double) number * 0.0625;
}

uint32_t HexitecApi::getCollectDcTime(double aFrameTime) {
	return (uint32_t)((aFrameTime * 1000 * HEXITEC_DARK_CORRECTION_FRAME_COUNT) + 1000);
}

double HexitecApi::getHumidity(uint16_t number) {
	return (((double) number * 125) / 65535) - 6;
}

double HexitecApi::getHvOut(double hvMon) {
	return (hvMon * 1621.65) - 1043.22;
}

double HexitecApi::getInternalReference(uint16_t number) {
	return (INTERNAL_REF_VOLTAGE / 4095) * (double) number;
}

double HexitecApi::getPeltierCurrent(double voltage) {
	return (voltage - 1.5) / 0.2;
}

double HexitecApi::getVoltage(uint16_t number, double internalReference) {
	return (internalReference / 4095) * (double) number;
}

int32_t HexitecApi::hexToString(uint32_t number, uint8_t digits, uint8_t* ptr) {
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::setw(digits)<< std::uppercase << number;
	std::string s = ss.str();
	for (auto i=0; i<digits; i++)
		*ptr++ = s[i];
	return NO_ERROR;
}

int32_t HexitecApi::hexToStringLE(uint32_t number, uint8_t digits, uint8_t* ptr )
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::setw(digits)<< std::uppercase << number;
	std::string s = ss.str();
	for (auto i=0; i<digits; i+=2) {
		*ptr++ = s[digits-i-2];
		*ptr++ = s[digits-i-1];
	}
	return NO_ERROR;
}

uint16_t HexitecApi::hvDacValFromHv(double hv) {
	double hvConverted = hv / -250;
	double dacStepVoltage = DAC_REF_VOLTAGE / 4095;

	if (hvConverted < 0) {
		hvConverted = 0;
	} else if (hvConverted > DAC_REF_VOLTAGE) {
		hvConverted = DAC_REF_VOLTAGE;
	}
	return (uint16_t)((hvConverted / dacStepVoltage) + 0.5);
}

double HexitecApi::hvDacValToHv(uint16_t number) {
	double dacStepVoltage = DAC_REF_VOLTAGE / 4095;
	return dacStepVoltage * (double) number * -250;
}

uint32_t HexitecApi::stringToHex(const uint8_t* source, uint8_t digits) {
	std::stringstream ss, ss2;
	uint32_t number;
	ss << source;
	ss2 << ss.str().substr(0,digits);
	ss2 >> std::hex >> number;
	return number;
}

uint16_t HexitecApi::temperatureDacValFromTemperature(double temperature) {
	double temperatureConvertedExp = exp(((1 / ((double) temperature + 273.15)) - (1 / 298.15)) * 3988);
	double temperatureConverted = (temperatureConvertedExp * 3) / (temperatureConvertedExp + 1);
	double dacStepVoltage = DAC_REF_VOLTAGE / 4095;
	if (temperatureConverted < 0) {
		temperatureConverted = 0;
	} else if (temperatureConverted > DAC_REF_VOLTAGE) {
		temperatureConverted = DAC_REF_VOLTAGE;
	}
	return (uint16_t)((temperatureConverted / dacStepVoltage) + 0.5);
}

double HexitecApi::temperatureDacValToTemperature(uint16_t number, double internalReference) {
	double dacVoltage = (internalReference / 4095) * (double) number;
	double term_ln = log(dacVoltage / (3 - dacVoltage));
	return (1 / ((term_ln / 3988) + (1 / 298.15))) - 273.15;
}

HexitecApi::HexitecArmedCb::HexitecArmedCb(HexitecApi& api) : m_api(api) {}
HexitecApi::HexitecArmedCb::~HexitecArmedCb() {}
void HexitecApi::HexitecArmedCb::armed() {
	m_api.enableTriggerGate();
}

HexitecApi::HexitecFinishCb::HexitecFinishCb(HexitecApi& api) : m_api(api) {}
HexitecApi::HexitecFinishCb::~HexitecFinishCb() {}
void HexitecApi::HexitecFinishCb::finished() {
	m_api.disableTriggerGate();
}

