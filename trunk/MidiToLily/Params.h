// Copyleft 2025 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		09jan25	initial version
 
*/

#pragma once

#include "ArrayEx.h"

class CParamBase {
public:
	class CMbtTime {	// time in measures, beats and ticks
	public:
		CMbtTime() {}
		CMbtTime(int nMeasure, int nBeat, int nTick);
		int		m_nMeasure;	// one-based measure number
		int		m_nBeat;	// one-based beat number
		int		m_nTick;	// zero-based tick
		CString	Format() const;
		bool	Scan(CString sTime);
		bool	operator==(const CMbtTime& mt) const;
		bool	operator!=(const CMbtTime& mt) const;
		bool	operator>(const CMbtTime& mt) const;
		bool	operator<(const CMbtTime& mt) const;
	};
	class CKeySig {	// key signature
	public:
		CKeySig() {}
		CKeySig(int nAccs, bool bIsMinor);
		bool	operator==(const CKeySig& ks) const;
		bool	operator!=(const CKeySig& ks) const;
		CString	Format() const;
		CString	FormatLily() const;
		int		m_nAccs;	// number of accidentals; positive for sharps, negative for flats
		int		m_bIsMinor;	// true if minor, else major
	};
	class CTimeSig {	// time signature
	public:
		CTimeSig() {}
		CTimeSig(int nNumer, int nDenom);
		bool	operator==(const CTimeSig& ts) const;
		bool	operator!=(const CTimeSig& ts) const;
		CString	Format() const;
		bool	Scan(CString sTimeSig);
		int		m_nNumer;	// numerator
		int		m_nDenom;	// denominator
	};
	class CMetroMark {	// metronome mark
	public:
		CMetroMark() {}
		CMetroMark(WORD nDenom, BYTE nDots, int nBPM);
		bool	operator==(const CMetroMark& mm) const;
		bool	operator!=(const CMetroMark& mm) const;
		CString	Format() const;
		bool	Scan(CString sMark, int* piFirstChar = NULL);
		WORD	m_nDenom;	// denominator; 1 = whole, 2 = half, 4 = quarter, etc.
		BYTE	m_nDots;	// number of dots to extend denominator by
		int		m_nBPM;		// number of beats per minute
	};
	class CTempo : public CMetroMark {	// tempo, derived from metronome mark
	public:
		CTempo() {}
		CTempo(WORD nDenom, BYTE nDots, int nBPM, LPCTSTR pszDescription = NULL);
		bool	operator==(const CTempo& t) const;
		bool	operator!=(const CTempo& t) const;
		CString	Format() const;
		bool	Scan(CString sTempo);
		CString	m_sDescription;	// descriptive text
	};
	class COttava : public CMbtTime {	// octave shift
	public:
		COttava() {}
		COttava(int nMeasure, int nBeat, int nTick, int nShift);
		bool	operator>(const COttava& ot) const;
		bool	operator<(const COttava& ot) const;
		int		m_nShift;		// signed number of octaves to shift, or zero to reset
	};
	typedef CArrayEx<COttava, COttava&> COttavaArray;
	typedef CArrayEx<COttavaArray, COttavaArray&> COttavaArrayArray;
	class CKeySigChange : public CKeySig {	// key signature change
	public:
		CKeySigChange() {}
		CKeySigChange(int nMeasure, int nAccs, bool bIsMinor);
		bool	operator>(const CKeySigChange& ksc) const;
		bool	operator<(const CKeySigChange& ksc) const;
		int		m_nMeasure;		// one-based measure number at which change occurs
	};
	typedef CArrayEx<CKeySigChange, CKeySigChange&> CKeySigChangeArray;
	class CTimeSigChange : public CTimeSig {	// time signature change
	public:
		CTimeSigChange() {}
		CTimeSigChange(int nMeasure, int nNumer, int nDenom);
		bool	operator>(const CTimeSigChange& tsc) const;
		bool	operator<(const CTimeSigChange& tsc) const;
		int		m_nMeasure;		// one-based measure number at which change occurs
	};
	typedef CArrayEx<CTimeSigChange, CTimeSigChange&> CTimeSigChangeArray;
	class CTempoChange : public CTempo {	// tempo change
	public:
		CTempoChange() {}
		CTempoChange(int nMeasure, WORD nDenom, BYTE nDots, int nBPM, LPCTSTR pszDescription = NULL);
		bool	operator>(const CTempoChange& tc) const;
		bool	operator<(const CTempoChange& tc) const;
		int		m_nMeasure;		// one-based measure number at which change occurs
	};
	typedef CArrayEx<CTempoChange, CTempoChange&> CTempoChangeArray;
	static	bool	IsPowerOfTwo(int n);
};

