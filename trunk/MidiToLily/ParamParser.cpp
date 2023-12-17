// Copyleft 2023 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		08dec23	initial version
 
*/

#include "stdafx.h"
#include "ParamParser.h"
#include "MidiToLily.h"	// for throw exception method
#include "VersionInfo.h"

extern CWinApp theApp;

const LPCTSTR CParamParser::m_arrFlagName[FLAGS] = {
	#define PARAMDEF(name) _T(#name),
	#include "ParamDef.h"	// use preprocessor to generate array of flag names
};

const LPCTSTR CParamParser::m_pszCopyrightNotice = _T("Copyleft 2023 Chris Korda");

CParamParser::CParamParser()
{
	m_iFlag = FLAG_NONE;
	m_nAction = ACT_CONTINUE;
}

void CParamParser::OnError(CString m_sErrMsg)
{
	CMidiToLily::OnError(m_sErrMsg);
}

void CParamParser::OnQuant(CString sParam, int& nDuration)
{
	int	nError = IDS_CLA_QUANT_INVALID;	// assume failure
	int	nParam;
	// if parameter scans to an integer greater than zero
	if (_stscanf_s(sParam.GetString(), _T("%d"), &nParam) == 1 && nParam > 0) {
		// find parameter's first set bit, starting from least significant bit
		DWORD	iBit;	// receives index of first set bit
		if (_BitScanForward(&iBit, nParam)) {	// if set bit found
			nDuration = 1 << iBit;	// convert index to a power of two
			if (nDuration == nParam) {	// if parameter is a power of two
				nError = 0;
			} else {	// parameter isn't a power of two
				nError = IDS_CLA_QUANT_NOT_POWER;
			}
		}
	}
	if (nError) {	// if an error occurred
		m_sErrMsg.Format(nError, sParam.GetString());
		OnError(m_sErrMsg);
	}
}

void CParamParser::OnOffset(CString sParam)
{
	if (_stscanf_s(sParam.GetString(), _T("%d"), &m_nOffset) != 1) {
		m_sErrMsg.Format(IDS_CLA_OFFSET_INVALID, sParam.GetString());
		OnError(m_sErrMsg);
	}
}

void CParamParser::OnClef(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		const TCHAR cSeparator = '=';
		LPCTSTR pszToken = sToken;
		int	iSeparator = sToken.Find(cSeparator);	// find separator
		if (iSeparator < 0) {	// if separator not found
			m_sErrMsg.Format(IDS_CLA_CLEF_NO_SEPARATOR, pszToken, cSeparator);
			OnError(m_sErrMsg);
		}
		CString	sClef(sToken.Mid(iSeparator + 1));
		if (sClef.IsEmpty()) {	// if clef not specified
			m_sErrMsg.Format(IDS_CLA_CLEF_NO_NAME, pszToken);
			OnError(m_sErrMsg);
		}
		int	iTrack;
		int	nConvs = _stscanf_s(sToken.GetString(), _T("%d"), &iTrack);
		if (nConvs != 1 || iTrack < 0 || iTrack > SHRT_MAX) {	// if invalid track index
			m_sErrMsg.Format(IDS_CLA_CLEF_BAD_TRACK_INDEX, pszToken);
			OnError(m_sErrMsg);
		}
		m_arrClef.SetAtGrow(iTrack, sClef);
	}
}

void CParamParser::OnSection(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		LPCTSTR pszToken = sToken;
		int	nMeasure;
		int	nConvs = _stscanf_s(sToken.GetString(), _T("%d"), &nMeasure);
		if (nConvs != 1) {	// if measure number not specified
			m_sErrMsg.Format(IDS_CLA_SECTION_BAD_MEASURE, pszToken);
			OnError(m_sErrMsg);
		}
		nMeasure--;	// internally, measure is zero-based index
		if (nMeasure < 0) {	// if invalid measure index
			m_sErrMsg.Format(IDS_CLA_SECTION_BAD_MEASURE, pszToken);
			OnError(m_sErrMsg);
		}
		m_arrSection.Add(nMeasure);
	}
}

