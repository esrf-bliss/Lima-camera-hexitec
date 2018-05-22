// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GIGE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GIGE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef GIGE_H
#define GIGE_H

#ifdef __linux__
#define GIGE_API
#define GIGE_CDECL
#else
#ifdef GIGE_EXPORTS
#define GIGE_API __declspec(dllexport)
#else
#define GIGE_API __declspec(dllimport)
#endif
#define GIGE_CDECL __cdecl
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
#include <PvDeviceInfoGEV.h>
#include <queue>

#ifdef __linux__
#include <aS_messages.h>
#include <thread>
#include <mutex>
#include <cstdint>
#include <cstring>
#include <memory>

typedef uint8_t		u8,	*p_u8;
typedef int8_t		i8,	*p_i8;
typedef uint16_t	u16,*p_u16;
typedef int16_t		i16,*p_i16;
typedef uint32_t	u32,*p_u32;
typedef int32_t		i32,*p_i32;
typedef uint64_t	u64,*p_u64;
typedef int64_t		i64,*p_i64;
typedef float		sgl,*p_sgl;
typedef double		dbl,*p_dbl;
typedef char*		str8;
typedef std::vector<u8>	u8_vec;
typedef std::vector<u8_vec>	fb_vec;
#else
#include <new>
#include <math.h>
#endif

#define AS_HEXITEC_MIN_FRAME_TIMEOUT (u32)25

#ifdef __linux__
namespace GigE
{
class AcqFinishCallback
{
public:
	AcqFinishCallback() {};
	virtual ~AcqFinishCallback() {}
	virtual void finished() = 0;
};
class AcqArmedCallback
{
public:
	AcqArmedCallback() {};
	virtual ~AcqArmedCallback() {}
	virtual void armed() = 0;
};
class TransferBufferReadyCallback
{
public:
	TransferBufferReadyCallback() {};
	virtual ~TransferBufferReadyCallback() {}
	virtual void bufferReady(p_u8 aTransferBuffer, u32 aCurrentFrameWithinBuffer) = 0;
};

#else
using namespace aS;
typedef void (__cdecl *p_readyCallBack)( hdl deviceHdl);
typedef void (__cdecl *p_bufferCallBack)( p_u8 transferBuffer, u32 frameCount );
#endif

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

class GIGE_API GigEDevice
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
#ifdef __linux__
	std::mutex			    cAvailableTransferBufferLock;
	std::shared_ptr<AcqArmedCallback> cReadyCallBack;
	std::shared_ptr<AcqFinishCallback> cFinishCallBack;
	std::shared_ptr<TransferBufferReadyCallback> cBufferCallBack;
#else
	CRITICAL_SECTION		cAvailableTransferBufferLock;
	p_readyCallBack			cFinishCallBack;
	p_readyCallBack			cReadyCallBack;
	p_bufferCallBack		cBufferCallBack;
#endif
	u64						cAcquiredImages;
	u8						cContinuous;
	u8						cStopAcquisition;
	dbl						cFrameTime;
	u32						cFrameTimeOut;

public:
	GigEDevice(const str8 aDeviceDescriptor);
	~GigEDevice();

#ifdef __linux__
	int32_t startAcq();
	int32_t stopAcq();
	int32_t retrieveBuffer(uint8_t *buffer, uint32_t FrameTimeOut);
#endif
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
#ifdef __linux__
	void					RegisterAcqArmedCallBack(std::shared_ptr<AcqArmedCallback>& cbk);
	void					RegisterAcqFinishCallBack(std::shared_ptr<AcqFinishCallback>& cbk);
	void					RegisterTransferBufferReadyCallBack(std::shared_ptr<TransferBufferReadyCallback>& cbk);
#else
	void					RegisterAcqArmedCallBack( p_readyCallBack aAcqArmedCallBack );
	void					RegisterAcqFinishCallBack( p_readyCallBack aAcqFinishCallBack );
	void					RegisterTransferBufferReadyCallBack( p_bufferCallBack aTransferBufferReadyCallBack );
#endif
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

} // namespace GigE
#endif // GIGE_H
