#include "pch.h"
#include <algorithm>
#include <limits>
#include "GasDoc.h"
#include <cmath>
#include <sstream>
#include "GasApplication.h"
#include "importLibraries/json.hpp"
#include "StringUtils.h"
#include <cctype>

float GasDoc::m_fDfltVA;
float GasDoc::m_fDfltCO;
float GasDoc::m_fDfltWeight;
int GasDoc::m_nBeep;
int GasDoc::m_nDfltSpeed;
float GasDoc::m_fDfltFGF;
std::string GasDoc::m_szDfltCircuit;
COMP_ARRAY GasDoc::m_fDfltVolume;
COMP_ARRAY GasDoc::m_fDfltRatio;

// STATS
int gTotVernier[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int GasDoc::safeStringToInt(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (const std::invalid_argument&) {
        return 0; // Return 0 if conversion fails
    } catch (const std::out_of_range&) {
        return 0; // Return 0 if the value is out of range
    }
}

/////////////////////////////////////////////////////////////////////////////
// GasDoc construction/destruction
GasDoc::GasDoc()
    : m_bOpened(false), m_bClosed(false), description(""), isUntitled(false)
{
    m_gasArray.push_back(new Gas(this));
    m_dwAvgSched = 0;
    started = false;
    m_SerializationFlags = SER_NONE;
    save_nScaleMinutes = static_cast<uint16_t>(15);

    op.clear();
    si.clear();
}

bool GasDoc::newDocument()
{
    static int sequenceNumber = 1;
    isUntitled = true;
    m_curFile = "Untitled" + std::to_string(sequenceNumber++) + ".gas";
    DeleteContents();
    InitDocument();
    if (m_gasArray.at(0)->m_nAgent == -1)
    { // Couldn't read in any valid agents
        return false;
    }
    return true;
}

// Remove any contents and any indications that there ARE contents
void GasDoc::DeleteContents()
{
    colors.clear();
    m_nActiveGas = 0;

    // Lose any 2nd gasses
    for (size_t i = 1; i < m_gasArray.size();)
        delete m_gasArray[i];

    m_gasArray.clear();
    m_gasArray.push_back(new Gas(this));

    // Lose primary results
    m_gasArray.at(0)->DeleteContents();

    // Lose any old anesthetics
    m_anesArray.clear();

    // Set everything to 'just starting and empty'
    m_theState = INITIAL_STATE;
    m_dwNextDisplayTime = 0;
    m_dwTime = 0;
    m_dwHighTime = 0;
    m_dwCalcTime = 0;
    m_dwCalcStart = 0;
    m_dwOldStart = 0;
    m_dwOldNext = 0;
    m_nLastSampAt = -1;
    m_nBookmarks = 0;
    for (int i = 0; i < MAX_COMPART; ++i)
        m_targetLists[i].clear();
    save_nActiveGas = NGS + 100;
}

void GasDoc::ReadProfile()
{
    m_nBeep = safeStringToInt(std::get<std::string>(gasApp->ReadProfile("Defaults", "Click", "0")));

    std::string str = std::get<std::string>(gasApp->ReadProfile("Defaults", "Speed", ""));
    if (toLowerCase(str) != "afap")
        m_nDfltSpeed = AFAP;
    else{
		std::cout << str << std::endl;
		m_nDfltSpeed = safeStringToInt(str);
	}

    switch (m_nDfltSpeed)
    {
    case 1: case 2: case 5: case 10: case 20: case 60: case 120: case AFAP:
        break;
    default:
        m_nDfltSpeed = 20; break;
    }

    str = std::get<std::string>(gasApp->ReadProfile("Defaults", "Circuit", "Open"));
    if (str[0] == 's' || str[0] == 'S')
        m_szDfltCircuit = "Semi-closed";
    else if (str[0] == 'i' || str[0] == 'I')
        m_szDfltCircuit = "Ideal";
    else if (str[0] == 'c' || str[0] == 'C')
        m_szDfltCircuit = "Closed";
    else
        m_szDfltCircuit = "Open";

    m_fDfltFGF = std::stof(std::get<std::string>(gasApp->ReadProfile("Defaults", "FGF", "-1.0")));
    if (m_fDfltFGF < 0.0F || m_fDfltFGF > MAX_FGF) m_fDfltFGF = INIT_SEMI_FGF;

    m_fDfltVA = std::stof(std::get<std::string>(gasApp->ReadProfile("Defaults", "VA", "-1.0")));
    if (m_fDfltVA < 0.0F || m_fDfltVA > MAX_VA) m_fDfltVA = STD_VA;

    m_fDfltCO = std::stof(std::get<std::string>(gasApp->ReadProfile("Defaults", "CO", "-1.0")));
    if (m_fDfltCO < 0.0F || m_fDfltCO > MAX_CO) m_fDfltCO = STD_CO;

    m_fDfltWeight = std::stof(std::get<std::string>(gasApp->ReadProfile("Defaults", "Weight", "0.0")));
    if (m_fDfltWeight < MIN_WEIGHT || m_fDfltWeight > MAX_WEIGHT) m_fDfltWeight = STD_WEIGHT;

    // Read in the compartment volumes
    m_fDfltVolume[CKT] = std::stof(std::get<std::string>(gasApp->ReadProfile("Volumes", "CKT", "0.0")));
    if (m_fDfltVolume[CKT] <= 0.0 || m_fDfltVolume[CKT] > 100.0) m_fDfltVolume[CKT] = 8.0F;

    m_fDfltVolume[ALV] = std::stof(std::get<std::string>(gasApp->ReadProfile("Volumes", "ALV", "0.0")));
    if (m_fDfltVolume[ALV] <= 0.0 || m_fDfltVolume[ALV] > 100.0) m_fDfltVolume[ALV] = 2.5F;

    m_fDfltVolume[VRG] = std::stof(std::get<std::string>(gasApp->ReadProfile("Volumes", "VRG", "0.0")));
    if (m_fDfltVolume[VRG] <= 0.0 || m_fDfltVolume[VRG] > 100.0) m_fDfltVolume[VRG] = 6.0F;

    m_fDfltVolume[MUS] = std::stof(std::get<std::string>(gasApp->ReadProfile("Volumes", "MUS", "0.0")));
    if (m_fDfltVolume[MUS] <= 0.0 || m_fDfltVolume[MUS] > 1000.0) m_fDfltVolume[MUS] = 33.0F;

    m_fDfltVolume[FAT] = std::stof(std::get<std::string>(gasApp->ReadProfile("Volumes", "FAT", "0.0")));
    if (m_fDfltVolume[FAT] <= 0.0 || m_fDfltVolume[FAT] > 100.0) m_fDfltVolume[FAT] = 14.5F;

    m_fDfltVolume[VEN] = std::stof(std::get<std::string>(gasApp->ReadProfile("Volumes", "VEN", "0.0")));
    if (m_fDfltVolume[VEN] <= 0.0 || m_fDfltVolume[VEN] > 10.0) m_fDfltVolume[VEN] = 1.0F;

    // Read in the CO ratio
    float fSum = 0.0F;
    m_fDfltRatio[VRG] = std::stof(std::get<std::string>(gasApp->ReadProfile("Ratio", "VRG", "-1.0")));
    if (m_fDfltRatio[VRG] >= 0.0)
        fSum += m_fDfltRatio[VRG];

    m_fDfltRatio[MUS] = std::stof(std::get<std::string>(gasApp->ReadProfile("Ratio", "MUS", "-1.0")));
    if (m_fDfltRatio[MUS] >= 0.0)
        fSum += m_fDfltRatio[MUS];

    m_fDfltRatio[FAT] = std::stof(std::get<std::string>(gasApp->ReadProfile("Ratio", "FAT", "-1.0")));
    if (m_fDfltRatio[FAT] >= 0.0)
        fSum += m_fDfltRatio[FAT];

    if (fSum < 1.001 && fSum > 0.999F) {
        m_fDfltRatio[VRG] = std::max(m_fDfltRatio[VRG], 0.0F);
        m_fDfltRatio[MUS] = std::max(m_fDfltRatio[MUS], 0.0F);
        m_fDfltRatio[FAT] = std::max(m_fDfltRatio[FAT], 0.0F);
    }
    else if (fSum == 100.0) {
        if (m_fDfltRatio[VRG] < 0.0F) m_fDfltRatio[VRG] = 0.0F; else m_fDfltRatio[VRG] /= 100.0F;
        if (m_fDfltRatio[MUS] < 0.0F) m_fDfltRatio[MUS] = 0.0F; else m_fDfltRatio[MUS] /= 100.0F;
        if (m_fDfltRatio[FAT] < 0.0F) m_fDfltRatio[FAT] = 0.0F; else m_fDfltRatio[FAT] /= 100.0F;
    }
    else {
        m_fDfltRatio[VRG] = 0.76F;
        m_fDfltRatio[MUS] = 0.18F;
        m_fDfltRatio[FAT] = 0.06F;
    }
    m_fDfltRatio[CKT] = m_fDfltRatio[ALV] = m_fDfltRatio[VEN] = 1.0F;
}



// Set default parameters for the document.  Should DeleteContents() first
void GasDoc::InitDocument() {
    try {
        // Ensure gasApp is initialized
        if (!gasApp) {
            std::cerr << "gasApp is not initialized." << std::endl;
            return;
        }

        // Ensure m_anesArray is initialized and not empty
        m_anesArray.ReadProfile(); // Get anesthetic parameters

        if (m_anesArray.size() > 0) { // There are anesthetics in the INI file
            std::string str = std::get<std::string>(gasApp->ReadProfile("Defaults", "Agent", ""));
            auto i = m_anesArray.LookupSection(str);
            if (i < 0 || static_cast<size_t>(i) >= m_anesArray.size()) {
                std::cerr << "Invalid index returned by LookupSection: " << i << std::endl;
                return;
            }

            i = std::max(i, 0);
            if (m_gasArray.size() <= 0 || !m_gasArray.at(0)) {
                std::cerr << "m_gasArray is empty or not initialized." << std::endl;
                return;
            }

            m_gasArray.at(0)->SetAgent(m_anesArray[i]->m_strName);
            m_gasArray.at(0)->SetDEL(m_anesArray[i]->m_fDefDEL);
            m_gasArray.at(0)->SetInitialResults();
            AddDefaultAgentColor(m_anesArray[i]->m_strName);
        } else { // No valid anesthetics!!
            std::cerr << "No valid anesthetics found in the INI file." << std::endl;
            return; // Catch this later
        }

        SetTimeVariables(DT);
        m_fWeight = m_fDfltWeight;
        double factor = static_cast<double>(m_fWeight) / STD_WEIGHT;
        factor = sqrt(sqrt(factor * factor * factor));
        m_bRtnEnb = m_bUptEnb = true;
        m_bFlush = false;
        m_bVapEnb = false;
        m_nSpeed = m_nDfltSpeed;

        m_strCircuit = m_szDfltCircuit;
        if (m_strCircuit[0] == 'S' || m_strCircuit[0] == 'I') {
            m_fFGF = m_fDfltFGF;
        } else if (m_strCircuit[0] == 'C') {
            m_fFGF = STD_FGF * factor;
        } else {
            m_fFGF = 10.0;
        }

        m_fVA = m_fDfltVA * factor;
        m_fCO = m_fDfltCO * factor;

        // Read in the compartment volumes and ratios
        for (int i = 0; i < MAX_COMPART; ++i) {
            m_fVolume[i] = m_fDfltVolume[i];
            m_fRatio[i] = m_fDfltRatio[i];
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception in InitDocument: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in InitDocument." << std::endl;
    }
}

void GasDoc::SetTimeVariables(uint16_t dt)
{
    m_cMSec_dx = dt; // Set time variables
    m_fdt = m_cMSec_dx / 60000.0F;
    m_dwSampTime = (uint32_t)SAMP_EVERY * dt;

    uint32_t nBreathsPerHour = (60 * 60 * 1000 + dt - 1) / dt;
    uint32_t nSamplesPerHour = nBreathsPerHour / SAMP_EVERY + MAX_ADJ_PER_HOUR;
    uint32_t nMaxHours = MAX_SAMP / nSamplesPerHour;
    if (nMaxHours > MAX_HOURS)
        nMaxHours = MAX_HOURS;
    m_dwMaxTime = nMaxHours * 3600L * 1000L;
}

bool GasDoc::duplicateAgents()
{
    std::vector<std::string> agents;
    for (size_t i = 0; i < m_gasArray.size(); ++i)
    {
        agents.push_back(m_gasArray[i]->m_strAgent);
    }
    std::sort(agents.begin(), agents.end());
    auto last = std::unique(agents.begin(), agents.end());
    return (last != agents.end());
}

// Reset the state from some sample
void GasDoc::ResetState(const GasSample& samp)
{
    m_theState = INITIAL_STATE;
    m_dwNextDisplayTime = m_dwTime = 0;

    // Set the circuit type
    switch (samp.m_uCktType)
    {
    case OPEN_CKT:
        m_strCircuit = "Open"; break;
    case SEMI_CKT:
        m_strCircuit = "Semi-closed"; break;
    case CLOSED_CKT:
        m_strCircuit = "Closed"; break;
    case IDEAL_CKT:
        m_strCircuit = "Ideal"; break;
    default:
        assert(false);
    }

    // Set the state variables
    m_fWeight = samp.m_fWeight;
    m_bRtnEnb = samp.m_bRtnEnb;
    m_bUptEnb = samp.m_bUptEnb;
    m_bVapEnb = samp.m_bVapEnb;
    m_bFlush = samp.m_bFlush;
    m_fFGF = samp.m_fFGF;
    m_fVA = samp.m_fVA;
    m_fCO = samp.m_fCO;
}

// Compute terms invariant with CO, weight and selected anesthetic

void GasDoc::ComputeTerms(float /*fFGF*/, float /*fVA*/, float fCO, float fWeight, int nAgent,
                          COMP_ARRAY &fEffectiveFlowTo, COMP_ARRAY (&fExp)[MAX_VERNIER])
{
    COMP_ARRAY &pf = m_anesArray[nAgent]->m_fSolubility;
    float fWtFactor = fWeight / STD_WEIGHT;
    float fLambdaBG = m_anesArray[nAgent]->m_fLambdaBG;
    float efTot = fCO * fLambdaBG;
    fEffectiveFlowTo[VRG] = efTot * m_fRatio[VRG];
    fEffectiveFlowTo[MUS] = efTot * m_fRatio[MUS];
    fEffectiveFlowTo[FAT] = efTot * m_fRatio[FAT];

    for (int i = 0; i < VEN; ++i) {
        double d = m_fdt / ((double)m_fVolume[i] * pf[i]);

        switch (i)
        {
        case CKT:
            // (times (FGF + VA), computed each time...)
            break;
        case ALV:
            // (times (VA + CO*lambda), computed each time...)
            d /= fWtFactor;
            break;
        default:
            d *= fEffectiveFlowTo[i] / fWtFactor;
        }
        d = 0 - d;
        for (int j = 0; j < MAX_VERNIER; ++j) {
            if (i == CKT || i == ALV)
                fExp[j][i] = float(d); // (exp computed each time...)
            else
                fExp[j][i] = float(exp(d));
            d /= 2;
        }
    }
}

void GasDoc::ChangeState(void *that, const AState theState)
{
    m_theState = theState;

    m_nTempmarks = 0;            // Delete all temporary bookmarks when state changes
    m_dwNextDisplayTime = 0L;    // No last time requested
}

void GasDoc::rewind()
{
    if (m_theState == RUNNING_STATE)
    {
        ChangeState(nullptr, STOPPED_STATE);
    }

    ResetState(*m_gasArray.at(0)->m_pSampArray->at(0));
    for (size_t ng = 0; ng < m_gasArray.size(); ++ng)
        m_gasArray.at(ng)->LoadFirstSampleState();
}

// The following routine is slightly mis-named.  It is called whenever
// any conditions (settings) of the simulation are changed to chop off
// any subsequent calculations based on the current settings.  However,
// its primary responsibility is to insure that any further computation
// will cause a new sample to be taken, and may be called even if there
// ARE no future calculations.  (see sample.h)

void GasDoc::Truncate()
{
    GasSampArray& sampArray = *m_gasArray.at(0)->m_pSampArray;
    int nNextSamp, ng;
    const int ngs = m_gasArray.size();

    // Clear out any flushing / injections pending
    for (ng = 0; ng < ngs; ++ng)
    {
        m_gasArray.at(ng)->m_nInjections = 0;
    }
    m_bFlush = false;

    if (m_dwCalcTime == 0)
    { // if there are no calculations yet...
        assert(m_gasArray.at(0)->m_pCurSamp == nullptr);
        assert(sampArray.empty());
        return;
    }

    // We are changing conditions at the current time. Invalidate any
    // subsequent samples and calculations.
    if (m_dwCalcTime > m_dwTime)
    { // If we've calculated for time beyond m_dwTime
        int i;

        // Set calc time to the tick preceding or at m_dwTime.
        const uint32_t dwNewCalcTime = (m_dwTime / m_cMSec_dx) * m_cMSec_dx;

        // If the new time isn't in the current buffer, then prime the buffer from history
        if (dwNewCalcTime < m_dwCalcStart)
        {
            for (ng = 0; ng < ngs; ++ng)
            {
                RES_ARRAY& res = ResultsAt(dwNewCalcTime, ng);
                float* pf1 = *(m_gasArray.at(ng)->m_pfCurResults + static_cast<int>(dwNewCalcTime / m_cMSec_dx) % SAMP_EVERY);
                float* pf2 = res;
                for (i = 0; i < MAX_RESULT; ++i)
                    *pf1++ = *pf2++;
            }
            m_dwCalcStart = dwNewCalcTime; // new earliest calculation
        }

        // Point to the sample for the new calc time. Discard if it needs to be re-taken
        int nCurSamp = SampAt(dwNewCalcTime);
        if (dwNewCalcTime == sampArray[nCurSamp]->m_dwBaseTime)
        { // don't keep if coincidental with current time
            nNextSamp = nCurSamp;
            m_nLastSampAt = -1; // invalidate cache
        }
        else
        { // keep the earlier sample
            nNextSamp = nCurSamp + 1;
            m_dwLastSampAtEnd = dwNewCalcTime;
        }

        // Delete any samples beyond the one for the new calc time
        int nMaxSamp = sampArray.size();
        if (nMaxSamp > nNextSamp)
        {
            for (ng = 0; ng < ngs; ++ng)
            {
                Gas* g = m_gasArray.at(ng);
                GasSampArray& sampArrayDel = *g->m_pSampArray;

                for (i = nNextSamp; i < nMaxSamp; ++i)
                    delete sampArrayDel[i];
                for (int index = nNextSamp; index < nMaxSamp; ++index)
                    sampArrayDel.erase(sampArrayDel.begin() + nNextSamp);

                // If we threw any away, we must've thrown out the last
                g->m_pCurSamp = nullptr;
            }
        }

        // Make sure the secondary buffer contains no future data
        if (m_dwOldNext > dwNewCalcTime)
        {
            m_dwOldNext = dwNewCalcTime + m_cMSec_dx;
            // Just for consistency...
            m_dwOldStart = std::min(m_dwOldStart, m_dwOldNext);
        }

        m_dwCalcTime = dwNewCalcTime; // reset last calculation
        m_dwHighTime = m_dwTime; // new logical 'end'
    }

    for (ng = 0; ng < ngs; ++ng)
        if (m_gasArray.at(ng)->m_pCurSamp != nullptr)
        {
            assert(m_gasArray.at(ng)->m_pCurSamp->m_dwBaseTime < m_dwCalcTime); // no nulls
            m_gasArray.at(ng)->m_pCurSamp = nullptr; // force taking of new sample
        }
}

// Return the index into m_nSampArray for the sample just before dwTime
int GasDoc::SampAt(uint32_t dwTime)
{
    if (m_nLastSampAt >= 0 && dwTime < m_dwLastSampAtEnd && dwTime >= m_dwLastSampAtTime)
        return m_nLastSampAt; // optimize

    GasSampArray& sampArray = *(m_gasArray.at(0)->m_pSampArray);
    int nMaxSamp = sampArray.size();
    assert(nMaxSamp != 0);

    // Special case: At current calc time, we may not yet have
    // taken a sample, so just return the latest...
    if (dwTime >= m_dwCalcTime)
        return nMaxSamp - 1;

    // Get the sample at the beginning of the interval containing dwTime
    short nCurSamp = m_nSampIdx[(short)(dwTime / m_cMSec_dx / SAMP_EVERY)];
    assert(nCurSamp >= 0 && nCurSamp < nMaxSamp);
    assert(sampArray[nCurSamp]->m_dwBaseTime <= dwTime);

    // Search forward until we find one after dwTime
    while (nCurSamp < nMaxSamp && sampArray[nCurSamp]->m_dwBaseTime <= dwTime)
        nCurSamp++;

    // Remember for next time
    m_dwLastSampAtEnd = nCurSamp < nMaxSamp ? sampArray[nCurSamp]->m_dwBaseTime : m_dwCalcTime;
    m_dwLastSampAtTime = sampArray[--nCurSamp]->m_dwBaseTime;
    m_nLastSampAt = nCurSamp;
    return nCurSamp;
}


// Return a reference to the RES_ARRAY for time dwTime.  This may require
// going back into history and recomputing forward from some sample.
RES_ARRAY& GasDoc::ResultsAt(uint32_t dwTime, int nGas)
{
    int i;

    // We don't prognosticate
    assert(dwTime / m_cMSec_dx <= m_dwCalcTime / m_cMSec_dx);

    // Is it in the current buffer?
    if (dwTime >= m_dwCalcStart)
    { // oh goody
        i = static_cast<int>((dwTime / m_cMSec_dx) % SAMP_EVERY);
        return m_gasArray.at(nGas)->m_pfCurResults[i];
    }

    // How about the last buffer?
    if (dwTime >= m_dwOldStart && dwTime < m_dwOldNext)
    {
        i = static_cast<int>((dwTime / m_cMSec_dx) % SAMP_EVERY);
        return m_gasArray.at(nGas)->m_pfOldResults[i];
    }

    // Damn. Must recalc forward from the samples.
    return ResultsFromHistoryAt(dwTime, nGas);
}

RES_ARRAY& GasDoc::ResultsFromHistoryAt(uint32_t dwTime, int nGas)
{
    uint32_t dwt, dwNext, dwBaseTime;
    int ngs = m_gasArray.size();
    assert(ngs <= NGS);
    GasSample* samps[NGS];
    RES_ARRAY fResults[NGS];
    COMP_ARRAY fExp[NGS][MAX_VERNIER], (*pfExp[NGS])[MAX_VERNIER];
    COMP_ARRAY fEffectiveFlowTo[NGS], *pfEFT[NGS];
    int njections[NGS];
    float fUptake[NGS], fTotUptake;
    float* pf;
    int i, nres, ng;

    auto nPrevSamp = static_cast<std::vector<GasSample*>::size_type>(SampAt(dwTime));
    dwBaseTime = m_gasArray.at(0)->m_pSampArray->at(nPrevSamp)->m_dwBaseTime;
    nres = static_cast<int>((dwBaseTime / m_cMSec_dx) % SAMP_EVERY);
    for (ng = 0; ng < ngs; ++ng)
    {
        Gas* g = m_gasArray.at(ng);
        GasSample& samp = *g->m_pSampArray->at(nPrevSamp);

        samps[ng] = &samp;
        njections[ng] = samp.m_uInjections;

        // See if we have computational factors already
        if (g->m_nTermAgent != static_cast<int>(samp.m_wAnesID) || g->m_fTermFGF != samp.m_fFGF
            || g->m_fTermVA != samp.m_fVA || g->m_fTermCO != samp.m_fCO || g->m_fTermWeight != samp.m_fWeight)
        { // Need to recalc other factors... double damn
            ComputeTerms(samp.m_fFGF, samp.m_fVA, samp.m_fCO, samp.m_fWeight, samp.m_wAnesID, fEffectiveFlowTo[ng], fExp[ng]);
            pfExp[ng] = fExp + ng;
            pfEFT[ng] = fEffectiveFlowTo + ng;
        }
        else
        { // At least we can point to the ones we've got
            pfExp[ng] = &g->m_fExp;
            pfEFT[ng] = &g->m_fEffectiveFlowTo;
        }
        // Copy the first result from the sample
        for (i = 0, pf = samp.m_fResults; i < MAX_RESULT; ++i)
            g->m_pfOldResults[nres][i] = fResults[ng][i] = pf[i];
    }

    // Stop at the next sample or the end of time
    if (nPrevSamp < (m_gasArray.at(0)->m_pSampArray->size() - 1))
        dwNext = m_gasArray.at(0)->m_pSampArray->at(nPrevSamp + 1)->m_dwBaseTime;
    else
    { // Should have been in the current buffer, but for back-tracking
        assert(dwTime + m_dwSampTime >= m_dwCalcTime);
        dwNext = std::min(dwBaseTime + m_dwSampTime, m_dwMaxTime);
    }

    // Loop up to the next sample time & recalc
    bool bFlush = samps[0]->m_bFlush;
    for (dwt = dwBaseTime + m_cMSec_dx; dwt < dwNext; dwt += m_cMSec_dx)
    { // Loop over the duration of the sample
        int nv, nVernier = 0;
    retry:
        for (nv = 0; nv < 1 << nVernier; ++nv)
        {
            fTotUptake = 0.0F;
            for (ng = 0; ng < ngs; ++ng)
            { // For each gas, calculate uptake and save total
                fUptake[ng] = CalcUptake(*samps[ng], fResults[ng], (*pfExp[ng])[nVernier], nVernier);
                fTotUptake += fUptake[ng];
            }
            for (ng = 0; ng < ngs; ++ng)
            { // For each gas, calculate results
                if (!Calc(*samps[ng], njections[ng], bFlush && (nv == 0), fResults[ng], *pfEFT[ng],
                          (*pfExp[ng])[nVernier], dwt, fTotUptake, fUptake[ng], nVernier))
                {
                    ++nVernier; // we need a finer integration
                    for (ng = 0; ng < ngs; ++ng)
                    { // For each gas, reset the results
                        for (i = 0, pf = m_gasArray.at(ng)->m_pfOldResults[nres]; i < MAX_RESULT; ++i)
                            fResults[ng][i] = pf[i];
                    }
                    goto retry;
                }
            }
        }
        // point into circular queue and store results
        nres = static_cast<int>((dwt / m_cMSec_dx) % SAMP_EVERY);
        for (ng = 0; ng < ngs; ++ng)
        { // For each gas, store results
            for (i = 0, pf = m_gasArray.at(ng)->m_pfOldResults[nres]; i < MAX_RESULT; ++i)
                pf[i] = fResults[ng][i];
            njections[ng] = 0;
        }
        bFlush = false;
    }

    // Save the new 'old' interval
    if (dwBaseTime <= m_dwOldStart && m_dwOldStart <= dwNext && dwNext <= m_dwOldNext)
    {
        m_dwOldStart = dwBaseTime;
        if (m_dwOldNext - m_dwOldStart > m_dwSampTime)
            m_dwOldNext = m_dwOldStart + m_dwSampTime;
    }
    else if (m_dwOldStart <= dwBaseTime && dwBaseTime <= m_dwOldNext && m_dwOldNext <= dwNext)
    {
        m_dwOldNext = dwNext;
        if (dwNext > m_dwSampTime)
        {
            m_dwOldStart = std::max(dwNext - m_dwSampTime, m_dwOldStart);
        }
    }
    else
    {
        m_dwOldStart = dwBaseTime;
        m_dwOldNext = dwNext;
    }

    // This is why we came
    i = static_cast<int>((dwTime / m_cMSec_dx) % SAMP_EVERY);
    return m_gasArray.at(nGas)->m_pfOldResults[i];
}
 
/////////////////////////////////////////////////////////////////////////////
// Routine to calculate a set of simulation results given a previous set. It
// will step forward dt/1<<nVernier seconds, so you should call this routine
// (and the CalcUptake routine) 1<<nVernier times to advance the simulation one
// breath. If any deltas exceed x% (see code) of the max delivered setting,
// Calc returns FALSE. You must then call Calc again after resetting the
// results and doubling the vernier.

// NOTE: Results are in percent litres anesthesia (La/L*100) EXCEPT!!!
// DEL(ivered) and UPT(ake), which are in litres (La).
/////////////////////////////////////////////////////////////////////////////

// FALSE if nVernier needs doubling
bool GasDoc::Calc(GasSample &samp, int nInjections, bool bFlush, RES_ARRAY &fResults,
                  COMP_ARRAY &fEffectiveFlowTo,
                  COMP_ARRAY &fExp,
                  uint32_t dwTime,
                  float fTotUptake, float fUptake, int nVernier)
{
    GasAnesthetic &anes = *m_anesArray[samp.m_wAnesID];
    bool bMyRet = true, bFixedCKT = false;
    COMP_ARRAY fTarget;
    float f, g, fVapor, fDEL, fVA, fFGF, fBloodFlow;
    float fExpCKT = 99.0F, fExpALV = 99.0F;
    int nDiv = 1 << nVernier;

    assert(nVernier < MAX_VERNIER);

    fDEL = samp.m_fDEL; // set given delivered pressure (La/L*100)
    fVA = samp.m_fVA;   // and alveolar ventilation (L/m)
    fFGF = samp.m_fFGF; // and fresh gas flow (L/m)

    // Take into account vaporization effect on FGF, if desired.
    if (samp.m_bVapEnb && anes.m_bIsLiquid)
    {
        if (fDEL < 99.0F)
            fFGF /= (100.0F - fDEL) / 100.0F; //(L/m)
        else
            fFGF *= 100.0F;
    }

    // Compute amount delivered due to vaporization
    fVapor = fDEL * fFGF * (m_fdt / nDiv) / 100.0F; //(La)

    // Add delivered due to injection
    if (nInjections)
    {
        assert(anes.m_bIsLiquid);
        f = static_cast<float>(anes.m_wVolatility) * samp.m_fUnitDose;
        f *= static_cast<float>(nInjections) / 1000.0F / nDiv; //(La)
        fVapor += f;                                           // increase delivered
        if (samp.m_bVapEnb)                                    // If desired,...
            fFGF += f / (m_fdt / nDiv);                        // ...correct the fresh gas flow
    }

    // Calculate effective delivered tension
    if (fFGF > 0.0)
        fDEL = 100.0 * fVapor / (fFGF * (m_fdt / nDiv));
    else
        fDEL = 0.0;

    // Compute effective FGF and VA
    fEffectiveFlowTo[CKT] = fFGF * anes.m_fSolubility[CKT]; //(L/m)
    fEffectiveFlowTo[ALV] = fVA * anes.m_fSolubility[ALV];  //(L/m)

    switch (samp.m_uCktType)
    {
    case OPEN_CKT:
        // In open circuit, delivered determines CKT pressure
        fResults[CKT] = fDEL; // (La/L*100)
        bFixedCKT = true;
        break;

    case IDEAL_CKT:
        // In an ideal circuit, new mixture displaces pure exhaled gas with
        // no mixing, so delivered, FGF, VA and ALV determine CKT pressure.
        if (fEffectiveFlowTo[CKT] < fEffectiveFlowTo[ALV])
        {
            f = fEffectiveFlowTo[CKT] / fEffectiveFlowTo[ALV];
            fResults[CKT] = f * fDEL + (1.0F - f) * fResults[ALV]; // (La/L*100)
        }
        else
            fResults[CKT] = fDEL; // (La/L*100)
        bFixedCKT = true;
        break;

    default:
        // Anything else, CKT pressure is not pre-determined. Compute a rate
        // of change, as for all other compartments. Start with ambient if
        // it was flushed.
        if (bFlush)
            fResults[CKT] = anes.m_fAmbient;

        // Compute target tension in breathing circuit (La/L*100)
        g = fEffectiveFlowTo[CKT] + fEffectiveFlowTo[ALV];

        if (g != 0.0F)
        {
            f = fEffectiveFlowTo[CKT] * fDEL + fEffectiveFlowTo[ALV] * fResults[ALV];

            // Correct for exhaled agent due to uptake effect.
            // (NOTE: La/L = 1 in gaseous state.)
            if (samp.m_bUptEnb && fTotUptake < 0.0F)
                // if desired and emerging, add venous effect on CKT
                f -= fTotUptake * fResults[ALV];

            fTarget[CKT] = f / g;
        }
        else
            fTarget[CKT] = fResults[CKT];

        break;
    }

    // Compute target tension in alveoli (La/L*100)
    fBloodFlow = anes.m_fLambdaBG * samp.m_fCO;
    g = fEffectiveFlowTo[ALV] + fBloodFlow;

    if (g != 0.0F) {
        f = fEffectiveFlowTo[ALV] * fResults[CKT] + fBloodFlow * fResults[VEN];

        // Correct for constant lung capacity. NOTE: L/La is 1.0
        // in gaseous state:
        if (samp.m_bUptEnb) {
            // if uptake enabled
            if (fTotUptake > 0.0F)
                // if inducing
                f += fResults[CKT] * fTotUptake;
            else
                // else emerging
                f += fResults[ALV] * fTotUptake;
        }

        fTarget[ALV] = f / g;
    } else {
        fTarget[ALV] = fResults[ALV];
    }

    // Compute target tensions for VRG, MUS, FAT (La/L*100)
    fTarget[VRG] = fResults[ALV];
    fTarget[MUS] = fResults[ALV];
    fTarget[FAT] = fResults[ALV];

    // TIME FOR RESULTS!

    // Add to total delivered and total uptake
    fResults[DEL] += fVapor;
    fResults[UPT] += fUptake * (m_fdt / nDiv); // (La)

    // Compute pressure change in CKT, ALV, VRG, MUS, FAT
    // (La/m*100) * (m/L) = (La/L*100) or percent atm.
    for (int compart = CKT; compart <= FAT; ++compart) {
        if (compart == CKT && bFixedCKT)
            // Already computed...
            continue;

        f = fExp[compart];
        switch (compart) {
        case CKT:
            f = fExpCKT = float(exp(f * (fEffectiveFlowTo[CKT] + fEffectiveFlowTo[ALV])));
            break;
        case ALV:
            f = fExpALV = float(exp(f * (fEffectiveFlowTo[ALV] + fBloodFlow)));
            break;
        }
        fResults[compart] = fTarget[compart] * (1 - f) + fResults[compart] * f;

        if (nVernier + 1 >= MAX_VERNIER)
            // No more vernier room...
            continue;

        if (!bMyRet)
            // Already flagged dubious results
            continue;

        if (fResults[compart] < 0.0F) { // Don't allow negative pressure
            bMyRet = false;
        }
    }

    if (nVernier + 1 < MAX_VERNIER) { // If there's more vernier room...
        // First compute number of breaths until we've
        // gone VERNIER_TICKS breaths past a setting change.
        int i = VERNIER_TICKS - int((dwTime - samp.m_dwBaseTime) / m_cMSec_dx);
        if (i > 0) { // If vernier hasn't already ticked away...
            if (fExpCKT < 0.1F || fExpALV < 0.1F)
                // If influence of current value of CKT or ALV is less than 10%...
                bMyRet = false;
        }
    }

	// Compute venous pressure if user wants it
	if ( samp.m_bRtnEnb )
		fResults[VEN] = fResults[VRG] * m_fRatio[VRG] + fResults[MUS] * m_fRatio[MUS]
		                + fResults[FAT] * m_fRatio[FAT];
	else
		fResults[VEN] = 0.0F;           

	return bMyRet;
}

//Compute uptake rate this breath

float GasDoc::CalcUptake(GasSample &samp, RES_ARRAY &fResults, COMP_ARRAY &fExp, int nVernier)
{
    COMP_ARRAY &pf = m_anesArray[samp.m_wAnesID]->m_fSolubility;
    float fWtFactor = samp.m_fWeight / STD_WEIGHT;
    float fDiff;
    double fUptake = 0.0;

    for (int i = VRG; i < VEN; ++i) {
        fDiff = fResults[ALV] - fResults[i];
        fUptake += m_fVolume[i] * pf[i] * fDiff * (1 - fExp[i]);
    }
    fUptake *= fWtFactor * (1 << nVernier) / m_fdt / 100.0F; // (La/m)
    if (!samp.m_bRtnEnb) {
        fDiff = fResults[VRG] * m_fRatio[VRG] + fResults[MUS] * m_fRatio[MUS]
                + fResults[FAT] * m_fRatio[FAT];
        fUptake += samp.m_fCO * m_anesArray[samp.m_wAnesID]->m_fLambdaBG * fDiff / 100.0F;
    }
    return (float)fUptake;
}


// This routine is called in spare time to either compute new results
// or dispatch results to the Views if we have gone far enough. It
// returns TRUE if successful, FALSE if neither is possible.
//
// Enter with dwCalcTime = time at last computation and dwNewTime
// set to the next time of interest.

bool GasDoc::ExtendResultSet()
{
    int ngs = m_gasArray.size();
    float fUptake[NGS], fTotUptake;
    int nTicks, nNext, i, ng, nv, nVernier = 0;
    long dwTimeDiff;
    div_t div_result;

    assert(ngs <= NGS);

    // Always remove from run queue when stopping!
    assert(m_theState == RUNNING_STATE); // comment out

    if (m_dwNextDisplayTime != 0L && m_dwNextDisplayTime <= m_dwCalcTime)
    { // We have calculated to a point that the user is interested in
        HandleInflectionPoint();
        return true;
    }

    // If settings are historical
    if (m_dwTime < m_dwHighTime || m_dwCalcTime > m_dwMaxTime) // or we're at the end of time
        return false; // ...we cannot do anything

    if (GetCurrentSpeed() != AFAP)
    { // If we are not going as fast as possible...
        // How far ahead of where we need to be are we?
        dwTimeDiff = (long)m_dwCalcTime;
        if (m_dwNextDisplayTime != 0L)
            dwTimeDiff -= (long)m_dwNextDisplayTime;
        else
            dwTimeDiff -= (long)m_dwTime;
        // a full buffer
        if (dwTimeDiff >= (long)m_dwSampTime || dwTimeDiff / GetSpeed() >= 5L * 60L * 1000L) // or 5 minutes
            return false; // ...is far enough
    }

    // Compute ticks since t0
    nTicks = (int)(m_dwCalcTime / m_cMSec_dx);

    // Compute samples since t0 and ticks left over
    div_result = div(nTicks, SAMP_EVERY);
    nTicks = div_result.rem;

    nNext = (nTicks + 1) % SAMP_EVERY; // point to next slot

    // if @ even SAMP_EVERY boundary, save a pointer
    if (!nTicks)
        m_nSampIdx[div_result.quot] = (short)m_gasArray.at(0)->m_pSampArray->size();

    for (ng = 0; ng < ngs; ++ng)
    {
        Gas* g = m_gasArray.at(ng);

        // Start a new sample block if necessary
        if (g->m_pCurSamp == nullptr || !nTicks)
        { // if we should start a new sample block
            g->m_pCurSamp = new GasSample(g, g->m_pfCurResults[nTicks]);
            assert(g->m_pCurSamp != nullptr);
            g->m_pSampArray->push_back(g->m_pCurSamp);
            if (g->m_pCurSamp->m_dwBaseTime == 0)
                g->SetInitialSamp(new GasSample(*g->m_pCurSamp));
            m_nLastSampAt = -1; // Invalidate cache
        }

        // Check the terms (Always!)
        if (g->m_nTermAgent != g->m_nAgent || g->m_fTermFGF != m_fFGF
            || g->m_fTermVA != m_fVA || g->m_fTermCO != m_fCO || g->m_fTermWeight != m_fWeight)
        {
            ComputeTerms(m_fFGF, m_fVA, m_fCO, m_fWeight, g->m_nAgent, g->m_fEffectiveFlowTo, g->m_fExp);
            g->m_nTermAgent = g->m_nAgent; // Remember where these came from
            g->m_fTermFGF = m_fFGF;
            g->m_fTermVA = m_fVA;
            g->m_fTermCO = m_fCO;
            g->m_fTermWeight = m_fWeight;
        }
    }

    // Step forward
retry:
    for (ng = 0; ng < ngs; ++ng)
    { // For each gas, copy the starting point to the next result slot
        Gas* g = m_gasArray.at(ng);

        for (i = 0; i < MAX_RESULT; ++i)
            g->m_pfCurResults[nNext][i] = g->m_pfCurResults[nTicks][i];
    }
    for (nv = 0; nv < 1 << nVernier; ++nv)
    {
        fTotUptake = 0.0F;
        for (ng = 0; ng < ngs; ++ng)
        { // For each gas, get uptake rate and add to total uptake
            Gas* g = m_gasArray.at(ng);
            fUptake[ng] = CalcUptake(*g->m_pCurSamp, g->m_pfCurResults[nNext], g->m_fExp[nVernier], nVernier);
            fTotUptake += fUptake[ng];
        }
        for (ng = 0; ng < ngs; ++ng)
        {
            Gas* g = m_gasArray.at(ng);

            if (!Calc(*g->m_pCurSamp, g->m_nInjections, m_bFlush && (nv == 0),
                      g->m_pfCurResults[nNext], g->m_fEffectiveFlowTo, g->m_fExp[nVernier], m_dwCalcTime,
                      fTotUptake, fUptake[ng], nVernier))
            {
                nVernier++; // we need a finer integration
                goto retry;
            }
        }
    }
    gTotVernier[nVernier] += 1;

    // Count any injection and flushing we just did
    for (ng = 0; ng < ngs; ++ng)
    {
        m_gasArray.at(ng)->m_nInjections = 0;
    }
    m_bFlush = false;

    // We did it!
    m_dwCalcTime += m_cMSec_dx;
    if (m_dwCalcStart + m_dwSampTime <= m_dwCalcTime)
        // if one fell off the end of the buffer...
        m_dwCalcStart = m_dwCalcTime + m_cMSec_dx - m_dwSampTime;

    return true;
}

// When we hit the new time set above, call the views to update them
void GasDoc::HandleInflectionPoint()
{
    // Make sure we've gotten that far
    assert(m_dwNextDisplayTime != 0L && m_dwNextDisplayTime < m_dwCalcTime + m_cMSec_dx);
    uint32_t dwPrev = m_dwTime;
    m_dwTime = m_dwNextDisplayTime;
    m_dwNextDisplayTime = 0L;

    // Process any intervening events when crossing breath boundaries
    if (dwPrev / m_cMSec_dx != m_dwTime / m_cMSec_dx)
    {
        GasSample& samp = *m_gasArray.at(0)->m_pSampArray->at(SampAt(m_dwTime));
        // weight change (ridiculous!)
        if (m_fWeight != samp.m_fWeight)
            SetWeight(nullptr, samp.m_fWeight);
        // circuit change
        switch (samp.m_uCktType)
        {
        case OPEN_CKT:
            if (m_strCircuit[0] != 'O')
                SetCircuit(nullptr, "Open");
            break;
        case SEMI_CKT:
            if (m_strCircuit[0] != 'S')
                SetCircuit(nullptr, "Semi-closed");
            break;
        case CLOSED_CKT:
            if (m_strCircuit[0] != 'C')
                SetCircuit(nullptr, "Closed");
            break;
        case IDEAL_CKT:
            if (m_strCircuit[0] != 'I')
                SetCircuit(nullptr, "Ideal");
            break;
        default:
            assert(false);
        }
        // FGF, VA, CO changes
        if (m_fFGF != samp.m_fFGF) SetFGF(nullptr, samp.m_fFGF);
        if (m_fVA != samp.m_fVA) SetVA(nullptr, samp.m_fVA);
        if (m_fCO != samp.m_fCO) SetCO(nullptr, samp.m_fCO);
        // Return enabled change
        m_bRtnEnb = samp.m_bRtnEnb;
        // Uptake enabled change
        m_bUptEnb = samp.m_bUptEnb;
        // Vaporization enabled change
        m_bVapEnb = samp.m_bVapEnb;

        for (size_t ng = 0; ng < m_gasArray.size(); ++ng)
        {
            Gas* g = m_gasArray.at(ng);
            GasSample& samp = *g->m_pSampArray->at(SampAt(m_dwTime));

            // DEL change
            if (g->m_fDEL != samp.m_fDEL) SetDEL(nullptr, ng, samp.m_fDEL);
            // agent change
            if (g->m_nAgent != static_cast<int>(samp.m_wAnesID))
                SetAgent(nullptr, ng, m_anesArray[samp.m_wAnesID]->m_strName);
            // unit dose change
            g->m_fUnitDose = samp.m_fUnitDose;

            // Store the results conveniently
            RES_ARRAY& fResults = ResultsAt(m_dwTime, ng);
            for (int i = 0; i < MAX_RESULT; ++i)
                g->m_fResults[i] = fResults[i];
        }
    }

    if (m_dwTime > m_dwHighTime)
    { // if we've broken new ground
        m_dwHighTime = m_dwTime; // Reset user's view of 'end'
    }

    // Change the state if at breakpoint or end of sim. Else,
    // see if there's room for more calcs.
    if (m_dwTime >= m_dwMaxTime)
        ChangeState(nullptr, END_STATE);
    else if (m_bBreak)
        ChangeState(nullptr, STOPPED_STATE);
}

// The following overloaded versions of GetXXX retrieve values
// for a particular time.

float GasDoc::GetFGF(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);

    if (dwt >= m_dwCalcTime)
    { // If at the current time
        if (m_bFlush) return 30.0F;
        GasAnesthetic& anes = *m_anesArray[m_gasArray.at(nGas)->m_nAgent];
        if (m_bVapEnb && anes.m_bIsLiquid)
            return m_fFGF * 100.0F / (100.0F - std::min(m_gasArray.at(nGas)->m_fDEL, 99.0F));
        else
            return m_fFGF;
    }
    // Else in history
    GasSample& samp = *(*m_gasArray.at(0)->m_pSampArray)[SampAt(dwt)];
    GasAnesthetic& anes = *m_anesArray[samp.m_wAnesID];
    if (samp.m_bFlush && samp.m_dwBaseTime + m_cMSec_dx > dwt) return 30.0F;
    if (samp.m_bVapEnb && anes.m_bIsLiquid)
        return samp.m_fFGF * 100.0F / (100.0F - std::min(samp.m_fDEL, 99.0F));
    else
        return samp.m_fFGF;
}

float GasDoc::GetDEL(float fMin, int nGas, bool* pbInjected, float fLastMin)
// ASSUMES anes.m_fSolubility[CKT] is 1!!
{
    uint32_t dwt = GetDwTime(fMin);
    float f;

    if (dwt >= m_dwCalcTime)
    { // If at the current time
        if (pbInjected != nullptr)
            *pbInjected = m_gasArray.at(nGas)->m_nInjections;
        if (m_bFlush) return 0.0F;
        if (dwt < m_cMSec_dx) return m_gasArray.at(nGas)->m_fDEL;
        uint32_t dwtPrev = dwt - m_cMSec_dx;
        GasSample& sampPrev = *(*m_gasArray.at(nGas)->m_pSampArray)[SampAt(dwtPrev)];
        if (sampPrev.m_fFGF == 0.0F) return m_gasArray.at(nGas)->m_fDEL;
        return (m_gasArray.at(nGas)->m_fResults[DEL] - ResultsAt(dwtPrev, nGas)[DEL])
               * 100.0F / GetFGF(dwtPrev / 60000.0F, nGas) / m_fdt;
    }
    // Else in history
    int nSamp = SampAt(dwt);
    if (pbInjected != nullptr)
    {
        *pbInjected = false;
        if (fLastMin >= 0.0F && fLastMin != fMin)
        {
            int n1, n2;
            uint32_t dwt1, dwt2;

            if (fLastMin < fMin)
            {
                dwt1 = GetDwTime(fLastMin);
                dwt2 = dwt;
                n1 = SampAt(dwt1);
                n2 = nSamp;
            }
            else if (fLastMin > fMin)
            {
                dwt1 = dwt;
                dwt2 = GetDwTime(fLastMin);
                n1 = nSamp;
                n2 = SampAt(dwt2);
            }
            else
			{
                n1 = 1; 
				n2 = 0; // (just to keep compiler happy; not executed)
			}
            for (int n = n1; !*pbInjected && n <= n2; n++)
            {
                GasSample& samp = *(*m_gasArray.at(nGas)->m_pSampArray)[n];
                *pbInjected = samp.m_uInjections != 0 && samp.m_dwBaseTime >= dwt1 && samp.m_dwBaseTime < dwt2;
            }
        }
    }
    GasSample& samp = *(*m_gasArray.at(nGas)->m_pSampArray)[nSamp];
    if (samp.m_bFlush && samp.m_dwBaseTime + m_cMSec_dx > dwt) return 0.0F;
    if (dwt < m_cMSec_dx) return samp.m_fDEL;
    uint32_t dwtPrev = dwt - m_cMSec_dx;
    GasSample& sampPrev = *(*m_gasArray.at(nGas)->m_pSampArray)[SampAt(dwtPrev)];
    if (sampPrev.m_fFGF == 0.0F) return samp.m_fDEL;
    f = ResultsAt(dwtPrev, nGas)[DEL];
    return (ResultsAt(dwt, nGas)[DEL] - f)
           * 100.0F / GetFGF(dwtPrev / 60000.0F, nGas) / m_fdt;
}

float GasDoc::GetVA(float fMin)
{
    uint32_t dwt = GetDwTime(fMin);

    if (dwt >= m_dwCalcTime)
    { // If at the current time
        if (dwt < m_cMSec_dx || !m_bUptEnb) return m_fVA;
        uint32_t dwtPrev = dwt - m_cMSec_dx;
        float fTot = 0.0F;
        for (std::vector<Gas*>::size_type i = 0; i < m_gasArray.size(); ++i)
        {
            float f = ResultsAt(dwtPrev, i)[UPT];
            fTot += (m_gasArray.at(i)->m_fResults[UPT] - f);
        }
        return fTot / m_fdt + m_fVA;
    }
    // Else in history
    GasSample& samp = *(*m_gasArray.at(0)->m_pSampArray)[SampAt(dwt)];
    if (dwt < m_cMSec_dx || !samp.m_bUptEnb) return samp.m_fVA;
    uint32_t dwtPrev = dwt - m_cMSec_dx;
    float fTot = 0.0F;
    for (std::vector<Gas*>::size_type i = 0; i < m_gasArray.size(); ++i)
    {
        float f = ResultsAt(dwtPrev, i)[UPT];
        fTot += (ResultsAt(dwt, i)[UPT] - f);
    }
    return fTot / m_fdt + samp.m_fVA;
}

float GasDoc::GetCO(float fMin)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime) return m_fCO;
    return (*m_gasArray.at(0)->m_pSampArray)[SampAt(dwt)]->m_fCO;
}

