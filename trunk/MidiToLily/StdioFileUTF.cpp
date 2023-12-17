// Copyleft 2023 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		29nov23	initial version
 
*/

#include "stdafx.h"
#include "StdioFileUTF.h"

CStdioFileUTF::CStdioFileUTF(LPCTSTR lpszFileName, UINT nOpenFlags)
{
	CommonInit(lpszFileName, nOpenFlags, NULL);
}

BOOL CStdioFileUTF::Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
	static const LPCTSTR szMode[] = {
		_T("rt,ccs=UTF-8"),
		_T("wt,ccs=UTF-8"),
	};
	bool	bIsWrite = (nOpenFlags & modeWrite) != 0;
	FILE	*pStream;
	errno_t	nErr =_tfopen_s(&pStream, lpszFileName, szMode[bIsWrite]);
	if (nErr != 0) {
		if (pError != NULL) {
			pError->m_cause = CFileException::ErrnoToException(nErr);
			pError->m_lOsError = nErr;
			pError->m_strFileName = lpszFileName;
		}
		return false;
	}
	m_pStream = pStream;
	return true;
}