inline CParamBase::CMbtTime::CMbtTime(int nMeasure, int nBeat, int nTick)
{
	m_nMeasure = nMeasure;
	m_nBeat = nBeat;
	m_nTick = nTick;
}

inline bool CParamBase::CMbtTime::operator==(const CMbtTime& mt) const
{
	return m_nMeasure == mt.m_nMeasure && m_nBeat == mt.m_nBeat && m_nTick == mt.m_nTick;
}

inline bool CParamBase::CMbtTime::operator!=(const CMbtTime& mt) const
{
	return m_nMeasure != mt.m_nMeasure || m_nBeat != mt.m_nBeat || m_nTick != mt.m_nTick;
}

inline bool CParamBase::CMbtTime::operator>(const CMbtTime& mt) const
{
	if (m_nMeasure > mt.m_nMeasure)
		return true;
	if (m_nMeasure < mt.m_nMeasure)
		return false;
	if (m_nBeat > mt.m_nBeat)
		return true;
	if (m_nBeat < mt.m_nBeat)
		return false;
	return m_nTick > mt.m_nTick;
}

inline bool CParamBase::CMbtTime::operator<(const CMbtTime& mt) const
{
	if (m_nMeasure < mt.m_nMeasure)
		return true;
	if (m_nMeasure > mt.m_nMeasure)
		return false;
	if (m_nBeat < mt.m_nBeat)
		return true;
	if (m_nBeat > mt.m_nBeat)
		return false;
	return m_nTick < mt.m_nTick;
}

inline CParamBase::CKeySig::CKeySig(int nAccs, bool bIsMinor)
{
	 m_nAccs = nAccs;
	 m_bIsMinor = bIsMinor;
}

inline bool CParamBase::CKeySig::operator==(const CKeySig& ks) const
{
	return m_nAccs == ks.m_nAccs && m_bIsMinor == ks.m_bIsMinor;
}

inline bool CParamBase::CKeySig::operator!=(const CKeySig& ks) const
{
	return m_nAccs != ks.m_nAccs || m_bIsMinor != ks.m_bIsMinor;
}

inline CParamBase::CTimeSig::CTimeSig(int nNumer, int nDenom)
{
	m_nNumer = nNumer;
	m_nDenom = nDenom;
}

inline bool CParamBase::CTimeSig::operator==(const CTimeSig& ts) const
{
	return m_nNumer == ts.m_nNumer && m_nDenom == ts.m_nDenom;
}

inline bool CParamBase::CTimeSig::operator!=(const CTimeSig& ts) const
{
	return m_nNumer != ts.m_nNumer || m_nDenom != ts.m_nDenom;
}

inline CParamBase::CMetroMark::CMetroMark(WORD nDenom, BYTE nDots, int nBPM) 
{
	m_nDenom = nDenom;
	m_nDots = nDots;
	m_nBPM = nBPM;
}

inline bool CParamBase::CMetroMark::operator==(const CMetroMark& mm) const
{
	return m_nDenom == mm.m_nDenom && m_nDots == mm.m_nDots && m_nBPM == mm.m_nBPM;
}

inline bool CParamBase::CMetroMark::operator!=(const CMetroMark& mm) const
{
	return m_nDenom != mm.m_nDenom || m_nDots != mm.m_nDots || m_nBPM != mm.m_nBPM;
}

inline CParamBase::CTempo::CTempo(WORD nDenom, BYTE nDots, int nBPM, LPCTSTR pszDescription)
	: CMetroMark(nDenom, nDots, nBPM), m_sDescription(pszDescription)
{
}

