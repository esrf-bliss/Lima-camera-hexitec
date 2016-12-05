#ifndef GIGE_H
#define GIGE_H

#ifdef WIN32
	#ifdef GIGE_EXPORTS
		#define GIGE_API __declspec(dllexport)
	#else
		#define GIGE_API __declspec(dllimport)
	#endif
	#define GIGE_CDECL __cdecl
#else  /* Unix */
	#define GIGE_API
	#define GIGE_CDECL
	#include <pthread.h>
#endif

#include <cstdint>
#include <queue>
#include <cstring>

#include "aS_messages.h"

#include <PvSystem.h>
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

namespace GigE
{

typedef void (GIGE_CDECL *p_readyCallBack)();
typedef void (GIGE_CDECL *p_bufferCallBack)(uint8_t* transferBuffer, uint32_t frameCount);

typedef struct GigEDeviceInfo {
	PvString Vendor;
	PvString Model;
	PvString ManufacturerInfo;
	PvString SerialNumber;
	PvString UserId;
	PvString MacAddress;
	PvString IpAddress;
	PvString NetMask;
	PvString GateWay;
} GigEDeviceInfo;

typedef struct GigEDeviceInfoStr {
	char* Vendor;
	char* Model;
	char* ManufacturerInfo;
	char* SerialNumber;
	char* UserId;
	char* MacAddress;
	char* IpAddress;
	char* NetMask;
	char* GateWay;
} GigEDeviceInfoStr;

class GIGE_API GigEDevice
{
public:
	GigEDevice(const char* aDeviceDescriptor);
	~GigEDevice();
	int32_t		startAcquisition();
	int32_t		stopAcquisition();
	int32_t		retrieveBuffer(uint8_t* Buffer, uint32_t FrameTimeOut);
	PvResult	GetLastResult();
	int32_t		OpenSerialPort(PvDeviceSerial SerialPort, uint32_t RxBufferSize, uint8_t UseTermChar, uint8_t TermChar);
	int32_t		CloseSerialPort();
	int32_t		FlushRxBuffer();
	int32_t		ReadSerialPort(uint8_t* RxBuffer, uint32_t RxBufferSize, uint32_t *BytesRead, uint32_t TimeOut);
	int32_t		WriteSerialPort(const uint8_t* TxBuffer, uint32_t TxBufferSize, uint32_t *BytesWritten);
	int32_t		OpenStream();
	int32_t		CloseStream();
	int32_t		SetImageFormatControl(const char* PixelFormat,
									uint64_t Width,
									uint64_t Height,
									uint64_t OffsetX,
									uint64_t OffsetY,
									const char* SensorTaps,
									const char* TestPattern);
	int32_t		CreatePipeline(uint32_t BufferCount);
	int32_t		ClosePipeline();
	int32_t		AcquireImage(uint32_t* ImageCount, uint8_t* Buffer, uint32_t FrameTimeOut);
	int32_t		AcquireImageThread(uint32_t ImageCount, uint32_t FrameTimeOut);

	int32_t		GetIntegerValue(const PvString Property, int64_t &Value);
	void		SetTransferBuffer(uint32_t TransferBufferCount, uint32_t TransferBufferFrameCount);
	void		ReturnBuffer(uint8_t* Buffer);
	void		RegisterTransferBufferReadyCallBack(p_bufferCallBack aTransferBufferReadyCallBack);
	uint64_t	GetAcquiredImageCount();
//	void		StopAcquisition();

	static int32_t	GetErrorDescription(PvResult aPleoraErrorCode,
										char* aPleoraErrorCodeString,
										uint32_t* aPleoraErrorCodeStringLen,
										char* aPleoraErrorDescription,
										uint32_t* aPleoraErrorDescriptionLen);

	GigEDeviceInfoStr		GetDeviceInfoStr();

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
	int64_t					cBlocksDroppedVal;
	PvGenInteger			*cBlocksDropped;
	int64_t					cBlockIDsMissingVal;
	PvGenInteger			*cBlockIDsMissing;

	PvDeviceAdapter			*cDeviceAdapter;
	PvDeviceSerialPort		cPort;
	uint8_t					cUseTermChar;
	uint8_t					cTermChar;
	std::vector<std::vector<uint8_t>>		cTransferBuffer;
	uint32_t				cTransferBufferFrameCount;
	std::queue <uint8_t*>	cAvailableTransferBuffer;
	pthread_mutex_t			cAvailableTransferBufferLock;
	p_readyCallBack			cReadyCallBack;
	p_bufferCallBack		cBufferCallBack;
	uint64_t				cAcquiredImages;
	uint8_t					cContinuous;
	uint8_t					cStopAcquisition;

	PvResult Connect(const char* aDeviceDescriptor);
	PvResult ConfigureSerialUart(PvDeviceSerial SerialPort);
	PvResult ConfigureSerialBulk(PvDeviceSerial SerialPort);
	PvResult ConfigureStream();
	void ClearQueue();
	void InitializeQueue();
	void BufferReadyCallBack(uint8_t* aTransferBuffer, uint32_t aCurrentFrameWithinBuffer);
};

} // namespace GigE
#endif // GIGE_H