float GasDoc::GetCKT(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[CKT];
    return ResultsAt(dwt, nGas)[CKT];
}

float GasDoc::GetALV(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[ALV];
    return ResultsAt(dwt, nGas)[ALV];
}

float GasDoc::GetART(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[ALV];
    return ResultsAt(dwt, nGas)[ALV];
}

float GasDoc::GetVRG(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[VRG];
    return ResultsAt(dwt, nGas)[VRG];
}

float GasDoc::GetMUS(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[MUS];
    return ResultsAt(dwt, nGas)[MUS];
}

float GasDoc::GetFAT(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[FAT];
    return ResultsAt(dwt, nGas)[FAT];
}

float GasDoc::GetVEN(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[VEN];
    return ResultsAt(dwt, nGas)[VEN];
}

float GasDoc::GetCostRatio(int nGas)
{
    GasAnesthetic& anes = *m_anesArray[m_gasArray.at(nGas)->m_nAgent];
    float fBotVol = anes.m_nBottleSize * (float)anes.m_wVolatility;
    if (fBotVol == 0.0F) return 0.0F;
    return 1000.0F * anes.m_fBottleCost / fBotVol;
}

float GasDoc::GetUptake(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[UPT];
    else
        return ResultsAt(dwt, nGas)[UPT];
}

