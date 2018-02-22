// GigE.cpp : Defines the exported functions for the DLL application.
//

#ifndef __linux__
#include "stdafx.h"
#endif
#include "GigE.h"
#include <cmath>
#include <iostream>

#ifdef __linux__ // Changes added for Linux -------------------------------------------------------------------------------------

using namespace GigE;


int32_t GigEDevice::startAcq() {
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

int32_t GigEDevice::stopAcq() {
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
	if (cResult.IsOK()) {
		if (lResult.IsOK()) {
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
	}
	return cResult.GetCode();
}

void GigEDevice::RegisterAcqArmedCallBack(std::shared_ptr<AcqArmedCallback>& aAcqArmedCallBack)
{
	cReadyCallBack = aAcqArmedCallBack;
}
void GigEDevice::RegisterAcqFinishCallBack(std::shared_ptr<AcqFinishCallback>& aAcqFinishCallBack )
{
	cFinishCallBack = aAcqFinishCallBack;
}
void GigEDevice::RegisterTransferBufferReadyCallBack(std::shared_ptr<TransferBufferReadyCallback>& aTransferBufferReadyCallBack)
{
	cBufferCallBack = aTransferBufferReadyCallBack;
}
#endif // End of specific Linux changes -------------------------------------------------------------------------------------


GigEDevice::~GigEDevice() 
{
#ifndef __linux__
	DeleteCriticalSection( &cAvailableTransferBufferLock );
#endif
	if (cPort.IsOpened())
	{
		cPort.Close();
	}

	if( cDeviceAdapter != NULL )
	{
		delete cDeviceAdapter;
	}

	if( cDevice != NULL )
	{
		if( cDevice->IsConnected() )
		{
			cDevice->Disconnect();
		}

		PvDevice::Free( cDevice );
	}
}

i32	GigEDevice::AcquireImage( p_u32 ImageCount, p_u8 Buffer, u32 FrameTimeOut )
{
	PvBuffer		*lBuffer = NULL;
	p_u8			lRawBuffer = NULL;
	u32				lSize = 0;
	PvResult		lResult;
	PvPayloadType	lType;
	u32				i=0;
	u32				lImageCount = *ImageCount;
	p_u8			lPointer = Buffer;
	u32				lTimeOut = FrameTimeOut;

	cAcqResult = PvResult::Code::OK;

	if( !cStream )
	{
		return AS_GIGE_STREAM_NOT_AVAILABLE;
	}

	if( !cPipeline )
	{
		return AS_GIGE_PIPELINE_NOT_AVAILABLE;
	}
	
	cResult = cResetCmd->Execute();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_RESET_COMMAND_ERROR;
	}	
	
	cResult = cPipeline->Start();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_PIPELINE_START_ERROR;
	}

	cResult = cDevice->StreamEnable();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_STREAM_ENABLE_ERROR;
	}

	cResult = cStartCmd->Execute();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_START_COMMAND_ERROR;
	}

	if( cReadyCallBack )
	{
#ifdef __linux__
		cReadyCallBack->armed();
#else
		cReadyCallBack( this );
#endif
	}

	while ( cAcqResult.IsOK() && (i<lImageCount) )
	{
		cAcqResult = cPipeline->RetrieveNextBuffer( &lBuffer, lTimeOut, &lResult );
	
		if( cAcqResult.IsOK() )
		{
			if( lResult.IsOK() )
			{
				lType = lBuffer->GetPayloadType();

				if( lType == PvPayloadTypeImage )
				{
					lRawBuffer = lBuffer->GetDataPointer();
					lSize = lBuffer->GetSize();

					memcpy( lPointer, lRawBuffer, lSize );
					
					if (!i)
					{
						lTimeOut = cFrameTimeOut;
					}

					i++;

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

			if( cAcqResult.IsOK() )
			{
				cResult = cPipeline->ReleaseBuffer( lBuffer );
			}
			else
			{
				cPipeline->ReleaseBuffer( lBuffer );
			}

			if( cAcqResult.IsOK() )
			{
				cAcqResult = cBlocksDropped->GetValue( cBlocksDroppedVal );

				if( cAcqResult.IsOK() && cBlocksDroppedVal )
				{
					cAcqResult = PvResult::Code::ABORTED;
				}
			}

			if( cAcqResult.IsOK() )
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

	if( !cResult.IsOK() )
	{
		return AS_GIGE_STOP_COMMAND_ERROR;
	}

	if( cFinishCallBack )
	{
#ifdef __linux__
		cFinishCallBack->finished();
#else
		cFinishCallBack( this );
#endif
	}

	cResult = cDevice->StreamDisable();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_STREAM_DISABLE_ERROR;
	}

	cResult = cPipeline->Stop();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_PIPELINE_STOP_ERROR;
	}

	if( !cAcqResult.IsOK() )
	{
		cResult = cAcqResult;
		return AS_GIGE_ACQUISION_ABORTED_ERROR;
	}

	return AS_NO_ERROR;
}

i32	GigEDevice::AcquireImageThread( u32 ImageCount, u32 FrameTimeOut )
{
	PvBuffer		*lBuffer = NULL;
	p_u8			lRawBuffer = NULL;
	u32				lSize = 0;
	PvResult		lAcqResult = PvResult::Code::OK;
	PvPayloadType	lType;
	u32				lCurrentFrameWithinBuffer=0;
	p_u8			lTransferBuffer = NULL;
	p_u8			lPointer = NULL;
	i32				lResult = AS_NO_ERROR;
	u32				lTimeOut = FrameTimeOut;

	cAcqResult = PvResult::Code::OK;
	
	ClearQueue();
	InitializeQueue();

	cAcquiredImages = 0;

	if( ImageCount )
	{
		cContinuous = 0;
	}
	else
	{
		cContinuous = 1;
	}

	if( !cStream )
	{
		return AS_GIGE_STREAM_NOT_AVAILABLE;
	}

	if( !cPipeline )
	{
		return AS_GIGE_PIPELINE_NOT_AVAILABLE;
	}
	
	cResult = cResetCmd->Execute();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_RESET_COMMAND_ERROR;
	}

	cResult = cPipeline->Start();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_PIPELINE_START_ERROR;
	}

	cResult = cDevice->StreamEnable();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_STREAM_ENABLE_ERROR;
	}

	cResult = cStartCmd->Execute();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_START_COMMAND_ERROR;
	}

	cStopAcquisition = 0;

	if( cReadyCallBack )
	{
#ifdef __linux__
		cReadyCallBack->armed();
#else
		cReadyCallBack( this );
#endif
	}

	while ( cAcqResult.IsOK() && ((cAcquiredImages<ImageCount) || cContinuous) )
	{
		if( cStopAcquisition )
		{
			break;
		}
		
		if( lCurrentFrameWithinBuffer == 0 )	// new Transferbuffer
		{
#ifdef __linux__
			cAvailableTransferBufferLock.lock();
#else
			EnterCriticalSection( &cAvailableTransferBufferLock );
#endif
			if (cAvailableTransferBuffer.size())
			{
				lTransferBuffer = cAvailableTransferBuffer.front();
				lPointer = lTransferBuffer;
				cAvailableTransferBuffer.pop();
#ifdef __linux__
	            cAvailableTransferBufferLock.unlock();
#else
				LeaveCriticalSection( &cAvailableTransferBufferLock );
#endif
			}
			else
			{
#ifdef __linux__
	            cAvailableTransferBufferLock.unlock();
#else
				LeaveCriticalSection( &cAvailableTransferBufferLock );
#endif
				lResult = AS_GIGE_NO_TRANSFER_BUFFER_AVAILABLE;
				break;
			}
		}
		
		cAcqResult = cPipeline->RetrieveNextBuffer( &lBuffer, lTimeOut, &lAcqResult );
	
		if( cAcqResult.IsOK() )
		{
			if( lAcqResult.IsOK() )
			{
				lType = lBuffer->GetPayloadType();

				if( lType == PvPayloadTypeImage )
				{
					lRawBuffer = lBuffer->GetDataPointer();
					lSize = lBuffer->GetSize();

					std::copy( lRawBuffer, lRawBuffer + lSize, lPointer );
					
					lCurrentFrameWithinBuffer++;

					if (!cAcquiredImages)
					{
						lTimeOut = cFrameTimeOut;
					}

					cAcquiredImages++;
					lPointer = lPointer + lSize;

					if( lCurrentFrameWithinBuffer >= cTransferBufferFrameCount )
					{
						BufferReadyCallBack( lTransferBuffer, lCurrentFrameWithinBuffer );

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

			if( cAcqResult.IsOK() )
			{
				cAcqResult = cPipeline->ReleaseBuffer( lBuffer );
			}
			else
			{
				cPipeline->ReleaseBuffer( lBuffer );
			}
			
			if( cAcqResult.IsOK() )
			{
				cAcqResult = cBlocksDropped->GetValue( cBlocksDroppedVal );

				if( cAcqResult.IsOK() && cBlocksDroppedVal )
				{
					lResult = AS_GIGE_BLOCKS_DROPPED;
					break;
				}
			}

			if( cAcqResult.IsOK() )
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

	if( lCurrentFrameWithinBuffer )
	{
		BufferReadyCallBack( lTransferBuffer, lCurrentFrameWithinBuffer );
	}

	cResult = cStopCmd->Execute();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_STOP_COMMAND_ERROR;
	}

	if( cFinishCallBack )
	{
#ifdef __linux__
		cFinishCallBack->finished();
#else
		cFinishCallBack( this );
#endif
	}

	cResult = cDevice->StreamDisable();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_STREAM_DISABLE_ERROR;
	}

	cResult = cPipeline->Stop();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_PIPELINE_STOP_ERROR;
	}

	if( !cAcqResult.IsOK() )
	{
		cResult = cAcqResult;
		return AS_GIGE_ACQUISION_ABORTED_ERROR;
	}

	return lResult;
}

void GigEDevice::BufferReadyCallBack( p_u8 aTransferBuffer, u32 aCurrentFrameWithinBuffer )
{
	if ( cBufferCallBack )
	{
		cBufferCallBack->bufferReady( aTransferBuffer, aCurrentFrameWithinBuffer );
	}
	else
	{
#ifdef __linux__
        cAvailableTransferBufferLock.lock();
		cAvailableTransferBuffer.push(aTransferBuffer);
        cAvailableTransferBufferLock.unlock();
#else
		EnterCriticalSection( &cAvailableTransferBufferLock );
		cAvailableTransferBuffer.push(aTransferBuffer);
		LeaveCriticalSection( &cAvailableTransferBufferLock );
#endif
	}
}

void GigEDevice::ClearQueue()
{
	while( cAvailableTransferBuffer.size() )
	{
		cAvailableTransferBuffer.pop();
	}
}

i32	GigEDevice::ClosePipeline()
{
	cResult = PvResult::Code::OK;
	
	if( !cPipeline )
	{
		return AS_NO_ERROR;
	}

	if( cPipeline->IsStarted() )
	{
		cResult = cPipeline->Stop();
	}
	delete cPipeline;
	cPipeline = NULL;

	if( !cResult.IsOK() )
	{
		return AS_GIGE_PIPELINE_STOP_ERROR;
	}

	return AS_NO_ERROR;
}

i32 GigEDevice::CloseSerialPort()
{
	cResult = cPort.Close();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_SERIAL_PORT_CLOSE_ERROR;
	}

	return AS_NO_ERROR;
}

