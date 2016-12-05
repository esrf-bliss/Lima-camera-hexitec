#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <processlib/Data.h>

typedef std::chrono::high_resolution_clock Clock;

Data sort(Data& srcData) {
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	uint16_t row[width];
	uint16_t *dptr = (uint16_t*)srcData.data();
	int bytesInRow = width*sizeof(uint16_t);

	Data dstData;
	dstData = srcData;
//	Buffer *newBuffer = new Buffer(dstData.size());
//	dstData.setBuffer(newBuffer);
//	newBuffer->unref();

	uint16_t *rptr = (uint16_t*)dstData.data();
	for (auto i=0; i<height; i++) {
		// rearrange the row
		for (auto j=0; j<width; j++) {
				row[j] = *dptr++;
		}
		// now copy it back to the return buffer
		memcpy(rptr, row, bytesInRow);
		rptr += width;
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

void chargeSharingAddition_3x3(Data& srcData, int eventThresh, int emax) {
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
void chargeSharingAddition_5x5(Data& srcData, int eventThresh, int emax) {
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
void chargeSharingDiscrimination_3x3(Data& srcData, int eventThresh, int emax) {
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
void chargeSharingDiscrimination_5x5(Data& srcData, int eventThresh, int emax) {
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
void nextFrameCorrection(Data& srcData, Data& lastFrame, int threshold) {
	uint16_t* dptr = (uint16_t*)srcData.data();
	uint16_t* lfptr = (uint16_t*)lastFrame.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	if (lastFrame.dimensions[0] != width || lastFrame.dimensions[1] != height) {
		std::cout << "Frames not the same size";
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

void createSpectra(Data& spectrum, Data& srcData, int binWidth, int lowThreshold, int highThreshold) {
	// Add Data to total spectra per pixels
	uint16_t* dptr = (uint16_t*) srcData.data();
	uint16_t* sptr = (uint16_t*) spectrum.data();
	int width = srcData.dimensions[0];
	int height = srcData.dimensions[1];
	for (auto i=0; i<height; i++) {
		for (auto j=0; j<width; j++, dptr++) {
			if (*dptr > lowThreshold && *dptr < highThreshold) {
				int idx = int(ceil(*dptr/binWidth)-1)*width*height + i*width + j;
				*(sptr+idx) += 1;
				std::cout << *dptr << " " << idx << " spectrum[" << int(ceil(*dptr/binWidth)-1) << "," << i << "," << j << "]=" << *(sptr+idx) << std::endl;
			}
		}
	}
}

Data process(Data srcData) {
	int eventThresh = 10;
	int emax = 500;
	int nextFrameThreshold = 100;

	Data dstData = sort(srcData);
	Data lastFrame = dstData;

//	uint16_t* dptr = (uint16_t*) dstData.data();
//	int width = dstData.dimensions[0];
//	int height = dstData.dimensions[1];
//	for (auto i = 0; i < height; i++) {
//		for (auto j = 0; j < width; j++, dptr++) {
//			std::cout << *dptr << " ";
//		}
//		std::cout << std::endl;
//	}
//	std::cout << std::endl;
//	std::cout << "depth " << srcData.depth() << std::endl;

	nextFrameCorrection(dstData, lastFrame, nextFrameThreshold);

//	dptr = (uint16_t*) dstData.data();
//	width = dstData.dimensions[0];
//	height = dstData.dimensions[1];
//	for (auto i = 0; i < height; i++) {
//		for (auto j = 0; j < width; j++, dptr++) {
//			std::cout << *dptr << " ";
//		}
//		std::cout << std::endl;
//	}
//	std::cout << std::endl;

	chargeSharingAddition_3x3(dstData, eventThresh, emax);
//	chargeSharingAddition_5x5(dstData, eventThresh, emax);
//	chargeSharingDiscrimination_3x3(dstData, eventThresh, emax);
//	chargeSharingDiscrimination_5x5(dstData, eventThresh, emax);
	return dstData;
}

int main() {
	std::vector<int> spectrum_dims;
	int binWidth = 10;
	int nbins = (1000/binWidth)+1;
	int lowThreshold = 9;
	int highThreshold = 500;
	Data spectrum;
	spectrum_dims.push_back(nbins);
	spectrum_dims.push_back(10);
	spectrum_dims.push_back(10);
	spectrum.dimensions = spectrum_dims;
	Buffer *newerBuffer = new Buffer(10*10*2*nbins);
	spectrum.setBuffer(newerBuffer);
	newerBuffer->unref();

	std::vector<int> dimensions;
	Data srcData;
	srcData.type = Data::UINT16;
	dimensions.push_back(10);
	dimensions.push_back(10);
	srcData.dimensions = dimensions;
	Buffer *newBuffer = new Buffer(10*10*2);
	srcData.setBuffer(newBuffer);
	newBuffer->unref();

	uint16_t* buff = (uint16_t*)srcData.data();
	for (auto i=0; i<100; i++)
		buff[i] = 8;
	buff[0] = 65;
	buff[1] = 35;
	buff[10] = 30;
	buff[9] = 2;
	buff[90] = 3;
	buff[89] = 45;
//	buff[98] = 40;
	buff[99] = 85;
	buff[35] = 25;
	buff[36] = 95;
	buff[45] = 20;
	buff[62] = 123;
	buff[76] = 51;

	Data dstData;
	auto t1 = Clock::now();
	for (auto i=0; i<10000; i++) {
		dstData = process(srcData);
		createSpectra(spectrum, dstData, binWidth, lowThreshold, highThreshold);
	}
	auto t2 = Clock::now();
	std::cout << "Delta t2-t1: " << std::chrono::duration_cast<std::chrono::nanoseconds>
			 (t2 - t1).count() << " nanoseconds" << std::endl;

	uint16_t* dptr = (uint16_t*) dstData.data();
	int width = dstData.dimensions[0];
	int height = dstData.dimensions[1];
	for (auto i = 0; i < height; i++) {
		for (auto j = 0; j < width; j++, dptr++) {
			std::cout << *dptr << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
	uint16_t* sptr = (uint16_t*) spectrum.data();
	int bins = spectrum.dimensions[0];
	width = spectrum.dimensions[1];
	height = spectrum.dimensions[2];
	std::cout << bins << std::endl;
	std::cout << width << std::endl;
	std::cout << height << std::endl;
	for (auto k = 0; k < bins; k++) {
		for (auto i = 0; i < height; i++) {
			for (auto j = 0; j < width; j++, sptr++) {
				if (*sptr > 0) {
					int idx = k*height*width+i*width+j;
					std::cout << idx << " spectrum[" << k << "," << i << "," << j << "]=" << *sptr << std::endl;
				}
			}
		}
	}
	std::cout << std::endl;
	return 0;
}
