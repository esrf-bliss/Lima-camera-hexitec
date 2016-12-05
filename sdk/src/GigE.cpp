// GigE.cpp : Defines the exported functions for the DLL application.
//

#ifdef WIN32
#include "stdafx.h"
#endif
#include "GigE.h"
#include <cmath>
#include <iostream>
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

using namespace GigE;

GigEDevice::~GigEDevice() 
{
#ifdef WIN32
	DeleteCriticalSection( &cAvailableTransferBufferLock );
#else
	pthread_mutex_destroy(&cAvailableTransferBufferLock);
#endif
	if (cPort.IsOpened())
	{
		cPort.Close();
	}
	if (cDeviceAdapter != NULL)
	{
		delete cDeviceAdapter;
	}

	if (cDevice != NULL)
	{
		if (cDevice->IsConnected())
		{
			cDevice->Disconnect();
		}

		PvDevice::Free(cDevice);
	}
}

// Changes added for Linux -------------------------------------------------------------------------------------

int32_t GigEDevice::startAcquisition() {
	if (!cStream) {
		return AS_GIGE_STREAM_NOT_AVAILABLE;
	}
	if (!cPipeline) {
		return AS_GIGE_PIPELINE_NOT_AVAILABLE;
	}
	cResult = cResetCmd->Execute();
	if (!cResult.IsOK()) {
		return AS_GIGE_RESET_COMMAND_ERROR;
	}
	cResult = cPipeline->Start();
	if (!cResult.IsOK()) {
		return AS_GIGE_PIPELINE_START_ERROR;
	}
	cResult = cDevice->StreamEnable();
	if (!cResult.IsOK()) {
		return AS_GIGE_STREAM_ENABLE_ERROR;
	}
	cResult = cStartCmd->Execute();
	if (!cResult.IsOK()) {
		return AS_GIGE_START_COMMAND_ERROR;
	}
	return AS_NO_ERROR;
}

int32_t GigEDevice::stopAcquisition() {
	cResult = cStopCmd->Execute();
	if (!cResult.IsOK()) {
		return AS_GIGE_STOP_COMMAND_ERROR;
	}
	cResult = cDevice->StreamDisable();
	if (!cResult.IsOK()) {
		return AS_GIGE_STREAM_DISABLE_ERROR;
	}
	cResult = cPipeline->Stop();
	if (!cResult.IsOK()) {
		return AS_GIGE_PIPELINE_STOP_ERROR;
	}
	if (!cAcqResult.IsOK()) {
		cResult = cAcqResult;
		return AS_GIGE_ACQUISION_ABORTED_ERROR;
	}
	return AS_NO_ERROR;
}

int32_t GigEDevice::retrieveBuffer(uint8_t *buffer, uint32_t FrameTimeOut) {
	PvResult lResult;
	PvPayloadType lType;
	PvBuffer *lBuffer = NULL;
	uint8_t* lRawBuffer = NULL;
	uint32_t lSize = 0;
	uint8_t* lPointer = buffer;

	cResult = cPipeline->RetrieveNextBuffer(&lBuffer, FrameTimeOut, &lResult);
//	std::cout << "cAcqResult " << cAcqResult.GetCode() << std::endl;
	if (cResult.IsOK()) {
//		std::cout << "cResult is OK" << std::endl;
		if (lResult.IsOK()) {
//			std::cout << "lResult is OK" << std::endl;
			lType = lBuffer->GetPayloadType();
			if (lType == PvPayloadTypeImage) {
				lRawBuffer = lBuffer->GetDataPointer();
				lSize = lBuffer->GetSize();
				// ToDo really need to avoid this by passing in the supplied buffer
				memcpy(lPointer, lRawBuffer, lSize);
				lPointer = lPointer + lSize;
			} else {
				cResult = PvResult::Code::INVALID_DATA_FORMAT;
			}
		} else {
			cResult = lResult;
		}
		if (cResult.IsOK()) {
			cResult = cPipeline->ReleaseBuffer(lBuffer);
		} else {
			cPipeline->ReleaseBuffer(lBuffer);
		}
		if (cResult.IsOK()) {
			cResult = cBlocksDropped->GetValue(cBlocksDroppedVal);
			if (cResult.IsOK() && cBlocksDroppedVal) {
				cResult = PvResult::Code::ABORTED;
			}
		}
		if (cResult.IsOK()) {
			cResult = cBlockIDsMissing->GetValue(cBlockIDsMissingVal);

			if (cResult.IsOK() && cBlockIDsMissingVal) {
				std::cout << "cBlockIDsMissingVal-------" << cBlockIDsMissingVal << std::endl;
				cResult = PvResult::Code::ERR_OVERFLOW;
			}
		}
	}
	return cResult.GetCode();
}

