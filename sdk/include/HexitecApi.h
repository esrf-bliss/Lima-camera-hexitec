#ifndef HEXITEC_API_H
#define HEXITEC_API_H

#ifdef WIN32
	#ifdef HEXITEC_EXPORTS
		#define __declspec(dllexport)
	#else
		#define __declspec(dllimport)
	#endif
	#define HEXITEC_CDECL __cdecl
#else  /* Unix */
	#define HEXITEC_API
	#define HEXITEC_CDECL
#endif

#include <cstdint>
#include <pthread.h>
#include <GigE.h>

namespace HexitecAPI {

const int NO_ERROR = 0;
const int OPENFILE_ERR = 0xC;
const uint8_t MODULE_ADDRESS = 0x90;
const double DAC_REF_VOLTAGE = 3.0;
const double INTERNAL_REF_VOLTAGE = 2.048;

const double HEXITEC_CLOCK_SPEED = 42500000.0;
const int HEXITEC_SETUP_REGISTER_SIZE = 10;
const uint8_t HEXITEC_SETUP_REGISTER_START_ADDRESS = 0x2f;
const int HEXITEC_DARK_CORRECTION_FRAME_COUNT = 8192;
const int HEXITEC_MAX_STREAM_REGISTER_COUNT = 60;
const int HEXITEC_TARGET_TEMPERATURE_LL = 10;
const int HEXITEC_TARGET_TEMPERATURE_UL = 40;
const int COLLECT_DC_NOT_READY = 0x20;

const uint8_t FPGA_FW_CHECK_CUSTOMER_REG = 0x80;
const uint8_t FPGA_FW_CHECK_PROJECT_REG = 0x81;
const uint8_t FPGA_FW_CHECK_VERSION_REG = 0x82;

const int FPGA_FW_CHECK_CUSTOMER_ERROR = 0x8;
const int FPGA_FW_CHECK_PROJECT_ERROR = 0x9;
const int FPGA_FW_CHECK_VERSION_ERROR = 0xA;

enum Control : uint8_t {
	CONTROL_DISABLED = 0,
	CONTROL_ENABLED = 1
};
typedef enum Control Control;

class FpgaRegister {
public:
	uint8_t address;
	uint8_t value;
};

typedef std::vector<FpgaRegister>FpgaRegisterVector; //, *p_FpgaRegister_vec;

enum HexitecGain : uint8_t {
	HEXITEC_GAIN_HIGH = 0, HEXITEC_GAIN_LOW = 1
};
typedef enum HexitecGain HexitecGain;

enum HexitecAdcSample : uint8_t {
	HEXITEC_ADC_SAMPLE_RISING_EDGE = 0, HEXITEC_ADC_SAMPLE_FALLING_EDGE = 1
};
typedef enum HexitecAdcSample HexitecAdcSample;

union Reg2Byte {
	uint8_t size1[2];
	uint16_t size2;
};
typedef union Reg2Byte Reg2Byte;

union Reg4Byte {
	uint8_t size1[4];
	uint16_t size2[2];
	uint32_t size4;
};
typedef union Reg4Byte Reg4Byte;

class HexitecSystemConfig {
public:
	uint8_t				AdcDelay;
	uint8_t				SyncSignalDelay;
	HexitecAdcSample	AdcSample;
	uint8_t				AdcClockPhase;
	double				Umid;
	double				DetCtrl;
	double				TargetTemperature;
};

class HexitecBiasConfig {
public:
	double    BiasVoltage;
	double    RefreshVoltage;
};

class HexitecSetupRegister {
public:
	uint8_t ReadEn[HEXITEC_SETUP_REGISTER_SIZE];
	uint8_t PowerEn[HEXITEC_SETUP_REGISTER_SIZE];
	uint8_t CalEn[HEXITEC_SETUP_REGISTER_SIZE];
};

class HexitecSensorConfig {
public:
	HexitecGain				Gain;
	Reg2Byte				Row_S1;
	uint8_t					S1_Sph;
	uint8_t					Sph_S2;
	Reg2Byte				Vcal2_Vcal1;
	double					Vcal;
	uint8_t					WaitClockCol;
	uint8_t					WaitClockRow;
	HexitecSetupRegister	SetupCol;
	HexitecSetupRegister	SetupRow;
};

class HexitecOperationMode {
public:
	Control DcUploadDarkCorrectionValues;
	Control DcCollectDarkCorrectionValues;
	Control DcEnableDarkCorrectionCountingMode;
	Control DcEnableDarkCorrectionSpectroscopicMode;
	Control DcSendDarkCorrectionValues;
	Control DcDisableVcalPulse;
	Control DcTestMode;
	Control DcEnableTriggeredCountingMode;
	Control EdUploadThresholdValues;
	Control EdDisableCountingMode;
	Control EdTestMode;
	Reg2Byte EdCycles;
	Control enSyncMode;
	Control enTriggerMode;
};

class HexitecApi
{
public:
	HexitecApi(const std::string deviceDescriptor, uint32_t timeout);
	~HexitecApi();
	int32_t readConfiguration(std::string fname);
	int32_t createPipelineOnly(uint32_t bufferCount);
	std::string getErrorDescription();
	int32_t getFramesAcquired();
	int32_t startAcquisition();
	int32_t stopAcquisition();
	int32_t setHvBiasOn(bool onOff);
	int32_t retrieveBuffer(uint8_t* buffer, uint32_t frametimeout);
	int32_t acquireFrame(uint32_t& frameCount, uint8_t* buffer, uint32_t frametimeout);
	int32_t acquireFrames(uint32_t frameCount, uint64_t& framesAcquired, uint32_t frametimeout);
	int32_t checkFirmware(uint8_t& customerId, uint8_t& projectId, uint8_t& version, uint8_t forceEqualVersion);
	int32_t closePipeline();
	int32_t closeSerialPort();
	int32_t closeStream();
	int32_t collectOffsetValues(uint32_t collectDctimeout);
	int32_t configureDetector(uint8_t& width, uint8_t& height,double& frameTime, uint32_t& collectDcTime);
	void    copyBuffer(uint8_t* sourceBuffer, uint8_t* destBuffer, uint32_t byteCount);
	int32_t createPipeline(uint32_t bufferCount, uint32_t transferBufferCount, uint32_t transferBufferFrameCount);
//	int32_t getErrorMsg(int32_t error, char* errorMsg, uint32_t length);
	int32_t getDeviceInformation(GigE::GigEDeviceInfoStr& deviceInfoStr);
	double  getFrameTime(uint8_t width, uint8_t height);
	int32_t getIntegerValue(const std::string propertyName, int64_t &value);
	int32_t getLastResult(uint32_t& internalErrorCode, std::string errorCodeString, std::string errorDescription);
	int32_t getSensorConfig(HexitecSensorConfig& sensorConfig);
	int32_t getTriggerState(uint8_t& trigger1, uint8_t& trigger2, uint8_t& trigger3);
//	int32_t getSystemErrorMsg(int32_t sysError, char* sysErrorMsg, uint32_t length);
	int32_t initDevice(uint32_t& internalErrorCode, std::string& errorCodeString, std::string& errorDescription);
	int32_t initFwDefaults(uint8_t setHv, double& hvSetPoint, uint8_t width, uint8_t height,
			HexitecSensorConfig& sensorConfig, HexitecOperationMode& operationMode, double& frameTime, uint32_t& collectDcTime);
	int32_t openSerialPort(PvDeviceSerial serialPort, uint32_t rxBufferSize, uint8_t useTermChar, uint8_t termChar);
	int32_t openStream();
	int32_t readEnvironmentValues(double& humidity,double& ambientTemperature,double& asicTemperature,
			double& adcTemperature,double& ntcTemperature);
	int32_t readOperatingValues(double& v3_3,double& hvMon,double& hvOut,double& v1_2,double& v1_8,double& v3,
			double& v2_5,double& v3_3ln,double& v1_65ln,double& v1_8ana,double& v3_8ana,double& peltierCurrent,
			double& ntcTemperature);
	int32_t registerTransferBufferReadyCallBack(GigE::p_bufferCallBack transferBufferReadyCallBack);
	int32_t returnBuffer(uint8_t* transferBuffer);
	int32_t setDAC(double& vCal,double& uMid,double& hvSetPoint,double& detCtrl,double& targetTemperature);
	int32_t setFrameFormatControl(const std::string pixelFormat, uint64_t width, uint64_t height, uint64_t offsetX,
			uint64_t offsetY, const std::string sensorTaps, const std::string testPattern);
	int32_t setSensorConfig(HexitecSensorConfig sensorConfig);
//	int32_t stopAcquisition();
	int32_t setTriggeredFrameCount(uint32_t frameCount);
	int32_t uploadOffsetValues(Reg2Byte* offsetValues, uint32_t offsetValuesLength);

private:
	GigE::GigEDevice *gigeDevice;
	std::string m_deviceDescriptor;
	pthread_mutex_t mutexLock;
	uint32_t m_timeout;
	HexitecSensorConfig m_sensorConfig;
	HexitecOperationMode m_operationMode;
	HexitecSystemConfig m_systemConfig;
	HexitecBiasConfig m_biasConfig;

