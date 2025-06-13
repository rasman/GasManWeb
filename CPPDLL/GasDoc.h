#ifndef GASDOC_H
#define GASDOC_H
#include "framework.h"
#include <string>
#include <vector>
#include "Gas.h"
#include "GasAnes.h"
#include "GasGlobal.h"
#include "importLibraries/json.hpp"

using json = nlohmann::json; // Declare the namespace for convenience

class GasDoc
{
public:
    GasDoc();

    bool loadJsonFile(const char *json, int len);
    std::string dumpCSV(int startSecond, int endSecond, int everySeconds);
    bool newDocument();

    void rewind();
    void defaultAgentColors();
    bool duplicateAgents();
    void setColorToDefault(const int nGas);
    void changeColor(int nGas, const std::string& color);
    void colorSwap(int first, int second);

    inline std::string GetDescription() const { return description; }
    inline void SetDescription(const std::string& text) { description = text; }
    inline GasAnesthetic *GetAgent(int nGas) const { return (GasAnesthetic*)m_anesArray.at(m_gasArray.at(nGas)->m_nAgent); }
    inline float GetDEL(int nGas) const { return m_gasArray.at(nGas)->m_fDEL; }
    inline float GetUnitDose(int nGas) const { return m_gasArray.at(nGas)->m_fUnitDose; }
    inline float GetUptake(int nGas) const { return m_gasArray.at(nGas)->m_fResults[UPT]; }
    inline float GetDelivered(int nGas) const { return m_gasArray.at(nGas)->m_fResults[DEL]; }
    inline float GetCKT(int nGas) const { return m_gasArray.at(nGas)->m_fResults[CKT]; }
    inline float GetALV(int nGas) const { return m_gasArray.at(nGas)->m_fResults[ALV]; }
    inline float GetART(int nGas) const { return m_gasArray.at(nGas)->m_fResults[ALV]; }
    inline float GetVRG(int nGas) const { return m_gasArray.at(nGas)->m_fResults[VRG]; }
    inline float GetMUS(int nGas) const { return m_gasArray.at(nGas)->m_fResults[MUS]; }
    inline float GetFAT(int nGas) const { return m_gasArray.at(nGas)->m_fResults[FAT]; }
    inline float GetVEN(int nGas) const { return m_gasArray.at(nGas)->m_fResults[VEN]; }
    inline float GetUptakeCost(int nGas) { return m_gasArray.at(nGas)->m_fResults[UPT] * GetCostRatio(nGas); }
    inline float GetDeliveredCost(int nGas) { return m_gasArray.at(nGas)->m_fResults[DEL] * GetCostRatio(nGas); }
    inline std::string GetCircuit() const { return m_strCircuit; }
    inline int GetSpeed() const { return m_nSpeed; }
    inline int GetCurrentSpeed() const { return m_theState == RUNNING_STATE && m_bSpeedup ? AFAP : m_nSpeed; }
    inline float GetTime() const { return (float)(m_dwTime / 1000.0F); }
    inline float GetHighTime() const { return (float)(m_dwHighTime / 1000.0F); }
    inline bool IsOver() const { return m_dwTime >= m_dwHighTime; }
    inline uint32_t GetHighCalcTime() { return m_dwCalcTime; }
    inline float GetFGF() const { return m_fFGF; }
    inline float GetVA() const { return m_fVA; }
    inline float GetCO() const { return m_fCO; }
    inline float GetWeight() const { return m_fWeight; }
    inline AState GetState() const { return m_theState; }

    float GetVA(float fMin);
    float GetCO(float fMin);
    float GetFGF(float fMin, int nGas);
    float GetDEL(float fMin, int nGas, bool *pbInjected = 0, float fLast = 0.0F);
    float GetCKT(float fMin, int nGas);
    float GetALV(float fMin, int nGas);
    float GetART(float fMin, int nGas);
    float GetVRG(float fMin, int nGas);
    float GetMUS(float fMin, int nGas);
    float GetFAT(float fMin, int nGas);
    float GetVEN(float fMin, int nGas);
    float GetUptake(float fMin, int nGas);
    float GetUptakeCost(float fMin, int nGas);
    float GetDelivered(float fMin, int nGas);
    float GetDeliveredCost(float fMin, int nGas);

