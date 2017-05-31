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
#include "HexitecSavingTask.h"
#include <algorithm>
#include <vector>
#include "processlib/PoolThreadMgr.h"
#include "processlib/TaskMgr.h"

using namespace lima::Hexitec;

HexitecSavingTask::HexitecSavingTask(SavingCtrlObj& savingCtrlObj, int stream_idx) :
		LinkTask(true), m_saving(savingCtrlObj), m_stream_idx(stream_idx) {
}

HexitecSavingTask::~HexitecSavingTask() {
}

Data HexitecSavingTask::process(Data& aData) {
	DEB_MEMBER_FUNCT();
	int nframe;
	int frameNb;
	int width;
	int height;
	uint16_t *dptr = (uint16_t*) aData.data();
	DEB_PARAM() << DEB_VAR1(aData);
	if (aData.dimensions.size() == 2) {
		nframe = 1;
		width = aData.dimensions[0];
		height = aData.dimensions[1];
	} else {
		width = aData.dimensions[0];
		height = aData.dimensions[1];
		nframe = aData.dimensions[2];
	}
	for (auto i = 0; i < nframe; i++) {
		frameNb = (nframe == 1) ? aData.frameNumber : i;
		FrameDim frameDim(width, height, Bpp16);
		DEB_TRACE() << DEB_VAR3(width, height, frameNb);
		HwFrameInfo hwFrameInfo(frameNb, dptr, &frameDim, aData.timestamp, true,
				HwFrameInfo::OwnerShip::Shared);
		DEB_TRACE() << DEB_VAR4(m_stream_idx, frameNb, width, height);
		m_saving.writeFrame(hwFrameInfo, m_stream_idx);
		dptr += (width*height);
	}
	return aData;
}
