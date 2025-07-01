#include "pch.h"
#include "GasAnes.h"
#include <cassert>
#include <string>
#include "GasApplication.h"
#include "GasDoc.h"
#include <variant>
#include "trim.h"


bool ichar_equals(char a, char b)
{
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
}

bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), ichar_equals);
}
/////////////////////////////////////////////////////////////////////////////
// Anesthetic construction/destruction

GasAnesthetic::GasAnesthetic()
{
    m_strSectionName = "Unknown";
    m_strName = "Unknown";
    m_strFmt = "%4.2f";
    m_defaultColor = std::string();
    m_fLambdaBG = 1.4F;
    m_fMAX_DEL = 5.0F;
    m_fDefDEL = 0.0F;
    m_fAmbient = 0.0F;
    m_fMAC = 0.0F;
    m_fSolubility[CKT] = 1.0F;
    m_fSolubility[ALV] = 1.0F;
    m_fSolubility[VRG] = 2.6F;
    m_fSolubility[MUS] = 5.2F;
    m_fSolubility[FAT] = 60.0F;
    m_fSolubility[VEN] = m_fLambdaBG;
    m_wVolatility = 0;
    m_fDefUnitDose = 0.0F;
    m_bIsLiquid = false;
    m_fBottleCost = 0.0F;
    m_nBottleSize = 0;
    m_wHighFGF = 0;
    m_wHighVA = 0;
    m_wHighCO = 0;
}

GasAnesthetic::~GasAnesthetic()
{
}

/////////////////////////////////////////////////////////////////////////////
// Anesthetic Implementation

// Assignment

GasAnesthetic& GasAnesthetic::operator=(const GasAnesthetic& otherAnes)
{
    if (&otherAnes == this) return *this;
    m_strSectionName = otherAnes.m_strSectionName;
    m_strName = otherAnes.m_strName;
    m_strFmt = otherAnes.m_strFmt;
    m_fLambdaBG = otherAnes.m_fLambdaBG;
    m_fMAX_DEL = otherAnes.m_fMAX_DEL;
    m_fDefDEL = otherAnes.m_fDefDEL;
    m_fAmbient = otherAnes.m_fAmbient;
    m_fMAC = otherAnes.m_fMAC;
    for (int i = 0; i < MAX_COMPART; i++)
        m_fSolubility[i] = otherAnes.m_fSolubility[i];
    m_wVolatility = otherAnes.m_wVolatility;
    m_fDefUnitDose = otherAnes.m_fDefUnitDose;
    m_bIsLiquid = otherAnes.m_bIsLiquid;
    m_fBottleCost = otherAnes.m_fBottleCost;
    m_nBottleSize = otherAnes.m_nBottleSize;
    m_defaultColor = otherAnes.m_defaultColor;
    return *this;
}

// Read in profile of anesthetic. Return false if required parameter
// missing, or on format error.

bool GasAnesthetic::Erm(const std::string &strParam)
{
    std::string strMessage = "<p>Error reading '" + strParam + "' parameter for anesthetic '" + m_strSectionName + "'. Agent ignored.";
    return false;
}

bool GasAnesthetic::ReadProfile() {
    std::string str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "LocalName", std::string()));

    if (str.length())
        m_strName = str;
    else
        m_strName = m_strSectionName;

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "Format", std::string()));
    int i = str.length();
    if (i != 0) {
        if (i != 5) return Erm("Format");
        if (!(isdigit(str[1]) && isdigit(str[3]))) return Erm("Format");
        if (str[0] != '%' || str[2] != '.' || str[4] != 'f') return Erm("Format");
        m_strFmt = str;
    }

    m_fSolubility[VEN] = m_fLambdaBG = (float)std::get<double>(gasApp->ReadProfile(m_strSectionName, "Lambda", 0.0));
    if (m_fLambdaBG <= 0.0F || m_fLambdaBG > 100.0F) return Erm("Lambda");
	
    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "Max", std::string()));
    if (str.length()) {
        m_fMAX_DEL = std::stof(str);
        if (m_fMAX_DEL <= 0.0F || m_fMAX_DEL > 2000.0F) return Erm("Max");
    }

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "High", std::string()));
    if (str.length()) {
        m_wHighDEL = std::stoul(str);
        if (m_wHighDEL <= 0 || m_wHighDEL > static_cast<uint16_t>(m_fMAX_DEL)) return Erm("High");
    } else {
        m_wHighDEL = static_cast<uint16_t>(m_fMAX_DEL);
    }

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "Default", std::string()));
    if (str.length()) {
        m_fDefDEL = std::stof(str);
        if (m_fDefDEL < 0.0F || m_fDefDEL > m_fMAX_DEL) return Erm("Default");
    } else {
        m_fDefDEL = m_fMAX_DEL / 2.0F;
    }

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "Ambient", std::string()));
    if (str.length()) {
        m_fAmbient = std::stof(str);
        if (m_fAmbient <= 0.0F || m_fAmbient > m_fMAX_DEL) return Erm("Ambient");
    }

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "MAC", std::string()));
    if (str.length()) {
        m_fMAC = std::stof(str);
        if (m_fMAC <= 0.0F || m_fMAC > m_fMAX_DEL) return Erm("MAC");
    }

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "Volatility", std::string()));
    if (str.length()) {
        m_wVolatility = std::stoul(str);
        if (m_wVolatility <= 0 || m_wVolatility > 1000) return Erm("Volatility");
    }

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "DefUnitDose", std::string()));
    if (str.length()) {
        m_fDefUnitDose = std::stof(str);
        if (m_fDefUnitDose <= 0.0F || m_fDefUnitDose > 50.0F) return Erm("DefUnitDose");
    }

    m_bIsLiquid = m_fDefUnitDose != 0.0F;

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "BottleCost", std::string()));
    if (str.length()) {
        m_fBottleCost = std::stof(str);
        if (m_fBottleCost <= 0.0F || m_fBottleCost > MAX_BOTTLE_COST) return Erm("BottleCost");
    }

    str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, "BottleSize", std::string()));
    if (str.length()) {
        m_nBottleSize = GasDoc::safeStringToInt(str);
        if (m_nBottleSize <= 0 || m_nBottleSize > MAX_BOTTLE_SIZE) return Erm("BottleSize");
    }

    m_defaultColor = DefaultColorFromProfile(m_strSectionName);

    for (int i = 0; i < MAX_COMPART; ++i) {
        str = std::get<std::string>(gasApp->ReadProfile(m_strSectionName, listCompart[i], std::string()));
        if (!str.length()) {
            if (!(i == VRG || i == MUS || i == FAT)) continue;
            return Erm(listCompart[i]);
        }
        m_fSolubility[i] = std::stof(str);
        if (m_fSolubility[i] <= 0.0F || m_fSolubility[i] > 500.0F) return Erm(listCompart[i]);
    }

    return true;
}


