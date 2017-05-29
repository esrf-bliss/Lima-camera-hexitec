// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GIGE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GIGE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GIGE_EXPORTS
#define GIGE_API __declspec(dllexport)
#else
#define GIGE_API __declspec(dllimport)
#endif

//#include <PvSampleUtils.h>
#include <PvSystem.h>
//#include <PvInterface.h>
#include <PvDevice.h>
#include <PvDeviceGEV.h>
#include <PvDeviceU3V.h>
#include <PvStream.h>
#include <PvStreamGEV.h>
#include <PvStreamU3V.h>
#include <PvPipeline.h>
#include <PvBuffer.h>
#include <PvDeviceSerialPort.h>
#include <PvDeviceAdapter.h>
#include <queue>
#include <new>
#include <math.h>

#define AS_MODULE_ADDRESS (u8)0x90
#define AS_DAC_REF_VOLTAGE (dbl)3
#define AS_INTERNAL_REF_VOLTAGE (dbl)2.048
#define AS_HEXITEC_CLOCK_SPEED (dbl)42500000
#define AS_HEXITEC_SETUP_REGISTER_SIZE 10
#define AS_HEXITEC_SETUP_REGISTER_START_ADDRESS (u8)0x2f
#define AS_HEXITEC_DARK_CORRECTION_FRAME_COUNT 8192
#define AS_HEXITEC_MAX_STREAM_REGISTER_COUNT 60
#define AS_HEXITEC_TARGET_TEMPERATURE_LL 10
#define AS_HEXITEC_TARGET_TEMPERATURE_LL_0x00 -10
#define AS_HEXITEC_TARGET_TEMPERATURE_UL 40
#define AS_HEXITEC_SERIAL_TIMEOUT 1000
#define AS_HEXITEC_FRAME_TIMEOUT_MULTIPLIER (dbl)2.5
#define AS_HEXITEC_MIN_FRAME_TIMEOUT (u32)25

using namespace aS;

typedef void (__cdecl *p_readyCallBack)( hdl deviceHdl);
typedef void (__cdecl *p_bufferCallBack)( p_u8 transferBuffer, u32 frameCount );

typedef enum HexitecGain : u8 {
	AS_HEXITEC_GAIN_HIGH		= 0,
	AS_HEXITEC_GAIN_LOW			= 1
	} HexitecGain;

typedef enum HexitecAdcSample : u8 {
	AS_HEXITEC_ADC_SAMPLE_RISING_EDGE	= 0,
	AS_HEXITEC_ADC_SAMPLE_FALLING_EDGE	= 1
	} HexitecAdcSample;

typedef union Reg2Byte {
	u8 size1[2];
	u16 size2;
	} Reg2Byte, *Reg2BytePtr, **Reg2ByteHdl;

typedef union Reg4Byte {
	u8 size1[4];
	u16 size2[2];
	u32 size4;
	} Reg4Byte, *Reg4BytePtr, **Reg4ByteHdl;

typedef struct GigEDeviceInfo {
	PvString	Vendor;
	PvString	Model;
	PvString	ManufacturerInfo;
	PvString	SerialNumber;
	PvString	UserId;
	PvString	MacAddress;
	PvString	IpAddress;
	PvString	NetMask;
	PvString	GateWay;
	} GigEDeviceInfo, *GigEDeviceInfoPtr, **GigEDeviceInfoHdl;

#ifndef _M_X64
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct GigEDeviceInfoStr {
	str8		Vendor;
	str8		Model;
	str8		ManufacturerInfo;
	str8		SerialNumber;
	str8		UserId;
	str8		MacAddress;
	str8		IpAddress;
	str8		NetMask;
	str8		GateWay;
	} GigEDeviceInfoStr, *GigEDeviceInfoStrPtr, **GigEDeviceInfoStrHdl;

typedef struct HexitecSystemConfig {
	u8					AdcDelay;
	u8					SyncSignalDelay;
	HexitecAdcSample	AdcSample;
	u8					AdcClockPhase;
	} HexitecSystemConfig, *HexitecSystemConfigPtr, **HexitecSystemConfigHdl;

