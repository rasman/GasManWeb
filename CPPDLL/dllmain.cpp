#include "pch.h"
#include "GasApplication.h"
#include <fstream>
#include <locale>

#ifdef _WIN32
    #define LL_EXPORT __declspec(dllexport)
    #define BOOL bool
    #define HMODULE void*
    #define DWORD unsigned long
    #define LPVOID void*
#else
    #define LL_EXPORT
    #define APIENTRY
    #define __cdecl
    #define BOOL bool
    #define HMODULE void*
    #define DWORD unsigned long
    #define LPVOID void*
#endif

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case 1: // DLL_PROCESS_ATTACH
    case 2: // DLL_THREAD_ATTACH
    case 3: // DLL_THREAD_DETACH
    case 0: // DLL_PROCESS_DETACH
        break;
    }
    return true;
}
#endif

extern "C" LL_EXPORT const char* __cdecl GasManJsonToCsv(
    const char* jsonStr, int len, int startSecond, int endSecond, int everySeconds
)
{
    GasApplication app;

    if (startSecond < 0 || everySeconds <= 0 || endSecond < startSecond)
        return "Error: impossible time parameters";

    // Stop the application if the file gasman.ini does not exist
    std::ifstream iniFile(app.getIniFile());
    if (!iniFile.good()) {
        return "Error: no INI";
    }
    iniFile.close();

    std::string locale = std::locale("").name();
    app.ReadProfile("Application", "Locale", locale); // Keeping this line

    try {
        std::locale iniLocale(locale);
        std::locale::global(iniLocale);
    }
    catch (const std::runtime_error&) {
        locale = "en_US.UTF-8";
        std::locale defaultLocale(locale);
        std::locale::global(defaultLocale);
    }

    // GasApplication initialization
    if (!app.initialize())
        return "Error: unable to read INI";

    static const auto cvv = GasApplication::createCSV(jsonStr, len, startSecond, endSecond, everySeconds);
    return cvv.c_str();
}
