#ifndef GAS_H
#define GAS_H
#include "framework.h"
#include <string>
#include <cassert>
#include "GasGlobal.h"
#include "GasPtrList.h"
#include "GasSample.h"

class GasDoc;

class Gas
{
public:
    Gas();
    Gas(GasDoc* pDoc);
    virtual ~Gas();
    void Serialize(std::iostream &stream, bool storing, uint32_t serializationFlags);

    void Inject();
    void Flush();
    void SetAgent(const std::string& agent);
    void SetDEL(const float fDEL);
    void SetInitialSamp(GasSample *const &p) { delete m_pInitialSamp; m_pInitialSamp = p; }
    float asMacPercent(float x) const;

protected:
    void GasInit(); // Allocation
    void LoadAmbient(RES_ARRAY& results); // Initialize an array with ambient values
    void SetInitialResults(); // Fill in initial CKT, ALV, ...
    void LoadFirstSampleState(); // Set current values from time zero sample
    void LoadSampleState(const GasSample& samp); // Set current values from a sample
    void DeleteContents(); // Clear out document

public:
    static GasDoc *m_gpDoc; // Passed during creation
    int m_nAgent; // Current anesthetic
    std::string m_strAgent; // Local copy of anesthetic name

protected:
    GasDoc *m_pDoc; // Pointer to main document
    GasSampArray *m_pSampArray; // Pointer to a (big) sample array
    GasSample *m_pCurSamp; // Pointer to the latest sample
    GasSample *m_pInitialSamp; // (Usually ambient)
    
    // Due to the vagaries of segmentation, we generate a separate type
    // of pointer to a (big) array for results
    RES_ARRAY *m_pfCurResults; // One for current computations
    RES_ARRAY *m_pfOldResults; // One for back-tracking
    RES_ARRAY m_fResults; // Results at time m_dwTime (current)
    COMP_ARRAY m_fEffectiveFlowTo; // other computational factors

#ifdef EULAR
    COMP_ARRAY m_fdtOverEffectiveVolume;
#else
    COMP_ARRAY m_fExp[MAX_VERNIER];
    float m_fTermFGF; // Terms used 
    float m_fTermVA; // to calc the above
#endif

    int m_nTermAgent;
    int m_nInjections; // Remaining injections
    float m_fTermCO;
    float m_fTermWeight;
    float m_fDEL; // Current DEL setting
    float m_funused5; // Current FGF setting
    float m_funused6; // Current VA setting
    float m_funused7; // Current CO setting
    float m_funused8; // Current(!) Patient weight
    float m_fUnitDose; // Current Unit Dose
    bool m_bunused1; // User hit flush button
    bool m_bunused2; // Return Enabled
    bool m_bunused3; // Uptake Enabled
    bool m_bunused4; // Vaporization Enabled

private:
    friend class GasDoc;
    friend class GasSample;
    friend class GasPrintDocument;
};

/////////////////////////////////////////////////////////////////////////////
// An Array of second gasses

typedef GasPtrList<Gas> GasArray;

#endif