	int32_t disableSM();
	int32_t disableSyncMode();
	int32_t disableTriggerMode();
	int32_t enableFunctionBlocks(Control adcEnable, Control dacEnable, Control peltierEnable);
	int32_t enableSM();
	int32_t enableSyncMode();
	int32_t enableTriggerMode();
	int32_t getOperationMode(HexitecOperationMode& operationMode);
	int32_t readRegister(uint8_t registerAddress, uint8_t& value);
	int32_t readResolution(uint8_t& width, uint8_t& height);
	int32_t serialPortWriteRead(const uint8_t* txBuffer, uint32_t txBufferSize, uint32_t& bytesWritten, uint8_t* rxBuffer,
			uint32_t rxBufferSize, uint32_t& bytesRead);
	int32_t setOperationMode(HexitecOperationMode operationMode);
	int32_t writeAdcRegister(uint8_t registerAddress, uint8_t& value);
	int32_t writeRegister(uint8_t registerAddress, uint8_t& value);
	int32_t writeRegisterStream(FpgaRegisterVector &registerStream);

	// helper functions
	uint16_t dacValFromVoltage(double voltage);
	double   dacValToVoltage(uint16_t number);
	double   getAdcTemperature(uint16_t number);
	double   getAmbientTemperature(uint16_t number);
	double   getAsicTemperature(uint16_t number);
	uint32_t getCollectDcTime(double frameTime);
	double   getHumidity(uint16_t number);
	double   getHvOut(double hvMon);
	double   getInternalReference(uint16_t number);
	double   getPeltierCurrent(double voltage);
	double   getVoltage(uint16_t number, double internalReference);
	int32_t  hexToString(uint32_t number, uint8_t digits, uint8_t* ptr);
	int32_t  hexToStringLE(uint32_t number, uint8_t digits, uint8_t* ptr);
	uint16_t hvDacValFromHv(double hv);
	double	 hvDacValToHv(uint16_t number);
	uint32_t stringToHex(const uint8_t* source, uint8_t digits);
	uint16_t temperatureDacValFromTemperature(double temperature);
	double   temperatureDacValToTemperature(uint16_t number, double internalReference);
};

} // end namespace HexitecAPI
#endif // HEXITEC_API_H