i32 GigEDevice::CloseStream()
{
	i32	lResult = AS_NO_ERROR;
	
	if( cPipeline )
	{
		lResult = ClosePipeline();
	}

	if( ( lResult == AS_NO_ERROR ) && ( cStream != NULL ) )
	{
		cResult = cStream->Close();

		if( !cResult.IsOK() )
		{
			return AS_GIGE_STREAM_CLOSE_ERROR;
		}

		PvStream::Free( cStream );
		cStream = NULL;
	}

	return AS_NO_ERROR;
}

PvResult GigEDevice::ConfigureSerialBulk( PvDeviceSerial SerialPort )
{
	PvResult	lResult;
	u64			lBulkSelektor = SerialPort - 2;

	lResult = cDeviceParams->SetEnumValue( "BulkSelector", lBulkSelektor );
	lResult = cDeviceParams->SetEnumValue( "BulkMode", "UART" );
	lResult = cDeviceParams->SetEnumValue( "BulkBaudRate", "Baud38400" );
	lResult = cDeviceParams->SetEnumValue( "BulkNumOfStopBits", "One" );
	lResult = cDeviceParams->SetEnumValue( "BulkParity", "None" );

	return lResult;
}

PvResult GigEDevice::ConfigureSerialUart( PvDeviceSerial SerialPort )
{
	PvResult lResult;

	lResult = cDeviceParams->SetEnumValue( "Uart0BaudRate", "Baud38400" );
	lResult = cDeviceParams->SetEnumValue( "Uart0NumOfStopBits", "One" );
	lResult = cDeviceParams->SetEnumValue( "Uart0Parity", "None" );

	return lResult;
}

