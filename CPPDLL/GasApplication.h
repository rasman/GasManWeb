#ifndef GASAPPLICATION_H
#define GASAPPLICATION_H
#include "framework.h"
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <variant>
#include <sstream>
#include <locale> 
#include <codecvt>
#include "importLibraries/SimpleIni.h"

#include <filesystem>
#include <iostream>

class GasDoc;

using ProfileValue = std::variant<std::string, std::vector<std::string>, double>;

class GasSettings {
public:
    GasSettings(const std::string& fileName);
    ProfileValue get(const std::string& group, const std::string& name, const ProfileValue& default_value) const;
    void setValue(const std::string& group, const std::string& name, const ProfileValue& value);

private:
    mutable CSimpleIniA ini;
    std::string filename;

    void loadFile() const;
    void saveFile() const;
    std::vector<std::string> parseStringToVector(const std::string& str) const;
    std::string combineVectorToString(const std::vector<std::string>& vec) const;
};
	
class GasApplication
{
public:
    GasApplication();
    ~GasApplication();
	static std::string createCSV(const char* jsonStr, int len, int startSecond, int endSecond, int everySeconds);
    // Read preferences from gasman.ini
    ProfileValue ReadProfile(const std::string &group, const std::string &param, const ProfileValue &defVal) const;
    void WriteProfile(const std::string &group, const std::string &param, const ProfileValue &val);

    bool initialize(); // Initialization

    void addTemporaryFile(const std::string &file);
    inline std::string getIniFile() const { return inifile; }

    static std::vector<std::string> getColors();
    static std::string getNextAvailableColor(const GasDoc * const thisDoc);
    static std::vector<std::string> getAgents();

private:
    std::vector<std::string> temporaryFiles; // List of temporary files
    std::vector<std::string> filesToOpen; // if the application opens with a file specified.

    std::string inifile; // Path to gasman.ini
};


extern GasApplication *gasApp;

#endif // GASAPPLICATION_H
