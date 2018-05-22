//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
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
//
// HexitecSavingCtrlObj.cpp

#include "HexitecInterface.h"
#include "HexitecSavingCtrlObj.h"
#include "H5Cpp.h"
#include "lima/CtBuffer.h"
#include "lima/HwFrameInfo.h"

using namespace lima;
using namespace lima::Hexitec;
using namespace std;
using namespace H5;

static const char DIR_SEPARATOR = '/';
static const int RANK_ONE = 1;
static const int RANK_TWO = 2;
static const int RANK_THREE = 3;

/* Static function helper*/
DataType get_h5_type(unsigned char) {
	return PredType(PredType::NATIVE_UINT8);
}
DataType get_h5_type(char) {
	return PredType(PredType::NATIVE_INT8);
}
DataType get_h5_type(unsigned short) {
	return PredType(PredType::NATIVE_UINT16);
}
DataType get_h5_type(short) {
	return PredType(PredType::NATIVE_INT16);
}
DataType get_h5_type(unsigned int) {
	return PredType(PredType::NATIVE_UINT32);
}
DataType get_h5_type(int) {
	return PredType(PredType::NATIVE_INT32);
}
DataType get_h5_type(unsigned long long) {
	return PredType(PredType::NATIVE_UINT64);
}
DataType get_h5_type(long long) {
	return PredType(PredType::NATIVE_INT64);
}
DataType get_h5_type(float) {
	return PredType(PredType::NATIVE_FLOAT);
}
DataType get_h5_type(double) {
	return PredType(PredType::NATIVE_DOUBLE);
}
DataType get_h5_type(std::string& s) {
	return StrType(H5T_C_S1, s.size());
}
DataType get_h5_type(bool) {
	return PredType(PredType::NATIVE_UINT8);
}

template<class T>
void write_h5_dataset(Group group, const char* entry_name, T& val) {
	DataSpace dataspace(H5S_SCALAR);
	DataType datatype = get_h5_type(val);
	DataSet dataset(group.createDataSet(entry_name, datatype, dataspace));
	dataset.write(&val, datatype);
}

template<>
void write_h5_dataset(Group group, const char* entry_name, std::string& val) {
	DataSpace dataspace(H5S_SCALAR);
	DataType datatype = get_h5_type(val);
	DataSet dataset(group.createDataSet(entry_name, datatype, dataspace));
	dataset.write(val.c_str(), datatype);
}

template<class L, class T>
void write_h5_attribute(L location, const char* entry_name, T& val) {
	DataSpace dataspace(H5S_SCALAR);
	DataType datatype = get_h5_type(val);
	Attribute attr(location.createAttribute(entry_name, datatype, dataspace));
	attr.write(datatype, &val);
}

template<class L>
void write_h5_attribute(L location, const char* entry_name, std::string& val) {
	DataSpace dataspace(H5S_SCALAR);
	DataType datatype = get_h5_type(val);
	Attribute attr(location.createAttribute(entry_name, datatype, dataspace));
	attr.write(datatype, val.c_str());
}

/** @brief helper to calculate an optimized chuncking of the image data set
 *
 *
 */