typedef struct HexitecSetupRegister {
	u8				ReadEn[AS_HEXITEC_SETUP_REGISTER_SIZE];
	u8				PowerEn[AS_HEXITEC_SETUP_REGISTER_SIZE];
	u8				CalEn[AS_HEXITEC_SETUP_REGISTER_SIZE];
	} HexitecSetupRegister, *HexitecSetupRegisterPtr, **HexitecSetupRegisterHdl;

typedef struct HexitecSensorConfig {
	HexitecGain				Gain;
	Reg2Byte				Row_S1;
	u8						S1_Sph;
	u8						Sph_S2;
	Reg2Byte				Vcal2_Vcal1;
	u8						WaitClockCol;
	u8						WaitClockRow;
	HexitecSetupRegister	SetupCol;
	HexitecSetupRegister	SetupRow;
	} HexitecSensorConfig, *HexitecSensorConfigPtr, **HexitecSensorConfigHdl;

typedef struct HexitecOperationMode {
	Control			DcUploadDarkCorrectionValues;
	Control			DcCollectDarkCorrectionValues;
	Control			DcEnableDarkCorrectionCountingMode;
	Control			DcEnableDarkCorrectionSpectroscopicMode;
	Control			DcSendDarkCorrectionValues;
	Control			DcDisableVcalPulse;
	Control			DcTestMode;
	Control			DcEnableTriggeredCountingMode;
	Control			EdUploadThresholdValues;
	Control			EdDisableCountingMode;
	Control			EdTestMode;
	Reg2Byte		EdCycles;
	} HexitecOperationMode, *HexitecOperationModePtr, **HexitecOperationModeHdl;

#ifndef _M_X64
#pragma pack(pop)
#endif

class GigEDevice
{
private:
	PvSystem				cSystem;
	PvResult				cResult;
	PvResult				cAcqResult;
	PvDevice				*cDevice;
	PvGenParameterArray		*cDeviceParams;
	PvGenParameterArray		*cStreamParams;
	PvDeviceInfoType		cDeviceInfoType;
	const PvDeviceInfo		*cDeviceInfo;
	PvDeviceInfoGEV			*cDeviceInfoGEV;
	PvDeviceInfoU3V			*cDeviceInfoU3V;

	GigEDeviceInfo			cDeviceInformation;
	GigEDeviceInfoStr		cDeviceInformationStr;
	
	PvStream				*cStream;
	PvGenCommand			*cStartCmd;
	PvGenCommand			*cStopCmd;
	PvGenCommand			*cResetCmd;
	PvPipeline				*cPipeline;
	i64						cBlocksDroppedVal;
	PvGenInteger			*cBlocksDropped;
	i64						cBlockIDsMissingVal;
	PvGenInteger			*cBlockIDsMissing;

	PvDeviceAdapter			*cDeviceAdapter;
	PvDeviceSerialPort		cPort;
	u8						cUseTermChar;
	u8						cTermChar;
	fb_vec					cTransferBuffer;
	u32						cTransferBufferFrameCount;
	std::queue <p_u8>		cAvailableTransferBuffer;
	CRITICAL_SECTION		cAvailableTransferBufferLock;
	p_readyCallBack			cReadyCallBack;
	p_readyCallBack			cFinishCallBack;
	p_bufferCallBack		cBufferCallBack;
	u64						cAcquiredImages;
	u8						cContinuous;
	u8						cStopAcquisition;
	dbl						cFrameTime;
	u32						cFrameTimeOut;

public:
	GigEDevice(const str8 aDeviceDescriptor);
	~GigEDevice();
	PvResult				GetLastResult();
	i32						OpenSerialPort( PvDeviceSerial SerialPort, u32 RxBufferSize, u8 UseTermChar, u8 TermChar );
	i32						CloseSerialPort();
	i32						FlushRxBuffer();
	i32						ReadSerialPort( p_u8 RxBuffer, u32 RxBufferSize, u32 *BytesRead, u32 TimeOut );
	i32						WriteSerialPort( const p_u8 TxBuffer, u32 TxBufferSize, u32 *BytesWritten );

	i32						OpenStream( bool TimeoutCountedAsError, bool AbortCountedAsError );
	i32						CloseStream();
	