PvResult GigEDevice::ConfigureStream( bool TimeoutCountedAsError, bool AbortCountedAsError )
{
	PvResult lResult = PvResult::Code::OK;
	
	// If this is a GigE Vision device, configure GigE Vision specific streaming parameters
	PvDeviceGEV* lDeviceGEV = dynamic_cast<PvDeviceGEV *>( cDevice );
	if ( lDeviceGEV != NULL )
	{
		PvStreamGEV *lStreamGEV = static_cast<PvStreamGEV *>( cStream );

		// Negotiate packet size
		lResult = lDeviceGEV->NegotiatePacketSize();

		if( lResult.IsOK() )
		{
			// Configure device streaming destination
			lResult = lDeviceGEV->SetStreamDestination( lStreamGEV->GetLocalIPAddress(), lStreamGEV->GetLocalPort() );
		}
	}

	if( lResult.IsOK() )
	{
		lResult = cStreamParams->SetBooleanValue( "TimeoutCountedAsError", TimeoutCountedAsError );
	}

	if( lResult.IsOK() )
	{
		lResult = cStreamParams->SetBooleanValue( "AbortCountedAsError", AbortCountedAsError );
	}

	return lResult;
}

PvResult GigEDevice::Connect(const str8 aDeviceDescriptor)
{
	PvResult lResult;
	lResult= cSystem.FindDevice(aDeviceDescriptor, &cDeviceInfo);

	if( lResult.IsOK() )
	{
		cDevice = PvDevice::CreateAndConnect( cDeviceInfo, &lResult);
	}

	return lResult;
}

