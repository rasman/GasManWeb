#include "pch.h"
#include "Gas.h"
#include "GasDoc.h"
#include "GasSample.h"

GasDoc *Gas::m_gpDoc;

/////////////////////////////////////////////////////////////////////////////
// Gas construction/destruction

Gas::Gas()
{
    assert(m_gpDoc != nullptr);
    m_pDoc = m_gpDoc;
    GasInit();
}

Gas::Gas(GasDoc *pDoc)
{
    assert(pDoc != nullptr);
    m_pDoc = pDoc;
    GasInit();
}

void Gas::GasInit()
{
    m_pSampArray = new GasSampArray;
    m_pfCurResults = new RES_ARRAY[SAMP_EVERY];
    m_pfOldResults = new RES_ARRAY[SAMP_EVERY];
    if (!m_pSampArray || !m_pfCurResults || !m_pfOldResults)
    {
        if (m_pSampArray) delete m_pSampArray;
        if (m_pfCurResults) delete[] m_pfCurResults;
        if (m_pfOldResults) delete[] m_pfOldResults;
        m_pSampArray = nullptr;
        m_pfCurResults = nullptr;
        m_pfOldResults = nullptr;
    }
    m_pInitialSamp = nullptr;

    DeleteContents();
}

Gas::~Gas()
{
    delete m_pSampArray;
    delete m_pInitialSamp;
    delete[] m_pfCurResults;
    delete[] m_pfOldResults;
}

// Remove any contents and any indications that there ARE contents

void Gas::DeleteContents()
{
    // Lose any old samples
    m_pSampArray->clear();

    // Set everything to 'just starting and empty'

    delete m_pInitialSamp;
    m_pInitialSamp = nullptr;
    m_pCurSamp = nullptr;
    m_nAgent = -1;
    m_nTermAgent = -1; // No computational terms yet
}

void Gas::LoadAmbient(RES_ARRAY& results)
{
    float fAmbient = m_pDoc->m_anesArray[m_nAgent]->m_fAmbient;
    int i;

    for (i = 0; i < MAX_COMPART; ++i)
        results[i] = fAmbient;
    for (; i < MAX_RESULT; ++i)
        results[i] = 0.0F;
}

// Set the initial results for this anesthetic at time zero

void Gas::SetInitialResults()
{
    int i;

    if (m_pInitialSamp)
    {
        for (i = 0; i < MAX_RESULT; i++)
            m_pfCurResults[0][i] = m_fResults[i] = m_pInitialSamp->m_fResults[i];
    }
    else
    {
        LoadAmbient(m_fResults);
        for (i = 0; i < MAX_RESULT; ++i)
            m_pfCurResults[0][i] = m_fResults[i];
    }

    m_nInjections = 0;
}

// Reset the current values to the beginning of the simulation

void Gas::LoadFirstSampleState()
{
    assert(m_pSampArray->size());
    GasSample& samp = *m_pSampArray->at(0);
    LoadSampleState(samp);
}

// Reset the current values to the values of some sample

void Gas::LoadSampleState(const GasSample& samp)
{
    assert(samp.m_dwBaseTime == 0);
    m_nAgent = samp.m_wAnesID;
    assert(static_cast<size_t>(m_nAgent) < m_pDoc->m_anesArray.size());
	m_strAgent = m_pDoc->m_anesArray[m_nAgent]->m_strName;

	// Set the state variables

	m_fUnitDose = samp.m_fUnitDose;
	m_fDEL = samp.m_fDEL;
	m_nInjections = 0;

	// Set the initial state

	for ( int i = 0; i < MAX_RESULT; i++ )
		m_fResults[i] = samp.m_fResults[i];
}

void Gas::SetAgent( const std::string& agent )
{
	m_strAgent = agent;
	m_nAgent = m_pDoc->m_anesArray.Lookup(agent);
	assert(m_nAgent >= 0);
	m_fUnitDose = m_pDoc->m_anesArray[m_nAgent]->m_fDefUnitDose;
	if( m_pInitialSamp != NULL && m_pDoc->GetTime() == 0)
	{
		m_pInitialSamp->m_wAnesID = m_nAgent;
		m_pInitialSamp->m_fUnitDose = m_fUnitDose;
	}
}

void Gas::SetDEL( const float fDEL )
{
	m_fDEL = fDEL;
	if( m_pInitialSamp != NULL && m_pDoc->GetTime() == 0)
		m_pInitialSamp->m_fDEL = fDEL;
}

void Gas::Inject()
{
	m_nInjections++;
}

void Gas::Flush()
{
	m_nInjections = 0;
	m_fResults[CKT] = m_pDoc->m_anesArray[m_nAgent]->m_fAmbient;
}

float Gas::asMacPercent(float x) const
{
	float mac = m_pDoc->m_anesArray[m_nAgent]->m_fMAC;
	if (mac <= 0.0f)
		return 0.0f;
	return x * 100 / mac;
}

/////////////////////////////////////////////////////////////////////////////
// Gas serialization

void Gas::Serialize(std::iostream &stream, bool storing, uint32_t serializationFlags)
{	
	m_pSampArray->Serialize( stream, storing );

	int nSamps = m_pSampArray->size();
	
	if ( storing ) {
		if ( nSamps ) {
			for ( int i=0; i<1/*SAMP_EVERY*/; ++i ) {	// for bkwd compat; no longer used
				RES_ARRAY &arr = m_pfCurResults[i];
				for ( int y=0; y<MAX_RESULT; ++y )
					stream << arr[y ];
			}
		} else {
			if( m_pInitialSamp )
				m_pInitialSamp->Serialize( stream, storing, 0);
			else {
				RES_ARRAY res;
				LoadAmbient(res);
				GasSample samp(this, res);
				samp.Serialize( stream, storing, 0 );
			}
		}
	} else {
		if (nSamps) {
			m_pInitialSamp = new GasSample(*m_pSampArray->at(0));
			for ( int i=0; i<1/*SAMP_EVERY*/; ++i ) {	// for bkwd compat; no longer used
				RES_ARRAY &arr = m_pfCurResults[i];
				for ( int y=0; y<MAX_RESULT; ++y )
					stream >> arr[y ];
			}
			m_pCurSamp = m_pSampArray->back();
			LoadFirstSampleState();
		} else {
			GasSample *samp = new GasSample();
			samp->Serialize(stream, storing, 0);
			LoadSampleState(*samp);
			if( serializationFlags & GasDoc::SER_INITIAL_STATE )
				m_pInitialSamp = samp;
			else
				delete samp;
			SetInitialResults();
		}
	}
}