float GasDoc::GetUptakeCost(float fMin, int nGas)
{
    float f;
    float fRatio = GetCostRatio(nGas);
    if (fRatio == 0.0F) return 0.0F;

    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        f = m_gasArray.at(nGas)->m_fResults[UPT];
    else
        f = ResultsAt(dwt, nGas)[UPT];
    return f * fRatio;
}

float GasDoc::GetDelivered(float fMin, int nGas)
{
    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        return m_gasArray.at(nGas)->m_fResults[DEL];
    else
        return ResultsAt(dwt, nGas)[DEL];
}

float GasDoc::GetDeliveredCost(float fMin, int nGas)
{
    float f;
    float fRatio = GetCostRatio(nGas);
    if (fRatio == 0.0F) return 0.0F;

    uint32_t dwt = GetDwTime(fMin);
    if (dwt >= m_dwCalcTime)
        f = m_gasArray.at(nGas)->m_fResults[DEL];
    else
        f = ResultsAt(dwt, nGas)[DEL];
    return f * fRatio;
}

void GasDoc::SetAgent(void *that, int nGas, const std::string &agent)
{
    Gas *g = m_gasArray.at(nGas);
    g->SetAgent(agent);
}

void GasDoc::AddDefaultAgentColor(const std::string &agent)
{
    colors.push_back(getDefaultAgentColor(agent));
}

