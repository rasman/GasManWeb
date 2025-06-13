#ifndef GASANES_H
#define GASANES_H

#include "framework.h"
#include "GasGlobal.h"
#include "GasPtrList.h"
#include <string>
#include <cctype>  
#include <algorithm> 

class GasAnesthetic
{
public:
	GasAnesthetic();
	virtual ~GasAnesthetic();

// Operations
public:
	GasAnesthetic& operator=( const GasAnesthetic& otherAnes );
	bool ReadProfile();	// Look up above in gas.ini
	void Serialize( std::iostream &stream, bool storing, uint32_t serializationFlags );
	inline float ToMlLiquid( const float litersGas ) const {
		return m_wVolatility > 0 ? 1000 * litersGas / m_wVolatility : litersGas;
	};

private:
	bool Erm( const std::string &strParam );
	std::string DefaultColorFromProfile( const std::string& name );

// Attributes
public:
	std::string m_strSectionName;		// INI uses this name
	std::string m_strName;				// User sees this name
	std::string	m_strFmt;				// Format string, like "%4.1f"
	std::string m_defaultColor;			// What the ini tells us
	uint16_t	m_wVolatility;			// Gas produced for liquid injection
	uint16_t	m_wHighDEL;				// Highest gauge setting for DEL
	uint16_t	m_wHighFGF;
	uint16_t	m_wHighVA;
	uint16_t	m_wHighCO;
	float m_fLambdaBG;				// Blood/Gas solubility
	float m_fMAX_DEL;				// Highest allowed DEL setting
	float m_fDefDEL;				// Default vaporizer setting
	float m_fAmbient;				// float-ing around
	float m_fMAC;					// Minimum Alveolar Concentration (dose)
	float m_fDefUnitDose;			// Default liquid injected
	float m_fBottleCost;			// $ per bottle
	int	 m_nBottleSize;				// in ml
	bool m_bIsLiquid;				// Set if anesthetic is injectable/vaporized
	float m_fSolubility[MAX_COMPART];	// Lambda by compartment	
};

/////////////////////////////////////////////////////////////////////////////
//An Array of Anesthetic parameters

class GasAnesArray : public GasPtrList<GasAnesthetic>
{
public:
	void Insert( GasAnesthetic *pAnes );			// Put in sorted order
	int Lookup( const std::string &strName );			// Do linear search for localized name
	int LookupSection( const std::string &strName );	// Do linear search for section (canonical) name
	int ReadProfile();								// Read gas.ini
};
bool ichar_equals(char a, char b);
bool iequals(const std::string& a, const std::string& b);

#endif
