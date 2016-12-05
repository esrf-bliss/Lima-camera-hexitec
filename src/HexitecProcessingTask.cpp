//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2014
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
#include "HexitecProcessingTask.h"
#include <algorithm>
#include <vector>

using namespace lima::Hexitec;

/**
 * Takes the raw list of data and arranges into the correct 80x80 array.
 * The raw list of data will be (1,1),(1,21),(1,41),(1,61),(1,2),(1,22)…
 * So this sorts back to be the correct image/frame format and enlarges the
 * data array with zeros around the edge of the image to allow for the
 * scanning of neighbours in the next stage of processing. This saves
 * one image copy.
 */
Data HexitecProcessingTask::sort(Data& srcData) {
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	uint16_t row[width];
	uint16_t *dptr = (uint16_t*)srcData.data();
	int bytesInRow = width*sizeof(uint16_t);

	Data dstData;
	dstData = srcData;
//	if (!m_processingInPlaceFlag) {
		Buffer *newBuffer = new Buffer(srcData.size());
		memcpy(newBuffer->data, srcData.data(), srcData.size());
		dstData.setBuffer(newBuffer);
		newBuffer->unref();
//	}
	uint16_t *dstPtr = (uint16_t*)dstData.data();
	for (auto i=0; i<height; i++) {
		// rearrange the row
		for (auto j=0; j<width/4; j++) {
			for (auto k=0; k<4; k++) {
				row[j+(k*20)] = *dptr++;
			}
		}
		// now copy it back to the return buffer
		memcpy(dstPtr, row, bytesInRow);
		dstPtr += width;
	}
	return dstData;
}

void csa_3x3(Data& srcData, int yp, int xp, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	int sumE = 0;
	int maxE = 0;
	int count = 0;
	int maxX;
	int maxY;
	// Recalculate start & end to handle edge cases
	int starty = (yp < 0) ? 0 : yp;
	int startx = (xp < 0) ? 0 : xp;
	int endy = (yp+3 < height) ? yp+3 : height;
	int endx = (xp+3 < width) ? xp+3 : width;
	for (auto i=starty; i<endy; i++) {
		for (auto j=startx; j<endx; j++) {
			if (dptr[width*i+j] > eventThresh) {
				sumE += dptr[width*i+j];
				count++;
				if (dptr[width*i+j] > maxE) {
					maxE = dptr[width*i+j];
					maxX = j;
					maxY = i;
				}
			}
		}
	}
	if (count > 1 && sumE < emax) {
		for (auto i=starty; i<endy; i++) {
			for (auto j=startx; j<endx; j++) {
				dptr[width*i+j] = 0;
			}
		}
		dptr[width*maxY+maxX] = sumE;
	}
}

void csa_5x5(Data& srcData, int yp, int xp, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	int sumE = 0;
	int maxE = 0;
	int count = 0;
	int maxX;
	int maxY;
	// Recalculate start & end to handle edge cases
	int starty = (yp < 0) ? 0 : yp;
	int startx = (xp < 0) ? 0 : xp;
	int endy = (yp+5 < height) ? yp+5 : height;
	int endx = (xp+5 < width) ? xp+5 : width;
	for (auto i=starty; i<endy; i++) {
		for (auto j=startx; j<endx; j++) {
			if (dptr[width*i+j] > eventThresh) {
				sumE += dptr[width*i+j];
				count++;
				if (dptr[width*i+j] > maxE) {
					maxE = dptr[width*i+j];
					maxX = j;
					maxY = i;
				}
			}
		}
	}
	if (count > 1 && sumE < emax) {
		for (auto i=starty; i<endy; i++) {
			for (auto j=startx; j<endx; j++) {
				dptr[width*i+j] = 0;
			}
		}
		dptr[width*maxY+maxX] = sumE;
	}
}

/**
 * Charge sharing addition.
 * This looks at a frame of data to find single photons that are shared between pixels.
 * For 250um pitch it looks at nearest neighbours with a 3x3 array of pixels.
 * If there is an event above threshold (so a real events and not noise) then the sum
 * of all pixels is allocated to the pixel with the highest fraction.
 */
void HexitecProcessingTask::chargeSharingAddition_3x3(Data& srcData, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];

	for (auto i=0; i<height; i++) {
		for (auto j=0; j<width; j++, dptr++) {
			if (*dptr > eventThresh) {
				csa_3x3(srcData, i-1, j-1, eventThresh, emax);
			}
		}
	}
}

/**
 * Charge sharing addition.
 * This looks at a frame of data to find single photons that are shared between pixels.
 * For 500um pitch it looks at nearest neighbours with a 5x5 array of pixels.
 * If there is an event above threshold (so a real events and not noise) then the sum
 * of all pixels is allocated to the pixel with the highest fraction.
 *
 */
void HexitecProcessingTask::chargeSharingAddition_5x5(Data& srcData, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];

	for (auto i=0; i<height; i++) {
		for (auto j=0; j<width; j++, dptr++) {
			if (*dptr > eventThresh) {
				csa_5x5(srcData, i-2, j-2, eventThresh, emax);
			}
		}
	}
}

