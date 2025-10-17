// Copyleft 2023 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		08dec23	initial version
		01		27dec24	add subtitle, opus, piece and staves params
		02		06jan25	add time and key signature params
		03		09jan25	add tempo param
		04		17sep25	add combine param
		05		17oct25	add cmd and block params
 
*/

#include "stdafx.h"
#include "ParamParser.h"
#include "MidiToLily.h"	// for throw exception method
#include "VersionInfo.h"
#include "StdioFileUTF.h"
#include "Note.h"

extern CWinApp theApp;

const LPCTSTR CParamParser::m_arrFlagName[FLAGS] = {
	#define PARAMDEF(name) _T(#name),
	#include "ParamDef.h"	// use preprocessor to generate array of flag names
};

const LPCTSTR CParamParser::m_pszCopyrightNotice = _T("Copyleft 2024 Chris Korda");

CParamParser::CParamParser()
{
	m_iFlag = FLAG_NONE;
	m_nAction = ACT_CONTINUE;
}

void CParamParser::OnError(CString m_sErrMsg)
{
	CMidiToLily::OnError(m_sErrMsg);
}

LPCTSTR CParamParser::GetCurFlagName() const
{
	ASSERT(m_iFlag >= 0 && m_iFlag < FLAGS);
	return m_arrFlagName[m_iFlag];
}

int CParamParser::FindSeparator(CString sToken, TCHAR cSeparator, int iStart)
{
	int	iSeparator = sToken.Find(cSeparator, iStart);	// find separator
	if (iSeparator < 0) {	// if separator not found
		m_sErrMsg.Format(IDS_CLA_NO_SEPARATOR, GetCurFlagName(), sToken.GetString(), cSeparator);
		OnError(m_sErrMsg);
	}
	return iSeparator;
}

bool CParamParser::ParseInt(CString sToken, int& nResult, int nMinVal, int nMaxVal) const
{
	int	nConvs = _stscanf_s(sToken.GetString(), _T("%d"), &nResult);
	return nConvs == 1 && nResult >= nMinVal && nResult <= nMaxVal;
}

int CParamParser::ParseTrackIndex(CString sToken)
{
	int	iTrack;
	if (!ParseInt(sToken, iTrack, 0, SHRT_MAX)) {	// if invalid track index
		m_sErrMsg.Format(IDS_CLA_BAD_TRACK_INDEX, GetCurFlagName(), sToken.GetString());
		OnError(m_sErrMsg);
	}
	return iTrack;
}

int CParamParser::ParseStaveNumber(CString sToken)
{
	int	nStave;
	if (!ParseInt(sToken, nStave, 1, SHRT_MAX)) {	// if invalid stave number
		m_sErrMsg.Format(IDS_CLA_BAD_STAVE_NUMBER, GetCurFlagName(), sToken.GetString());
		OnError(m_sErrMsg);
	}
	return nStave;
}

int CParamParser::ParseMeasureNumber(CString sToken)
{
	int	nMeasure;
	if (!ParseInt(sToken, nMeasure, 1)) {	// if invalid measure number
		m_sErrMsg.Format(IDS_CLA_BAD_MEASURE, GetCurFlagName(), sToken.GetString());
		OnError(m_sErrMsg);
	}
	return nMeasure;
}

void CParamParser::OnQuant(CString sParam, int& nDuration)
{
	int	nParam;
	if (!ParseInt(sParam, nParam, 1)) {	// if invalid duration
		m_sErrMsg.Format(IDS_CLA_QUANT_INVALID, sParam.GetString());
		OnError(m_sErrMsg);
	}
	if (!IsPowerOfTwo(nParam)) {	// if duration isn't a power of two
		m_sErrMsg.Format(IDS_CLA_QUANT_NOT_POWER, GetCurFlagName(), sParam.GetString());
		OnError(m_sErrMsg);
	}
	nDuration = nParam;
}

void CParamParser::OnOffset(CString sParam)
{
	if (!ParseInt(sParam, m_nOffset)) {	// if invalid offset
		m_sErrMsg.Format(IDS_CLA_OFFSET_INVALID, sParam.GetString());
		OnError(m_sErrMsg);
	}
}