	i32						SetImageFormatControl( const str8 PixelFormat,
												   u64 Width,
												   u64 Height,
												   u64 OffsetX,
												   u64 OffsetY,
												   const str8 SensorTaps,
												   const str8 TestPattern );
	i32						CreatePipeline( u32 BufferCount );
	i32						ClosePipeline();
	i32						AcquireImage( p_u32 ImageCount, p_u8 Buffer, u32 FrameTimeOut );
	i32						AcquireImageThread( u32 ImageCount, u32 FrameTimeOut );

	i32						GetIntegerValue( const str8 Property, i64 &Value );
	i32						GetBufferHandlingThreadPriority();
	void					SetTransferBuffer( u32 TransferBufferCount, u32 TransferBufferFrameCount );
	void					ReturnBuffer( p_u8 Buffer );
	void					RegisterAcqArmedCallBack( p_readyCallBack aAcqArmedCallBack );
	void					RegisterAcqFinishCallBack( p_readyCallBack aAcqFinishCallBack );
	void					RegisterTransferBufferReadyCallBack( p_bufferCallBack aTransferBufferReadyCallBack );
	u64						GetAcquiredImageCount();
	void					StopAcquisition();
	void					SetFrameTime( dbl FrameTime );
	void					SetFrameTimeOut( u32 FrameTimeOut );
	
	static i32				GetErrorDescription( PvResult aPleoraErrorCode,
												 str8 aPleoraErrorCodeString,
												 p_u32 aPleoraErrorCodeStringLen,
												 str8 aPleoraErrorDescription,
												 p_u32 aPleoraErrorDescriptionLen);

	GigEDeviceInfoStr		GetDeviceInfoStr();

private:
	PvResult				Connect(const str8 aDeviceDescriptor);
	PvResult				ConfigureSerialUart( PvDeviceSerial SerialPort );
	PvResult				ConfigureSerialBulk( PvDeviceSerial SerialPort );
	PvResult				ConfigureStream( bool TimeoutCountedAsError, bool AbortCountedAsError );
	void					ClearQueue();
	void					InitializeQueue();
	void					BufferReadyCallBack( p_u8 aTransferBuffer, u32 aCurrentFrameWithinBuffer );
};

EXTERN_C	GIGE_API	i32		AcquireFrame(
												GigEDevice* deviceHdl,
												p_u32 frameCount,
												p_u8 buffer,
												u32 frameTimeOut );

EXTERN_C	GIGE_API	i32		AcquireFrames(
												GigEDevice* deviceHdl,
												u32 frameCount,
												p_u64 framesAcquired,
												u32 frameTimeOut );

