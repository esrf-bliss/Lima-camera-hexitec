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
#ifndef HEXITECSAVINGCTRLOBJ_H
#define HEXITECSAVINGCTRLOBJ_H

#include <processlib/Data.h>
#include "lima/HwInterface.h"
#include <vector>
#include <memory>
#ifndef SIPCOMPILATION
#include "H5Cpp.h"
#endif

namespace lima {
namespace Hexitec {

class Camera;

class SavingCtrlObj: public HwSavingCtrlObj {
DEB_CLASS_NAMESPC(DebModCamera, "SavingCtrlObj","Hexitec");

public:
	SavingCtrlObj(Camera& cam, int nb_streams);
	~SavingCtrlObj();

	class HwSavingStream {
	DEB_CLASS_NAMESPC(DebModCamera, "SavingCtrlObj","HwSavingStream");
	public:
		HwSavingStream(Camera& camera, int streamNb);
		bool isActive() const;
		void setActive(bool flag);
		void setDirectory(const std::string& directory);
		void setPrefix(const std::string& prefix);
		void setSuffix(const std::string& suffix);
		void setOptions(const std::string& options);
		void setNextNumber(long number);
		void setIndexFormat(const std::string& indexFormat);
		void setFramesPerFile(long frames_per_file);
		void setSaveFormat(const std::string& format);
		void setOverwritePolicy(const std::string& overwritePolicy);

		void writeFrame(HwFrameInfoType& info);
		void readFrame(HwFrameInfoType&, int);
		void setCommonHeader(const HeaderMap& header);
		void resetCommonHeader();
		std::string getFullPath(int image_number) const;
		void prepare();
		void start();
		void stop();
		void close();

	private:
		Camera& m_cam;
		int m_nframes;
		int m_npixels;
		int m_nrasters;
		int m_nbins;

		int m_streamNb;
		bool m_active;
		std::string m_directory;
		std::string m_prefix;
		std::string m_suffix;
		std::string m_options;
		long m_next_number;
		std::string m_file_format;
		std::string m_index_format;
		std::string m_overwritePolicy;
		long m_frames_per_file;
#ifndef SIPCOMPILATION
		H5::H5File *m_file;
		H5::Group *m_entry;
		H5::Group *m_measurement_detector;
		H5::Group *m_instrument_detector;
		H5::DataSet *m_image_dataset;
		H5::DataSpace *m_image_dataspace;
#endif
	};

	void getPossibleSaveFormat(std::list<std::string> &format_list) const;
	bool isActive(int stream_idx) const;
	void setActive(bool flag, int stream_idx);
	void setDirectory(const std::string& directory, int stream_idx);
	void setPrefix(const std::string& prefix, int stream_idx);
	void setSuffix(const std::string& suffix, int stream_idx);
	void setOptions(const std::string& options, int stream_idx);
	void setNextNumber(long number, int stream_idx);
	void setIndexFormat(const std::string& indexFormat, int stream_idx);
	void setFramesPerFile(long frames_per_file, int stream_idx);
	void setSaveFormat(const std::string& format, int stream_idx);
	void setOverwritePolicy(const std::string& overwritePolicy, int stream_idx);
	void writeFrame(HwFrameInfoType& info, int stream_idx);
	void readFrame(HwFrameInfoType& info, int frame_nr, int stream_idx);
	void setCommonHeader(const HeaderMap& header);
	void resetCommonHeader();
	void close(int stream_idx);
	std::string _getFullPath(int image_number, int stream_idx) const;
	void _prepare(int stream_idx);
	void start(int stream_idx);
	void stop(int stream_idx);
	void getPossibleSaveFormat(std::list<std::string> &format_list, int stream_idx) const;

	int m_nb_streams;
	HwSavingStream **m_stream;
};

} // namespace Hexitec
} // namespace lima

#endif	// HEXITECSAVINGCTRLOBJ_H