// End of Linux changes -------------------------------------------------------------------------------------

int32_t GigEDevice::AcquireImage(uint32_t* ImageCount, uint8_t* Buffer, uint32_t FrameTimeOut)
{
	PvBuffer *lBuffer = NULL;
	uint8_t* lRawBuffer = NULL;
	uint32_t lSize = 0;
	PvResult lResult;
	PvPayloadType lType;
	uint32_t i = 0;
	uint32_t lImageCount = *ImageCount;
	uint8_t* lPointer = Buffer;

	cAcqResult = PvResult::Code::OK;

	if (!cStream)
	{
		return AS_GIGE_STREAM_NOT_AVAILABLE;
	}

	if (!cPipeline)
	{
		return AS_GIGE_PIPELINE_NOT_AVAILABLE;
	}
	cResult = cResetCmd->Execute();

	if (!cResult.IsOK()) 
	{
		return AS_GIGE_RESET_COMMAND_ERROR;
	}

	cResult = cPipeline->Start();

	if (!cResult.IsOK())
	{
		return AS_GIGE_PIPELINE_START_ERROR;
	}
	
	cResult = cDevice->StreamEnable();

	if (!cResult.IsOK())
	{
		return AS_GIGE_STREAM_ENABLE_ERROR;
	}
	
	cResult = cStartCmd->Execute();

	if (!cResult.IsOK())
	{
		return AS_GIGE_START_COMMAND_ERROR;
	}
	
	while (cAcqResult.IsOK() && (i < lImageCount))
	{
		cAcqResult = cPipeline->RetrieveNextBuffer(&lBuffer, FrameTimeOut, &lResult);

		if (cAcqResult.IsOK())
		{
			if (lResult.IsOK())
			{
				lType = lBuffer->GetPayloadType();
				if (lType == PvPayloadTypeImage)
				{
					lRawBuffer = lBuffer->GetDataPointer();
					lSize = lBuffer->GetSize();
					memcpy(lPointer, lRawBuffer, lSize);
					i++;
					std::cout << "image counter " << i << std::endl;
					lPointer = lPointer + lSize;
				} 
				else 
				{
					cAcqResult = PvResult::Code::INVALID_DATA_FORMAT;
				}
			} 
			else 
			{
				cAcqResult = lResult;
			}
			if (cAcqResult.IsOK()) 
			{
				cResult = cPipeline->ReleaseBuffer(lBuffer);
			}
			else
			{
				cPipeline->ReleaseBuffer(lBuffer);
			}
			if (cAcqResult.IsOK())
			{
				cAcqResult = cBlocksDropped->GetValue(cBlocksDroppedVal);
				if (cAcqResult.IsOK() && cBlocksDroppedVal)
				{
					cAcqResult = PvResult::Code::ABORTED;
				}
			}
			if (cAcqResult.IsOK()) 
			{
				cAcqResult = cBlockIDsMissing->GetValue( cBlockIDsMissingVal );

				if( cAcqResult.IsOK() && cBlockIDsMissingVal )
				{
					cAcqResult = PvResult::Code::ERR_OVERFLOW;
				}
			}
		}
	}
	*ImageCount = i;
	cResult = cStopCmd->Execute();
	if (!cResult.IsOK())
	{
		return AS_GIGE_STOP_COMMAND_ERROR;
	}
	cResult = cDevice->StreamDisable();
	if (!cResult.IsOK())
	{
		return AS_GIGE_STREAM_DISABLE_ERROR;
	}
	cResult = cPipeline->Stop();
	if (!cResult.IsOK())
	{
		return AS_GIGE_PIPELINE_STOP_ERROR;
	}
	if (!cAcqResult.IsOK())
	{
		cResult = cAcqResult;
		return AS_GIGE_ACQUISION_ABORTED_ERROR;
	}
	return AS_NO_ERROR;
}

