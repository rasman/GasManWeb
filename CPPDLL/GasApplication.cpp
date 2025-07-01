#include "pch.h"
#include "GasApplication.h"
#include "GasDoc.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <string>
#include <variant>
#include <cstring> // For strlen and strcpy
#include <iostream> // For debug output
#include <locale> 
#include <codecvt>
#include <sstream>

GasApplication *gasApp = nullptr;
int zero = 0;

GasSettings::GasSettings(const std::string& fileName) : filename(fileName) {
    loadFile();
}

void GasSettings::loadFile() const {
    SI_Error rc = ini.LoadFile(filename.c_str());
    if (rc < 0) {
        std::cerr << "Failed to load INI file: " << filename << std::endl;
    } 
}

void GasSettings::saveFile() const {
    SI_Error rc = ini.SaveFile(filename.c_str());
    if (rc < 0) {
        throw std::runtime_error("Can't save " + filename);
    }
}

ProfileValue GasSettings::get(const std::string& group, const std::string& name, const ProfileValue& default_value) const {
    if (auto strVal = std::get_if<std::string>(&default_value)) {
        const char* value = ini.GetValue(group.c_str(), name.c_str(), strVal->c_str());
        return std::string(value);
    } else if (auto vecVal = std::get_if<std::vector<std::string>>(&default_value)) {
        std::string result = ini.GetValue(group.c_str(), name.c_str(), "");
        std::vector<std::string> vecResult = parseStringToVector(result);
        return vecResult.empty() ? *vecVal : vecResult;
    } else if (auto doubleVal = std::get_if<double>(&default_value)) {
        const char* valueStr = ini.GetValue(group.c_str(), name.c_str(), "");
        std::string valueStrCpp(valueStr); // Convert to std::string
        if (!valueStrCpp.empty() && valueStrCpp != "0.000000") {
            double value = std::stod(valueStrCpp);
            return value;
        } else {
            return *doubleVal;
        }
    }

    throw std::runtime_error("Unsupported variant type");
}

void GasSettings::setValue(const std::string& group, const std::string& name, const ProfileValue& value) {
    if (auto strVal = std::get_if<std::string>(&value)) {
        ini.SetValue(group.c_str(), name.c_str(), strVal->c_str());
    } else if (auto vecVal = std::get_if<std::vector<std::string>>(&value)) {
        std::string combined = combineVectorToString(*vecVal);
        ini.SetValue(group.c_str(), name.c_str(), combined.c_str());
    } else if (auto doubleVal = std::get_if<double>(&value)) {
        ini.SetDoubleValue(group.c_str(), name.c_str(), *doubleVal);
    }
    saveFile();
}

std::vector<std::string> GasSettings::parseStringToVector(const std::string& str) const {
    std::vector<std::string> result;
    std::istringstream iss(str);
    for (std::string s; std::getline(iss, s, ','); ) {
        result.push_back(s);
    }
    return result;
}

std::string GasSettings::combineVectorToString(const std::vector<std::string>& vec) const {
    std::string combined;
    for (const auto& item : vec) {
        if (!combined.empty()) {
            combined += ", ";
        }
        combined += item;
    }
    return combined;
}


// GasApplication constructor
GasApplication::GasApplication()
{
    gasApp = this;
    inifile = "./gasman.ini";
}

GasApplication::~GasApplication()
{
    for (const auto& file : temporaryFiles)
        if (std::ifstream(file).good())
            std::remove(file.c_str());
    temporaryFiles.clear();
}

std::string GasApplication::createCSV(const char* jsonStr, int len, int startSecond, int endSecond, int everySeconds) {
    GasDoc gasdoc;

    bool loadResult = gasdoc.loadJsonFile(jsonStr, len);
    
    std::string csv;
    if (loadResult) {
        csv = gasdoc.dumpCSV(startSecond, endSecond, everySeconds);
    } else {
        csv = "Error: cannot parse data file";
    }
    
    return csv;
}

/////////////////////////////////////////////////////////////////////////////
// GasApplication initialization

bool GasApplication::initialize()
{
    std::vector<std::string> colors = std::get<std::vector<std::string>>(ReadProfile("Colors", "Colors", std::vector<std::string>()));
    if (colors.empty()) {
        std::vector<std::string> addColors = {"red", "blue", "green", "orange", "purple", "black", "white", "yellow", "cyan", "darkBlue"};
        WriteProfile("Colors", "Colors", addColors);
    }

    InitializeGlobalLists();
    GasDoc::ReadProfile();
    return true;
}

ProfileValue GasApplication::ReadProfile(const std::string &group, const std::string &param, const ProfileValue &defVal) const
{
    GasSettings settings(getIniFile());
	return settings.get(group, param, defVal);
}

void GasApplication::WriteProfile(const std::string &group, const std::string &param, const ProfileValue &val) {
    GasSettings settings(getIniFile());
	settings.setValue(group, param, val);     
}

void GasApplication::addTemporaryFile(const std::string &file)
{
    temporaryFiles.push_back(file);
}

std::vector<std::string> GasApplication::getColors()
{
    std::vector<std::string> colors;
    std::vector<std::string> fromINI = std::get<std::vector<std::string>>(gasApp->ReadProfile("Colors", "Colors", std::vector<std::string>()));
    for (const auto& color : listColors)
    {
        if (std::find(fromINI.begin(), fromINI.end(), color) != fromINI.end())
        {
            colors.push_back(color);
        }
    }
    return colors;
}

std::vector<std::string> GasApplication::getAgents()
{
    std::vector<std::string> agents;
    std::vector<std::string> fromINI = std::get<std::vector<std::string>>(gasApp->ReadProfile("Agents", "Agents", std::vector<std::string>()));
    for (const auto& sectionName : fromINI)
    {
        std::string agent = std::get<std::string>(gasApp->ReadProfile(sectionName, "LocalName", std::string("")));
        agents.push_back(!agent.empty() ? agent : sectionName);
    }
    return agents;
}

std::string GasApplication::getNextAvailableColor(const GasDoc * const thisDoc)
{
    std::vector<std::string> colors = getColors();
    std::vector<std::string>::size_type nColors = colors.size();
    int uses[50][3] = {0};

    for (std::vector<GasAnesthetic*>::size_type j = 0; j < thisDoc->m_anesArray.size(); j++) {
        GasAnesthetic* anes = thisDoc->m_anesArray[j];
        auto it = std::find(colors.begin(), colors.end(), anes->m_defaultColor);
        if (it != colors.end())
            uses[std::distance(colors.begin(), it)][0] += 1;
    }

    for (const auto& doc : docs) {
        int useType = doc != thisDoc ? 1 : 2;
        for (const auto& color : doc->colors) {
            auto it = std::find(colors.begin(), colors.end(), color);
            if (it != colors.end())
                uses[std::distance(colors.begin(), it)][useType] += 1;
        }
    }

    int minUsedEntry = -1, minUsedTotal = 65535;
    for (std::vector<std::string>::size_type i = 0; i < nColors; i++) {
        int total = 0;
        for (int j = 0; j < 3; j++)
            total += uses[i][j];
        if (total == 0)
            return colors[i];
        if (total < minUsedTotal) {
            minUsedTotal = total;
            minUsedEntry = i;
        }
    }

    for (std::vector<std::string>::size_type i = 0; i < nColors; i++) {
        if (uses[i][0] + uses[i][2] == 0)
            return colors[i];
    }

    return colors[minUsedEntry];
}