static void calculate_chunck(hsize_t* data_size, hsize_t* chunck, int depth) {
	const double request_chunck_size = 256.;
	const double request_chunck_memory_size = 1024. * 1024.;
	double request_chunck_pixel_nb = request_chunck_memory_size / depth;
	long x_chunk = (long) ceil(double(data_size[2]) / request_chunck_size);
	long y_chunk = (long) ceil(double(data_size[1]) / request_chunck_size);
	long z_chunk = (long) ceil(request_chunck_pixel_nb / ((data_size[2] / x_chunk) * (data_size[1] / y_chunk)));

	hsize_t nb_image = data_size[0];
	while (hsize_t(z_chunk) > nb_image) {
		--x_chunk, --y_chunk;
		if (!x_chunk || !y_chunk)
			break;
		z_chunk = (long) ceil(request_chunck_pixel_nb / ((data_size[2] / x_chunk) * (data_size[1] / y_chunk)));
	}
	if (!x_chunk)
		x_chunk = 1;
	else if (x_chunk > 8)
		x_chunk = 8;

	if (!y_chunk)
		y_chunk = 1;
	else if (y_chunk > 8)
		y_chunk = 8;

	z_chunk = (long) ceil(request_chunck_pixel_nb / ((data_size[2] / x_chunk) * (data_size[1] / y_chunk)));

	if (hsize_t(z_chunk) > nb_image)
		z_chunk = nb_image;
	else if (!z_chunk)
		z_chunk = 1;

	chunck[0] = z_chunk;
	chunck[1] = (hsize_t) ceil(double(data_size[1]) / y_chunk);
	chunck[2] = (hsize_t) ceil(double(data_size[2]) / x_chunk);
}

//=============================================
// SavingCtrlObj
//=============================================
SavingCtrlObj::SavingCtrlObj(Camera& camera, int nb_streams) :
		HwSavingCtrlObj(HwSavingCtrlObj::COMMON_HEADER | HwSavingCtrlObj::MANUAL_WRITE, false), m_nb_streams(nb_streams) {
	DEB_CONSTRUCTOR();
	m_stream = new HwSavingStream *[m_nb_streams];
	for (auto i=0; i<m_nb_streams; i++) {
		m_stream[i] = new HwSavingStream(camera, i);
		m_stream[i]->setActive(true);
	}
}

SavingCtrlObj::~SavingCtrlObj() {
	for (int s = 0; s < m_nb_streams; ++s) {
		delete m_stream[s];
	}
	delete[] m_stream;
}

void SavingCtrlObj::getPossibleSaveFormat(std::list<std::string> &format_list) const {
	DEB_MEMBER_FUNCT();
	format_list.push_back(SavingCtrlObj::HDF5_FORMAT_STR);
}

void SavingCtrlObj::setActive(bool flag, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setActive(flag);
	}
}

bool SavingCtrlObj::isActive(int stream_idx) const {
	DEB_MEMBER_FUNCT();
	return m_stream[stream_idx]->isActive();

}

void SavingCtrlObj::setDirectory(const std::string& directory, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setDirectory(directory);
	}
}

void SavingCtrlObj::setPrefix(const std::string& prefix, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setPrefix(prefix);
	}
}

void SavingCtrlObj::setSuffix(const std::string& suffix, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setSuffix(suffix);
	}
}

void SavingCtrlObj::setOptions(const std::string& options, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setOptions(options);
	}
}

void SavingCtrlObj::setNextNumber(long number, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setNextNumber(number);
	}
}

void SavingCtrlObj::setIndexFormat(const std::string& indexFormat, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setIndexFormat(indexFormat);
	}
}

void SavingCtrlObj::setFramesPerFile(long frames_per_file, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setFramesPerFile(frames_per_file);
	}
}

void SavingCtrlObj::setSaveFormat(const std::string& format, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setSaveFormat(format);
	}
}
void SavingCtrlObj::setOverwritePolicy(const std::string& overwritePolicy, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->setOverwritePolicy(overwritePolicy);
	}
}

/** @brief write a frame manually
 */
void SavingCtrlObj::writeFrame(HwFrameInfoType& hwFrameInfo, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->writeFrame(hwFrameInfo);
	}
}

/** @brief read a frame manually
 */
void SavingCtrlObj::readFrame(HwFrameInfoType& info, int frame_nr, int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx] != nullptr) {
		m_stream[stream_idx]->readFrame(info, frame_nr);
	}
}

/** @brief set frames' common header
 */
void SavingCtrlObj::setCommonHeader(const HeaderMap& header) {
	DEB_MEMBER_FUNCT();
	for (int s = 0; s < m_nb_streams; ++s) {
		if (m_stream[s] != nullptr) {
			m_stream[s]->setCommonHeader(header);
		}
	}
}

