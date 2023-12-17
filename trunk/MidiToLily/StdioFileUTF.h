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

#pragma once

class CStdioFileUTF : public CStdioFile {
public:
	CStdioFileUTF() {}
	CStdioFileUTF(LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = NULL);
};