i32	GigEDevice::CreatePipeline( u32 BufferCount )
{
	u32 NeededBufferSize = 0;
	
	if( !cStream )
	{
		return AS_GIGE_STREAM_NOT_AVAILABLE;
	}

	if( cPipeline )
	{
		return AS_GIGE_PIPELINE_CREATED_ALREADY;
	}

	cPipeline = new PvPipeline( cStream );

	NeededBufferSize = cDevice->GetPayloadSize();

	for ( u32 i=0 ; i<cTransferBuffer.size() ; i++ )
	{
		try
		{
			cTransferBuffer[i].resize( cTransferBufferFrameCount * NeededBufferSize , 0 );
		}
		catch( std::bad_alloc )
		{
			cResult = PvResult::Code::NOT_ENOUGH_MEMORY;
		}
	}

	if( cResult.IsOK() )
	{
		cPipeline->SetBufferSize( NeededBufferSize );
		cResult = cPipeline->SetBufferCount( BufferCount );
	}

	if( !cResult.IsOK() )
	{
		cTransferBuffer.clear();
		delete cPipeline;
		cPipeline = NULL;
		return AS_GIGE_PIPELINE_CREATION_ERROR;
	}

	return AS_NO_ERROR;
}

i32	GigEDevice::FlushRxBuffer()
{
	cResult = cPort.FlushRxBuffer();

	if( !cResult.IsOK() )
	{
		return AS_GIGE_SERIAL_PORT_FLUSH_ERROR;
	}

	return AS_NO_ERROR;
}