EXTERN_C	GIGE_API	i32		CheckFirmware(
												GigEDevice* deviceHdl,
												p_u8 customerId,
												p_u8 projectId,
												p_u8 version,
												u8 forceEqualVersion,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		ClosePipeline(
												GigEDevice* deviceHdl );

EXTERN_C	GIGE_API	i32		CloseSerialPort(
												GigEDevice* deviceHdl );

EXTERN_C	GIGE_API	i32		CloseStream(
												GigEDevice* deviceHdl );

EXTERN_C	GIGE_API	i32		CollectOffsetValues(
												GigEDevice* deviceHdl,
												u32 timeOut,
												u32 collectDcTimeOut );

EXTERN_C	GIGE_API	i32		ConfigureDetector(
												GigEDevice* deviceHdl,
												const HexitecSensorConfigPtr sensorConfig,
												const HexitecOperationModePtr operationMode,
												const HexitecSystemConfigPtr systemConfig,
												p_u8 width,
												p_u8 height,
												p_dbl frameTime,
												p_u32 collectDcTime,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		ConfigureDetectorWithTrigger(
												GigEDevice* deviceHdl,
												const HexitecSensorConfigPtr sensorConfig,
												const HexitecOperationModePtr operationMode,
												const HexitecSystemConfigPtr systemConfig,
												p_u8 width,
												p_u8 height,
												p_dbl frameTime,
												p_u32 collectDcTime,
												u32 timeOut,
												Control enSyncMode,
												Control enTriggerMode );

EXTERN_C	GIGE_API	void	CopyBuffer(
												p_u8 sourceBuffer,
												p_u8 destBuffer,
												u32 byteCount );

EXTERN_C	GIGE_API	i32		CreatePipeline(
												GigEDevice* deviceHdl,
												u32 bufferCount,
												u32 transferBufferCount,
												u32 transferBufferFrameCount );

EXTERN_C	GIGE_API	i32		CreatePipelineOld(
												GigEDevice* deviceHdl,
												u32 bufferCount );

EXTERN_C	GIGE_API	i32		DisableSM(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		DisableSyncMode(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		DisableTriggerGate(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		DisableTriggerMode(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		EnableFunctionBlocks(
												GigEDevice* deviceHdl,
												Control	adcEnable,
												Control	dacEnable,
												Control	peltierEnable,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		EnableSM(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		EnableSyncMode(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		EnableTriggerGate(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		EnableTriggerMode(
												GigEDevice* deviceHdl,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		ExitDevice(
												GigEDevice* deviceHdl );

EXTERN_C	GIGE_API	i32		GetAsErrorMsg(
												i32 asError,
												str8 asErrorMsg,
												u32 length );

EXTERN_C	GIGE_API	i32		GetBufferHandlingThreadPriority(
												GigEDevice* deviceHdl,
												p_i32 priority );

EXTERN_C	GIGE_API	i32		GetDeviceInformation(
												GigEDevice* deviceHdl,
												GigEDeviceInfoStrPtr deviceInfoStr );

EXTERN_C	GIGE_API	dbl		GetFrameTime(
												const HexitecSensorConfigPtr sensorConfig,
												u8 width,
												u8 height );

EXTERN_C	GIGE_API	i32		GetIntegerValue(
												GigEDevice* deviceHdl,
												const str8 propertyName,
												i64 &value );

EXTERN_C	GIGE_API	i32		GetLastResult(
												GigEDevice* deviceHdl,
												p_u32 pleoraErrorCode,
												str8 pleoraErrorCodeString,
												p_u32 pleoraErrorCodeStringLen,
												str8 pleoraErrorDescription,
												p_u32 pleoraErrorDescriptionLen );

EXTERN_C	GIGE_API	i32		GetOperationMode(
												GigEDevice* deviceHdl,
												HexitecOperationModePtr operationMode,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		GetSensorConfig(
												GigEDevice* deviceHdl,
												HexitecSensorConfigPtr sensorConfig,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		GetSystemErrorMsg(
												i32 sysError,
												str8 sysErrorMsg,
												u32 length );

EXTERN_C	GIGE_API	i32		GetTriggerState(
												GigEDevice* deviceHdl,
												p_u8 trigger1,
												p_u8 trigger2,
												p_u8 trigger3,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		InitDevice(
												GigEDevice** deviceHdl,
												const str8 deviceDescriptor,
												p_u32 internalResult,
												str8 pleoraErrorCodeString,
												p_u32 pleoraErrorCodeStringLen,
												str8 pleoraErrorDescription,
												p_u32 pleoraErrorDescriptionLen );

EXTERN_C	GIGE_API	i32		InitFwDefaults(
												GigEDevice* deviceHdl,
												u8 setHv,
												p_dbl hvSetPoint,
												u32 timeOut,
												p_u8 width,
												p_u8 height,
												HexitecSensorConfigPtr sensorConfig,
												HexitecOperationModePtr operationMode,
												p_dbl frameTime,
												p_u32 collectDcTime );

EXTERN_C	GIGE_API	i32		OpenSerialPort(
												GigEDevice* deviceHdl,
												PvDeviceSerial serialPort,
												u32 rxBufferSize,
												u8 useTermChar,
												u8 termChar );

EXTERN_C	GIGE_API	i32		OpenStream(
												GigEDevice* deviceHdl );

EXTERN_C	GIGE_API	i32		ReadEnvironmentValues(
												GigEDevice* deviceHdl,
												p_dbl humidity,
												p_dbl ambientTemperature,
												p_dbl asicTemperature,
												p_dbl adcTemperature,
												p_dbl ntcTemperature,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		ReadOperatingValues(
												GigEDevice* deviceHdl,
												p_dbl v3_3,
												p_dbl hvMon,
												p_dbl hvOut,
												p_dbl v1_2,
												p_dbl v1_8,
												p_dbl v3,
												p_dbl v2_5,
												p_dbl v3_3ln,
												p_dbl v1_65ln,
												p_dbl v1_8ana,
												p_dbl v3_8ana,
												p_dbl peltierCurrent,
												p_dbl ntcTemperature,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		ReadRegister(
												GigEDevice* deviceHdl,
												u8 registerAddress,
												p_u8 value,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		ReadResolution(
												GigEDevice* deviceHdl,
												p_u8 width,
												p_u8 height,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		RegisterAcqArmedCallBack(
												GigEDevice* deviceHdl,
												p_readyCallBack acqArmedCallBack );

EXTERN_C	GIGE_API	i32		RegisterAcqFinishCallBack(
												GigEDevice* deviceHdl,
												p_readyCallBack acqFinishCallBack );

EXTERN_C	GIGE_API	i32		RegisterTransferBufferReadyCallBack(
												GigEDevice* deviceHdl,
												p_bufferCallBack transferBufferReadyCallBack );

EXTERN_C	GIGE_API	i32		ReturnBuffer(
												GigEDevice* deviceHdl,
												p_u8 transferBuffer );

EXTERN_C	GIGE_API	i32		SerialPortWriteRead(
												GigEDevice* deviceHdl,
												const p_u8 txBuffer,
												u32 txBufferSize,
												p_u32 bytesWritten,
												p_u8 rxBuffer,
												u32 rxBufferSize,
												p_u32 bytesRead,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		SetDAC(
												GigEDevice* deviceHdl,
												p_dbl vCal,
												p_dbl uMid,
												p_dbl hvSetPoint,
												p_dbl detCtrl,
												p_dbl targetTemperature,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		SetFrameFormatControl(
												GigEDevice* deviceHdl,
												const str8 pixelFormat,
												u64 width,
												u64 height,
												u64 offsetX,
												u64 offsetY,
												const str8 sensorTaps,
												const str8 testPattern );

EXTERN_C	GIGE_API	i32		SetFrameTimeOut(
												GigEDevice* deviceHdl,
												u32 frameTimeOut );

EXTERN_C	GIGE_API	i32		SetTriggeredFrameCount(
												GigEDevice* deviceHdl,
												u32 frameCount,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		StopAcquisition(
												GigEDevice* deviceHdl );

EXTERN_C	GIGE_API	i32		UploadOffsetValues(
												GigEDevice* deviceHdl,
												Reg2BytePtr offsetValues,
												u32 offsetValuesLength,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		WriteAdcRegister(
												GigEDevice* deviceHdl,
												u8 registerAddress,
												p_u8 value,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		WriteRegister(
												GigEDevice* deviceHdl,
												u8 registerAddress,
												p_u8 value,
												u32 timeOut );

EXTERN_C	GIGE_API	i32		WriteRegisterStream(
												GigEDevice* deviceHdl,
												FpgaRegister_vec &registerStream,
												u32 timeOut );

EXTERN_C	GIGE_API	p_u8	TestString(
												GigEDevice* deviceHdl );

i32 CheckTemperatureLimit( GigEDevice* deviceHdl, p_dbl temperature, u32 timeOut );

u16 DacValFromVoltage( dbl voltage );

dbl DacValToVoltage( u16 number );

void DisableTriggerGateCB( hdl deviceHdl);

void EnableTriggerGateCB( hdl deviceHdl);

dbl GetAdcTemperature( u16 number );

dbl GetAmbientTemperature( u16 number );

dbl GetAsicTemperature( u16 number );

u32	GetCollectDcTime( dbl frameTime );

dbl GetHumidity( u16 number );

dbl GetHvOut( dbl hvMon );

dbl GetInternalReference( u16 number );

dbl GetPeltierCurrent( dbl voltage );

dbl GetVoltage( u16 number, dbl internalReference );

i32 HexToString( u32 number, u8 digits, p_u8 target );

i32 HexToStringLE( u32 number, u8 digits, p_u8 target );

u16 HvDacValFromHv( dbl hv );

dbl	HvDacValToHv( u16 number );

i32	SetOperationMode( GigEDevice* deviceHdl, HexitecOperationMode operationMode, u32 timeOut);

i32	SetSensorConfig( GigEDevice* deviceHdl, HexitecSensorConfig sensorConfig, u32 timeOut);

i32 StringToHex( p_u8 source, u8 digits, p_u32 number );

u16 TemperatureDacValFromTemperature( dbl temperature );

dbl TemperatureDacValToTemperature( u16 number, dbl internalReference );