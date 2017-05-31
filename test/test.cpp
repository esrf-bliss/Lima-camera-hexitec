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

#include <string>
#include <iostream>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include "HexitecApi.h"
#include "INIReader.h"

using namespace std;
using namespace HexitecAPI;

int pack(std::string str, int idx) {
	int val = 0;
	for (auto j=0; j<8; j++, idx++) {
		val += str.at(idx) - 48 << j;
	}
}

int readConfiguration(string fname) {
	string line;
	std::string section;
	INIReader reader(fname);
	if (reader.ParseError() < 0) {
		std::cout << "I/O error while reading file." << std::endl;
		return 1;
	}
	section = "HexitecSystemConfig";
	m_hexitecSystemConfig.AdcDelay = reader.GetInteger(section, "ADC1 Delay", -1);
	m_hexitecSystemConfig.SyncSignalDelay = reader.GetInteger(section, "Delay sync signals", -1);
	m_hexitecSystemConfig.AdcSample = reader.GetInteger(section, "ADC sample", -1);
	m_hexitecSystemConfig.AdcClockPhase = reader.GetInteger(section, "ADC1 Clock Phase", -1);

	section = "HexitecSensorConfig";
	m_hexitecSensorConfig.Gain = reader.GetInteger(section, "Gain", -1);
	m_hexitecSensorConfig.Row_S1 = reader.GetInteger(section, "Row -> S1", -1);
	m_hexitecSensorConfig.S1_Sph = reader.GetInteger(section, "S1 -> Sph", -1);
	m_hexitecSensorConfig.Sph_S2 = reader.GetInteger(section, "Sph -> S2", -1);
	m_hexitecSensorConfig.Vcal2_Vcal1 = reader.GetInteger(section, "VCAL2 -> VCAL1", -1);
	m_hexitecSensorConfig.WaitClockCol = reader.GetInteger(section, "Wait clock column", -1);
	m_hexitecSensorConfig.WaitClockRow = reader.GetInteger(section, "Wait clock row", -1);

	section = "HexitecSetupRegister";
	std::string colEnbStr;
	std::string colPwrStr;
	std::string colCalStr;
	colEnbStr = reader.Get(section, "ColumnEn_1stChannel", "");
	colPwrStr = reader.Get(section, "ColumnPwr1stChannel", "");
	colCalStr = reader.Get(section, "ColumnCal1stChannel", "");
	colEnbStr += reader.Get(section, "ColumnEn_2ndChannel", "");
	colPwrStr += reader.Get(section, "ColumnPwr2ndChannel", "");
	colCalStr += reader.Get(section, "ColumnCal2ndChannel", "");
	colEnbStr += reader.Get(section, "ColumnEn_3rdChannel", "");
	colPwrStr += reader.Get(section, "ColumnPwr3rdChannel", "");
	colCalStr += reader.Get(section, "ColumnCal3rdChannel", "");
	colEnbStr += reader.Get(section, "ColumnEn_4thChannel", "");
	colPwrStr += reader.Get(section, "ColumnPwr4thChannel", "");
	colCalStr += reader.Get(section, "ColumnCal4thChannel", "");

	std::string rowEnbStr;
	std::string rowPwrStr;
	std::string rowCalStr;
	rowEnbStr = reader.Get(section, "RowEn_1stBlock", "");
	rowPwrStr = reader.Get(section, "RowPwr1stBlock", "");
	rowCalStr = reader.Get(section, "RowCal1stBlock", "");
	rowEnbStr += reader.Get(section, "RowEn_2ndBlock", "");
	rowPwrStr += reader.Get(section, "RowPwr2ndBlock", "");
	rowCalStr += reader.Get(section, "RowCal2ndBlock", "");
	rowEnbStr += reader.Get(section, "RowEn_3rdBlock", "");
	rowPwrStr += reader.Get(section, "RowPwr3rdBlock", "");
	rowCalStr += reader.Get(section, "RowCal3rdBlock", "");
	rowEnbStr += reader.Get(section, "RowEn_4thBlock", "");
	rowPwrStr += reader.Get(section, "RowPwr4thBlock", "");
	rowCalStr += reader.Get(section, "RowCal4thBlock", "");

	std::cout << colEnbStr << std::endl;
	std::cout << colPwrStr << std::endl;
	std::cout << colCalStr << std::endl;
	std::cout << rowEnbStr << std::endl;
	std::cout << rowPwrStr << std::endl;
	std::cout << rowCalStr << std::endl;

	int idx = 0;
	for (auto i=0; i<10; i++, idx+= 8) {
		m_hexitecSensorConfig.SetupCol.ReadEn[i] = pack(colEnbStr, idx);
		m_hexitecSensorConfig.SetupCol.PowerEn[i] = pack(colPwrStr, idx);
		m_hexitecSensorConfig.SetupCol.CalEn[i] = pack(colCalStr, idx);
		m_hexitecSensorConfig.SetupRow.ReadEn[i] = pack(rowEnbStr, idx);
		m_hexitecSensorConfig.SetupRow.PowerEn[i] = pack(rowPwrStr, idx);
		m_hexitecSensorConfig.SetupRow.CalEn[i] = pack(rowCalStr, idx);
	}
	return 0;
}

int main() {
	string fname = "./HexitecApi.ini";
	readConfiguration(fname);
}