void CParamParser::OnOttava(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		const TCHAR cSeparator = '=';
		LPCTSTR pszToken = sToken;
		int	iSeparator = sToken.Find(cSeparator);	// find separator
		if (iSeparator < 0) {	// if separator not found
			m_sErrMsg.Format(IDS_CLA_OTTAVA_NO_SEPARATOR, pszToken, cSeparator);
			OnError(m_sErrMsg);
		}
		CString	sOctave(sToken.Mid(iSeparator + 1));
		if (sOctave.IsEmpty()) {	// if octave shift not specified
			m_sErrMsg.Format(IDS_CLA_OTTAVA_NO_OCTAVE, pszToken);
			OnError(m_sErrMsg);
		}
		COttava	ot;
		int nConvs = _stscanf_s(sOctave.GetString(), _T("%d"), &ot.m_nShift);
		if (nConvs != 1) {	// if octave shift isn't a number
			m_sErrMsg.Format(IDS_CLA_OTTAVA_BAD_OCTAVE, pszToken);
			OnError(m_sErrMsg);
		}
		const TCHAR cTimeSeparator = '_';
		int	iTimeSeparator = sToken.Find(cTimeSeparator);	// find time separator
		if (iTimeSeparator < 0) {	// if time separator not found
			m_sErrMsg.Format(IDS_CLA_OTTAVA_NO_TIME_SEPARATOR, pszToken, cTimeSeparator);
			OnError(m_sErrMsg);
		}
		int	nTimeLen = iSeparator - iTimeSeparator - 1;
		if (nTimeLen < 1) {	// if time not specified
			m_sErrMsg.Format(IDS_CLA_OTTAVA_NO_TIME, pszToken);
			OnError(m_sErrMsg);
		}
		CString	sTime(sToken.Mid(iTimeSeparator + 1, nTimeLen));
		int	iTrack;
		nConvs = _stscanf_s(sToken.GetString(), _T("%d"), &iTrack);
		if (nConvs != 1 || iTrack < 0 || iTrack > SHRT_MAX) {	// if invalid track index
			m_sErrMsg.Format(IDS_CLA_OTTAVA_BAD_TRACK_INDEX, pszToken);
			OnError(m_sErrMsg);
		}
		nConvs = _stscanf_s(sTime.GetString(), _T("%d:%d:%d"), &ot.m_nMeasure, &ot.m_nBeat, &ot.m_nTick);
		if (nConvs != 3 || ot.m_nMeasure < 1 || ot.m_nBeat < 1 || ot.m_nTick < 0) {	// if invalid time
			m_sErrMsg.Format(IDS_CLA_OTTAVA_BAD_TIME_FORMAT, pszToken);
			OnError(m_sErrMsg);
		}
		int	nNewSize = max(m_arrOttavaArray.GetSize(), iTrack + 1);
		m_arrOttavaArray.SetSize(nNewSize);
		m_arrOttavaArray[iTrack].Add(ot);
	}
}

void CParamParser::OnLogging(CString sParam)
{
	if (sParam == '*') {	// if parameter is wildcard
		m_nLoggingMask = UINT_MAX;	// set all bits
	} else {	// not wildcard, so expecting a bitmask as a hexadecimal number
		int	nConvs = _stscanf_s(sParam.GetString(), _T("%x"), &m_nLoggingMask);
		if (nConvs != 1) {	// if conversion failed
			m_sErrMsg.Format(IDS_CLA_LOGGING_INVALID, sParam.GetString());
			OnError(m_sErrMsg);
		}
	}
}