void CParamParser::OnClef(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		int	iSeparator = FindSeparator(sToken, '=');	// find separator
		CString	sClef(sToken.Mid(iSeparator + 1));
		if (sClef.IsEmpty()) {	// if clef not specified
			m_sErrMsg.Format(IDS_CLA_CLEF_NO_NAME, sToken.GetString());
			OnError(m_sErrMsg);
		}
		int	iTrack = ParseTrackIndex(sToken);
		m_arrClef.SetAtGrow(iTrack, sClef);
	}
}

void CParamParser::OnSection(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		int	nMeasure = ParseMeasureNumber(sToken);
		m_arrSection.Add(nMeasure);
	}
}

void CParamParser::OnOttava(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		int	iSeparator = FindSeparator(sToken, '=');	// find separator
		CString	sOctave(sToken.Mid(iSeparator + 1));
		if (sOctave.IsEmpty()) {	// if octave shift not specified
			m_sErrMsg.Format(IDS_CLA_OTTAVA_NO_OCTAVE, sToken.GetString());
			OnError(m_sErrMsg);
		}
		COttava	ot;
		if (!ParseInt(sOctave, ot.m_nShift)) {	// if invalid octave shift
			m_sErrMsg.Format(IDS_CLA_OTTAVA_BAD_OCTAVE, sToken.GetString());
			OnError(m_sErrMsg);
		}
		int	iTimeSeparator = FindSeparator(sToken, '_');	// find time separator
		int	nTimeLen = iSeparator - iTimeSeparator - 1;
		if (nTimeLen < 1) {	// if time not specified
			m_sErrMsg.Format(IDS_CLA_OTTAVA_NO_TIME, sToken.GetString());
			OnError(m_sErrMsg);
		}
		CString	sTime(sToken.Mid(iTimeSeparator + 1, nTimeLen));
		if (!ot.Scan(sTime)) {	// if invalid MBT time
			m_sErrMsg.Format(IDS_CLA_OTTAVA_BAD_TIME_FORMAT, sToken.GetString());
			OnError(m_sErrMsg);
		}
		int	iTrack = ParseTrackIndex(sToken);
		int	nNewSize = max(m_arrOttavaArray.GetSize(), iTrack + 1);
		m_arrOttavaArray.SetSize(nNewSize);
		m_arrOttavaArray[iTrack].Add(ot);
	}
}

void CParamParser::OnStaves(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		int	iTrack = ParseTrackIndex(sToken);
		m_arrStave.Add(iTrack);
	}
}

void CParamParser::OnCombine(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		int	iSeparator = FindSeparator(sToken, '_');	// find separator
		// stave numbers are one-based, whereas stave indices are zero-based
		int	iStave0 = ParseStaveNumber(sToken.Left(iSeparator)) - 1;	// convert to index
		int	iStave1 = ParseStaveNumber(sToken.Mid(iSeparator + 1)) - 1;
		if (iStave0 == iStave1) {	// if stave is combined with itself
			m_sErrMsg.Format(IDS_CLA_BAD_STAVE_NUMBER, GetCurFlagName(), sToken.GetString());
			OnError(m_sErrMsg);
		}
		int	iStaveMax = max(iStave0, iStave1);
		int	nOldCombs = m_arrCombine.GetSize();
		int	nNewCombs = max(nOldCombs, iStaveMax + 1);
		m_arrCombine.FastSetSize(nNewCombs);
		for (int iComb = nOldCombs; iComb < nNewCombs; iComb++) {	// for each new element
			m_arrCombine[iComb] = -1;	// init to -1 indicating that stave isn't combined
		}
		m_arrCombine[iStave0] = iStave1;	// doubly-linked pair of combined staves
		m_arrCombine[iStave1] = iStave0;
	}
}