std::string GasDoc::getDefaultAgentColor(const std::string &agent)
{
    int i = m_anesArray.Lookup(agent);
    if (i >= 0)
    {
        std::string color = m_anesArray.at(i)->m_defaultColor;
        if (!color.empty())
            return color;
    }
    return gasApp->getNextAvailableColor(this);
}

std::string GasDoc::getDefaultAgentColor(const int nGas) const
{
    std::string color = m_anesArray.at(m_gasArray.at(nGas)->m_nAgent)->m_defaultColor;
    if (!color.empty())
        return color;
    return gasApp->getNextAvailableColor(this);
}

void GasDoc::SetCircuit(void *that, const std::string &circuit)
{
    m_strCircuit = circuit;
}

void GasDoc::SetWeight(void *that, const float &fWeight)
{
    m_fWeight = fWeight;
}

void GasDoc::SetVA(void *that, const float &fVA)
{
    m_fVA = fVA;
}

void GasDoc::SetCO(void *that, const float &fCO)
{
    m_fCO = fCO;
}

void GasDoc::SetDEL(void *that, int nGas, const float &fDEL)
{
    Gas *g = m_gasArray.at(nGas);
    g->SetDEL(fDEL);
}

void GasDoc::SetFGF(void *that, const float &fFGF)
{
    m_fFGF = fFGF;
}