void CParamParser::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	UNREFERENCED_PARAMETER(bLast);
	if (bFlag) {	// if caller passed us a flag 
		// integer parameters can be negative, but caller eats dashes
		switch (m_iFlag) {	// if processing one of the following flags
		case F_offset:
			if (pszParam[-1] == '-') {	// if previous character is dash
				pszParam--;	// assume dash is a negative sign and include it in parameter
				goto ParseParamEval;	// go evaluate parameter; dubious but it works
			}
			break;
		}
		if (m_iFlag >= 0) {	// if we're expecting an argument, it's missing
			m_sErrMsg.Format(IDS_CLA_MISSING_ARGUMENT, m_arrFlagName[m_iFlag]);
			OnError(m_sErrMsg);
		}
		CString	sParam(pszParam);
		sParam.MakeLower();	// ignore case in flag names
		int	iFlag = ArrayFind(m_arrFlagName, FLAGS, sParam);	// find flag in table
		switch (iFlag) {	// these flags don't take an argument
		case F_help:
			m_nAction = ACT_SHOW_HELP;
			break;
		case F_license:
			m_nAction = ACT_SHOW_LICENSE;
			break;
		case F_frenched:
			m_bFrenched = true;
			break;
		case F_verify:
			m_bVerify = true;
			break;
		default:
			// flag expects an argument
			if (bLast) {	// if last parameter, flag's argument is missing
				m_sErrMsg.Format(IDS_CLA_MISSING_ARGUMENT, pszParam);
				OnError(m_sErrMsg);
			}
			if (iFlag >= 0) {	// if flag found in table
				m_iFlag = iFlag;	// next parameter should be flag's argument
			} else {	// flag not found in table
				m_iFlag = FLAG_NONE;
				m_sErrMsg.Format(IDS_CLA_UNKNOWN_FLAG, pszParam);
				OnError(m_sErrMsg);
			}
		}
	} else {	// caller passed us a parameter
ParseParamEval:
		switch (m_iFlag) {
		case F_output:
			m_sOutput = pszParam;
			break;
		case F_title:
			m_sTitle = pszParam;
			break;
		case F_composer:
			m_sComposer = pszParam;
			break;
		case F_copyright:
			m_sCopyright = pszParam;
			break;
		case F_quant:
			OnQuant(pszParam, m_nQuantDenom);
			break;
		case F_triplet:
			OnQuant(pszParam, m_nTripletQuantDenom);
			break;
		case F_offset:
			OnOffset(pszParam);
			break;
		case F_clef:
			OnClef(pszParam);
			break;
		case F_section:
			OnSection(pszParam);
			break;
		case F_ottava:
			OnOttava(pszParam);
			break;
		case F_logging:
			OnLogging(pszParam);
			break;
		}
		m_iFlag = FLAG_NONE;	// flag is no longer pending
	}
}

bool CParamParser::ParseCommandLine()
{
	theApp.ParseCommandLine(*this);
	switch (m_nAction) {
	case ACT_SHOW_HELP:
		ShowHelp();
		return false;
	case ACT_SHOW_LICENSE:
		ShowLicense();
		return false;
	}
	return true;	// continue processing
}

void CParamParser::BreakIntoLines(CString sText, CStringArrayEx& arrLine, int nMaxLine)
{
	arrLine.RemoveAll();	// empty caller's array just in case
	CString	sOutLine;
	CString	sWord;
	int	iStart = 0;
	while (!(sWord = sText.Tokenize(_T(" "), iStart)).IsEmpty()) {	// while words remain
		int	nWordLen = sWord.GetLength();
		if (!sOutLine.IsEmpty()) {	// if current line isn't empty
			nWordLen++;
		}
		if (sOutLine.GetLength() + nWordLen >= nMaxLine) {	// if word doesn't fit
			arrLine.Add(sOutLine);	// add finished line to caller's array
			sOutLine.Empty();	// empty current line 
		}
		if (!sOutLine.IsEmpty()) {	// if current line isn't empty
			sOutLine += ' ';	// add separator to current line
		}
		sOutLine += sWord;	// add word to current line
	}
	if (!sOutLine.IsEmpty()) {	// if current line isn't empty
		arrLine.Add(sOutLine);	// add final line to caller's array
	}
}