/** @brief clear common header
 */
void SavingCtrlObj::resetCommonHeader() {
	DEB_MEMBER_FUNCT();
	for (int s = 0; s < m_nb_streams; ++s) {
		if (m_stream[s] != nullptr) {
			m_stream[s]->resetCommonHeader();
		}
	}
}

void SavingCtrlObj::_prepare(int stream_idx) {
	DEB_MEMBER_FUNCT();
	if (m_stream[stream_idx]->isActive()) {
		m_stream[stream_idx]->prepare();
	}
}

void SavingCtrlObj::start(int stream_idx) {
	DEB_MEMBER_FUNCT();
}

void SavingCtrlObj::stop(int stream_idx) {
	DEB_MEMBER_FUNCT();
}

void SavingCtrlObj::close(int stream_idx) {
	DEB_MEMBER_FUNCT();
	m_stream[stream_idx]->close();
}

std::string SavingCtrlObj::_getFullPath(int image_number, int stream_idx) const {
	return m_stream[stream_idx]->getFullPath(image_number);
}

//=============================================
// HwSavingStream
//=============================================
SavingCtrlObj::HwSavingStream::HwSavingStream(Camera& camera, int streamNb) :
		m_cam(camera), m_streamNb(streamNb) {
}

bool SavingCtrlObj::HwSavingStream::isActive() const {
	return m_active;
}

void SavingCtrlObj::HwSavingStream::setActive(bool active) {
	m_active = active;
}

void SavingCtrlObj::HwSavingStream::setDirectory(const std::string& directory) {
	m_directory = directory;
}

void SavingCtrlObj::HwSavingStream::setPrefix(const std::string& prefix) {
	m_prefix = prefix;
}

void SavingCtrlObj::HwSavingStream::setSuffix(const std::string& suffix) {
	m_suffix = suffix;
}

void SavingCtrlObj::HwSavingStream::setOptions(const std::string& options) {
	m_options = options;
}

void SavingCtrlObj::HwSavingStream::setNextNumber(long number) {
	m_next_number = number;
}

void SavingCtrlObj::HwSavingStream::setIndexFormat(const std::string& indexFormat) {
	m_index_format = indexFormat;
}

void SavingCtrlObj::HwSavingStream::setFramesPerFile(long frames_per_file) {
	DEB_MEMBER_FUNCT();
	m_frames_per_file = frames_per_file;
}

void SavingCtrlObj::HwSavingStream::setSaveFormat(const std::string& format) {
	m_file_format = format;
}

void SavingCtrlObj::HwSavingStream::setOverwritePolicy(const std::string& overwritePolicy) {
	m_overwritePolicy = overwritePolicy;
}

void SavingCtrlObj::HwSavingStream::readFrame(HwFrameInfoType&, int) {
	DEB_MEMBER_FUNCT();
	THROW_HW_ERROR(NotSupported) << "No available for this Hardware";
}

void SavingCtrlObj::HwSavingStream::resetCommonHeader() {
	DEB_MEMBER_FUNCT();
	THROW_HW_ERROR(NotSupported) << "No available for this Hardware";
}

std::string SavingCtrlObj::HwSavingStream::getFullPath(int image_number) const {
	char nbBuffer[32];
	snprintf(nbBuffer, sizeof(nbBuffer), m_index_format.c_str(), image_number);
#ifdef __unix
	const char SEPARATOR = '/';
#else    // WINDOW
	const char SEPARATOR = '\\';
#endif
	std::string fullpath;
	fullpath = m_directory + SEPARATOR;
	fullpath += m_prefix;
	fullpath += nbBuffer;
	fullpath += m_suffix;
	return fullpath;
}

void SavingCtrlObj::HwSavingStream::start() {
}

void SavingCtrlObj::HwSavingStream::stop() {
}

