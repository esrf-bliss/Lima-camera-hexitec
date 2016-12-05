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

#include "processlib/LinkTask.h"
#include "lima/HwFrameInfo.h"

namespace lima {
namespace Hexitec {

class HexitecSavingTask: public LinkTask {
DEB_CLASS_NAMESPC(DebModControl,"HexitecSavingTask","Control");
public:
	HexitecSavingTask(SavingCtrlObj& savingCtrlObj, int stream_idx) :
			LinkTask(true), m_saving(savingCtrlObj), m_stream_idx(stream_idx) {
	}

	Data process(Data& aData) {
		DEB_MEMBER_FUNCT();
		DEB_PARAM() << DEB_VAR1(aData);
		FrameDim frameDim(aData.dimensions[0], aData.dimensions[1], Bpp16);
		DEB_TRACE() << DEB_VAR3(aData.dimensions[0], aData.dimensions[1], aData.frameNumber);
		HwFrameInfo hwFrameInfo(aData.frameNumber, aData.data(), &frameDim, aData.timestamp, true, HwFrameInfo::OwnerShip::Shared);
		DEB_TRACE() << "saving frame " << aData.frameNumber;
		m_saving.writeFrame(hwFrameInfo, m_stream_idx);
		return aData;
	}

private:
	SavingCtrlObj m_saving;
	int m_stream_idx;
};

//
//class HexitecSavingCBK: public TaskEventCallback {
//	DEB_CLASS_NAMESPC(DebModControl,"HexitecSavingCBK","Control");
//public:
//	_SaveCBK(Stream& stream) : m_stream(stream) {}
//	virtual void finished(Data &aData) {
//		DEB_MEMBER_FUNCT();
//		DEB_PARAM() << DEB_VAR1(aData);
//
//		m_stream[m_stream_idx].saveFinished(aData);
//	}
}
}