u64 GigEDevice::GetAcquiredImageCount()
{
	return cAcquiredImages;
}

GigEDeviceInfoStr GigEDevice::GetDeviceInfoStr()
{
	return cDeviceInformationStr;
}

i32 GigEDevice::GetErrorDescription( PvResult aPleoraErrorCode,
									 str8 aPleoraErrorCodeString,
									 p_u32 aPleoraErrorCodeStringLen,
									 str8 aPleoraErrorDescription,
									 p_u32 aPleoraErrorDescriptionLen)
{
	u32			lSize = 0;
	i32			dResult = AS_NO_ERROR;

	if( aPleoraErrorCodeString && aPleoraErrorCodeStringLen && *aPleoraErrorCodeStringLen )
	{
		if( aPleoraErrorCode.GetCodeString().GetLength() > ((*aPleoraErrorCodeStringLen)-1) )
		{
			lSize = (*aPleoraErrorCodeStringLen)-1;
			dResult = AS_BUFFER_TO_SMALL;
		}
		else
		{
			lSize = aPleoraErrorCode.GetCodeString().GetLength();
		}		
		memcpy( aPleoraErrorCodeString, aPleoraErrorCode.GetCodeString().GetAscii(), lSize );
		aPleoraErrorCodeString[lSize] = 0x00;
	}

	if( aPleoraErrorCodeStringLen )
	{
		*aPleoraErrorCodeStringLen = (aPleoraErrorCode.GetCodeString().GetLength())+1;
	}

	if( aPleoraErrorDescription && aPleoraErrorDescriptionLen && *aPleoraErrorDescriptionLen )
	{
		if( aPleoraErrorCode.GetDescription().GetLength() > ((*aPleoraErrorDescriptionLen)-1) )
		{
			lSize = (*aPleoraErrorDescriptionLen)-1;
			dResult = AS_BUFFER_TO_SMALL;
		}
		else
		{
			lSize = aPleoraErrorCode.GetDescription().GetLength();
		}
		memcpy( aPleoraErrorDescription, aPleoraErrorCode.GetDescription().GetAscii(), lSize );
		aPleoraErrorDescription[lSize] = 0x00;
	}

	if( aPleoraErrorDescriptionLen )
	{
		*aPleoraErrorDescriptionLen = (aPleoraErrorCode.GetDescription().GetLength())+1;
	}

	return dResult;
}

i32	GigEDevice::GetBufferHandlingThreadPriority()
{
	if( cPipeline )
	{
		return cPipeline->GetBufferHandlingThreadPriority();
	}
	else
	{
		return -1;
	}
}

i32 GigEDevice::GetIntegerValue( const str8 Property, i64 &Value )
{
	i32 lResult = AS_NO_ERROR;
	
	cResult = cStreamParams->GetIntegerValue( Property, Value );

	if( !cResult.IsOK() )
	{
		lResult = AS_GIGE_GET_INTEGER_VALUE_ERROR;
	}

	return lResult;
}

PvResult GigEDevice::GetLastResult()
{
	return cResult;
}