void CParamParser::OnTimeSignature(CString sParam)
{
	CString	sToken;
	int	iParamStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iParamStart)).IsEmpty()) {
		int	iTokenStart = 0;
		CString	sMeasure(sToken.Tokenize(_T("="), iTokenStart));
		CTimeSigChange	tsc;
		tsc.m_nMeasure = ParseMeasureNumber(sToken);
		CString	sTimeSig(sToken.Tokenize(_T(""), iTokenStart));
		if (!tsc.Scan(sTimeSig)) {	// if conversion failed or range error
			m_sErrMsg.Format(IDS_CLA_TIME_BAD_SIGNATURE, sToken.GetString());
			OnError(m_sErrMsg);
		}
		if (!IsPowerOfTwo(tsc.m_nDenom)) {	// if denominator isn't a power of two
			m_sErrMsg.Format(IDS_CLA_TIME_NOT_POWER, sToken.GetString());
			OnError(m_sErrMsg);
		}
		m_arrTimeSig.Add(tsc);
	}
}

void CParamParser::OnKeySignature(CString sParam)
{
	CString	sToken;
	int	iParamStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iParamStart)).IsEmpty()) {
		int	iTokenStart = 0;
		CString	sMeasure(sToken.Tokenize(_T("="), iTokenStart));
		CKeySigChange	ksc;
		ksc.m_nMeasure = ParseMeasureNumber(sToken);
		CString	sKeySig(sToken.Tokenize(_T(""), iTokenStart));
		CString	sOrigKeySig(sKeySig);
		if (sKeySig.GetLength() && sKeySig.Right(1) == 'm') {
			ksc.m_bIsMinor = true;
			sKeySig.Delete(sKeySig.GetLength() - 1);
		} else {
			ksc.m_bIsMinor = false;
		}
		ksc.m_nAccs = CNote::GetLilyKeyIdx(sKeySig, ksc.m_bIsMinor);
		if (ksc.m_nAccs == INT_MAX) {
			m_sErrMsg.Format(IDS_CLA_KEY_BAD_SIGNATURE, sOrigKeySig.GetString());
			OnError(m_sErrMsg);
		}
		m_arrKeySig.Add(ksc);
	}
}

void CParamParser::OnTempo(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		int	iSeparator = FindSeparator(sToken, '=');	// find separator
		CString	sTempo(sToken.Mid(iSeparator + 1));
		CTempoChange	tc;
		if (!tc.Scan(sTempo)) {
			m_sErrMsg.Format(IDS_CLA_KEY_BAD_TEMPO, sTempo.GetString());
			OnError(m_sErrMsg);
		}
		tc.m_nMeasure = ParseMeasureNumber(sToken);
		m_arrTempo.Add(tc);
	}
}

void CParamParser::OnCommand(CString sParam)
{
	CString	sToken;
	int	iStart = 0;
	while (!(sToken = sParam.Tokenize(_T(","), iStart)).IsEmpty()) {
		int	iSeparator = FindSeparator(sToken, '=');	// find separator
		CString	sCmd(sToken.Mid(iSeparator + 1));
		CSchedCmd	sc;
		sc.m_sCmd = sCmd;
		int	iTimeSeparator = FindSeparator(sToken, '_');	// find time separator
		int	nTimeLen = iSeparator - iTimeSeparator - 1;
		if (nTimeLen < 1) {	// if time not specified
			m_sErrMsg.Format(IDS_CLA_CMD_NO_TIME, sToken.GetString());
			OnError(m_sErrMsg);
		}
		CString	sTime(sToken.Mid(iTimeSeparator + 1, nTimeLen));
		if (!sc.Scan(sTime)) {	// if invalid MBT time
			m_sErrMsg.Format(IDS_CLA_CMD_BAD_TIME_FORMAT, sToken.GetString());
			OnError(m_sErrMsg);
		}
		int	iTrack = ParseTrackIndex(sToken);
		int	nNewSize = max(m_arrSchedCmdArray.GetSize(), iTrack + 1);
		m_arrSchedCmdArray.SetSize(nNewSize);
		m_arrSchedCmdArray[iTrack].Add(sc);
	}
}

void CParamParser::OnBlock(CString pszParam)
{
	m_arrBlock.Add(pszParam);
}

