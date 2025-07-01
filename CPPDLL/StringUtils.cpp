// StringUtils.cpp
#include "pch.h"
#include "StringUtils.h"
#include <algorithm>
#include <cctype>

std::string toLowerCase(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lowerStr;
}
