// GasManWeb.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

// Define function pointer type with __stdcall
typedef const char* (__cdecl *GasManJsonToCsvFunc)(const char*, int, int, int, int);

int main() {
    // (1) Read JSON from file
    std::ifstream jsonFile("data.json");
    if (!jsonFile) {
        std::cerr << "Error: Unable to open data.json\n";
        return 1;
    }
    
    std::string jsonStr((std::istreambuf_iterator<char>(jsonFile)), std::istreambuf_iterator<char>());
    jsonFile.close();
    
    // Get length of JSON string
    int jsonLen = static_cast<int>(jsonStr.length());

    // (2) Load DLL dynamically
    HINSTANCE hDLL = LoadLibrary(TEXT("CPPDLL.dll"));
    if (!hDLL) {
        std::cerr << "Error: Failed to load CPPDLL.dll\n";
        return 1;
    }

    // (3) Use reinterpret_cast for type-safe function pointer assignment
    GasManJsonToCsvFunc GasManJsonToCsv = reinterpret_cast<GasManJsonToCsvFunc>(GetProcAddress(hDLL, "GasManJsonToCsv"));
    if (!GasManJsonToCsv) {
        std::cerr << "Error: Failed to locate GasManJsonToCsv in DLL\n";
        FreeLibrary(hDLL);
        return 1;
    }

    // Call function with required parameters
    const char* resultCsv = GasManJsonToCsv(jsonStr.c_str(), jsonLen, 0, 60, 5);
    
    // (4) Output the result
    std::cout << "CSV Output:\n" << resultCsv << '\n';

    // Free DLL
    FreeLibrary(hDLL);
    return 0;
}