void SavingCtrlObj::HwSavingStream::prepare() {
	DEB_MEMBER_FUNCT();
	DEB_ALWAYS() << "Entering SavingCtrlObj prepare stream " << m_streamNb;
	std::string filename;
	if (m_suffix != ".hdf")
		THROW_HW_ERROR(lima::Error) << "Suffix must be .hdf";

	try {
		// Turn off the auto-printing when failure occurs so that we can
		// handle the errors appropriately
		H5::Exception::dontPrint();

		// Get the fully qualified filename
		char number[16];
		snprintf(number, sizeof(number), m_index_format.c_str(), m_next_number);
		filename = m_directory + DIR_SEPARATOR + m_prefix + number + m_suffix;
		DEB_TRACE() << "Opening filename " << filename << " with overwritePolicy " << m_overwritePolicy;

		if (m_overwritePolicy == "Overwrite") {
			// overwrite existing file
			m_file = new H5File(filename, H5F_ACC_TRUNC);
		} else if (m_overwritePolicy == "Abort") {
			// fail if file already exists
			m_file = new H5File(filename, H5F_ACC_EXCL);
		} else {
			THROW_CTL_ERROR(Error) << "Append and multiset  not supported !";
		}
		m_entry = new Group(m_file->createGroup("/entry"));
		string nxentry = "NXentry";
		write_h5_attribute(*m_entry, "NX_class", nxentry);
		string title = "Lima Hexitec detector";
		write_h5_dataset(*m_entry, "title", title);

		Size size;
		m_cam.getDetectorImageSize(size);
		m_nrasters = size.getHeight();
		m_npixels = size.getWidth();
		m_nframes = m_frames_per_file;
		{
			// ISO 8601 Time format
			time_t now;
			time(&now);
			char buf[sizeof("2011-10-08T07:07:09Z")];
			strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
			string stime = string(buf);
			write_h5_dataset(*m_entry, "start_time", stime);
		}
		Group instrument = Group(m_entry->createGroup("Instrument"));
		string nxinstrument = "NXinstrument";
		write_h5_attribute(instrument, "NX_class", nxinstrument);
		m_instrument_detector = new Group(instrument.createGroup("Hexitec"));
		string nxdetector = "NXdetector";
		write_h5_attribute(*m_instrument_detector, "NX_class", nxdetector);

		Group measurement = Group(m_entry->createGroup("measurement"));
		string nxcollection = "NXcollection";
		write_h5_attribute(measurement, "NX_class", nxcollection);
		m_measurement_detector = new Group(measurement.createGroup("Hexitec"));
		write_h5_attribute(*m_measurement_detector, "NX_class", nxdetector);

		Group det_info = Group(m_instrument_detector->createGroup("detector_information"));
		Group det_params = Group(det_info.createGroup("parameters"));
		double rate;
		m_cam.getFrameRate(rate);
		write_h5_dataset(det_params, "frame rate", rate);

		Group env_info = Group(det_info.createGroup("environment"));
		Camera::Environment env;
		m_cam.getEnvironmentalValues(env);
		write_h5_dataset(env_info, "humidity", env.humidity);
		write_h5_dataset(env_info, "ambientTemperature", env.ambientTemperature);
		write_h5_dataset(env_info, "asicTemperature", env.asicTemperature);
		write_h5_dataset(env_info, "adcTemperature", env.adcTemperature);
		write_h5_dataset(env_info, "ntcTemperature", env.ntcTemperature);

		Group oper_info = Group(det_info.createGroup("operating_values"));
		Camera::OperatingValues opvals;
		m_cam.getOperatingValues(opvals);
		write_h5_dataset(oper_info, "v3_3 ", opvals.v3_3);
		write_h5_dataset(oper_info, "hvMon", opvals.hvMon);
		write_h5_dataset(oper_info, "hvOut", opvals.hvOut);
		write_h5_dataset(oper_info, "v1_2", opvals.v1_2);
		write_h5_dataset(oper_info, "v1_8", opvals.v1_8);
		write_h5_dataset(oper_info, "v3", opvals.v3);
		write_h5_dataset(oper_info, "v2_5", opvals.v2_5);
		write_h5_dataset(oper_info, "v3_3ln", opvals.v3_3ln);
		write_h5_dataset(oper_info, "v1_65ln", opvals.v1_65ln);
		write_h5_dataset(oper_info, "v1_8ana", opvals.v1_8ana);
		write_h5_dataset(oper_info, "v3_8ana", opvals.v3_8ana);
		write_h5_dataset(oper_info, "peltierCurrent", opvals.peltierCurrent);
		write_h5_dataset(oper_info, "ntcTemperature", opvals.ntcTemperature);

		Group process_info = Group(det_info.createGroup("processing_values"));
		int value;
		m_cam.getLowThreshold(value);
		write_h5_dataset(process_info, "LowThreshold ", value);
		m_cam.getHighThreshold(value);
		write_h5_dataset(process_info, "HighThreshold", value);
		int binWidth;
		m_cam.getBinWidth(binWidth);
		write_h5_dataset(process_info, "BinWidth", binWidth);
        int specLen;
        m_cam.getSpecLen(specLen);
        write_h5_dataset(process_info, "SpecLen", specLen);
        int nbins = (specLen / binWidth);

        int saveOpt;
        m_cam.getSaveOpt(saveOpt);

        // StreamNb == 3 is a bit of a kludge for now!!
        if (saveOpt & Camera::SaveSummed && m_streamNb == 3) {
            DEB_TRACE() << "create the spectrum data structure in the file";
            // create the image data structure in the file
            hsize_t data_dims[2], max_dims[2];
            data_dims[1] = nbins;
            data_dims[0] = m_nframes;
            m_image_dataspace = new DataSpace(RANK_TWO, data_dims); // create new dspace
            m_image_dataset = new DataSet(
                    m_measurement_detector->createDataSet("spectrum", PredType::NATIVE_UINT64, *m_image_dataspace));
        } else {
            DEB_TRACE() << "create the image data structure in the file";
            // create the image data structure in the file
            hsize_t data_dims[3], max_dims[3];
            data_dims[1] = m_nrasters;
            data_dims[2] = m_npixels;
            data_dims[0] = m_nframes;
            max_dims[1] = m_nrasters;
            max_dims[2] = m_npixels;
            max_dims[0] = H5S_UNLIMITED;
            // Create property list for the dataset and setup chunk size
            DSetCreatPropList plist;
            hsize_t chunk_dims[3];
            // calculate a optimized chunking
            calculate_chunck(data_dims, chunk_dims, 2);
            plist.setChunk(RANK_THREE, chunk_dims);
            m_image_dataspace = new DataSpace(RANK_THREE, data_dims, max_dims); // create new dspace
            if (saveOpt & Camera::SaveHistogram && m_streamNb == 2) {
                m_image_dataset = new DataSet(
                    m_measurement_detector->createDataSet("raw_image", PredType::NATIVE_UINT32, *m_image_dataspace, plist));
            } else {
                m_image_dataset = new DataSet(
                    m_measurement_detector->createDataSet("raw_image", PredType::NATIVE_UINT16, *m_image_dataspace, plist));
            }
        }
	} catch (FileIException &error) {
		THROW_CTL_ERROR(Error) << "File " << filename << " not opened successfully";
	}
	// catch failure caused by the DataSet operations
	catch (DataSetIException& error) {
		THROW_CTL_ERROR(Error) << "DataSet " << filename << " not created successfully";
		error.printError();
	}
	// catch failure caused by the DataSpace operations
	catch (DataSpaceIException& error) {
		THROW_CTL_ERROR(Error) << "DataSpace " << filename << " not created successfully";
	}
	// catch failure caused by any other HDF5 error
	catch (H5::Exception &e) {
		THROW_CTL_ERROR(Error) << e.getCDetailMsg();
	}
	// catch anything not hdf5 related
	catch (Exception &e) {
		THROW_CTL_ERROR(Error) << e.getErrMsg();
	}
}

