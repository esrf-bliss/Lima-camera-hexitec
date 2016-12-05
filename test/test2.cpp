#include <cstdint>
#include <iostream>
#include "HexitecApi.h"

using namespace HexitecAPI;

int main() {
	int32_t result;
	uint32_t errorCode;
	std::string errorCodeString;
	std::string errorDescription;
	std::string fname = "./HexitecApi.ini";

	HexitecApi hexitec("169.254.63.243", 600);
	hexitec.readConfiguration(fname);
	hexitec.initDevice(errorCode, errorCodeString, errorDescription);
	std::cout << "Error code :" << errorCode << std::endl;
	std::cout << "Error      :" << errorCodeString << std::endl;
	std::cout << "Description:" << errorDescription << std::endl;
	uint8_t useTermChar = true;
	result = hexitec.openSerialPort( PvDeviceSerialBulk0, (2<<16), useTermChar, 0x0d);
	std::cout << "return code from openSerialPort = " << result << std::endl;
	uint8_t customerId;
	uint8_t projectId;
	uint8_t version;
	uint8_t forceEqualVersion = false;
	hexitec.checkFirmware(customerId, projectId, version, forceEqualVersion);
	std::cout << "customerId :" << int(customerId) << std::endl;
	std::cout << "projectId  :" << int(projectId) << std::endl;
	std::cout << "version    :" << int(version) << std::endl;
	uint8_t width;
	uint8_t height;
	double frameTime;
	uint32_t collectDcTime;
	result = hexitec.configureDetector(width, height, frameTime, collectDcTime);
	std::cout << "result        :" << result << std::endl;
	std::cout << "width         :" << int(width) << std::endl;
	std::cout << "height        :" << int(height) << std::endl;
	std::cout << "frameTime     :" << frameTime << std::endl;
	std::cout << "collectDcTime :" << collectDcTime << std::endl;
	double humidity;
	double ambientTemperature;
	double asicTemperature;
	double adcTemperature;
	double ntcTemperature;
	result = hexitec.readEnvironmentValues(humidity, ambientTemperature, asicTemperature, adcTemperature, ntcTemperature);
	std::cout << "result             :" << result << std::endl;
	std::cout << "humidity           :" << humidity << std::endl;
	std::cout << "ambientTemperature :" << ambientTemperature << std::endl;
	std::cout << "asicTemperature    :" << asicTemperature << std::endl;
	std::cout << "adcTemperature     :" << adcTemperature << std::endl;
	std::cout << "ntcTemperature     :" << ntcTemperature << std::endl;
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
	result = hexitec.readOperatingValues(v3_3, hvMon, hvOut, v1_2, v1_8, v3, v2_5, v3_3ln, v1_65ln,
			v1_8ana, v3_8ana, peltierCurrent, ntcTemperature);
	std::cout << "result         :" << result << std::endl;
	std::cout << "v3_3           :" << v3_3 << std::endl;
	std::cout << "hvMon          :" << hvMon << std::endl;
	std::cout << "hvOut          :" << hvOut << std::endl;
	std::cout << "v1_2           :" << v1_2 << std::endl;
	std::cout << "v1_8           :" << v1_8 << std::endl;
	std::cout << "v3             :" << v3 << std::endl;
	std::cout << "v2_5           :" << v2_5 << std::endl;
	std::cout << "v3_3ln         :" << v3_3ln << std::endl;
	std::cout << "v1_65ln        :" << v1_65ln << std::endl;
	std::cout << "v1_8ana        :" << v1_8ana << std::endl;
	std::cout << "v3_8ana        :" << v3_8ana << std::endl;
	std::cout << "peltierCurrent :" << peltierCurrent << std::endl;
	std::cout << "ntcTemperature :" << ntcTemperature << std::endl;

	result = hexitec.setFrameFormatControl("Mono16", 80, 80, 0, 0, "One","Off");
	std::cout << "frameformat result     :" << result << std::endl;
	//openStream should be called before createPipeline
	result = hexitec.openStream();
	std::cout << "stream result     :" << result << std::endl;
	int
	bufferCount = 10;
	result = hexitec.createPipelineOnly(bufferCount);
	std::cout << "pipe result       :" << result << std::endl;

	uint16_t *buffer = new uint16_t[bufferCount*80*80];
	uint8_t *bptr = (uint8_t*) buffer;

	uint32_t frametimeout = 600;
	std::cout << "bptr b4           :" << &bptr << std::endl;

	for (auto i = 0; i < 64000; i++) {
		buffer[i] = 0;
	}
	hexitec.startAcquisition();
	for (auto m=0; m<10; m++) {
		std::cout << "!!!!!!!!!!!!!!! Frame " << m << std::endl;
		int rc = hexitec.retrieveBuffer(bptr, frametimeout);
		std::cout << "retrieve rc " << rc << std::endl;
		bptr+= 80*80*2;
	}
	for (auto k = 0; k < bufferCount; k++) {
		std::cout << "!!!!!!!!!!!!!!! Sub Frame " << k << std::endl;
		for (auto i = 0; i < 80; i++) {
			for (auto j = 0; j < 80; j++) {
				std::cout << (int) buffer[k*80*80 +i * 80 + j] << " ";
			}
			std::cout << "|" << std::endl;
		}
	}
	hexitec.stopAcquisition();
	return 0;
}

// Using our own buffers!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void CreateBuffers(PvDevice* aDevice, PvStream* aStream) {
//#define BUFFER_COUNT ( 16 )
//	gBufferCount = (aStream->GetQueuedBufferMaximum() < BUFFER_COUNT) ? aStream->GetQueuedBufferMaximum() :
//	BUFFER_COUNT;
//	gPvBuffers = new PvBuffer[gBufferCount];
//	for (uint32_t i = 0; i < gBufferCount; i++) {
//		// Attach the memory of our imaging buffer to a PvBuffer. The PvBuffer is used as a shell
//		// that allows directly acquiring image data into the memory owned by our imaging buffer
//		gPvBuffers[i].GetImage()->Attach(gImagingBuffers[i].GetTopPtr(), static_cast<uint32_t>(lWidth),
//				static_cast<uint32_t>(lHeight), PvPixelRGB8);
//
//		// Set eBUS SDK buffer ID to the buffer/image index
//		gPvBuffers[i].SetID(i);
//	}
//	// Queue all buffers in the stream
//	for (uint32_t i = 0; i < gBufferCount; i++) {
//		aStream->QueueBuffer(gPvBuffers + i);
//	}
//}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
