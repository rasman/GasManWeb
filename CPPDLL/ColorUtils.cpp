#include "pch.h"
#include "ColorUtils.h"

RGB ToRGB(const std::string &color) {
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