void SavingCtrlObj::HwSavingStream::writeFrame(HwFrameInfoType& frame_info) {
	DEB_MEMBER_FUNCT();
	try {
		FrameDim fdim = frame_info.frame_dim;
		Size size = fdim.getSize();
		int width = size.getWidth();
		int height = size.getHeight();
        int saveOpt;
        m_cam.getSaveOpt(saveOpt);

		if (height == 1) {
            uint64_t* image_data = (uint64_t*) frame_info.frame_ptr;
	        hsize_t slab_dim[2];
	        slab_dim[1] = width;
	        slab_dim[0] = 1;
	        DataSpace slabspace = DataSpace(RANK_TWO, slab_dim);

	        hsize_t offset[] = { (unsigned) frame_info.acq_frame_nb, 0};
            hsize_t count[] = { 1, (unsigned) width};
            DEB_TRACE() << DEB_VAR4(m_streamNb, frame_info.acq_frame_nb, offset, count);
            m_image_dataspace->selectHyperslab(H5S_SELECT_SET, count, offset);
            m_image_dataset->write(image_data, PredType::NATIVE_UINT64, slabspace, *m_image_dataspace);
		} else {
		    hsize_t slab_dim[3];
            slab_dim[2] = size.getWidth();
            slab_dim[1] = size.getHeight();
            slab_dim[0] = 1;
            DataSpace slabspace = DataSpace(RANK_THREE, slab_dim);

            hsize_t offset[] = { (unsigned) frame_info.acq_frame_nb, 0, 0};
            hsize_t count[] = { 1, (unsigned) width, (unsigned) height };
            DEB_TRACE() << DEB_VAR4(m_streamNb, frame_info.acq_frame_nb, offset, count);
            m_image_dataspace->selectHyperslab(H5S_SELECT_SET, count, offset);
            if (saveOpt & Camera::SaveHistogram && m_streamNb == 2) {
                m_image_dataset->write((uint32_t*) frame_info.frame_ptr, PredType::NATIVE_UINT32, slabspace, *m_image_dataspace);
            } else {
                m_image_dataset->write((uint16_t*) frame_info.frame_ptr, PredType::NATIVE_UINT16, slabspace, *m_image_dataspace);
            }
        }
	}
	catch (H5::Exception &e) {
		THROW_CTL_ERROR(Error) << e.getCDetailMsg();
	}
}