int32_t GigEDevice::AcquireImageThread(uint32_t ImageCount, uint32_t FrameTimeOut)
{
	PvBuffer *lBuffer = NULL;
	uint8_t* lRawBuffer = NULL;
	uint32_t lSize = 0;
	PvResult lAcqResult = PvResult::Code::OK;
	PvPayloadType lType;
	uint32_t lCurrentFrameWithinBuffer = 0;
	uint8_t* lTransferBuffer = NULL;
	uint8_t* lPointer = NULL;
	int32_t lResult = AS_NO_ERROR;

	cAcqResult = PvResult::Code::OK;
	ClearQueue();
	InitializeQueue();
	cAcquiredImages = 0;
	if (ImageCount)
	{
		cContinuous = 0;
	}
	else
	{
		cContinuous = 1;
	}
	if (!cStream)
	{
		return AS_GIGE_STREAM_NOT_AVAILABLE;
	}
	if (!cPipeline)
	{
		return AS_GIGE_PIPELINE_NOT_AVAILABLE;
	}
	cResult = cResetCmd->Execute();
	if (!cResult.IsOK())
	{
		return AS_GIGE_RESET_COMMAND_ERROR;
	}
	cResult = cPipeline->Start();
	if (!cResult.IsOK())
	{
		return AS_GIGE_PIPELINE_START_ERROR;
	}
	cResult = cDevice->StreamEnable();
	if (!cResult.IsOK())
	{
		return AS_GIGE_STREAM_ENABLE_ERROR;
	}
	cResult = cStartCmd->Execute();
	if (!cResult.IsOK())
	{
		return AS_GIGE_START_COMMAND_ERROR;
	}
	cStopAcquisition = 0;
	if (cReadyCallBack)
	{
		cReadyCallBack();
	}
	while (cAcqResult.IsOK() && ((cAcquiredImages < ImageCount) || cContinuous))
	{
		if (cStopAcquisition)
		{
			break;
		}
		if (lCurrentFrameWithinBuffer == 0)    // new Transferbuffer
		{
#ifdef WIN32
			EnterCriticalSection( &cAvailableTransferBufferLock );
#else
			pthread_mutex_lock(&cAvailableTransferBufferLock);
#endif
			if (cAvailableTransferBuffer.size())
			{
				lTransferBuffer = cAvailableTransferBuffer.front();
				lPointer = lTransferBuffer;
				cAvailableTransferBuffer.pop();
#ifdef WIN32
				LeaveCriticalSection( &cAvailableTransferBufferLock );
#else
				pthread_mutex_unlock(&cAvailableTransferBufferLock);
#endif
			}
			else
			{
#ifdef WIN32
				LeaveCriticalSection( &cAvailableTransferBufferLock );
#else
				pthread_mutex_unlock(&cAvailableTransferBufferLock);
#endif
				lResult = AS_GIGE_NO_TRANSFER_BUFFER_AVAILABLE;
				break;
			}
		}
		cAcqResult = cPipeline->RetrieveNextBuffer(&lBuffer, FrameTimeOut, &lAcqResult);
		if (cAcqResult.IsOK())
		{
			if (lAcqResult.IsOK())
			{
				lType = lBuffer->GetPayloadType();
				if (lType == PvPayloadTypeImage)
				{
					lRawBuffer = lBuffer->GetDataPointer();
					lSize = lBuffer->GetSize();
					std::copy(lRawBuffer, lRawBuffer + lSize, lPointer);
					lCurrentFrameWithinBuffer++;
					cAcquiredImages++;
					lPointer = lPointer + lSize;
					if (lCurrentFrameWithinBuffer >= cTransferBufferFrameCount)
					{
						BufferReadyCallBack(lTransferBuffer, lCurrentFrameWithinBuffer);
						lCurrentFrameWithinBuffer = 0;
					}
				}
				else
				{
					cAcqResult = PvResult::Code::INVALID_DATA_FORMAT;
				}
			}
			else
			{
				cAcqResult = lAcqResult;
			}
			if (cAcqResult.IsOK())
			{
				cAcqResult = cPipeline->ReleaseBuffer(lBuffer);
			}
			else
			{
				cPipeline->ReleaseBuffer(lBuffer);
			}
			if (cAcqResult.IsOK())
			{
				cAcqResult = cBlocksDropped->GetValue(cBlocksDroppedVal);
				if (cAcqResult.IsOK() && cBlocksDroppedVal)
				{
					lResult = AS_GIGE_BLOCKS_DROPPED;
					break;
				}
			}
			if (cAcqResult.IsOK())
			{
				cAcqResult = cBlockIDsMissing->GetValue( cBlockIDsMissingVal );

				if( cAcqResult.IsOK() && cBlockIDsMissingVal )
				{
					lResult = AS_GIGE_BLOCKS_IDS_MISSING;
					break;
				}
			}
		}
	}
	cStopAcquisition = 0;
	if (lCurrentFrameWithinBuffer)
	{
		BufferReadyCallBack(lTransferBuffer, lCurrentFrameWithinBuffer);
	}
	cResult = cStopCmd->Execute();
	if (!cResult.IsOK())
	{
		return AS_GIGE_STOP_COMMAND_ERROR;
	}
	cResult = cDevice->StreamDisable();
	if (!cResult.IsOK())
	{
		return AS_GIGE_STREAM_DISABLE_ERROR;
	}
	cResult = cPipeline->Stop();
	if (!cResult.IsOK())
	{
		return AS_GIGE_PIPELINE_STOP_ERROR;
	}
	if (!cAcqResult.IsOK())
	{
		cResult = cAcqResult;
		return AS_GIGE_ACQUISION_ABORTED_ERROR;
	}
	return lResult;
}

