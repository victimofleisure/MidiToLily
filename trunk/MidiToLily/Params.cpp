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

#include "stdafx.h"
#include "Params.h"
#include "Note.h"

CString CParamBase::CMbtTime::Format() const
{
	CString	sMidiTime;
	sMidiTime.Format(_T("%d:%d:%d"), m_nMeasure, m_nBeat, m_nTick);
	return sMidiTime;
}

bool CParamBase::CMbtTime::Scan(CString sTime)
{
	CMbtTime	t(1, 1, 0);	// init with defaults as beat and tick are optional
	int	nConvs = _stscanf_s(sTime.GetString(), _T("%d:%d:%d"), &t.m_nMeasure, &t.m_nBeat, &t.m_nTick);
	if (nConvs < 1 || t.m_nMeasure < 1 || t.m_nBeat < 1 || t.m_nTick < 0)	// if invalid time
		return false;
	*this = t;	// set members
	return true;
}

CString	CParamBase::CTimeSig::Format() const
{
	CString	sTimeSig;
	sTimeSig.Format(_T("%d/%d"), m_nNumer, m_nDenom);
	return sTimeSig;
}

bool CParamBase::CTimeSig::Scan(CString sTimeSig)
{
	CTimeSig	ts;
	int	nConvs = _stscanf_s(sTimeSig.GetString(), _T("%d/%d"), &ts.m_nNumer, &ts.m_nDenom);
	if (nConvs != 2 || ts.m_nNumer < 1 || ts.m_nDenom < 1)	// if conversion failed or range error
		return false;
	*this = ts;	// set members
	return true;
}

CString	CParamBase::CKeySig::Format() const
{
	static const LPCTSTR	pszTonality[2] = {
		_T(""),
		_T("m"),
	};
	CString	sKeySig(CNote::GetKeyName(m_nAccs, m_bIsMinor));
	sKeySig += pszTonality[m_bIsMinor];
	return sKeySig;
}

CString	CParamBase::CKeySig::FormatLily() const
{
	static const LPCTSTR	pszTonality[2] = {
		_T(" \\major"),
		_T(" \\minor"),
	};
	CString	sKeySig(CNote::GetLilyKeyName(m_nAccs, m_bIsMinor));
	sKeySig += pszTonality[m_bIsMinor];
	return sKeySig;
}

CString CParamBase::CMetroMark::Format() const
{
	CString	sDenom;
	sDenom.Format(_T("%d"), m_nDenom);
	for (int iDot = 0; iDot < m_nDots; iDot++) {
		sDenom += '.';
	}
	CString	sBPM;
	sBPM.Format(_T("%d"), m_nBPM);
	return sDenom + _T(" = ") + sBPM;
}

bool CParamBase::CMetroMark::Scan(CString sMark, int* piFirstChar)
{
	CMetroMark	mm;
	if (piFirstChar != NULL)	// if caller requested index of mark's first character
		*piFirstChar = 0;	// safety first
	int	iSeparator = sMark.ReverseFind('=');	// reverse find separator
	if (iSeparator < 1)	// if separator not found or is first character
		return false;	// can't proceed
	int	iChar = iSeparator;	// reverse iterate from separator
	while (iChar > 0 && sMark[iChar - 1] == ' ') {	// skip spaces
		iChar--;
	}
	mm.m_nDots = 0;	// reset dots
	while (iChar > 0 && sMark[iChar - 1] == '.') {	// skip periods
		iChar--;
		mm.m_nDots++;	// count dots
	}
	while (iChar > 0 && isdigit(sMark[iChar - 1])) {	// skip digits
		iChar--;
	}
	// iChar should now be index of denominator's first digit
	if (piFirstChar != NULL)	// if caller requested index of mark's first character
		*piFirstChar = iChar;	// pass index back to caller
	CString	sBPM(sMark.Mid(iSeparator + 1));
	if (_stscanf_s(sBPM.GetString(), _T("%d"), &mm.m_nBPM) != 1)	// scan beats per minute
		return false;
	// must use h prefix because denominator is 16-bit, else dots get clobbered
	if (_stscanf_s(sMark.Mid(iChar), _T("%hd"), &mm.m_nDenom) != 1)	// scan denominator
		return false;
	if (!IsPowerOfTwo(mm.m_nDenom))	// if denominator isn't a power of two
		return false;
	*this = mm;	// set members
	return true;
}

CString CParamBase::CTempo::Format() const
{
	CString	sTempo;
	if (!m_sDescription.IsEmpty()) {
		sTempo = '"' + m_sDescription + '"';
	}
	if (m_nDenom) {	// if valid denominator
		CString	sMark(CMetroMark::Format());
		if (!sTempo.IsEmpty())
			sTempo += ' ';
		sTempo += sMark;
	}
	return sTempo;
}

bool CParamBase::CTempo::Scan(CString sTempo)
{
	CMetroMark	mm;
	int	iMetroStart;
	if (mm.Scan(sTempo, &iMetroStart)) {	// if valid metronome mark
		CMetroMark&	base = *this;
		base = mm;	// update base class members
		sTempo = sTempo.Left(iMetroStart);	// remove mark from argument
	}
	// if description is enclosed in double quotes, strip them
	int	iStart = 0;
	m_sDescription = sTempo.Tokenize(_T("\""), iStart);
	return true;
}

bool CParamBase::IsPowerOfTwo(int n)
{
	ASSERT(n > 0);
	return (n & (n - 1)) == 0;
}

void CParamBase::RenameExtension(CString& sPath, CString sExtension)
{
	LPTSTR pszPath = sPath.GetBuffer(sPath.GetLength() + sExtension.GetLength());
	PathRemoveExtension(pszPath);
	PathAddExtension(pszPath, sExtension);
	sPath.ReleaseBuffer();
}