void SavingCtrlObj::HwSavingStream::close() {
	DEB_MEMBER_FUNCT();

	Group data = Group(m_entry->createGroup("Data"));
	// Create hard link to the Data group.
	//data.link(H5L_TYPE_HARD, "/entry/Instrument/Hexitec/spectra", "spectra");

	// ISO 8601 Time format
	time_t now;
	time(&now);
	char buf[sizeof("2011-10-08T07:07:09Z")];
	strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
	string etime = string(buf);
	write_h5_dataset(*m_entry, "end_time", etime);

	DEB_TRACE() << "closing stream " << m_streamNb;
	m_file->close();
	stop();

	delete m_measurement_detector;
	delete m_instrument_detector;
	delete m_image_dataspace;
	delete m_image_dataset;

	delete m_entry;
	delete m_file;
}

void SavingCtrlObj::HwSavingStream::setCommonHeader(const HeaderMap& headerMap) {
	DEB_MEMBER_FUNCT();

	if (!headerMap.empty()) {
		Group header = Group(m_entry->createGroup("Header"));
		for (map<string, string>::const_iterator it = headerMap.begin(); it != headerMap.end(); it++) {
			string key = it->first;
			string value = it->second;
			write_h5_dataset(header, key.c_str(), value);
		}
	}
}