void GigEDevice::BufferReadyCallBack(uint8_t* aTransferBuffer, uint32_t aCurrentFrameWithinBuffer)
{
	if (cBufferCallBack)
	{
		cBufferCallBack(aTransferBuffer, aCurrentFrameWithinBuffer);
	}
	else
	{
#ifdef WIN32
		EnterCriticalSection( &cAvailableTransferBufferLock );
		cAvailableTransferBuffer.push(aTransferBuffer);
		LeaveCriticalSection( &cAvailableTransferBufferLock );
#else
		pthread_mutex_lock(&cAvailableTransferBufferLock);
		cAvailableTransferBuffer.push(aTransferBuffer);
		pthread_mutex_unlock(&cAvailableTransferBufferLock);
#endif
	}
}

void GigEDevice::ClearQueue()
{
	while (cAvailableTransferBuffer.size())
	{
		cAvailableTransferBuffer.pop();
	}
}

int32_t GigEDevice::ClosePipeline()
{
	cResult = PvResult::Code::OK;

	if (!cPipeline)
	{
		return AS_NO_ERROR;
	}
	if (cPipeline->IsStarted())
	{
		cResult = cPipeline->Stop();
	}
	delete cPipeline;
	cPipeline = NULL;
	if (!cResult.IsOK())
	{
		return AS_GIGE_PIPELINE_STOP_ERROR;
	}
	return AS_NO_ERROR;
}

int32_t GigEDevice::CloseSerialPort()
{
	cResult = cPort.Close();

	if (!cResult.IsOK())
	{
		return AS_GIGE_SERIAL_PORT_CLOSE_ERROR;
	}
	return AS_NO_ERROR;
}

int32_t GigEDevice::CloseStream()
{
	int32_t lResult = AS_NO_ERROR;

	if (cPipeline)
	{
		lResult = ClosePipeline();
	}
	if ((lResult == AS_NO_ERROR) && (cStream != NULL))
	{
		cResult = cStream->Close();
		if (!cResult.IsOK())
		{
			return AS_GIGE_STREAM_CLOSE_ERROR;
		}
		PvStream::Free(cStream);
		cStream = NULL;
	}
	return AS_NO_ERROR;
}

PvResult GigEDevice::ConfigureSerialBulk(PvDeviceSerial SerialPort)
{
	PvResult lResult;
	uint64_t lBulkSelektor = SerialPort - 2;

	lResult = cDeviceParams->SetEnumValue("BulkSelector", lBulkSelektor);
	lResult = cDeviceParams->SetEnumValue("BulkMode", "UART");
	lResult = cDeviceParams->SetEnumValue("BulkBaudRate", "Baud38400");
	lResult = cDeviceParams->SetEnumValue("BulkNumOfStopBits", "One");
	lResult = cDeviceParams->SetEnumValue("BulkParity", "None");
	return lResult;
}

PvResult GigEDevice::ConfigureSerialUart(PvDeviceSerial SerialPort)
{
	PvResult lResult;

	lResult = cDeviceParams->SetEnumValue("Uart0BaudRate", "Baud38400");
	lResult = cDeviceParams->SetEnumValue("Uart0NumOfStopBits", "One");
	lResult = cDeviceParams->SetEnumValue("Uart0Parity", "None");
	return lResult;
}

