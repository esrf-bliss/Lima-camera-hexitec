// GigE.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "aS_lib.h"
#include "GigE.h"

InstanceManager			hTable;

EXTERN_C	GIGE_API	i32		AcquireFrame(
												GigEDevice* deviceHdl,
												p_u32 frameCount,
												p_u8 buffer,
												u32 frameTimeOut )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->AcquireImage( frameCount, buffer, frameTimeOut );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		AcquireFrames(
												GigEDevice* deviceHdl,
												u32 frameCount,
												p_u64 framesAcquired,
												u32 frameTimeOut )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->AcquireImageThread( frameCount, frameTimeOut );
		*framesAcquired = deviceHdl->GetAcquiredImageCount();
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		CheckFirmware(
												GigEDevice* deviceHdl,
												p_u8 customerId,
												p_u8 projectId,
												p_u8 version,
												u8 forceEqualVersion,
												u32 timeOut )
{
	i32 lResult = AS_NO_ERROR;
	u8 lRequiredCustomerId = *customerId;
	u8 lRequiredProjectId = *projectId;
	u8 lRequiredVersion = *version;

	lResult = ReadRegister( deviceHdl, AS_FPGA_FW_CHECK_CUSTOMER_REG, customerId, timeOut);

	if( lResult == AS_NO_ERROR )
	{
		lResult = ReadRegister( deviceHdl, AS_FPGA_FW_CHECK_PROJECT_REG, projectId, timeOut);
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = ReadRegister( deviceHdl, AS_FPGA_FW_CHECK_VERSION_REG, version, timeOut);
	}

	if( (lResult == AS_NO_ERROR) && (lRequiredCustomerId != *customerId) )
	{
		lResult = AS_FPGA_FW_CHECK_CUSTOMER_ERROR;
	}

	if( (lResult == AS_NO_ERROR) && (lRequiredProjectId != *projectId) )
	{
		lResult = AS_FPGA_FW_CHECK_PROJECT_ERROR;
	}

	if( (lResult == AS_NO_ERROR) && forceEqualVersion && (lRequiredVersion != *version) )
	{
		lResult = AS_FPGA_FW_CHECK_VERSION_ERROR;
	}
	else if( (lResult == AS_NO_ERROR) && (lRequiredVersion > *version) )
	{
		lResult = AS_FPGA_FW_CHECK_VERSION_ERROR;
	}

	return lResult;
}