void csd_3x3(Data& srcData, int yp, int xp, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	int sumE = 0;
	int count = 0;
	// Recalculate start & end to handle edge cases
	int starty = (yp < 0) ? 0 : yp;
	int startx = (xp < 0) ? 0 : xp;
	int endy = (yp+3 < height) ? yp+3 : height;
	int endx = (xp+3 < width) ? xp+3 : width;
	for (auto i=starty; i<endy; i++) {
		for (auto j=startx; j<endx; j++) {
			if (dptr[width*i+j] > eventThresh) {
				sumE += dptr[width*i+j];
				count++;
			}
		}
	}
	if (count > 1 && sumE < emax) {
		for (auto i=starty; i<endy; i++) {
			for (auto j=startx; j<endx; j++) {
				dptr[width*i+j] = 0;
			}
		}
	}
}
void csd_5x5(Data& srcData, int yp, int xp, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	int sumE = 0;
	int count = 0;
	// Recalculate start & end to handle edge cases
	int starty = (yp < 0) ? 0 : yp;
	int startx = (xp < 0) ? 0 : xp;
	int endy = (yp+5 < height) ? yp+5 : height;
	int endx = (xp+5 < width) ? xp+5 : width;
	for (auto i=starty; i<endy; i++) {
		for (auto j=startx; j<endx; j++) {
			if (dptr[width*i+j] > eventThresh) {
				sumE += dptr[width*i+j];
				count++;
			}
		}
	}
	if (count > 1 && sumE < emax) {
		for (auto i=starty; i<endy; i++) {
			for (auto j=startx; j<endx; j++) {
				dptr[width*i+j] = 0;
			}
		}
	}
}

/**
 * Charge Sharing Discrimination
 * Very similar to charge sharing addition but this is so if we see a single photon
 * spread over neighbouring pixels we make this zero.
 */
void HexitecProcessingTask::chargeSharingDiscrimination_3x3(Data& srcData, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];

	for (auto i=0; i<height; i++) {
		for (auto j=0; j<width; j++, dptr++) {
			if (*dptr > eventThresh) {
				csd_3x3(srcData, i-1, j-1, eventThresh, emax);
			}
		}
	}
}

void HexitecProcessingTask::chargeSharingDiscrimination_5x5(Data& srcData, int eventThresh, int emax) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];

	for (auto i=0; i<height; i++) {
		for (auto j=0; j<width; j++, dptr++) {
			if (*dptr > eventThresh) {
				csd_5x5(srcData, i-2, j-2, eventThresh, emax);
			}
		}
	}
}

/**
 * There is a chance that a signal can still be rising when it is readout;
 * or when a pixel is reset it is reset to the falling edge of the electronics following an event.
 * This will give false events in the spectrum so this just looks at frame N and N+1;
 * if there is an event in pixel (x,y) in frame N then we ignore it in frame N+1.
 */
void HexitecProcessingTask::nextFrameCorrection(Data& srcData, Data& lastFrame, int threshold) {
	DEB_MEMBER_FUNCT();
	uint16_t* dptr = (uint16_t*)srcData.data();
	uint16_t* lfptr = (uint16_t*)lastFrame.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	if (lastFrame.dimensions[0] != width || lastFrame.dimensions[1] != height) {
		THROW_HW_ERROR(Error) << "Frames not the same size";
	} else {
		for (auto i=0; i<height; i++) {
			for (auto j=0; j<width; j++, dptr++, lfptr++) {
				if (*dptr > threshold && *lfptr > threshold) {
					*dptr = 0;
				}
			}
		}
	}
}


/**
 * Process raw data from Hexitec detector
 * 1. Re-format raw data
 * 2. Perform charge sharing discrimination or addition
 * 3. Perform next frame correction
 */
Data HexitecProcessingTask::process(Data& srcData) {
	int frameNb = srcData.frameNumber;
	std::cout << "Processing frame" << frameNb << std::endl;
	if (frameNb == 0) {
		int nbins = (m_speclen / m_binWidth) + 1;
		int width = srcData.dimensions[0];
		int height = srcData.dimensions[1];
	}
	Data dstData = sort(srcData);
	m_lastFrame = dstData;
	if (m_asicPitch == 250) {
		switch (m_processType) {
		case Camera::CSA:
			chargeSharingAddition_3x3(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::CSD:
			chargeSharingDiscrimination_3x3(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::CSA_NF:
			if (frameNb > 0) {
				nextFrameCorrection(dstData, m_lastFrame, m_lowThreshold);
			}
			chargeSharingAddition_3x3(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::CSD_NF:
			if (frameNb > 0) {
				nextFrameCorrection(dstData, m_lastFrame, m_lowThreshold);
			}
			chargeSharingDiscrimination_3x3(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::SORT:
		default:
			break;
		}
	} else {
		switch (m_processType) {
		case Camera::CSA:
			chargeSharingAddition_5x5(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::CSD:
			chargeSharingDiscrimination_5x5(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::CSA_NF:
			if (frameNb > 0) {
				nextFrameCorrection(dstData, m_lastFrame, m_lowThreshold);
			}
			chargeSharingAddition_5x5(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::CSD_NF:
			if (frameNb > 0) {
				nextFrameCorrection(dstData, m_lastFrame, m_lowThreshold);
			}
			chargeSharingDiscrimination_5x5(dstData, m_lowThreshold, m_highThreshold);
			break;
		case Camera::SORT:
		default:
			break;
		}
	}
	return dstData;
}

HexitecProcessingTask::HexitecProcessingTask(SavingCtrlObj& savingCtrlObj, int stream_idx, Camera::ProcessType processType, int asicPitch,
		int binWidth, int speclen, int lowThreshold, int highThreshold) :
	LinkTask(true), m_saving(savingCtrlObj), m_stream_idx(stream_idx),
	m_processType(processType), m_asicPitch(asicPitch), m_binWidth(binWidth), m_speclen(speclen),
	m_lowThreshold(lowThreshold), m_highThreshold(highThreshold) {
};
