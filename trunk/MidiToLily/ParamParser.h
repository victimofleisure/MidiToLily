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
 
*/

#pragma once

#include "Params.h"

class CParamParser : public CCommandLineInfo, public CParams {
public:
// Construction
	CParamParser();

// Constants
	enum {	// define flags
		#define PARAMDEF(name) F_##name,
		#include "ParamDef.h"	// use preprocessor to generate flag enum
		FLAGS,
		FLAG_NONE = -1,			// invalid flag
	};

// Attributes
	static	LPCTSTR	GetFlagName(int iFlag);

// Operations
	bool	ParseCommandLine();
	static	CString	GetAppVersionString();
	static	void	ShowAppVersion();
	static	void	ShowLicense();
	static	void	ShowHelp();
	static	void	BreakIntoLines(CString sText, CStringArrayEx& arrLine, int nMaxLine = 80);
	static	void	WriteHelpMarkdown(LPCTSTR pszOutputPath);

// Overrides
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);

protected:
// Constants
	enum {	// define post-parsing actions
		ACT_CONTINUE,
		ACT_EXIT,
		ACT_SHOW_HELP,
		ACT_SHOW_LICENSE,
	};
	static const LPCTSTR m_arrFlagName[FLAGS];	// array of flag names
	static const LPCTSTR m_pszCopyrightNotice;	// application's copyright notice

// Member data
	int		m_iFlag;		// index of flag being processed, or -1 if none
	int		m_nAction;		// action to take after parsing; see enum above
	CString	m_sErrMsg;		// error message buffer

// Helpers
	static	void	OnError(CString sErrMsg);
	LPCTSTR	GetCurFlagName() const;
	int		FindSeparator(CString sToken, TCHAR cSeparator, int iStart = 0);
	bool	ParseInt(CString sToken, int& nResult, int nMinVal = INT_MIN, int nMaxVal = INT_MAX) const;
	int		ParseTrackIndex(CString sToken);
	int		ParseMeasureNumber(CString sToken);
	int		ParseStaveNumber(CString sToken);
	void	OnQuant(CString sParam, int& nDuration);
	void	OnOffset(CString sParam);
	void	OnClef(CString sParam);
	void	OnSection(CString sParam);
	void	OnOttava(CString sParam);
	void	OnStaves(CString sParam);
	void	OnCombine(CString sParam);
	void	OnTimeSignature(CString sParam);
	void	OnKeySignature(CString sParam);
	void	OnTempo(CString sParam);
	void	OnParams(CString sParam);
	void	OnLogging(CString sParam);
	static	CString	UnpackHelp(CString& sParam, int nParamHelpResID, bool bArgumentUpperCase = true);
	static	void	ShowParamHelp(LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase = true);
	static	void	WriteParamHelpMarkdown(CStdioFile& fOut, LPCTSTR pszParamName, int nParamHelpResID, bool bArgumentUpperCase = true);
};

inline LPCTSTR CParamParser::GetFlagName(int iFlag)
{
	ASSERT(iFlag >= 0 && iFlag < FLAGS);
	return m_arrFlagName[iFlag];
}