    void SetWeight(void *that, const float &fWeight);
    void SetDEL(void *that, int nGas, const float &DEL);
    void SetFGF(void *that, const float &FGF);
    void SetVA(void *that, const float &VA);
    void SetCO(void *that, const float &CO);
    void SetCircuit(void *that, const std::string &circuit);
    void SetAgent(void *that, int nGas, const std::string &agent);
    void Inject();
    void Flush();

    static void ReadProfile();
    void ChangeState(void *that, const AState theState);
    bool ExtendResultSet(); // Compute more data
	static int safeStringToInt(const std::string& str);
protected:
    void Truncate(); // Discard future results/samples
    void InitDocument(); // Set default values
    void ResetState(const GasSample& samp); // Set state from some sample
    void DeleteContents(); // Clear out document

    void ComputeTerms(float fFGF, float fVA, float fCO, float fWeight, int nAgent,
                      COMP_ARRAY &fEffectiveFlowTo, COMP_ARRAY (&fExp)[MAX_VERNIER]);
    // Calculate one breath
    bool Calc(GasSample &samp, int nInjections, bool bFlush, RES_ARRAY& fPressure,
              COMP_ARRAY& fEffectiveFlowTo, COMP_ARRAY& fExp, uint32_t dwTime,
              float fTotUptake, float fUptake, int nVernier);
    float CalcUptake(GasSample &samp, RES_ARRAY &fResults, COMP_ARRAY& fExp, int nVernier);
    void HandleInflectionPoint(); // Tell views requested time's here
    int SampAt(uint32_t dwTime); // Locate the sample @ time dwTime
    RES_ARRAY &ResultsAt(uint32_t dwTime, int nGas); // Obtain results at time dwTime
    float GetCostRatio(int nGas);

private:
    void SetTimeVariables(uint16_t dt);
	
    RES_ARRAY &ResultsFromHistoryAt(uint32_t dwTime, int nGas);
    inline uint32_t GetDwTime(float fMin) {
        uint32_t dwt = uint32_t(fMin * 60000.0F);
        assert(dwt <= m_dwHighTime + 100L);
        return std::min(dwt, m_dwHighTime);
    }
    void AddDefaultAgentColor(const std::string &agent);
    std::string getDefaultAgentColor(const std::string &agent);
    std::string getDefaultAgentColor(const int nGas) const;
    EventList SampComp(GasSample &s1, GasSample &s2);
    json createJsonElement(const std::string& name, const GasAttributeMap& attributes, const std::vector<json>& children);
    inline int getSampleCount() const { return m_gasArray.front()->m_pSampArray->size(); }
    bool loadJsonParams(json& elem); // Update parameter type here
    bool loadJsonPatient(json& elem);
    bool loadJsonPatientWeight(json& elem);
    bool loadJsonPatientVolumes(json& elem);
    bool updateVolume(json& elem, const std::string& name, AResult compartment);
    bool updateFlow(json& elem, const std::string& name, AResult compartment); // Correct parameter type
    bool updateFloat(json& elem, const std::string& name, float& target); // Correct parameter type
    bool loadJsonPatientFlows(json& elem);
    bool loadJsonAgents(json& elem);
    bool loadJsonAgent(json& elem);
    bool updateSolubility(GasAnesthetic& anes, json& elem, const std::string& name, AResult compartment); // Correct parameter type
    bool updateUint16(json& elem, const std::string& name, uint16_t& target);
    bool updateInt(json& elem, const std::string& name, int& target);
    bool loadJsonSettings(json& elem);
    bool loadJsonSetting(json& elem);
    bool loadJsonResults(json& elem);
    bool loadJsonResult(json& elem);
	bool loadJsonTime(json& elem);
	void loadJsonRunTo(uint32_t time);
	bool loadJsonCircuit(json& elem);
	bool loadJsonAgentSettings(json& elem);
	bool loadJsonAgentSetting(json& elem);
    //signals:
    //    void updateControl();

