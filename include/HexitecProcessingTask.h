//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2016
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
#ifndef HEXITEC_PROCESSINGTASK_H
#define HEXITEC_PROCESSINGTASK_H

#include "processlib/LinkTask.h"
#include "lima/HwFrameInfo.h"
//#include "HexitecSavingCtrlObj.h"
#include "HexitecSavingTask.h"
#include "HexitecCamera.h"

namespace lima {
namespace Hexitec {

class HexitecProcessingTask : public LinkTask {
DEB_CLASS_NAMESPC(DebModControl,"HexitecProcessingTask","Control");
public:
//HexitecProcessingTask(SavingCtrlObj& savingCtrlObj, int stream_idx, Camera::ProcessType processType, int asicPitch,
	HexitecProcessingTask(HexitecSavingTask* savingTask, Camera::ProcessType processType, int asicPitch,
			int binWidth, int speclen, int lowThreshold, int highThreshold);
	~HexitecProcessingTask();

	Data process(Data& aData);
	void setGlobalHistogram(Data& data);
	Data getGlobalHistogram();
	int getNbins();
	int getNbProcessedFrames();
//	void setPreviousFrame(Data& data);

private:
	Data sort(Data& srcData);
	void chargeSharingAddition_3x3(Data& data);
	void chargeSharingAddition_5x5(Data& data);
	void chargeSharingDiscrimination_3x3(Data& data);
	void chargeSharingDiscrimination_5x5(Data& data);
	void nextFrameCorrection(Data& data, Data& lastFrame);
	void createSpectra(Data& srcData);

	void csa_3x3(Data& srcData, int yp, int xp);
	void csa_5x5(Data& srcData, int yp, int xp);
	void csd_3x3(Data& srcData, int yp, int xp);
	void csd_5x5(Data& srcData, int yp, int xp);

	Data m_globalHist;
	HexitecSavingTask* m_savingTask;
//	SavingCtrlObj m_saving;
//	int m_stream_idx;
	int m_processType;
	int m_asicPitch;
	int m_binWidth;
	int m_speclen;
	int m_lowThreshold;
	int m_highThreshold;
	int m_nbins;
//	Data m_lastFrame;
	std::atomic<int> processedCounter;
};

} // endif Hexitec
} // endif lima
#endif