GigEDevice::GigEDevice(const str8 aDeviceDescriptor)
{
	cDeviceInfo					= NULL;
	cDeviceInfoGEV				= NULL;
	cDeviceInfoU3V				= NULL;
	cDevice						= NULL;
	cDeviceParams				= NULL;
	cStreamParams				= NULL;
	cDeviceAdapter				= NULL;
	cStream						= NULL;
	cPipeline					= NULL;
	cStartCmd					= NULL;
	cStopCmd					= NULL;
	cResetCmd					= NULL;
	cTransferBufferFrameCount	= 0;
	cTransferBuffer.clear();
	cUseTermChar				= 0;
	cTermChar					= 0;
	cReadyCallBack				= NULL;
	cFinishCallBack				= NULL;
	cBufferCallBack				= NULL;
	cAcquiredImages				= 0;
	cContinuous					= 0;
	cStopAcquisition			= 0;
	cBlocksDroppedVal			= 0;
	cBlocksDropped				= NULL;
	cBlockIDsMissingVal			= 0;
	cBlockIDsMissing			= NULL;
	cFrameTime					= 0;
	cFrameTimeOut				= 0;

	cResult						= PvResult::Code::OK;
	cAcqResult					= PvResult::Code::OK;

#ifndef __linux__
	InitializeCriticalSectionAndSpinCount(&cAvailableTransferBufferLock, 0x0400);
#endif
	ClearQueue();

	cResult = Connect( aDeviceDescriptor );

	if( cResult.IsOK() )
	{
		cDeviceAdapter	= new PvDeviceAdapter( cDevice );
		cDeviceParams	= cDevice->GetParameters();
		cStartCmd		= dynamic_cast<PvGenCommand *>( cDeviceParams->Get( "AcquisitionStart" ) );
		cStopCmd		= dynamic_cast<PvGenCommand *>( cDeviceParams->Get( "AcquisitionStop" ) );
	}

	if( cResult.IsOK() )
	{
		cDeviceInformation.Vendor				= cDeviceInfo->GetVendorName();
		cDeviceInformationStr.Vendor			= (str8)cDeviceInformation.Vendor.GetAscii();

		cDeviceInformation.Model				= cDeviceInfo->GetModelName();
		cDeviceInformationStr.Model				= (str8)cDeviceInformation.Model.GetAscii();

		cDeviceInformation.ManufacturerInfo		= cDeviceInfo->GetManufacturerInfo();
		cDeviceInformationStr.ManufacturerInfo	= (str8)cDeviceInformation.ManufacturerInfo.GetAscii();

		cDeviceInformation.SerialNumber			= cDeviceInfo->GetSerialNumber();
		cDeviceInformationStr.SerialNumber		= (str8)cDeviceInformation.SerialNumber.GetAscii();

		cDeviceInformation.UserId				= cDeviceInfo->GetUserDefinedName();
		cDeviceInformationStr.UserId			= (str8)cDeviceInformation.UserId.GetAscii();

		cDeviceInfoType							= cDeviceInfo->GetType();
		
		switch( cDeviceInfoType )
		{
			case PvDeviceInfoType::PvDeviceInfoTypeGEV:
				cDeviceInfoGEV = (PvDeviceInfoGEV *)(cDeviceInfo);

				cDeviceInformation.MacAddress		= cDeviceInfoGEV->GetMACAddress();
				cDeviceInformationStr.MacAddress	= (str8)cDeviceInformation.MacAddress.GetAscii();

				cDeviceInformation.IpAddress		= cDeviceInfoGEV->GetIPAddress();
				cDeviceInformationStr.IpAddress		= (str8)cDeviceInformation.IpAddress.GetAscii();

				cDeviceInformation.NetMask			= cDeviceInfoGEV->GetSubnetMask();
				cDeviceInformationStr.NetMask		= (str8)cDeviceInformation.NetMask.GetAscii();

				cDeviceInformation.GateWay			= cDeviceInfoGEV->GetDefaultGateway();
				cDeviceInformationStr.GateWay		= (str8)cDeviceInformation.GateWay.GetAscii();

				break;
			
			default:
				break;
		}
	}
}