PvResult GigEDevice::ConfigureStream()
{
	PvResult lResult = PvResult::Code::OK;

	// If this is a GigE Vision device, configure GigE Vision specific streaming parameters
	PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV *>(cDevice);
	if (lDeviceGEV != NULL)
	{
		PvStreamGEV *lStreamGEV = static_cast<PvStreamGEV *>(cStream);
		// Negotiate packet size
		lResult = lDeviceGEV->NegotiatePacketSize();
		if (lResult.IsOK())
		{
			// Configure device streaming destination
			lResult = lDeviceGEV->SetStreamDestination(lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort());
		}
	}
	return lResult;
}

PvResult GigEDevice::Connect(const char* aDeviceDescriptor)
{
	PvResult lResult;
	std::cout << "cSystem.FindDevice..................." << std::endl;
	lResult = cSystem.FindDevice(aDeviceDescriptor, &cDeviceInfo);
	std::cout << "cSystem.FindDevice...done............" << std::endl;
	if (lResult.IsOK())
	{
		std::cout << "PvDevice::CreateAndConnect..................." << std::endl;
		auto t1 = Clock::now();
		cDevice = PvDevice::CreateAndConnect(cDeviceInfo, &lResult);
		auto t2 = Clock::now();
		std::cout << "Delta t2-t1: " << std::chrono::duration_cast<std::chrono::nanoseconds>
				 (t2 - t1).count() << " nanoseconds" << std::endl;
		std::cout << "PvDevice::CreateAndConnect..Done............." << std::endl;
	}
	return lResult;
}

int32_t GigEDevice::CreatePipeline(uint32_t BufferCount)
{
	uint32_t NeededBufferSize = 0;

	if (!cStream)
	{
		return AS_GIGE_STREAM_NOT_AVAILABLE;
	}
	if (cPipeline)
	{
		return AS_GIGE_PIPELINE_CREATED_ALREADY;
	}
	cPipeline = new PvPipeline(cStream);
	NeededBufferSize = cDevice->GetPayloadSize();
	for (uint32_t i = 0; i < cTransferBuffer.size(); i++)
	{
		try
		{
			cTransferBuffer[i].resize(cTransferBufferFrameCount * NeededBufferSize, 0);
		}
		catch (std::bad_alloc )
		{
			cResult = PvResult::Code::NOT_ENOUGH_MEMORY;
		}
	}
	if (cResult.IsOK())
	{
		cPipeline->SetBufferSize(NeededBufferSize);
		cResult = cPipeline->SetBufferCount(BufferCount);
	}
	if (!cResult.IsOK())
	{
		cTransferBuffer.clear();
		delete cPipeline;
		cPipeline = NULL;
		return AS_GIGE_PIPELINE_CREATION_ERROR;
	}
	return AS_NO_ERROR;
}

int32_t GigEDevice::FlushRxBuffer()
{
	cResult = cPort.FlushRxBuffer();

	if (!cResult.IsOK())
	{
		return AS_GIGE_SERIAL_PORT_FLUSH_ERROR;
	}
	return AS_NO_ERROR;
}

uint64_t GigEDevice::GetAcquiredImageCount()
{
	return cAcquiredImages;
}

GigEDeviceInfoStr GigEDevice::GetDeviceInfoStr()
{
	return cDeviceInformationStr;
}

int32_t GigEDevice::GetErrorDescription(PvResult aPleoraErrorCode, char* aPleoraErrorCodeString, uint32_t* aPleoraErrorCodeStringLen,
		char* aPleoraErrorDescription, uint32_t* aPleoraErrorDescriptionLen)
{
	uint32_t lSize = 0;
	int32_t dResult = AS_NO_ERROR;

	if (aPleoraErrorCodeString && aPleoraErrorCodeStringLen && *aPleoraErrorCodeStringLen)
	{
		if (aPleoraErrorCode.GetCodeString().GetLength() > ((*aPleoraErrorCodeStringLen)-1))
		{
			lSize = (*aPleoraErrorCodeStringLen)-1;
			dResult = AS_BUFFER_TO_SMALL;
		}
		else
		{
			lSize = aPleoraErrorCode.GetCodeString().GetLength();
		}
		memcpy(aPleoraErrorCodeString, aPleoraErrorCode.GetCodeString().GetAscii(), lSize);
		aPleoraErrorCodeString[lSize] = 0x00;
	}
	if (aPleoraErrorCodeStringLen)
	{
		*aPleoraErrorCodeStringLen = (aPleoraErrorCode.GetCodeString().GetLength())+1;
	}
	if (aPleoraErrorDescription && aPleoraErrorDescriptionLen && *aPleoraErrorDescriptionLen)
	{
		if (aPleoraErrorCode.GetDescription().GetLength() > ((*aPleoraErrorDescriptionLen)-1))
	{
			lSize = (*aPleoraErrorDescriptionLen)-1;
			dResult = AS_BUFFER_TO_SMALL;
		}
		else
		{
			lSize = aPleoraErrorCode.GetDescription().GetLength();
		}
		memcpy(aPleoraErrorDescription, aPleoraErrorCode.GetDescription().GetAscii(), lSize);
		aPleoraErrorDescription[lSize] = 0x00;
	}
	if (aPleoraErrorDescriptionLen)
	{
		*aPleoraErrorDescriptionLen = (aPleoraErrorCode.GetDescription().GetLength())+1;
	}
	return dResult;
}