EXTERN_C	GIGE_API	i32		ClosePipeline(
												GigEDevice* deviceHdl )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->ClosePipeline();
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		CloseSerialPort(
												GigEDevice* deviceHdl )
{				
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->CloseSerialPort();
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		CloseStream(
												GigEDevice* deviceHdl )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->CloseStream();
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		CollectOffsetValues(
												GigEDevice* deviceHdl,
												u32 timeOut,
												u32 collectDcTimeOut )
{
	i32 dResult = AS_NO_ERROR;
	u8	lValue = 0;
	HexitecOperationMode lCurrentMode;
	HexitecOperationMode lCollectMode;

	dResult = GetOperationMode( deviceHdl, &lCurrentMode, timeOut );

	if( dResult == AS_NO_ERROR )
	{
		lCollectMode = lCurrentMode;
		lCollectMode.DcCollectDarkCorrectionValues = Control::AS_CONTROL_ENABLED;
		lCollectMode.DcUploadDarkCorrectionValues = Control::AS_CONTROL_DISABLED;
		lCollectMode.DcDisableVcalPulse = Control::AS_CONTROL_ENABLED;
		
		dResult = DisableSM( deviceHdl, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{		
		dResult = SetOperationMode( deviceHdl, lCollectMode, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{
		dResult = EnableSM( deviceHdl, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{
		Sleep( collectDcTimeOut );
		dResult = ReadRegister( deviceHdl, 0x89, &lValue, timeOut );
	}

	if( dResult == AS_NO_ERROR && !( lValue & 0x01 ) )
	{
		dResult = AS_COLLECT_DC_NOT_READY;
	}

	if( dResult == AS_NO_ERROR )
	{		
		dResult = DisableSM( deviceHdl, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{		
		dResult = SetOperationMode( deviceHdl, lCurrentMode, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{
		dResult = EnableSM( deviceHdl, timeOut );
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		ConfigureDetector(
												GigEDevice* deviceHdl,
												const HexitecSensorConfigPtr sensorConfig,
												const HexitecOperationModePtr operationMode,
												const HexitecSystemConfigPtr systemConfig,
												p_u8 width,
												p_u8 height,
												p_dbl frameTime,
												p_u32 collectDcTime,
												u32 timeOut )
{
	return ConfigureDetectorWithTrigger( deviceHdl, sensorConfig, operationMode, systemConfig, width, height, frameTime, collectDcTime, timeOut, AS_CONTROL_DISABLED, AS_CONTROL_DISABLED );
}

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
												Control enTriggerMode )
{
	i32 lResult = AS_NO_ERROR;
	u8 lValue = 0;

	lResult = DisableSM( deviceHdl, timeOut );

	if( lResult == AS_NO_ERROR )
	{
		if( enSyncMode )
		{
			lResult = EnableSyncMode( deviceHdl, timeOut );
		}
		else
		{
			lResult = DisableSyncMode( deviceHdl, timeOut );
		}
	}

	if( lResult == AS_NO_ERROR )
	{
		if( enTriggerMode )
		{
			lResult = EnableTriggerMode( deviceHdl, timeOut );
		}
		else
		{
			lResult = DisableTriggerMode( deviceHdl, timeOut );
		}
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = 0x01;
		lResult = WriteRegister( deviceHdl, 0x07, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = (*systemConfig).AdcDelay;
		lResult = WriteRegister( deviceHdl, 0x09, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = (*systemConfig).SyncSignalDelay;
		lResult = WriteRegister( deviceHdl, 0x0e, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = (*systemConfig).AdcSample;
		lResult = WriteRegister( deviceHdl, 0x14, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = SetSensorConfig( deviceHdl, *sensorConfig, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = SetOperationMode( deviceHdl, *operationMode, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = EnableFunctionBlocks( deviceHdl, Control::AS_CONTROL_DISABLED, Control::AS_CONTROL_ENABLED, Control::AS_CONTROL_ENABLED, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = EnableSM( deviceHdl, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = EnableFunctionBlocks( deviceHdl, Control::AS_CONTROL_ENABLED, Control::AS_CONTROL_ENABLED, Control::AS_CONTROL_ENABLED, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = (*systemConfig).AdcClockPhase;
		lResult = WriteAdcRegister( deviceHdl, 0x16, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = ReadResolution( deviceHdl, width, height, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		*frameTime = GetFrameTime( sensorConfig, *width, *height );
		*collectDcTime = GetCollectDcTime( *frameTime );
	}

	return lResult;
}

EXTERN_C	GIGE_API	void	CopyBuffer(
												p_u8 sourceBuffer,
												p_u8 destBuffer,
												u32 byteCount )
{
	memcpy( destBuffer, sourceBuffer, byteCount );
}

EXTERN_C	GIGE_API	i32		CreatePipeline(
												GigEDevice* deviceHdl,
												u32 bufferCount,
												u32 transferBufferCount,
												u32 transferBufferFrameCount )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		deviceHdl->SetTransferBuffer( transferBufferCount, transferBufferFrameCount );
		dResult = deviceHdl->CreatePipeline( bufferCount );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		CreatePipelineOld(
												GigEDevice* deviceHdl,
												u32 bufferCount )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->CreatePipeline( bufferCount );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		DisableSM(
												GigEDevice* deviceHdl,
												u32 timeOut )
{
	u8	lValue = AS_CONTROL_DISABLED;	
	return WriteRegister( deviceHdl, 0x01, &lValue, timeOut);
}

EXTERN_C	GIGE_API	i32		DisableSyncMode(
												GigEDevice* deviceHdl,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	
	lRxBuffer.resize(7, 0);
	lTxBuffer.resize(8, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x43;
	lTxBuffer[3] = 0x30;
	lTxBuffer[4] = 0x41;
	lTxBuffer[5] = 0x30;
	lTxBuffer[6] = 0x31;
	lTxBuffer[7] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	return lResult;
}

EXTERN_C	GIGE_API	i32		DisableTriggerMode(
												GigEDevice* deviceHdl,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	
	lRxBuffer.resize(7, 0);
	lTxBuffer.resize(8, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x43;
	lTxBuffer[3] = 0x30;
	lTxBuffer[4] = 0x41;
	lTxBuffer[5] = 0x30;
	lTxBuffer[6] = 0x32;
	lTxBuffer[7] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	return lResult;
}

EXTERN_C	GIGE_API	i32		EnableFunctionBlocks(
												GigEDevice* deviceHdl,
												Control	adcEnable,
												Control	dacEnable,
												Control	peltierEnable,
												u32 timeOut )
{
	u8		lValue = 0;
	i32		lResult = AS_NO_ERROR;
	string8			lTxBuffer = "";
	string8			lRxBuffer = "";
	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;

	lValue = adcEnable;
	lValue = lValue + (dacEnable * 0x02);
	lValue = lValue + (peltierEnable * 0x04);

	lRxBuffer.resize(5, 0);
	lTxBuffer.resize(6, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x55;
	HexToString( lValue, 2, (p_u8)&lTxBuffer[3] );
	lTxBuffer[5] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	return lResult;
}

EXTERN_C	GIGE_API	i32		EnableSM(
												GigEDevice* deviceHdl,
												u32 timeOut )
{
	u8	lValue = AS_CONTROL_ENABLED;
	return WriteRegister( deviceHdl, 0x01, &lValue, timeOut);
}

EXTERN_C	GIGE_API	i32		EnableSyncMode(
												GigEDevice* deviceHdl,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	
	lRxBuffer.resize(7, 0);
	lTxBuffer.resize(8, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x42;
	lTxBuffer[3] = 0x30;
	lTxBuffer[4] = 0x41;
	lTxBuffer[5] = 0x30;
	lTxBuffer[6] = 0x31;
	lTxBuffer[7] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	return lResult;
}

EXTERN_C	GIGE_API	i32		EnableTriggerMode(
												GigEDevice* deviceHdl,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	
	lRxBuffer.resize(7, 0);
	lTxBuffer.resize(8, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x42;
	lTxBuffer[3] = 0x30;
	lTxBuffer[4] = 0x41;
	lTxBuffer[5] = 0x30;
	lTxBuffer[6] = 0x32;
	lTxBuffer[7] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	return lResult;
}

EXTERN_C	GIGE_API	i32		ExitDevice(
												GigEDevice* deviceHdl )
{				
	i32 dResult = AS_NO_ERROR;
	
	dResult = hTable.RemInstance(deviceHdl);
	
	if ( dResult == AS_NO_ERROR )
	{
		delete deviceHdl;
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		GetAsErrorMsg(
												i32 asError,
												str8 asErrorMsg,
												u32 length )
{
	hdl		ghResDll	= NULL;
	i32		charCnt		= 0;
	i32		lAsError	= asError;
	p_i32	pArgs[]		= { &lAsError };
	
	if (asErrorMsg != NULL)
	{
		ghResDll = LoadLibrary(AS_MESSAGE_DLL);
			
		if (ghResDll != NULL)
		{
			charCnt = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, ghResDll, lAsError, 0, asErrorMsg, length, (va_list*)pArgs);
			FreeLibrary((HMODULE)ghResDll);
		}

		if ((ghResDll == NULL) || (charCnt == 0))
		{
			return GetLastError();
		}
	}

	return AS_NO_ERROR;
}

EXTERN_C	GIGE_API	i32		GetDeviceInformation(
												GigEDevice* deviceHdl,
												GigEDeviceInfoStrPtr deviceInfoStr )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		*deviceInfoStr = deviceHdl->GetDeviceInfoStr();
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	dbl		GetFrameTime(
												const HexitecSensorConfigPtr sensorConfig,
												u8 width,
												u8 height )
{
	dbl clkPeriod = 1 / AS_HEXITEC_CLOCK_SPEED;
	u32 rowClks = ( ( (*sensorConfig).Row_S1.size2 + (*sensorConfig).S1_Sph + (*sensorConfig).Sph_S2 + (width / 4) + (*sensorConfig).WaitClockCol ) * 2 ) + 10;
	u32 frameClks = ( height * rowClks ) + 4 + ( (*sensorConfig).WaitClockRow * 2 );
	u32 frameClks3 = ( frameClks * 3 ) + 2;
	dbl FrameTime = ((dbl)frameClks3) * clkPeriod / 3;
	
	return FrameTime;
}

EXTERN_C	GIGE_API	i32		GetIntegerValue(
												GigEDevice* deviceHdl,
												const str8 propertyName,
												i64 &value )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->GetIntegerValue( propertyName, value );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		GetLastResult(
												GigEDevice* deviceHdl,
												p_u32 pleoraErrorCode,
												str8 pleoraErrorCodeString,
												p_u32 pleoraErrorCodeStringLen,
												str8 pleoraErrorDescription,
												p_u32 pleoraErrorDescriptionLen )
{
	i32 dResult = AS_NO_ERROR;
	PvResult lResult = PvResult::Code::OK;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		lResult = deviceHdl->GetLastResult( );
		*pleoraErrorCode = lResult.GetCode();
		dResult = GigEDevice::GetErrorDescription( lResult, pleoraErrorCodeString, pleoraErrorCodeStringLen, pleoraErrorDescription, pleoraErrorDescriptionLen );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		GetOperationMode(
												GigEDevice* deviceHdl,
												HexitecOperationModePtr operationMode,
												u32 timeOut )
{
	u8		lValue = 0;
	i32		lResult = AS_NO_ERROR;

	lResult = ReadRegister( deviceHdl, 0x24, &lValue, timeOut );

	if( lResult == AS_NO_ERROR )
	{
		(*operationMode).DcUploadDarkCorrectionValues = (Control)(lValue & 0x01);
		(*operationMode).DcCollectDarkCorrectionValues = (Control)((lValue & 0x02) >> 1);
		(*operationMode).DcEnableDarkCorrectionCountingMode = (Control)((lValue & 0x04) >> 2);
		(*operationMode).DcEnableDarkCorrectionSpectroscopicMode = (Control)((lValue & 0x08) >> 3);
		(*operationMode).DcSendDarkCorrectionValues = (Control)((lValue & 0x10) >> 4);
		(*operationMode).DcDisableVcalPulse = (Control)((lValue & 0x20) >> 5);
		(*operationMode).DcTestMode = (Control)((lValue & 0x40) >> 6);
		(*operationMode).DcEnableTriggeredCountingMode = (Control)((lValue & 0x80) >> 7);

		lResult = ReadRegister( deviceHdl, 0x27, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*operationMode).EdUploadThresholdValues = (Control)(lValue & 0x01);
		(*operationMode).EdDisableCountingMode = (Control)((lValue & 0x02) >> 1);
		(*operationMode).EdTestMode = (Control)((lValue & 0x04) >> 2);

		lResult = WriteRegister( deviceHdl, 0x28, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*operationMode).EdCycles.size1[0] = lValue;

		lResult = WriteRegister( deviceHdl, 0x29, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*operationMode).EdCycles.size1[1] = lValue;
	}

	return lResult;
}

EXTERN_C	GIGE_API	i32		GetSensorConfig(
												GigEDevice* deviceHdl,
												HexitecSensorConfigPtr sensorConfig,
												u32 timeOut )
{
	u8		lValue = 0;
	i32		lResult = AS_NO_ERROR;
	u8		i = 0;
	u8		lSetupReg = AS_HEXITEC_SETUP_REGISTER_START_ADDRESS;

	lResult = ReadRegister( deviceHdl, 0x06, &lValue, timeOut );

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).Gain = (HexitecGain)lValue;

		lResult = ReadRegister( deviceHdl, 0x02, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).Row_S1.size1[0] = lValue;

		lResult = ReadRegister( deviceHdl, 0x03, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).Row_S1.size1[1] = lValue;

		lResult = ReadRegister( deviceHdl, 0x04, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).S1_Sph = lValue;

		lResult = ReadRegister( deviceHdl, 0x05, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).Sph_S2 = lValue;

		lResult = ReadRegister( deviceHdl, 0x18, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).Vcal2_Vcal1.size1[0] = lValue;

		lResult = ReadRegister( deviceHdl, 0x19, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).Vcal2_Vcal1.size1[1] = lValue;

		lResult = ReadRegister( deviceHdl, 0x1a, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).WaitClockCol = lValue;

		lResult = ReadRegister( deviceHdl, 0x1b, &lValue, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		(*sensorConfig).WaitClockRow = lValue;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lResult = ReadRegister( deviceHdl, lSetupReg, &lValue, timeOut );
			(*sensorConfig).SetupRow.PowerEn[i] = lValue;
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lResult = ReadRegister( deviceHdl, lSetupReg, &lValue, timeOut );
			(*sensorConfig).SetupRow.CalEn[i] = lValue;
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lResult = ReadRegister( deviceHdl, lSetupReg, &lValue, timeOut );
			(*sensorConfig).SetupRow.ReadEn[i] = lValue;
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lResult = ReadRegister( deviceHdl, lSetupReg, &lValue, timeOut );
			(*sensorConfig).SetupCol.PowerEn[i] = lValue;
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lResult = ReadRegister( deviceHdl, lSetupReg, &lValue, timeOut );
			(*sensorConfig).SetupCol.CalEn[i] = lValue;
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lResult = ReadRegister( deviceHdl, lSetupReg, &lValue, timeOut );
			(*sensorConfig).SetupCol.ReadEn[i] = lValue;
		}

		lSetupReg++;
	}

	return lResult;
}

EXTERN_C	GIGE_API	i32		GetSystemErrorMsg(
												i32 sysError,
												str8 sysErrorMsg,
												u32 length )
{
	i32		charCnt		= 0;
		
	if (sysErrorMsg != NULL)
	{
		charCnt = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, sysError, 0, sysErrorMsg, length, NULL);

		if( charCnt == 0 )
		{
			return GetLastError();
		}
	}

	return AS_NO_ERROR;
}

EXTERN_C	GIGE_API	i32		GetTriggerState(
												GigEDevice* deviceHdl,
												p_u8 trigger1,
												p_u8 trigger2,
												p_u8 trigger3,
												u32 timeOut )
{
	i32 lResult = AS_NO_ERROR;
	u8 lValue = 0;
	
	lResult = ReadRegister( deviceHdl, 0x85, &lValue, timeOut );
		
	if( lResult == AS_NO_ERROR )
	{
		*trigger1 = lValue & 0x01;
		*trigger2 = (lValue & 0x02) >> 1;
		*trigger3 = (lValue & 0x04) >> 2;
	}
	else
	{
		trigger1 = 0;
		trigger2 = 0;
		trigger3 = 0;
	}

	return lResult;
}

EXTERN_C	GIGE_API	i32		InitDevice(
												GigEDevice** deviceHdl,
												const str8 deviceDescriptor,
												p_u32 internalResult,
												str8 pleoraErrorCodeString,
												p_u32 pleoraErrorCodeStringLen,
												str8 pleoraErrorDescription,
												p_u32 pleoraErrorDescriptionLen )
{
	i32				iResult = AS_NO_ERROR;
	PvResult		lResult = PvResult::Code::OK;

	*deviceHdl = new GigEDevice(deviceDescriptor);

	lResult = (*deviceHdl)->GetLastResult();
	*internalResult = lResult.GetCode();
	GigEDevice::GetErrorDescription( lResult, pleoraErrorCodeString, pleoraErrorCodeStringLen, pleoraErrorDescription, pleoraErrorDescriptionLen );

	if ( lResult.IsOK() )
	{
		iResult = hTable.AddInstance(*deviceHdl);
	}
	else
	{
		iResult = AS_GIGE_INIT_DEVICE_ERROR;
	}

	if ( iResult != AS_NO_ERROR )
	{
		delete (*deviceHdl);
		*deviceHdl = NULL;
	}

	return iResult;
}

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
												p_u32 collectDcTime )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				lHv = (u32)HvDacValFromHv( *hvSetPoint );

	lResult = DisableSyncMode( deviceHdl, timeOut );

	if( lResult == AS_NO_ERROR )
	{
		lResult = DisableTriggerMode( deviceHdl, timeOut );
	}

	lRxBuffer.resize(7, 0);
	if( setHv )
	{
		lTxBuffer.resize(8, 0);
		HexToString( lHv, 4, (p_u8)&lTxBuffer[3] );
		lTxBuffer[7] = 0x0d;
	}
	else
	{
		lTxBuffer.resize(4, 0);
		lTxBuffer[3] = 0x0d;
	}

	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x56;

	if( lResult == AS_NO_ERROR )
	{
		lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	
	}

	StringToHex( (p_u8)&lRxBuffer[2], 4, &lHv );

	*hvSetPoint = HvDacValToHv( (u16)lHv );

	if( lResult == AS_NO_ERROR )
	{
		lResult = ReadResolution( deviceHdl, width, height, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = GetSensorConfig( deviceHdl, sensorConfig, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lResult = GetOperationMode( deviceHdl, operationMode, timeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		*frameTime = GetFrameTime( sensorConfig, *width, *height );
		*collectDcTime = GetCollectDcTime( *frameTime );
	}

	return lResult;
}

EXTERN_C	GIGE_API	i32		OpenSerialPort(
												GigEDevice* deviceHdl,
												PvDeviceSerial serialPort,
												u32 rxBufferSize,
												u8 useTermChar,
												u8 termChar )
{				
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->OpenSerialPort( serialPort, rxBufferSize, useTermChar, termChar );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		OpenStream(
												GigEDevice* deviceHdl )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->OpenStream();
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		ReadEnvironmentValues(
												GigEDevice* deviceHdl,
												p_dbl humidity,
												p_dbl ambientTemperature,
												p_dbl asicTemperature,
												p_dbl adcTemperature,
												p_dbl ntcTemperature,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				lHumidity = 0;
	u32				lAmbientTemperature = 0;
	u32				lAsicTemperature = 0;
	u32				lAdcTemperature = 0;
	u32				lNtcTemperature = 0;
	u32				lInternalReference = 0;
	
	lRxBuffer.resize(27, 0);
	lTxBuffer.resize(4, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x52;
	lTxBuffer[3] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	StringToHex( (p_u8)&lRxBuffer[2], 4, &lAmbientTemperature );
	StringToHex( (p_u8)&lRxBuffer[6], 4, &lHumidity );
	StringToHex( (p_u8)&lRxBuffer[10], 4, &lAsicTemperature );
	StringToHex( (p_u8)&lRxBuffer[14], 4, &lAdcTemperature );
	StringToHex( (p_u8)&lRxBuffer[18], 4, &lInternalReference );
	StringToHex( (p_u8)&lRxBuffer[22], 4, &lNtcTemperature );

	*ambientTemperature = GetAmbientTemperature( (u16)lAmbientTemperature );
	*humidity = GetHumidity( (u16)lHumidity );
	*asicTemperature = GetAsicTemperature( (u16)lAsicTemperature );
	*adcTemperature = GetAdcTemperature( (u16)lAdcTemperature );
	*ntcTemperature = TemperatureDacValToTemperature( (u16)lNtcTemperature, GetInternalReference( (u16)lInternalReference ) );

	return lResult;
}

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
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				l3v3 = 0;
	u32				lHvMon = 0;
	u32				lHvOut = 0;
	u32				l1v2 = 0;
	u32				l1v8 = 0;
	u32				l3v = 0;
	u32				l2v5 = 0;
	u32				l3v3ln = 0;
	u32				l1v65ln = 0;
	u32				l1v8ana = 0;
	u32				l3v8ana = 0;
	u32				lPeltierCurrent = 0;
	u32				lNtcTemperature = 0;
	
	lRxBuffer.resize(51, 0);
	lTxBuffer.resize(4, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x50;
	lTxBuffer[3] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	StringToHex( (p_u8)&lRxBuffer[38], 4, &l3v3 );
	StringToHex( (p_u8)&lRxBuffer[2], 4, &lHvMon );
	StringToHex( (p_u8)&lRxBuffer[6], 4, &l1v2 );
	StringToHex( (p_u8)&lRxBuffer[10], 4, &l1v8 );
	StringToHex( (p_u8)&lRxBuffer[14], 4, &l3v );
	StringToHex( (p_u8)&lRxBuffer[18], 4, &l2v5 );
	StringToHex( (p_u8)&lRxBuffer[22], 4, &l3v3ln );
	StringToHex( (p_u8)&lRxBuffer[26], 4, &l1v65ln );
	StringToHex( (p_u8)&lRxBuffer[30], 4, &l1v8ana );
	StringToHex( (p_u8)&lRxBuffer[34], 4, &l3v8ana );
	StringToHex( (p_u8)&lRxBuffer[42], 4, &lPeltierCurrent );
	StringToHex( (p_u8)&lRxBuffer[46], 4, &lNtcTemperature );

	*v3_3 = GetInternalReference( (u16)l3v3 );
	*hvMon = GetVoltage( lHvMon, *v3_3 );
	*hvOut = GetHvOut( *hvMon );
	*v1_2 = GetVoltage( l1v2, *v3_3 );
	*v1_8 = GetVoltage( l1v8, *v3_3 );
	*v3 = GetVoltage( l3v, *v3_3 );
	*v2_5 = GetVoltage( l2v5, *v3_3 );
	*v3_3ln = GetVoltage( l3v3ln, *v3_3 );
	*v1_65ln = GetVoltage( l1v65ln, *v3_3 );
	*v1_8ana = GetVoltage( l1v8ana, *v3_3 );
	*v3_8ana = GetVoltage( l3v8ana, *v3_3 );
	*peltierCurrent = GetPeltierCurrent( GetVoltage( lPeltierCurrent, *v3_3 ) );
	*ntcTemperature = TemperatureDacValToTemperature( (u16)lNtcTemperature, *v3_3 );

	return lResult;
}

EXTERN_C	GIGE_API	i32		ReadRegister(
												GigEDevice* deviceHdl,
												u8 registerAddress,
												p_u8 value,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				lValue = 0;

	lRxBuffer.resize(5, 0);
	lTxBuffer.resize(6, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x41;
	HexToString( registerAddress, 2, (p_u8)&lTxBuffer[3] );
	lTxBuffer[5] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	StringToHex( (p_u8)&lRxBuffer[2], 2, &lValue );

	*value = (u8)lValue;

	return lResult;
}

EXTERN_C	GIGE_API	i32		ReadResolution(
												GigEDevice* deviceHdl,
												p_u8 width,
												p_u8 height,
												u32 timeOut )
{
	i32 lResult = AS_NO_ERROR;
	
	lResult = ReadRegister( deviceHdl, 0x83, width, timeOut );
	*width = *width * 4;
	
	if( lResult == AS_NO_ERROR )
	{
		lResult = ReadRegister( deviceHdl, 0x84, height, timeOut );
	}

	return lResult;
}

EXTERN_C	GIGE_API	i32		RegisterTransferBufferReadyCallBack(
												GigEDevice* deviceHdl,
												p_bufferCallBack transferBufferReadyCallBack )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		deviceHdl->RegisterTransferBufferReadyCallBack( transferBufferReadyCallBack );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		ReturnBuffer(
												GigEDevice* deviceHdl,
												p_u8 transferBuffer )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		deviceHdl->ReturnBuffer( transferBuffer );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		SerialPortWriteRead(
												GigEDevice* deviceHdl,
												const p_u8 txBuffer,
												u32 txBufferSize,
												p_u32 bytesWritten,
												p_u8 rxBuffer,
												u32 rxBufferSize,
												p_u32 bytesRead,
												u32 timeOut )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( (dResult == AS_NO_ERROR) && txBufferSize )
	{
		dResult = deviceHdl->FlushRxBuffer();

		if( dResult == AS_NO_ERROR )
		{
			dResult = deviceHdl->WriteSerialPort( txBuffer, txBufferSize, bytesWritten );
		}
	}

	if( (dResult == AS_NO_ERROR) && rxBufferSize )
	{
		dResult = deviceHdl->ReadSerialPort( rxBuffer, rxBufferSize, bytesRead, timeOut );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		SetDAC(
												GigEDevice* deviceHdl,
												p_dbl vCal,
												p_dbl uMid,
												p_dbl hvSetPoint,
												p_dbl detCtrl,
												p_dbl targetTemperature,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				lVcal = (u32)DacValFromVoltage( *vCal );
	u32				lUmid = (u32)DacValFromVoltage( *uMid );
	u32				lHvSetPoint = (u32)HvDacValFromHv( *hvSetPoint );
	u32				lDetCtrl = (u32)DacValFromVoltage( *detCtrl );
	u32				lTargetTemperature = (u32)TemperatureDacValFromTemperature( *targetTemperature );
	
	lRxBuffer.resize(23, 0);
	lTxBuffer.resize(24, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x54;
	HexToString( lVcal, 4, (p_u8)&lTxBuffer[3] );
	HexToString( lUmid, 4, (p_u8)&lTxBuffer[7] );
	HexToString( lHvSetPoint, 4, (p_u8)&lTxBuffer[11] );
	HexToString( lDetCtrl, 4, (p_u8)&lTxBuffer[15] );
	HexToString( lTargetTemperature, 4, (p_u8)&lTxBuffer[19] );
	lTxBuffer[23] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	StringToHex( (p_u8)&lRxBuffer[2], 4, &lVcal );
	StringToHex( (p_u8)&lRxBuffer[6], 4, &lUmid );
	StringToHex( (p_u8)&lRxBuffer[10], 4, &lHvSetPoint );
	StringToHex( (p_u8)&lRxBuffer[14], 4, &lDetCtrl );
	StringToHex( (p_u8)&lRxBuffer[18], 4, &lTargetTemperature );

	*vCal = DacValToVoltage( (u16)lVcal );
	*uMid = DacValToVoltage( (u16)lUmid );
	*hvSetPoint = HvDacValToHv( (u16)lHvSetPoint );
	*detCtrl = DacValToVoltage( (u16)lDetCtrl );
	*targetTemperature = TemperatureDacValToTemperature( (u16)lTargetTemperature, AS_DAC_REF_VOLTAGE );

	return lResult;
}

EXTERN_C	GIGE_API	i32		SetFrameFormatControl(
												GigEDevice* deviceHdl,
												const str8 pixelFormat,
												u64 width,
												u64 height,
												u64 offsetX,
												u64 offsetY,
												const str8 sensorTaps,
												const str8 testPattern )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		dResult = deviceHdl->SetImageFormatControl( pixelFormat, width, height, offsetX, offsetY, sensorTaps, testPattern );
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		SetTriggeredFrameCount(
												GigEDevice* deviceHdl,
												u32 frameCount,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	
	lRxBuffer.resize(19, 0);
	lTxBuffer.resize(14, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x44;
	lTxBuffer[3] = 0x32;
	lTxBuffer[4] = 0x41;
	HexToStringLE( frameCount, 8, (p_u8)&lTxBuffer[5] );
	lTxBuffer[13] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	return lResult;
}

EXTERN_C	GIGE_API	i32		StopAcquisition(
												GigEDevice* deviceHdl )
{
	i32 dResult = AS_NO_ERROR;

	dResult = hTable.SetInstanceBusy(deviceHdl);
	
	if( dResult == AS_NO_ERROR )
	{
		deviceHdl->StopAcquisition();
	}

	if ( dResult == AS_NO_ERROR )
	{
		dResult = hTable.SetInstanceReady(deviceHdl);
	}
	else
	{
		hTable.SetInstanceReady(deviceHdl);
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		UploadOffsetValues(
												GigEDevice* deviceHdl,
												Reg2BytePtr offsetValues,
												u32 offsetValuesLength,
												u32 timeOut )
{
	i32 dResult = AS_NO_ERROR;
	u8	lValue = 0;
	HexitecOperationMode lCurrentMode;
	HexitecOperationMode lUploadMode;
	u32 lValuesToTransmit = offsetValuesLength * 2;
	FpgaRegister_vec lFpgaRegisterStream;
	u32 i = 0;
	u32 lRegsWritten = 0;

	if( !offsetValues )
	{
		dResult = AS_NULL_POINTER;
	}

	if( dResult == AS_NO_ERROR )
	{
		dResult = GetOperationMode( deviceHdl, &lCurrentMode, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{
		lUploadMode = lCurrentMode;
		lUploadMode.DcCollectDarkCorrectionValues = Control::AS_CONTROL_DISABLED;
		lUploadMode.DcUploadDarkCorrectionValues = Control::AS_CONTROL_ENABLED;
		
		dResult = DisableSM( deviceHdl, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{		
		dResult = SetOperationMode( deviceHdl, lUploadMode, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{
		dResult = EnableSM( deviceHdl, timeOut );
	}

	while( (dResult == AS_NO_ERROR) && lValuesToTransmit )
	{
		if( lValuesToTransmit < AS_HEXITEC_MAX_STREAM_REGISTER_COUNT )
		{
			lFpgaRegisterStream.resize( lValuesToTransmit );
		}
		else
		{
			lFpgaRegisterStream.resize( AS_HEXITEC_MAX_STREAM_REGISTER_COUNT );
		}

		for( i=0 ; i<lFpgaRegisterStream.size() ; i++ )
		{
			lFpgaRegisterStream[i].address = (u8)(lRegsWritten%2) + 0x25;
			lFpgaRegisterStream[i].value = offsetValues[lRegsWritten/2].size1[lRegsWritten%2];
			lRegsWritten++;
		}
			
		dResult = WriteRegisterStream( deviceHdl, lFpgaRegisterStream, timeOut );
		lValuesToTransmit = lValuesToTransmit - (u32)lFpgaRegisterStream.size();
	}

	if( dResult == AS_NO_ERROR )
	{		
		dResult = DisableSM( deviceHdl, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{		
		dResult = SetOperationMode( deviceHdl, lCurrentMode, timeOut );
	}

	if( dResult == AS_NO_ERROR )
	{
		dResult = EnableSM( deviceHdl, timeOut );
	}

	return dResult;
}

EXTERN_C	GIGE_API	i32		WriteAdcRegister(
												GigEDevice* deviceHdl,
												u8 registerAddress,
												p_u8 value,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				lValue = 0;

	lRxBuffer.resize(7, 0);
	lTxBuffer.resize(8, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x53;
	HexToString( registerAddress, 2, (p_u8)&lTxBuffer[3] );
	HexToString( *value, 2, (p_u8)&lTxBuffer[5] );
	lTxBuffer[7] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	StringToHex( (p_u8)&lRxBuffer[4], 2, &lValue );

	*value = (u8)lValue;

	return lResult;
}

EXTERN_C	GIGE_API	i32		WriteRegister(
												GigEDevice* deviceHdl,
												u8 registerAddress,
												p_u8 value,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				lValue = 0;

	lRxBuffer.resize(7, 0);
	lTxBuffer.resize(8, 0);
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x40;
	HexToString( registerAddress, 2, (p_u8)&lTxBuffer[3] );
	HexToString( *value, 2, (p_u8)&lTxBuffer[5] );
	lTxBuffer[7] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	StringToHex( (p_u8)&lRxBuffer[4], 2, &lValue );

	*value = (u8)lValue;

	return lResult;
}

EXTERN_C	GIGE_API	i32		WriteRegisterStream(
												GigEDevice* deviceHdl,
												FpgaRegister_vec &registerStream,
												u32 timeOut )
{
	i32	lResult = AS_NO_ERROR;

	string8			lTxBuffer = "";
	string8			lRxBuffer = "";

	u32				lBytesWritten = 0;
	u32				lBytesRead = 0;
	u32				i = 0;
	u32				lBufferIndex = 5;
	u32				lValue = 0;
	
	lRxBuffer.resize( ( registerStream.size() * 4 ) + 3, 0 );
	lTxBuffer.resize( ( registerStream.size() * 4 ) + 6, 0 );
	lTxBuffer[0] = 0x23;
	lTxBuffer[1] = AS_MODULE_ADDRESS;
	lTxBuffer[2] = 0x46;
	HexToString( (u8)registerStream.size(), 2, (p_u8)&lTxBuffer[3] );

	for( i=0 ; i<registerStream.size() ; i++ )
	{
		HexToString( registerStream[i].address, 2, (p_u8)&lTxBuffer[ lBufferIndex ] );
		lBufferIndex = lBufferIndex + 2;
		HexToString( registerStream[i].value, 2, (p_u8)&lTxBuffer[ lBufferIndex ] );
		lBufferIndex = lBufferIndex + 2;
	}
	
	lTxBuffer[ lTxBuffer.size() - 1 ] = 0x0d;

	lResult = SerialPortWriteRead( deviceHdl, (p_u8)&lTxBuffer[0], (u32)lTxBuffer.size(), &lBytesWritten, (p_u8)&lRxBuffer[0], (u32)lRxBuffer.size(), &lBytesRead, timeOut);	

	if( lBytesRead >= lRxBuffer.size() )
	{
		lBufferIndex = 4;
		
		for( i=0 ; i<registerStream.size() ; i++ )
		{
			StringToHex( (p_u8)&lRxBuffer[lBufferIndex], 2, &lValue );
			registerStream[i].value = (u8)lValue;
			lBufferIndex = lBufferIndex + 4;
		}		
	}

	return lResult;
}

u16 DacValFromVoltage( dbl voltage )
{
	dbl lVoltage = voltage;
	dbl dacStepVoltage = AS_DAC_REF_VOLTAGE / 4095;

	if( lVoltage < 0 )
	{
		lVoltage = 0;
	}
	else if( lVoltage > AS_DAC_REF_VOLTAGE )
	{
		lVoltage = AS_DAC_REF_VOLTAGE;
	}
	
	return (u16)((lVoltage/dacStepVoltage)+0.5);
}

dbl DacValToVoltage( u16 number )
{
	dbl dacStepVoltage = AS_DAC_REF_VOLTAGE / 4095;
	return dacStepVoltage * (dbl)number;
}

dbl GetAdcTemperature( u16 number )
{
	return (dbl)number * 0.0625;
}

dbl GetAmbientTemperature( u16 number )
{
	return (((dbl)number * 175.72) / 65535) - 46.85;
}

dbl GetAsicTemperature( u16 number )
{
	return (dbl)number * 0.0625;
}

u32	GetCollectDcTime( dbl aFrameTime )
{
	return (u32)((aFrameTime * 1000 * AS_HEXITEC_DARK_CORRECTION_FRAME_COUNT) + 1000);
}

dbl GetHumidity( u16 number )
{
	return (((dbl)number * 125) / 65535) - 6;
}

dbl GetHvOut( dbl hvMon )
{
	return (hvMon * 1621.65) - 1043.22;
}

dbl GetInternalReference( u16 number )
{
	return (AS_INTERNAL_REF_VOLTAGE / 4095) * (dbl)number;
}

dbl GetPeltierCurrent( dbl voltage )
{
	return (voltage - 1.5) / 0.2;
}

dbl GetVoltage( u16 number, dbl internalReference )
{
	return (internalReference / 4095) * (dbl)number;
}

i32 HexToString( u32 number, u8 digits, p_u8 target )
{
	i32	lResult = 0;
	string8	lBuffer;
	lBuffer.resize( digits + 1, 0 );
	lResult = sprintf_s( &lBuffer[0], digits + 1, "%0*X", digits, number );
	memcpy( target, &lBuffer[0], digits );
	return lResult;
}

i32 HexToStringLE( u32 number, u8 digits, p_u8 target )
{
	u8 lIdx = 0;
	i32	lResult = 0;
	string8	lBuffer;
	lBuffer.resize( digits + 1, 0 );
	lResult = sprintf_s( &lBuffer[0], digits + 1, "%0*X", digits, number );
	for ( lIdx = 0 ; lIdx < digits ; lIdx = lIdx + 2 )
	{
		target[lIdx] = lBuffer[digits - lIdx - 2];
		target[lIdx + 1] = lBuffer[digits - lIdx - 1];
	}
	return lResult;
}

u16 HvDacValFromHv( dbl hv )
{
	dbl hvConverted = hv/-250;
	dbl dacStepVoltage = AS_DAC_REF_VOLTAGE / 4095;

	if( hvConverted < 0 )
	{
		hvConverted = 0;
	}
	else if( hvConverted > AS_DAC_REF_VOLTAGE )
	{
		hvConverted = AS_DAC_REF_VOLTAGE;
	}
	
	return (u16)((hvConverted/dacStepVoltage)+0.5);
}

dbl HvDacValToHv( u16 number )
{
	dbl dacStepVoltage = AS_DAC_REF_VOLTAGE / 4095;
	return dacStepVoltage * (dbl)number * -250;
}

i32		SetOperationMode(
												GigEDevice* aDeviceHdl,
												HexitecOperationMode aOperationMode,
												u32 aTimeOut)
{
	u8		lValue = 0;
	i32		lResult = AS_NO_ERROR;

	lValue = aOperationMode.DcUploadDarkCorrectionValues;
	lValue = lValue + (aOperationMode.DcCollectDarkCorrectionValues * 0x02);
	lValue = lValue + (aOperationMode.DcEnableDarkCorrectionCountingMode * 0x04);
	lValue = lValue + (aOperationMode.DcEnableDarkCorrectionSpectroscopicMode * 0x08);
	lValue = lValue + (aOperationMode.DcSendDarkCorrectionValues * 0x10);
	lValue = lValue + (aOperationMode.DcDisableVcalPulse * 0x20);
	lValue = lValue + (aOperationMode.DcTestMode * 0x40);
	lValue = lValue + (aOperationMode.DcEnableTriggeredCountingMode * 0x80);

	lResult = WriteRegister( aDeviceHdl, 0x24, &lValue, aTimeOut );

	if( lResult == AS_NO_ERROR )
	{
		lValue = aOperationMode.EdUploadThresholdValues;
		lValue = lValue + (aOperationMode.EdDisableCountingMode * 0x02);
		lValue = lValue + (aOperationMode.EdTestMode * 0x04);

		lResult = WriteRegister( aDeviceHdl, 0x27, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aOperationMode.EdCycles.size1[0];

		lResult = WriteRegister( aDeviceHdl, 0x28, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aOperationMode.EdCycles.size1[1];

		lResult = WriteRegister( aDeviceHdl, 0x29, &lValue, aTimeOut );
	}

	return lResult;
}

i32		SetSensorConfig(
												GigEDevice* aDeviceHdl,
												HexitecSensorConfig aSensorConfig,
												u32 aTimeOut)
{
	u8		lValue = 0;
	i32		lResult = AS_NO_ERROR;
	u8		i = 0;
	u8		lSetupReg = AS_HEXITEC_SETUP_REGISTER_START_ADDRESS;

	lValue = aSensorConfig.Gain;
	lResult = WriteRegister( aDeviceHdl, 0x06, &lValue, aTimeOut );

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.Row_S1.size1[0];
		lResult = WriteRegister( aDeviceHdl, 0x02, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.Row_S1.size1[1];
		lResult = WriteRegister( aDeviceHdl, 0x03, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.S1_Sph;
		lResult = WriteRegister( aDeviceHdl, 0x04, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.Sph_S2;
		lResult = WriteRegister( aDeviceHdl, 0x05, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.Vcal2_Vcal1.size1[0];
		lResult = WriteRegister( aDeviceHdl, 0x18, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.Vcal2_Vcal1.size1[1];
		lResult = WriteRegister( aDeviceHdl, 0x19, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = 0x00;
		lResult = WriteRegister( aDeviceHdl, 0x20, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = 0x00;
		lResult = WriteRegister( aDeviceHdl, 0x21, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = 0x00;
		lResult = WriteRegister( aDeviceHdl, 0x22, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = 0x00;
		lResult = WriteRegister( aDeviceHdl, 0x23, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.WaitClockCol;
		lResult = WriteRegister( aDeviceHdl, 0x1a, &lValue, aTimeOut );
	}

	if( lResult == AS_NO_ERROR )
	{
		lValue = aSensorConfig.WaitClockRow;
		lResult = WriteRegister( aDeviceHdl, 0x1b, &lValue, aTimeOut );
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lValue = aSensorConfig.SetupRow.PowerEn[i];
			lResult = WriteRegister( aDeviceHdl, lSetupReg, &lValue, aTimeOut );
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lValue = aSensorConfig.SetupRow.CalEn[i];
			lResult = WriteRegister( aDeviceHdl, lSetupReg, &lValue, aTimeOut );
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lValue = aSensorConfig.SetupRow.ReadEn[i];
			lResult = WriteRegister( aDeviceHdl, lSetupReg, &lValue, aTimeOut );
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lValue = aSensorConfig.SetupCol.PowerEn[i];
			lResult = WriteRegister( aDeviceHdl, lSetupReg, &lValue, aTimeOut );
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lValue = aSensorConfig.SetupCol.CalEn[i];
			lResult = WriteRegister( aDeviceHdl, lSetupReg, &lValue, aTimeOut );
		}

		lSetupReg++;
	}

	for( i=0 ; i<AS_HEXITEC_SETUP_REGISTER_SIZE ; i++)
	{
		if( lResult == AS_NO_ERROR )
		{
			lValue = aSensorConfig.SetupCol.ReadEn[i];
			lResult = WriteRegister( aDeviceHdl, lSetupReg, &lValue, aTimeOut );
		}

		lSetupReg++;
	}

	return lResult;
}

i32 StringToHex( p_u8 source, u8 digits, p_u32 number )
{
	u32 lMaxDigits = 3;
	i32 lResult = 0;
	string8 lFormat;
	lFormat.resize( lMaxDigits + 2 );
	lFormat[0] = '%';
	lResult = sprintf_s( &lFormat[1], lMaxDigits, "%u", digits );
	if( lResult > 0 )
	{
		lFormat.resize( lResult + 2 );
		lFormat[ lResult + 1 ] = 'x';
	}
	else
	{
		return lResult;
	}
	return sscanf_s( (const char*)source, lFormat.c_str() , number );
}

u16 TemperatureDacValFromTemperature( dbl temperature )
{
	dbl lTemperature = temperature;

	if( lTemperature < AS_HEXITEC_TARGET_TEMPERATURE_LL )
	{
		lTemperature = AS_HEXITEC_TARGET_TEMPERATURE_LL;
	}
	else if( lTemperature > AS_HEXITEC_TARGET_TEMPERATURE_UL )
	{
		lTemperature = AS_HEXITEC_TARGET_TEMPERATURE_UL;
	}	
	
	dbl temperatureConvertedExp = exp(((1 / ((dbl)lTemperature + 273.15)) - (1 / 298.15)) * 3988);
	dbl temperatureConverted = (temperatureConvertedExp * 3) / (temperatureConvertedExp + 1);
	dbl dacStepVoltage = AS_DAC_REF_VOLTAGE / 4095;

	if( temperatureConverted < 0 )
	{
		temperatureConverted = 0;
	}
	else if( temperatureConverted > AS_DAC_REF_VOLTAGE )
	{
		temperatureConverted = AS_DAC_REF_VOLTAGE;
	}
	
	return (u16)((temperatureConverted/dacStepVoltage)+0.5);
}

dbl TemperatureDacValToTemperature( u16 number, dbl internalReference )
{
	dbl dacVoltage = (internalReference / 4095) * (dbl)number;
	dbl term_ln = log(dacVoltage / (3-dacVoltage));
	return (1/((term_ln / 3988) + (1 / 298.15))) - 273.15;
}

GigEDevice::~GigEDevice()
{
	DeleteCriticalSection( &cAvailableTransferBufferLock );

	if( cPort.IsOpened() )
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

	while ( cAcqResult.IsOK() && (i<lImageCount) )
	{
		cAcqResult = cPipeline->RetrieveNextBuffer( &lBuffer, FrameTimeOut, &lResult );
	
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
		cReadyCallBack();
	}

	while ( cAcqResult.IsOK() && ((cAcquiredImages<ImageCount) || cContinuous) )
	{
		if( cStopAcquisition )
		{
			break;
		}
		
		if( lCurrentFrameWithinBuffer == 0 )	// new Transferbuffer
		{
			EnterCriticalSection( &cAvailableTransferBufferLock );
			if( cAvailableTransferBuffer.size() )
			{			
				lTransferBuffer = cAvailableTransferBuffer.front();
				lPointer = lTransferBuffer;
				cAvailableTransferBuffer.pop();
				LeaveCriticalSection( &cAvailableTransferBufferLock );
			}
			else
			{
				LeaveCriticalSection( &cAvailableTransferBufferLock );
				lResult = AS_GIGE_NO_TRANSFER_BUFFER_AVAILABLE;
				break;
			}
		}
		
		cAcqResult = cPipeline->RetrieveNextBuffer( &lBuffer, FrameTimeOut, &lAcqResult );
	
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
		cBufferCallBack( aTransferBuffer, aCurrentFrameWithinBuffer );
	}
	else
	{
		EnterCriticalSection( &cAvailableTransferBufferLock );
		cAvailableTransferBuffer.push( aTransferBuffer );
		LeaveCriticalSection( &cAvailableTransferBufferLock );
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

PvResult GigEDevice::ConfigureStream()
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
	cBufferCallBack				= NULL;
	cAcquiredImages				= 0;
	cContinuous					= 0;
	cStopAcquisition			= 0;
	cBlocksDroppedVal			= 0;
	cBlocksDropped				= NULL;
	cBlockIDsMissingVal		= 0;
	cBlockIDsMissing			= NULL;

	cResult						= PvResult::Code::OK;
	cAcqResult					= PvResult::Code::OK;

	InitializeCriticalSectionAndSpinCount( &cAvailableTransferBufferLock, 0x0400 );
	
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

i32 GigEDevice::OpenStream()
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

		cResult = ConfigureStream();

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

void GigEDevice::RegisterTransferBufferReadyCallBack( p_bufferCallBack aTransferBufferReadyCallBack )
{
	cBufferCallBack = aTransferBufferReadyCallBack;
}

void GigEDevice::ReturnBuffer( p_u8 Buffer )
{
	EnterCriticalSection( &cAvailableTransferBufferLock );
	cAvailableTransferBuffer.push( Buffer );
	LeaveCriticalSection( &cAvailableTransferBufferLock );
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