void CParamParser::ShowParamHelp(LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sHelp(LDS(nParamHelpResID));
	CString	sParam(pszParamName);
	bool	bHaveParamName = !sParam.IsEmpty();
	if (bHaveParamName) {	// if parameter name was specified
		sParam.Insert(0, '-');	// insert parameter prefix
	}
	// parameter's help may specify an argument enclosed in square brackets
	if (sHelp[0] == '[') {	// if parameter help specifies an argument
		int	iDelimiter = sHelp.Find(']', 1);	// find closing bracket
		if (iDelimiter >= 0) {	// if argument found
			CString	sArg;
			sArg = sHelp.Mid(1, iDelimiter - 1);	// extract argument
			if (bArgumentUpperCase)
				sArg.MakeUpper();	// make argument upper case
			if (bHaveParamName) {	// if parameter name was specified
				sParam += ' ';	// add separator
			}
			sParam += sArg;	// concatenate argument to parameter name
			sHelp = sHelp.Mid(iDelimiter + 2);	// remove argument from help string
		}
	}
	CStringArrayEx	arrLine;
	const int	nMaxLine = 80;
	const int	nMaxParam = 19;
	BreakIntoLines(sHelp, arrLine, nMaxLine - nMaxParam - 1);
	for (int iLine = 0; iLine < arrLine.GetSize(); iLine++) {	// for each line of help text
		// field length is negative so that parameter is left-adjusted
		_tprintf(_T("%*s %s\n"), -nMaxParam, sParam.GetString(), arrLine[iLine].GetString());
		sParam.Empty();	// so continuation lines don't repeat parameter
	}
}

CString	CParamParser::GetAppVersionString()
{
	VS_FIXEDFILEINFO	infoApp;
	CVersionInfo::GetFileInfo(infoApp, NULL);
	CString	sVersion;
	sVersion.Format(_T("%d.%d.%d.%d"), 
		HIWORD(infoApp.dwFileVersionMS), LOWORD(infoApp.dwFileVersionMS),
		HIWORD(infoApp.dwFileVersionLS), LOWORD(infoApp.dwFileVersionLS));
	return sVersion;
}

void CParamParser::ShowAppVersion()
{
	CString	sVersionLine(theApp.m_pszAppName + CString(' ') + GetAppVersionString());
	_putts(sVersionLine.GetString());
}

void CParamParser::ShowHelp()
{
	ShowAppVersion();
	CString	sHelpUsage(LDS(IDS_HELP_USAGE) + '\n');
	_putts(sHelpUsage.GetString());
	ShowParamHelp(_T(""), IDS_HELP_PARAM_path);	// show path help
	#define PARAMDEF(name) ShowParamHelp(_T(#name), IDS_HELP_PARAM_##name);
	#include "ParamDef.h"	// use preprocessor to generate help for each flag
	CString	sExampleHead('\n' + LDS(IDS_HELP_EXAMPLES));
	_putts(sExampleHead.GetString());	// show some examples too
	ShowParamHelp(_T(""), IDS_EXAMPLE_QUANT, false);	// don't change argument's case
	ShowParamHelp(_T(""), IDS_EXAMPLE_SECTION, false);
	ShowParamHelp(_T(""), IDS_EXAMPLE_CLEF, false);
	ShowParamHelp(_T(""), IDS_EXAMPLE_OTTAVA, false);
}

void CParamParser::ShowLicense()
{	
	ShowAppVersion();
	CString	sCopyright(m_pszCopyrightNotice);
	sCopyright += '\n';
	_putts(sCopyright.GetString());
	CString	sLicense(LDS(IDS_LICENSE));
	CStringArrayEx	arrLine;
	BreakIntoLines(sLicense, arrLine);
	for (int iLine = 0; iLine < arrLine.GetSize(); iLine++) {
		_putts(arrLine[iLine]);
	}
}