void GigEDevice::InitializeQueue()
{
	for( u32 i=0; i<cTransferBuffer.size() ; i++ )
	{
		cAvailableTransferBuffer.push( &cTransferBuffer[i][0] );
	}
}

i32 GigEDevice::OpenSerialPort( PvDeviceSerial SerialPort, u32 RxBufferSize, u8 UseTermChar, u8 TermChar )
{
	if( cPort.IsOpened() )
	{
		cResult = cPort.Close();
	}
	
	if( !cResult.IsOK() )
	{
		return AS_GIGE_SERIAL_PORT_CLOSE_ERROR;
	}

	if( !cPort.IsSupported( cDeviceAdapter, SerialPort ) )
	{
		return AS_GIGE_SERIAL_PORT_NOT_SUPPORTED;
	}

	if( (SerialPort == PvDeviceSerial0) || (SerialPort == PvDeviceSerial1) )
	{
		cResult = ConfigureSerialUart( SerialPort );
	}
	else if( (SerialPort >= PvDeviceSerialBulk0) && (SerialPort <= PvDeviceSerialBulk7) )
	{
		cResult = ConfigureSerialBulk( SerialPort );
	}

	if( !cResult.IsOK() )
	{
		return AS_GIGE_SERIAL_PORT_CONFIG_FAILED;
	}

	cResult = cPort.SetRxBufferSize( RxBufferSize );

	if( !cResult.IsOK() )
	{
		return AS_GIGE_SERIAL_PORT_SET_RX_BUFFER_FAILED;
	}

	cResult = cPort.Open( cDeviceAdapter, SerialPort );

	if( !cResult.IsOK() )
	{
		return AS_GIGE_SERIAL_PORT_OPEN_ERROR;
	}

	cUseTermChar = UseTermChar;
	cTermChar = TermChar;
		
	return AS_NO_ERROR;
}