inline bool CParamBase::CTempo::operator==(const CTempo& t) const
{
	return CMetroMark::operator==(t) && m_sDescription == t.m_sDescription;
}

inline bool CParamBase::CTempo::operator!=(const CTempo& t) const
{
	return CMetroMark::operator!=(t) && m_sDescription != t.m_sDescription;
}

inline CParamBase::COttava::COttava(int nMeasure, int nBeat, int nTick, int nShift)
	: CMbtTime(nMeasure, nBeat, nTick), m_nShift(nShift)
{
}

inline bool CParamBase::COttava::operator>(const COttava& ot) const
{
	return CMbtTime::operator>(ot);
}

inline bool CParamBase::COttava::operator<(const COttava& ot) const
{
	return CMbtTime::operator<(ot);
}

inline CParamBase::CKeySigChange::CKeySigChange(int nMeasure, int nAccs, bool bIsMinor)
	: CKeySig(nAccs, bIsMinor), m_nMeasure(nMeasure)
{
}

inline bool CParamBase::CKeySigChange::operator>(const CKeySigChange& ksc) const
{
	return m_nMeasure > ksc.m_nMeasure;
}

inline bool CParamBase::CKeySigChange::operator<(const CKeySigChange& ksc) const
{
	return m_nMeasure < ksc.m_nMeasure;
}

inline CParamBase::CTimeSigChange::CTimeSigChange(int nMeasure, int nNumer, int nDenom)
	: CTimeSig(nNumer, nDenom), m_nMeasure(nMeasure)
{
}

inline bool CParamBase::CTimeSigChange::operator>(const CTimeSigChange& tsc) const
{
	return m_nMeasure > tsc.m_nMeasure;
}

inline bool CParamBase::CTimeSigChange::operator<(const CTimeSigChange& tsc) const
{
	return m_nMeasure < tsc.m_nMeasure;
}

inline CParamBase::CTempoChange::CTempoChange(int nMeasure, WORD nDenom, BYTE nDots, int nBPM, LPCTSTR pszDescription)
	: CTempo(nDenom, nDots, nBPM, pszDescription), m_nMeasure(nMeasure)
{
}

inline bool CParamBase::CTempoChange::operator>(const CTempoChange& tc) const
{
	return m_nMeasure > tc.m_nMeasure;
}

inline bool CParamBase::CTempoChange::operator<(const CTempoChange& tc) const
{
	return m_nMeasure < tc.m_nMeasure;
}

class CParams : public CParamBase {
public:
	CParams();
	CString	m_sOutput;		// path of output file
	CString	m_sTitle;		// optional title string
	CString	m_sSubtitle;	// optional subtitle string
	CString	m_sOpus;		// optional opus string
	CString	m_sPiece;		// optional piece string
	CString	m_sComposer;	// optional composer string
	CString	m_sCopyright;	// optional copyright string
	bool	m_bFrenched;	// true if hiding empty staves
	bool	m_bVerify;		// true if verifying MIDI file
	int		m_nOffset;		// offset in ticks added to all events
	int		m_nQuantDenom;	// regular quantization denominator as a power of two, or zero if none
	int		m_nTripletQuantDenom;	// triplet quantization denominator as a power of two, or zero if none
	UINT	m_nLoggingMask;	// bitmask of enabled logging types; see enum below
	CStringArrayEx	m_arrClef;	// array of clef overrides; one item per track, omitting trailing empty clefs
	CIntArrayEx	m_arrSection;	// array of sections; each item is a one-based measure number
	COttavaArrayArray	m_arrOttavaArray;	// one array of ottava arrays per track
	CIntArrayEx	m_arrStave;	// array of track indices specifying which tracks are assigned to staves
	CKeySigChangeArray	m_arrKeySig;	// array of key signature changes
	CTimeSigChangeArray	m_arrTimeSig;	// array of time signature changes
	CTempoChangeArray	m_arrTempo;		// array of tempo changes
	void	Finalize();
	void	Log() const;
};