int32_t GigEDevice::GetIntegerValue(const PvString Property, int64_t &Value)
{
	int32_t lResult = AS_NO_ERROR;

	cResult = cStreamParams->GetIntegerValue(Property, Value);
	if (!cResult.IsOK())
	{
		lResult = AS_GIGE_GET_INTEGER_VALUE_ERROR;
	}
	return lResult;
}

PvResult GigEDevice::GetLastResult()
{
	return cResult;
}

GigEDevice::GigEDevice(const char* aDeviceDescriptor)
{
	cDeviceInfo = NULL;
	cDeviceInfoGEV = NULL;
	cDeviceInfoU3V = NULL;
	cDevice = NULL;
	cDeviceParams = NULL;
	cStreamParams = NULL;
	cDeviceAdapter = NULL;
	cStream = NULL;
	cPipeline = NULL;
	cStartCmd = NULL;
	cStopCmd = NULL;
	cResetCmd = NULL;
	cTransferBufferFrameCount = 0;
	cTransferBuffer.clear();
	cUseTermChar = 0;
	cTermChar = 0;
	cReadyCallBack = NULL;
	cBufferCallBack = NULL;
	cAcquiredImages = 0;
	cContinuous = 0;
	cStopAcquisition = 0;
	cBlocksDroppedVal = 0;
	cBlocksDropped = NULL;
	cBlockIDsMissingVal = 0;
	cBlockIDsMissing = NULL;

	cResult = PvResult::Code::OK;
	cAcqResult = PvResult::Code::OK;

#ifdef WIN32
	InitializeCriticalSectionAndSpinCount(&cAvailableTransferBufferLock, 0x0400);
#else
	cAvailableTransferBufferLock =  PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif
	ClearQueue();
	cResult = Connect(aDeviceDescriptor);
	if (cResult.IsOK())
	{
		cDeviceAdapter = new PvDeviceAdapter(cDevice);
		cDeviceParams = cDevice->GetParameters();
		cStartCmd = dynamic_cast<PvGenCommand *>(cDeviceParams->Get("AcquisitionStart"));
		cStopCmd = dynamic_cast<PvGenCommand *>(cDeviceParams->Get("AcquisitionStop"));
	}
	if (cResult.IsOK())
	{
		cDeviceInformation.Vendor = cDeviceInfo->GetVendorName();
		cDeviceInformationStr.Vendor = (char*) cDeviceInformation.Vendor.GetAscii();
		cDeviceInformation.Model = cDeviceInfo->GetModelName();
		cDeviceInformationStr.Model = (char*) cDeviceInformation.Model.GetAscii();
		cDeviceInformation.ManufacturerInfo = cDeviceInfo->GetManufacturerInfo();
		cDeviceInformationStr.ManufacturerInfo = (char*) cDeviceInformation.ManufacturerInfo.GetAscii();
		cDeviceInformation.SerialNumber = cDeviceInfo->GetSerialNumber();
		cDeviceInformationStr.SerialNumber = (char*) cDeviceInformation.SerialNumber.GetAscii();
		cDeviceInformation.UserId = cDeviceInfo->GetUserDefinedName();
		cDeviceInformationStr.UserId = (char*) cDeviceInformation.UserId.GetAscii();
		cDeviceInfoType = cDeviceInfo->GetType();
		switch (cDeviceInfoType)
		{
		case PvDeviceInfoType::PvDeviceInfoTypeGEV:
			cDeviceInfoGEV = (PvDeviceInfoGEV *) (cDeviceInfo);
			cDeviceInformation.MacAddress = cDeviceInfoGEV->GetMACAddress();
			cDeviceInformationStr.MacAddress = (char*) cDeviceInformation.MacAddress.GetAscii();
			cDeviceInformation.IpAddress = cDeviceInfoGEV->GetIPAddress();
			cDeviceInformationStr.IpAddress = (char*) cDeviceInformation.IpAddress.GetAscii();
			cDeviceInformation.NetMask = cDeviceInfoGEV->GetSubnetMask();
			cDeviceInformationStr.NetMask = (char*) cDeviceInformation.NetMask.GetAscii();
			cDeviceInformation.GateWay = cDeviceInfoGEV->GetDefaultGateway();
			cDeviceInformationStr.GateWay = (char*) cDeviceInformation.GateWay.GetAscii();
			break;
		default:
			break;
		}
		PvGenInteger *PayloadSize = cDevice->GetParameters()->GetInteger("PayloadSize" );
		PvGenInteger *Width = cDevice->GetParameters()->GetInteger( "Width" );
		PvGenInteger *Height = cDevice->GetParameters()->GetInteger( "Height" );
		PvGenInteger *Offx = cDevice->GetParameters()->GetInteger( "OffsetX" );
		PvGenInteger *Offy = cDevice->GetParameters()->GetInteger( "OffsetY" );
		PvGenEnum *PixelFormat = cDevice->GetParameters()->GetEnum( "PixelFormat" );
		int64_t pls, w ,h, pf, ox, oy;
		PayloadSize->GetValue(pls);
		Width->GetValue(w);
		Height->GetValue(h);
		Offx->GetValue(ox);
		Offy->GetValue(oy);
		PixelFormat->GetValue(pf);
	}
}
void GigEDevice::InitializeQueue()
{
	for (uint32_t i = 0; i < cTransferBuffer.size(); i++)
	{
		cAvailableTransferBuffer.push(&cTransferBuffer[i][0]);
	}
}

