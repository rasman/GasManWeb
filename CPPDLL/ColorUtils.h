#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H
#include "framework.h"
#include <algorithm>
#include <vector>
#include <string>

struct RGB {
    int r;
    int g;
    int b;
};

RGB ToRGB(const std::string &color);

#endif // COLOR_UTILS_H
