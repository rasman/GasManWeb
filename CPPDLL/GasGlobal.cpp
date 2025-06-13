#include "GasGlobal.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

std::vector<std::string> listCompart = {"CKT", "ALV", "VRG", "MUS", "FAT", "VEN"}; // For Internal Use; do not translate

std::vector<std::string> listTargetCompart;

std::vector<std::string> listCircuit;

std::vector<std::string> listSpeed;

std::vector<std::string> listTime;

std::vector<std::string> listColors;

static std::vector<std::string> listQColors;

// Remove the ToRGB function from here
/*
RGB ToRGB(const std::string &color)
{
    static std::vector<std::string> listColors = {"red", "darkRed", "blue", "darkBlue", "green", "darkGreen", "orange", "purple", "black", "white", "yellow", "cyan"};
    static std::vector<RGB> listRGBColors = {
        {255, 0, 0}, {139, 0, 0}, {0, 0, 255}, {0, 0, 139}, {0, 255, 0}, {0, 100, 0},
        {255, 165, 0}, {128, 0, 128}, {0, 0, 0}, {255, 255, 255}, {255, 255, 0}, {0, 255, 255}
    };

    auto it = std::find(listColors.begin(), listColors.end(), color);
    if (it != listColors.end())
        return listRGBColors[std::distance(listColors.begin(), it)];
    return {0, 0, 0}; // Default to black
}
*/

void InitializeGlobalLists()
{
    listTargetCompart = {"CKT", "ALV", "VRG", "MUS", "FAT", "VEN"};

    listCircuit = {"Open", "Semi-closed", "Closed", "Ideal"};

    listSpeed = {"1x", "2x", "5x", "10x", "20x", "60x", "120x", "AFAP"};

    listTime = {"5 Min", "10 Min", "15 Min", "30 Min", "45 Min", "1 Hr", "2 Hr", "6 Hr", "12 Hr", "1 Day"};

    listColors = {"red", "darkRed", "blue", "darkBlue", "green", "darkGreen", "orange", "purple", "black", "white", "yellow", "cyan"};
}

// This routine returns the base name of the file without the file path
std::string completeBaseName(const std::string &fullFileName)
{
    size_t pos = fullFileName.find_last_of("/\\");
    return (pos == std::string::npos) ? fullFileName : fullFileName.substr(pos + 1);
}

// This routine returns the name of the file without the file path
std::string fileName(const std::string &fullFileName)
{
    size_t pos = fullFileName.find_last_of("/\\");
    return (pos == std::string::npos) ? fullFileName : fullFileName.substr(pos + 1);
}

std::vector<GasDoc *> docs;