int32_t GigEDevice::OpenSerialPort(PvDeviceSerial SerialPort, uint32_t RxBufferSize, uint8_t UseTermChar, uint8_t TermChar) {
	if (cPort.IsOpened())
	{
		cResult = cPort.Close();
	}
	if (!cResult.IsOK())
	{
		return AS_GIGE_SERIAL_PORT_CLOSE_ERROR;
	}
	if (!cPort.IsSupported(cDeviceAdapter, SerialPort))
	{
		return AS_GIGE_SERIAL_PORT_NOT_SUPPORTED;
	}
	if ((SerialPort == PvDeviceSerial0) || (SerialPort == PvDeviceSerial1))
	{
		cResult = ConfigureSerialUart(SerialPort);
	}
	else if ((SerialPort >= PvDeviceSerialBulk0) && (SerialPort <= PvDeviceSerialBulk7))
	{
		cResult = ConfigureSerialBulk(SerialPort);
	}
	if (!cResult.IsOK())
	{
		return AS_GIGE_SERIAL_PORT_CONFIG_FAILED;
	}
	cResult = cPort.SetRxBufferSize(RxBufferSize);
	if (!cResult.IsOK())
	{
		return AS_GIGE_SERIAL_PORT_SET_RX_BUFFER_FAILED;
	}
	cResult = cPort.Open(cDeviceAdapter, SerialPort);
	if (!cResult.IsOK())
	{
		return AS_GIGE_SERIAL_PORT_OPEN_ERROR;
	}
	cUseTermChar = UseTermChar;
	cTermChar = TermChar;
	return AS_NO_ERROR;
}

int32_t GigEDevice::OpenStream()
{
	if (!cStream)
	{
		cStream = PvStream::CreateAndOpen(cDeviceInfo->GetConnectionID(), &cResult);
		if (!cResult.IsOK())
		{
			return AS_GIGE_STREAM_OPEN_ERROR;
		}
		cStreamParams = cStream->GetParameters();
		cResetCmd = dynamic_cast<PvGenCommand *>(cStreamParams->Get("Reset"));
		cBlocksDropped = dynamic_cast<PvGenInteger *>(cStreamParams->Get("BlocksDropped"));
		cBlockIDsMissing	= dynamic_cast<PvGenInteger *>( cStreamParams->Get( "BlockIDsMissing" ) );

		cResult = ConfigureStream();
		if (!cResult.IsOK())
		{
			return AS_GIGE_STREAM_CONFIG_ERROR;
		}
	}
	else
	{
		return AS_GIGE_STREAM_ALREADY_OPENED;
	}
	return AS_NO_ERROR;
}