void CParamBase::RemoveExtension(CString& sPath)
{
	LPTSTR pszPath = sPath.GetBuffer(sPath.GetLength());
	PathRemoveExtension(pszPath);
	sPath.ReleaseBuffer();
}

void CParamBase::RemoveFileSpec(CString& sPath)
{
	LPTSTR pszPath = sPath.GetBuffer(sPath.GetLength());
	PathRemoveFileSpec(pszPath);
	sPath.ReleaseBuffer();
}

bool CParamBase::PathsDiffer(LPCTSTR pszInPath, LPCTSTR pszOutPath)
{
	CString	sInFilename(PathFindFileName(pszInPath));
	CString	sOutFilename(PathFindFileName(pszOutPath));
	RemoveExtension(sInFilename);
	RemoveExtension(sOutFilename);
	if (sInFilename.CompareNoCase(sOutFilename))	// if file names differ
		return true;
	CString	sInFolder(pszInPath);
	CString	sOutFolder(pszOutPath);
	RemoveFileSpec(sInFolder);
	RemoveFileSpec(sOutFolder);
	if (sInFolder.CompareNoCase(sOutFolder))	// if folders differ
		return true;
	return false;	// paths don't differ
}

CParams::CParams()
{
	m_bFrenched = false;
	m_bVerify = false;
	m_nOffset = 0;
	m_nQuantDenom = 0;
	m_nTripletQuantDenom = 0;
	m_nLoggingMask = 0;
}

void CParams::Log() const
{
	_tprintf(_T("parameters:\n"));
	_tprintf(_T("Output = \"%s\"\n"), m_sOutput.GetString());
	_tprintf(_T("Title = \"%s\"\n"), m_sTitle.GetString());
	_tprintf(_T("Subtitle = \"%s\"\n"), m_sSubtitle.GetString());
	_tprintf(_T("Opus = \"%s\"\n"), m_sOpus.GetString());
	_tprintf(_T("Piece = \"%s\"\n"), m_sPiece.GetString());
	_tprintf(_T("Composer = \"%s\"\n"), m_sComposer.GetString());
	_tprintf(_T("Copyright = \"%s\"\n"), m_sCopyright.GetString());
	_tprintf(_T("Frenched = %d\n"), m_bFrenched);
	_tprintf(_T("Offset = %d\n"), m_nOffset);
	_tprintf(_T("Quant = %d\n"), m_nQuantDenom);
	_tprintf(_T("Triplet = %d\n"), m_nTripletQuantDenom);
	_tprintf(_T("Sections = %d\n"), m_arrSection.GetSize());
	for (int iSection = 0; iSection < m_arrSection.GetSize(); iSection++) {
		_tprintf(_T("Section %d = %d\n"), iSection, m_arrSection[iSection]);
	}
	for (int iClef = 0; iClef < m_arrClef.GetSize(); iClef++) {
		_tprintf(_T("Track %d clef = \"%s\"\n"), iClef, m_arrClef[iClef].GetString());
	}
	for (int iTrack = 0; iTrack < m_arrOttavaArray.GetSize(); iTrack++) {
		const COttavaArray& arrOttava = m_arrOttavaArray[iTrack];
		_tprintf(_T("Track %d Ottavas = %d\n"), iTrack, arrOttava.GetSize());
		for (int iOttava = 0; iOttava < arrOttava.GetSize(); iOttava++) {
			const COttava& ot = arrOttava[iOttava];
			_tprintf(_T("Ottava %d = %s %d\n"), iOttava, 
				ot.Format().GetString(), ot.m_nShift);
		}
	}
	_tprintf(_T("Staves = %d\n"), m_arrStave.GetSize());
	for (int iStave = 0; iStave < m_arrStave.GetSize(); iStave++) {
		_tprintf(_T("Stave %d track = %d\n"), iStave, m_arrStave[iStave]);
	}
	_tprintf(_T("TimeSigs = %d\n"), m_arrTimeSig.GetSize());
	for (int iTimeSig = 0; iTimeSig < m_arrTimeSig.GetSize(); iTimeSig++) {
		const CTimeSigChange&	tsc = m_arrTimeSig[iTimeSig];
		_tprintf(_T("TimeSig %d = %d %s\n"), iTimeSig, 
			tsc.m_nMeasure, tsc.Format().GetString());
	}
	_tprintf(_T("KeySigs = %d\n"), m_arrKeySig.GetSize());
	for (int iKeySig = 0; iKeySig < m_arrKeySig.GetSize(); iKeySig++) {
		const CKeySigChange&	ksc = m_arrKeySig[iKeySig];
		_tprintf(_T("KeySig %d = %d %s\n"), iKeySig, 
			ksc.m_nMeasure, ksc.Format().GetString());
	}
	_tprintf(_T("Tempos = %d\n"), m_arrTempo.GetSize());
	for (int iTempo = 0; iTempo < m_arrTempo.GetSize(); iTempo++) {
		const CTempoChange&	tsc = m_arrTempo[iTempo];
		_tprintf(_T("Tempo %d = %d %s\n"), iTempo, 
			tsc.m_nMeasure, tsc.Format().GetString());
	}
}

void CParams::Finalize()
{
	m_arrSection.Sort();	// sort sections in ascending order by measure number
	int	nTracks = m_arrOttavaArray.GetSize();
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each track
		m_arrOttavaArray[iTrack].Sort();	// sort ottavas in ascending order by time in ticks
	}
	m_arrTimeSig.Sort();	// sort time signature changes in ascending order by measure number
	m_arrKeySig.Sort();		// sort key signature changes in ascending order by measure number
	m_arrTempo.SortIndirect();	// sort tempo changes in ascending order by measure number
}