void GasDoc::Inject()
{
    m_gasArray.at(m_nActiveGas)->Inject();
}

void GasDoc::Flush()
{
    m_bFlush = true;
    for (std::vector<Gas*>::size_type ng = 0; ng < m_gasArray.size(); ++ng)
    {
        m_gasArray.at(ng)->Flush();
    }
}

std::string GasDoc::dumpCSV(int startSecond, int endSecond, int everySeconds)
{
    using namespace std::literals;

    int maxSecond = static_cast<int>(GetHighTime());
    std::ostringstream oss;

    oss << "Time,Agent,FGF,VA,CO,CKT,ALV,ART,VRG,MUS,FAT,VEN,Uptake,Delivered\n"s;
    for (int s = std::min(startSecond, maxSecond); s < std::min(endSecond, maxSecond); s += everySeconds)
    {
        auto fMin = s / 60.0F;
        auto hh = static_cast<int>(s / 3600);
        auto mm = static_cast<int>((s - hh * 3600) / 60);
        auto ss = static_cast<int>(s - (hh * 60 + mm) * 60);
        auto time = std::to_string(hh) + ":" + std::to_string(mm) + ":" + std::to_string(ss);

        const auto ngs = m_gasArray.size();
        for (size_t ng = 0; ng < ngs; ng++)
        {
            oss << time << ","
                << m_gasArray[ng]->m_strAgent << ","
                << GetFGF(fMin, ng) << ","
                << GetVA(fMin) << ","
                << GetCO(fMin) << ","
                << GetCKT(fMin, ng) << ","
                << GetALV(fMin, ng) << ","
                << GetART(fMin, ng) << ","
                << GetVRG(fMin, ng) << ","
                << GetMUS(fMin, ng) << ","
                << GetFAT(fMin, ng) << ","
                << GetVEN(fMin, ng) << ","
                << GetUptake(fMin, ng) << ","
                << GetDelivered(fMin, ng) << "\n";
        }
    }

    return oss.str();
}