int32_t GigEDevice::ReadSerialPort(uint8_t* RxBuffer, uint32_t RxBufferSize, uint32_t *BytesRead, uint32_t TimeOut)
{
	uint32_t lBytesRead = 0;

	if (cUseTermChar)
	{
		*BytesRead = 0;
		while (*BytesRead < RxBufferSize)
		{
			cResult = cPort.Read(RxBuffer + *BytesRead, RxBufferSize - *BytesRead, lBytesRead, TimeOut);
			if (!cResult.IsOK())
			{
				return AS_GIGE_SERIAL_PORT_READ_ERROR;
			}
			*BytesRead = *BytesRead + lBytesRead;
			if (RxBuffer[*BytesRead - 1] == cTermChar)
			{
				return AS_NO_ERROR;
			}
		}
		return AS_GIGE_SERIAL_PORT_READ_BUFFER_FULL;
	}
	else
	{
		cResult = cPort.Read(RxBuffer, RxBufferSize, lBytesRead, TimeOut);
		*BytesRead = lBytesRead;
		if (!cResult.IsOK())
		{
			return AS_GIGE_SERIAL_PORT_READ_ERROR;
		}
	}
	return AS_NO_ERROR;
}

void GigEDevice::RegisterTransferBufferReadyCallBack(p_bufferCallBack aTransferBufferReadyCallBack)
{
	cBufferCallBack = aTransferBufferReadyCallBack;
}

void GigEDevice::ReturnBuffer(uint8_t* Buffer)
{
#ifdef WIN32
	EnterCriticalSection( &cAvailableTransferBufferLock );
	cAvailableTransferBuffer.push(Buffer);
	LeaveCriticalSection( &cAvailableTransferBufferLock );
#else
	pthread_mutex_lock(&cAvailableTransferBufferLock);
	cAvailableTransferBuffer.push(Buffer);
	pthread_mutex_unlock(&cAvailableTransferBufferLock);
#endif
}

int32_t GigEDevice::SetImageFormatControl(const char* PixelFormat, uint64_t Width, uint64_t Height, uint64_t OffsetX, uint64_t OffsetY,
		const char* SensorTaps, const char* TestPattern)
		{
	cResult = cDeviceParams->SetEnumValue("PixelFormat", PixelFormat);
	if (cResult.IsOK())
	{
		cResult = cDeviceParams->SetIntegerValue("Width", Width);
	}
	if (cResult.IsOK())
	{
		cResult = cDeviceParams->SetIntegerValue("Height", Height);
	}
	if (cResult.IsOK())
	{
		cResult = cDeviceParams->SetIntegerValue("OffsetX", OffsetX);
	}
	if (cResult.IsOK())
	{
		cResult = cDeviceParams->SetIntegerValue("OffsetY", OffsetY);
	}
	if (cResult.IsOK())
	{
		cResult = cDeviceParams->SetEnumValue("SensorDigitizationTaps", SensorTaps);
	}
	if (cResult.IsOK())
	{
		cResult = cDeviceParams->SetEnumValue("TestPattern", TestPattern);
	}
	if (!cResult.IsOK())
	{
		return AS_GIGE_SET_IMAGE_FORMAT_ERROR;
	}
	return AS_NO_ERROR;
}

void GigEDevice::SetTransferBuffer(uint32_t TransferBufferCount, uint32_t TransferBufferFrameCount)
{
	cTransferBufferFrameCount = TransferBufferFrameCount;
	cTransferBuffer.resize(TransferBufferCount);
}

//void GigEDevice::StopAcquisition()
//{
//	cStopAcquisition = 1;
//}

int32_t GigEDevice::WriteSerialPort(const uint8_t* TxBuffer, uint32_t TxBufferSize, uint32_t *BytesWritten)
{
	uint32_t lBytesWritten = 0;

	cResult = cPort.Write(TxBuffer, TxBufferSize, lBytesWritten);

	*BytesWritten = lBytesWritten;

	if (!cResult.IsOK())
	{
		return AS_GIGE_SERIAL_PORT_WRITE_ERROR;
	}

	return AS_NO_ERROR;
}