std::string GasAnesthetic::DefaultColorFromProfile(const std::string& name)
{
    std::string str = std::get<std::string>(gasApp->ReadProfile(name, "Color", std::string()));
    if (str.length()) {
        std::vector<std::string> colors = gasApp->getColors();
        for( int i=colors.size()-1; i>=0; --i)
			if( !iequals(colors.at(i), str))
				  colors.erase(colors.begin()+i);
        if (colors.size() != 1) {
            Erm("Color");
            return std::string();
        }
        str = colors.at(0);
    }
    return str;
}

/////////////////////////////////////////////////////////////////////////////
// GasAnesthetic serialization

void GasAnesthetic::Serialize(std::iostream &stream, bool storing, uint32_t serializationFlags)
{
    if (storing) {
        stream << m_strSectionName << m_strName << m_strFmt;
        stream << m_wHighDEL;
        stream << m_fLambdaBG << m_fMAX_DEL << m_fDefDEL << m_fAmbient << m_fMAC
               << m_wVolatility << m_fDefUnitDose << m_fBottleCost << static_cast<uint16_t>(m_nBottleSize);
        for (int i = 0; i < MAX_COMPART; ++i)
            stream << m_fSolubility[i];
    } else {
        uint16_t w;
        stream >> m_strSectionName;
        if ((serializationFlags & GasDoc::SER_LOCAL_AGENT) != 0)
            stream >> m_strName;
        else
            m_strName = m_strSectionName;
        stream >> m_strFmt;
        stream >> m_wHighDEL;
        stream >> m_fLambdaBG >> m_fMAX_DEL >> m_fDefDEL >> m_fAmbient >> m_fMAC
               >> m_wVolatility >> m_fDefUnitDose >> m_fBottleCost >> w;
        m_nBottleSize = w;
        for (int i = 0; i < MAX_COMPART; ++i)
            stream >> m_fSolubility[i];
        m_bIsLiquid = m_fDefUnitDose != 0.0F;
        m_defaultColor = DefaultColorFromProfile(m_strName);
    }
}



/////////////////////////////////////////////////////////////////////////////
// AnesArray

// Overloaded operator to treat string as subscript

int GasAnesArray::Lookup(const std::string &strName)
{
    std::vector<GasAnesthetic*>::size_type i = 0;
    for (i = 0; i < size() && at(i)->m_strName < strName; i++);
    if (i >= size() || at(i)->m_strName > strName) return -1;
    return i;
}

int GasAnesArray::LookupSection(const std::string &strName)
{
    std::vector<GasAnesthetic*>::size_type i;
    for (i = 0; i < size() && at(i)->m_strSectionName != strName; i++);
    return i < size() ? i : -1;
}

void GasAnesArray::Insert(GasAnesthetic *pAnes)
{
	auto it = begin();
	while (it != end() && (*it)->m_strName < pAnes->m_strName) {
		++it;
	}
	insert(it, pAnes);
}

// Reads in the parameters for each anesthetic and returns count of same.
// MAY THROW MEMORY EXCEPTION!!

int GasAnesArray::ReadProfile() {
    int nAnesCount = 0;

    clear();
    ProfileValue value = gasApp->ReadProfile("Agents", "Agents", std::vector<std::string>());
    std::vector<std::string> strList = std::get<std::vector<std::string>>(value);

    for (auto& str : strList) {
        trim(str); // Trim leading and trailing spaces
    }

    std::vector<GasAnesthetic*>::size_type i;
    for (i = 0; i < strList.size(); ++i) {
        GasAnesthetic *pAnes = new GasAnesthetic;
        pAnes->m_strSectionName = strList[i];
        if (pAnes->ReadProfile()) {
            Insert(pAnes);
            nAnesCount++;
        } else {
            delete pAnes;
        }
    }

    return nAnesCount;
}