i32 GigEDevice::OpenStream( bool TimeoutCountedAsError, bool AbortCountedAsError )
{
	if( !cStream )
	{
		cStream = PvStream::CreateAndOpen( cDeviceInfo->GetConnectionID(), &cResult );

		if( !cResult.IsOK() )
		{
			return AS_GIGE_STREAM_OPEN_ERROR;
		}

		cStreamParams		= cStream->GetParameters();
		cResetCmd			= dynamic_cast<PvGenCommand *>( cStreamParams->Get( "Reset" ) );
		cBlocksDropped		= dynamic_cast<PvGenInteger *>( cStreamParams->Get( "BlocksDropped" ) );
		cBlockIDsMissing	= dynamic_cast<PvGenInteger *>( cStreamParams->Get( "BlockIDsMissing" ) );

		cResult = ConfigureStream( TimeoutCountedAsError, AbortCountedAsError );

		if( !cResult.IsOK() )
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

i32	GigEDevice::ReadSerialPort( p_u8 RxBuffer, u32 RxBufferSize, u32 *BytesRead, u32 TimeOut )
{
	u32 lBytesRead = 0;
	
	if( cUseTermChar )
	{
		*BytesRead = 0;

		while ( *BytesRead < RxBufferSize )
		{
			cResult = cPort.Read( RxBuffer + *BytesRead, RxBufferSize - *BytesRead, lBytesRead, TimeOut );

			if( !cResult.IsOK() )
			{
				return AS_GIGE_SERIAL_PORT_READ_ERROR;
			}

			*BytesRead = *BytesRead + lBytesRead;

			if ( RxBuffer[ *BytesRead-1 ] == cTermChar )
			{
				return AS_NO_ERROR;
			}
		}

		return  AS_GIGE_SERIAL_PORT_READ_BUFFER_FULL;
	}
	else
	{
		cResult = cPort.Read( RxBuffer, RxBufferSize, lBytesRead, TimeOut );

		*BytesRead = lBytesRead;

		if( !cResult.IsOK() )
		{
			return AS_GIGE_SERIAL_PORT_READ_ERROR;
		}
	}

	return AS_NO_ERROR;
}

#ifndef __linux__
void GigEDevice::RegisterAcqArmedCallBack( p_readyCallBack aAcqArmedCallBack )
{
	cReadyCallBack = aAcqArmedCallBack;
}

void GigEDevice::RegisterAcqFinishCallBack( p_readyCallBack aAcqFinishCallBack )
{
	cFinishCallBack = aAcqFinishCallBack;
}

void GigEDevice::RegisterTransferBufferReadyCallBack( p_bufferCallBack aTransferBufferReadyCallBack )
{
	cBufferCallBack = aTransferBufferReadyCallBack;
}
#endif

void GigEDevice::ReturnBuffer( p_u8 Buffer )
{
#ifdef __linux__
    cAvailableTransferBufferLock.lock();
	cAvailableTransferBuffer.push(Buffer);
    cAvailableTransferBufferLock.unlock();
#else
	EnterCriticalSection( &cAvailableTransferBufferLock );
	cAvailableTransferBuffer.push(Buffer);
	LeaveCriticalSection( &cAvailableTransferBufferLock );
#endif
}

void GigEDevice::SetFrameTime( dbl FrameTime )
{
	cFrameTime = FrameTime;
}

void GigEDevice::SetFrameTimeOut( u32 FrameTimeOut )
{
	if( FrameTimeOut > AS_HEXITEC_MIN_FRAME_TIMEOUT )
	{
		cFrameTimeOut = FrameTimeOut;
	}
	else
	{
		cFrameTimeOut = AS_HEXITEC_MIN_FRAME_TIMEOUT;
	}
}

i32	GigEDevice::SetImageFormatControl( const str8 PixelFormat, u64 Width, u64 Height, u64 OffsetX, u64 OffsetY, const str8 SensorTaps, const str8 TestPattern )
{
	cResult = cDeviceParams->SetEnumValue( "PixelFormat", PixelFormat );

	if( cResult.IsOK() )
	{
		cResult = cDeviceParams->SetIntegerValue( "Width", Width );
	}

	if( cResult.IsOK() )
	{
		cResult = cDeviceParams->SetIntegerValue( "Height", Height );
	}

	if( cResult.IsOK() )
	{
		cResult = cDeviceParams->SetIntegerValue( "OffsetX", OffsetX );
	}

	if( cResult.IsOK() )
	{
		cResult = cDeviceParams->SetIntegerValue( "OffsetY", OffsetY );
	}

	if( cResult.IsOK() )
	{
		cResult = cDeviceParams->SetEnumValue( "SensorDigitizationTaps", SensorTaps );
	}

	if( cResult.IsOK() )
	{
		cResult = cDeviceParams->SetEnumValue( "TestPattern", TestPattern );
	}

	if( !cResult.IsOK() )
	{
		return AS_GIGE_SET_IMAGE_FORMAT_ERROR;
	}

	return AS_NO_ERROR;
}

void GigEDevice::SetTransferBuffer( u32 TransferBufferCount, u32 TransferBufferFrameCount )
{
	cTransferBufferFrameCount = TransferBufferFrameCount;
	cTransferBuffer.resize( TransferBufferCount );
}

void GigEDevice::StopAcquisition()
{
	cStopAcquisition = 1;
}

i32	GigEDevice::WriteSerialPort( const p_u8 TxBuffer, u32 TxBufferSize, u32 *BytesWritten )
{
	u32 lBytesWritten = 0;

	cResult = cPort.Write( TxBuffer, TxBufferSize, lBytesWritten );

	*BytesWritten = lBytesWritten;

	if( !cResult.IsOK() )
	{
		return AS_GIGE_SERIAL_PORT_WRITE_ERROR;
	}

	return AS_NO_ERROR;
}