    // Attributes
public:
    GasArray m_gasArray; // Array of arrays of results
    std::vector<std::string> colors; // Chosen colors for above array
    GasAnesArray m_anesArray; // Array of anesthetic parameters
    int m_nActiveGas; // Gas# from last active view
    uint32_t m_dwSched; // Last scheduling # computed by app
    bool m_bOpened; // From a file
    bool m_bClosed; // going away
    int m_nBookmarks;
    uint32_t m_dwBookmarks[MAX_BKMK]; // We stop when we hit one
    std::vector<int> m_targetLists[MAX_COMPART]; // compartment->percent MAC targets
    float m_target[MAX_COMPART][2]; // MAC low and high targets (breakpoints)
    std::vector<int> m_targetCompartments; // Compartments where targets have been set
    int m_nTempmarks;
    uint32_t m_dwTempmarks[MAX_BKMK];
    // Patient Defaults
    static float m_fDfltVA;
    static float m_fDfltCO;
    static float m_fDfltWeight;
    static COMP_ARRAY m_fDfltVolume; // volume by compartment
    static COMP_ARRAY m_fDfltRatio; // proportion of CO to compartment
    // Program Defaults
    static int m_nBeep; // -2 off, -1 beep, 0-4 system sounds
    // Simulation Defaults
    static int m_nDfltSpeed; // default initial speed
    static std::string m_szDfltCircuit;
    static float m_fDfltFGF;
    // A Document is saved with the view parameters of view that was active
    // when it was saved:
    uint16_t save_nActiveGas;
    uint16_t save_HIGH_DEL;
    uint16_t save_bShowUptakeCost;
    uint16_t save_bShowMl;
    uint16_t save_HIGH_FGF;
    uint16_t save_HIGH_CO;
    uint16_t save_HIGH_VA;
    uint16_t save_nScaleMinutes; // Last choice of x-axis in minutes
    uint16_t save_nSpeed;

    enum aSerializationFlag
    {
        SER_NONE = 0,
        SER_COLORS = 1,
        SER_DESCRIPTION = SER_COLORS << 1,
        SER_LOCAL_AGENT = SER_DESCRIPTION << 1,
        SER_NEW_HEADER = SER_LOCAL_AGENT << 1,
        SER_INITIAL_STATE = SER_NEW_HEADER << 1,
        SER_TARGETS = SER_INITIAL_STATE << 1
    };

    std::string cond;
    uint32_t getMaxTime() { return m_dwMaxTime; }

protected:
    uint16_t m_cMSec_dx; // milliseconds between calcs
    uint32_t m_dwHighTime; // Highest dwTime
    uint32_t m_dwNextDisplayTime; // Next desired time
    uint32_t m_dwTime; // Current time
    uint32_t m_dwCalcTime; // Highest calculated time
    uint32_t m_dwMaxTime; // probably 20 hours in msecs
    uint32_t m_dwCalcStart; // Lowest calculated time in primary buffer
    uint32_t m_dwOldStart; // Beginning of buffered time
    uint32_t m_dwOldNext; // End of buffered time as we know it
    bool m_bBreak; // TRUE iff m_dwNewTime is bookmarked
    float m_fdt; // m_cMSec_dx/60k = dt in minutes
    uint32_t m_dwSampTime; // msec between samples
    short m_nSampIdx[MAX_SAMP]; // Index into sample array by interval
    short m_nLastSampAt; // Optimization storage for SampAt()
    uint32_t m_dwLastSampAtEnd;
    uint32_t m_dwLastSampAtTime;
    // The Document keeps track of the current settings
    std::string m_strCircuit; // Open, semi, or closed
    float m_fFGF; // Current FGF setting
    float m_fVA; // Current VA setting
    float m_fCO; // Current CO setting
    float m_fWeight; // Current(!) Patient weight
    bool m_bRtnEnb; // Return Enabled
    bool m_bUptEnb; // Uptake Enabled
    bool m_bVapEnb; // Vaporization Enabled
    bool m_bFlush; // User hit flush button
    int m_nSpeed; // 1 for 1x, 2 for 2x,...
    bool m_bSpeedup; // Temporary AFAP
    AState m_theState; // Running, Stopped, Initial...
    COMP_ARRAY m_fVolume; // Volume by compartment
    COMP_ARRAY m_fRatio; // Proportion of CO to compartment

private:
    uint32_t m_SerializationFlags;

    std::string description;
    uint32_t m_dwProgress;
    uint32_t m_dwAvgSched; // Last computed avg scheduling #
    bool started;
    std::vector<GasChildWindow *> childWindows; // List of children connected with this document
    std::string m_curFile; // Path to a file where data are stored
    bool isUntitled; // Check if the document is untitled

public:
    std::vector<std::string> op;
    std::vector<std::string> si;
    float oldval;
    float expv;

private:
    friend class GasSample;
};

#endif // GASDOC_H