void CParamParser::OnParams(CString sParam)
{
	m_iFlag = FLAG_NONE;	// flag is no longer pending
	CStdioFileUTF	fIn(sParam, CFile::modeRead);
	CString	sIn;
	while (fIn.ReadString(sIn)) {
		CString	sToken;
		int	iStart = 0;
		while (!(sToken = sIn.Tokenize(_T(" "), iStart)).IsEmpty()) {
			LPCTSTR	pszToken = sToken;
			bool	bFlag = false;
			if (pszToken[0] == '-' || pszToken[0] == '/') {
				bFlag = TRUE;
				pszToken++;	// remove flag specifier
			}
			ParseParam(pszToken, bFlag, false);
		}
	}
}

void CParamParser::OnLogging(CString sParam)
{
	if (sParam == '*') {	// if parameter is wildcard
		m_nLoggingMask = UINT_MAX;	// set all bits
	} else if (sParam == '?') {
		for (int iType = 0; iType < CMidiToLily::LOGGING_TYPES; iType++) {
			_tprintf(_T("%s = 0x%x\n"), CMidiToLily::GetLoggingTypeName(iType), 1 << iType);
		}
		m_nAction = ACT_EXIT;
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
			m_sErrMsg.Format(IDS_CLA_MISSING_ARGUMENT, GetCurFlagName());
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
		case F_subtitle:
			m_sSubtitle = pszParam;
			break;
		case F_opus:
			m_sOpus = pszParam;
			break;
		case F_piece:
			m_sPiece = pszParam;
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
		case F_staves:
			OnStaves(pszParam);
			break;
		case F_combine:
			OnCombine(pszParam);
			break;
		case F_time:
			OnTimeSignature(pszParam);
			break;
		case F_key:
			OnKeySignature(pszParam);
			break;
		case F_tempo:
			OnTempo(pszParam);
			break;
		case F_cmd:
			OnCommand(pszParam);
			break;
		case F_block:
			OnBlock(pszParam);
			break;
		case F_params:
			OnParams(pszParam);
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
		break;
	case ACT_SHOW_LICENSE:
		ShowLicense();
		break;
	}
	return m_nAction == ACT_CONTINUE;
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

CString CParamParser::UnpackHelp(CString& sParam, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sHelp(LDS(nParamHelpResID));
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
	return sHelp;
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
	#define HELPEXAMPLEDEF(name) ShowParamHelp(_T(""), IDS_EXAMPLE_##name, false);
	#include "ParamDef.h"	// use preprocessor to generate help examples
}

void CParamParser::WriteHelpMarkdown(LPCTSTR pszOutputPath)
{
	CStdioFile	fOut(pszOutputPath, CFile::modeCreate | CFile::modeWrite);
	fOut.WriteString(_T("### ") + LDS(IDS_HELP_USAGE) + _T("\n\n"));
	fOut.WriteString(_T("|Option|Description|\n|---|---|\n"));
	WriteParamHelpMarkdown(fOut, _T(""), IDS_HELP_PARAM_path);	// show path help
	#define PARAMDEF(name) WriteParamHelpMarkdown(fOut, _T(#name), IDS_HELP_PARAM_##name);
	#include "ParamDef.h"	// use preprocessor to generate help for each flag
	fOut.WriteString(_T("\n### Examples\n\n"));
	fOut.WriteString(_T("|Example|Description|\n|---|---|\n"));
	#define HELPEXAMPLEDEF(name) WriteParamHelpMarkdown(fOut, _T(""), IDS_EXAMPLE_##name, false);
	#include "ParamDef.h"	// use preprocessor to generate help examples
}

void CParamParser::ShowParamHelp(LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sParam(pszParamName);
	CString	sHelp(UnpackHelp(sParam, nParamHelpResID, bArgumentUpperCase));
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

void CParamParser::WriteParamHelpMarkdown(CStdioFile& fOut, LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase)
{
	CString	sParam(pszParamName);
	CString	sHelp(UnpackHelp(sParam, nParamHelpResID, bArgumentUpperCase));
	sParam.Replace(_T(" "), _T("&nbsp;"));	// non-breaking space
	sParam.Replace(_T("-"), _T("&#8209;"));	// non-breaking hyphen
	fOut.WriteString('|' + sParam + '|' + sHelp + _T("|\n"));
}