bool GasDoc::loadJsonFile(const char *jsonStr, int len)
{
    if (!newDocument()) {
        std::cerr << "Failed to create a new document." << std::endl;
        return false;
    }

    std::string jsonString(jsonStr, len);
    json domDoc = json::parse(jsonString, nullptr, false);

    if (domDoc.is_discarded()) {
        std::cerr << "Failed to parse JSON string." << std::endl;
        return false;
    }

    if (domDoc["tagName"] != "gasman") {
        std::cerr << "Invalid JSON tagName: " << domDoc["tagName"] << std::endl;
        return false;
    }

    if (domDoc.contains("description")) {
        SetDescription(domDoc["description"]);
    }

    bool gotParams = false, gotSettings = false;

    for (auto& elem : domDoc.items()) {
        std::string name = elem.key();
        if (name == "params") {
            json value = elem.value();
            gotParams = loadJsonParams(value);
        }
        else if (name == "settings") {
            json value = elem.value();
            gotSettings = gotParams ? loadJsonSettings(value) : false;
        }
    }

    if (gotParams && gotSettings) {
        rewind();
        return true;
    }

    std::cerr << "Failed to load JSON data." << std::endl;
    return false;
}

bool GasDoc::loadJsonParams(json& elem)
{
    for (auto& node : elem.items())
    {
        std::string name = node.key();
        if (name == "patient" && !loadJsonPatient(node.value())) {
            return false;
        }
        if (name == "agents" && !loadJsonAgents(node.value())) {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonPatient(json& elem)
{
    for (auto& node : elem.items())
    {
        std::string name = node.key();
        if (name == "weight" && !loadJsonPatientWeight(node.value())) {
            return false;
        }
        if (name == "volumes" && !loadJsonPatientVolumes(node.value())) {
            return false;
        }
        if (name == "flows" && !loadJsonPatientFlows(node.value())) {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonPatientWeight(json& elem)
{
    if (!elem.contains("unit") || !elem.contains("value") || elem["unit"] != "kilograms")
    {
        return false;
    }

    try
    {
        double weight = std::stod(elem["value"].get<std::string>());
        if (weight < MIN_WEIGHT || weight > MAX_WEIGHT)
        {
            return false;
        }

        SetWeight(nullptr, weight);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool GasDoc::loadJsonPatientVolumes(json& elem)
{
    return updateVolume(elem, "fat", FAT)
        && updateVolume(elem, "alv", ALV)
        && updateVolume(elem, "mus", MUS)
        && updateVolume(elem, "vrg", VRG)
        && updateVolume(elem, "ven", VEN);
}

bool GasDoc::updateVolume(json& elem, const std::string& name, AResult compartment)
{
    if (elem.contains(name))
    {
        try
        {
            double v = std::stod(elem[name].get<std::string>());
            m_fVolume[compartment] = STD_WEIGHT * v / m_fWeight;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonPatientFlows(json& elem)
{
    return updateFlow(elem, "fat", FAT)
        && updateFlow(elem, "mus", MUS)
        && updateFlow(elem, "vrg", VRG);
}

bool GasDoc::updateFlow(json& elem, const std::string& name, AResult compartment)
{
    if (elem.contains(name))
    {
        try
        {
            double flow = std::stod(elem[name].get<std::string>());
            m_fRatio[compartment] = flow;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonAgents(json& elem)
{
    m_gasArray.clear();
    colors.clear();
    for (auto& node : elem.items())
    {
        std::string name = node.key();
        if (name == "agent" && !loadJsonAgent(node.value()))
        {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonAgent(json& elem)
{
    if (!elem.contains("name"))
    {
        return false;
    }

    std::string agent = elem["name"];
    int nAgent = m_anesArray.Lookup(agent);
    if (nAgent < 0)
    {
        return false;
    }

    GasAnesthetic &anes = *m_anesArray[nAgent];
    if (!(
        updateFloat(elem, "lambdaBlood", anes.m_fLambdaBG)
        && updateSolubility(anes, elem, "lambdaVrg", VRG)
        && updateSolubility(anes, elem, "lambdaFat", FAT)
        && updateSolubility(anes, elem, "lambdaMus", MUS)
        && updateUint16(elem, "volatility", anes.m_wVolatility)
        && updateInt(elem, "bottleSize", anes.m_nBottleSize)
        && updateFloat(elem, "bottleCost", anes.m_fBottleCost)
        ))
    {
        return false;
    }

    Gas *gas = new Gas(this);
    gas->SetAgent(agent);
    gas->SetDEL(anes.m_fDefDEL);
    gas->SetInitialResults();

    m_gasArray.push_back(gas);
    AddDefaultAgentColor(agent);
    return true;
}

bool GasDoc::updateFloat(json& elem, const std::string& name, float& target)
{
    if (elem.contains(name))
    {
        try
        {
            target = std::stof(elem[name].get<std::string>());
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return true;
}

bool GasDoc::updateSolubility(GasAnesthetic& anes, json& elem, const std::string& name, AResult compartment)
{
    if (elem.contains(name))
    {
        try
        {
            anes.m_fLambdaBG = std::stof(elem[name].get<std::string>());
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return true;
}

bool GasDoc::updateUint16(json& elem, const std::string& name, uint16_t& target)
{
    if (elem.contains(name))
    {
        try
        {
            target = static_cast<uint16_t>(std::stoi(elem[name].get<std::string>()));
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return true;
}

bool GasDoc::updateInt(json& elem, const std::string& name, int& target)
{
    if (elem.contains(name))
    {
        try
        {
            target = std::stoi(elem[name].get<std::string>());
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return true;
}


bool GasDoc::loadJsonSettings(json& elem)
{
    for (auto& node : elem.items())
    {
        std::string name = node.key();
        if (name == "setting")
        {
            for (auto& setting : node.value())
            {
                if (!loadJsonSetting(setting)) {
                    return false;
                }
            }
        }
        if (name == "results" && !loadJsonResults(node.value())) {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonSetting(json& elem)
{

    if (!elem.contains("time")) {
        std::cerr << "Error: time element not found" << std::endl;
    }

    if (!loadJsonTime(elem) || !loadJsonCircuit(elem)) {
        std::cerr << "Failed to load time or circuit" << std::endl;
        return false;
    }

    try {
        if (elem.contains("va")) {
            float val = std::stof(elem["va"].get<std::string>());
            SetVA(nullptr, val);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to update va: " << e.what() << std::endl;
        return false;
    }

    try {
        if (elem.contains("fgf")) {
            float val = std::stof(elem["fgf"].get<std::string>());
            SetFGF(nullptr, val);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to update fgf: " << e.what() << std::endl;
        return false;
    }

    try {
        if (elem.contains("co")) {
            float val = std::stof(elem["co"].get<std::string>());
            SetCO(nullptr, val);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to update co: " << e.what() << std::endl;
        return false;
    }

    for (auto& node : elem.items())
    {
        std::string name = node.key();
        if (name == "agentsettings" && !loadJsonAgentSettings(node.value())) {
            std::cerr << "Failed to load agent settings" << std::endl;
            return false;
        }
    }
    return true;
}


bool GasDoc::loadJsonTime(json& elem)
{
    if (!elem.contains("time")) {
        return false;
    }

    std::string timeStr = elem["time"];
    std::vector<std::string> hms;
    std::istringstream iss(timeStr);
    std::string token;
    while (std::getline(iss, token, ':')) {
        hms.push_back(token);
    }

    if (hms.size() != 3) {
        return false;
    }

    uint32_t time = 0;
    try
    {
        int hours = safeStringToInt(hms[0]);
        int minutes = safeStringToInt(hms[1]);
        int seconds = safeStringToInt(hms[2]);

        time = hours * 3600 + minutes * 60 + seconds;
        time *= 1000;
    }
    catch (const std::exception&)
    {
        return false;
    }

    if (time < m_dwTime) {
        return false;
    }

    loadJsonRunTo(time);
    return true;
}

void GasDoc::loadJsonRunTo(uint32_t time)
{
    m_bSpeedup = true;
    const AState saveState = m_theState;
    m_theState = RUNNING_STATE;
    m_dwNextDisplayTime = 0L;
    while (m_dwTime < time)
    {
        if (!ExtendResultSet())
            throw std::runtime_error("Unable to progress through JSON");
        m_dwTime = m_dwHighTime = m_dwCalcTime;
    }
    m_theState = saveState;
}

bool GasDoc::loadJsonCircuit(json& elem)
{
    Truncate();
    if (!elem.contains("circuit"))
        return true;
    std::string circuit = elem["circuit"].get<std::string>();
    std::transform(circuit.begin(), circuit.end(), circuit.begin(), ::toupper);

    std::vector<std::string> szFlag = {
        "(UR)", "(R)", "(U)", "", "(UR)*", "(R)*", "(U)*", "*",
        "(VUR)", "(VR)", "(VU)", "(V)", "(VUR)*", "(VR)*", "(VU)*", "(V)*"
    };

    int taken;
    if (circuit.find("OPEN") == 0) {
        m_strCircuit = "Open";
        taken = 4;
    } else if (circuit.find("SEMI") == 0) {
        m_strCircuit = "Semi-closed";
        taken = 4;
    } else if (circuit.find("CLOSED") == 0) {
        m_strCircuit = "Closed";
        taken = 5;
    } else if (circuit.find("IDEAL") == 0) {
        m_strCircuit = "Ideal";
        taken = 5;
    } else {
        return false;
    }

    int flags = std::distance(szFlag.begin(), std::find(szFlag.begin(), szFlag.end(), circuit.substr(taken)));
    m_bVapEnb = (flags & 8) != 0;
    m_bFlush = (flags & 4) != 0;
    m_bRtnEnb = (flags & 2) != 0;
    m_bUptEnb = (flags & 1) != 0;

    if (m_bFlush)
        for (size_t ng = 0; ng < m_gasArray.size(); ++ng)
            m_gasArray.at(ng)->Flush();

    return true;
}

bool GasDoc::loadJsonAgentSettings(json& elem)
{
    for (auto& node : elem.items())
    {
        std::string name = node.key();
        if (name == "agentsetting" && !loadJsonAgentSetting(node.value())) {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonAgentSetting(json& elem)
{
    if (!elem.contains("agentname"))
        return false;

    std::string agent = elem["agentname"];
    size_t nGas;
    for (nGas = 0; nGas < m_gasArray.size() && agent != m_gasArray[nGas]->m_strAgent; nGas++);
    if (nGas >= m_gasArray.size())
        return false;

    Gas& gas = *m_gasArray.at(nGas);

    try {
        if (elem.contains("inject")) {
            float v = std::stof(elem["inject"].get<std::string>());
            gas.m_nInjections = std::round(v / gas.m_fUnitDose);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to update inject: " << e.what() << std::endl;
        return false;
    }

    try {
        if (elem.contains("del")) {
            float del = std::stof(elem["del"].get<std::string>());
            gas.m_fDEL = del;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to update del: " << e.what() << std::endl;
        return false;
    }

    return true;
}


bool GasDoc::loadJsonResults(json& elem)
{
    for (auto& node : elem.items())
    {
        std::string name = node.key();
        if (name == "result" && !loadJsonResult(node.value())) {
            return false;
        }
    }
    return true;
}

bool GasDoc::loadJsonResult(json& elem)
{
    return loadJsonTime(elem);
}

EventList GasDoc::SampComp(GasSample &s1, GasSample &s2)
{
    EventList events = 0;

    if (s1.m_fDEL != s2.m_fDEL) events |= UPD_DEL;
    if (s1.m_fCO != s2.m_fCO) events |= UPD_CO;
    if (s1.m_fFGF != s2.m_fFGF) events |= UPD_FGF;
    if (s1.m_fVA != s2.m_fVA) events |= UPD_VA;
    if (s1.m_fWeight != s2.m_fWeight) events |= UPD_WEIGHT;
    if (s1.m_wAnesID != s2.m_wAnesID) events |= UPD_AGENT;
    if (s2.m_uInjections || s2.m_bFlush) events |= UPD_INJECT;
    if (s1.m_bRtnEnb != s2.m_bRtnEnb || s1.m_bUptEnb != s2.m_bUptEnb || s1.m_bVapEnb != s2.m_bVapEnb || s1.m_uCktType != s2.m_uCktType)
        events |= UPD_CIRCUIT;
    return events;
}

json GasDoc::createJsonElement(const std::string& name, const GasAttributeMap& attributes, const std::vector<json>& children)
{
    json ret;
    ret["name"] = name;

    for (const auto& attribute : attributes)
    {
        const auto& key = attribute.first;
        const auto& value = attribute.second;
        
        ret[key] = value; // Directly assign value since it is a string
    }

    for (const auto& child : children)
        ret["children"].push_back(child);

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////
void GasDoc::setColorToDefault(const int nGas)
{
    colors[nGas] = getDefaultAgentColor(nGas);
}

void GasDoc::defaultAgentColors()
{
    for (size_t i = 0; i < m_gasArray.size(); ++i)
        colors[i] = getDefaultAgentColor(i);
}

void GasDoc::changeColor(int nGas, const std::string& color)
{
    colors[nGas] = color;
}

void GasDoc::colorSwap(int first, int second)
{
    std::swap(colors[first], colors[second]);
}
