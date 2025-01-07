// Copyleft 2023 Chris Korda
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or any later version.
/*
        chris korda
 
		revision history:
		rev		date	comments
        00		08dec23	initial version
		01		29dec24	add method for note name with octave
		02		07jan25	refactor for non-enharmonic keys
 
*/

#pragma once

enum {	// chromatic scale
	C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B, NOTES,
	KEYS = 15,
	DIATONES = 7,
};

class CNote {
public:
	enum {	// tonalities
		MAJOR,
		MINOR,
		TONALITIES
	};
	enum {	// special sharps and flats for note name table
		Fs = NOTES, Cs, Gs, Ds, As, Es, Bs, Cb, Fb, NOTE_NAMES
	};
	static LPCTSTR	GetNoteName(int nNote, int nKey = 0, int nTonality = MAJOR);
	static LPCTSTR	GetLilyNoteName(int nNote, int nKey = 0, int nTonality = MAJOR);
	static CString	GetMidiName(int nNote, int nKey = 0, int nTonality = MAJOR);
	static LPCTSTR	GetKeyName(int nKey, int nTonality = MAJOR);
	static LPCTSTR	GetLilyKeyName(int nKey, int nTonality = MAJOR);
	static LPCTSTR	GetNoteNameByIdx(int iNoteName);
	static LPCTSTR	GetLilyNoteNameByIdx(int iNoteName);
	static int	GetNoteNameIdx(int nNote, int nKey = 0, int nTonality = MAJOR);
	static int	GetKeyIdx(LPCTSTR pszNote, int nTonality = MAJOR);
	static int	GetLilyKeyIdx(LPCTSTR pszLilyNote, int nTonality = MAJOR);
	static bool	IsValidNote(int nNote);
	static bool	IsValidNoteName(int nNoteName);
	static bool	IsValidKey(int nNote);
	static bool	IsValidTonality(int nTonality);
	static int	Wrap(int Val, int Modulo);
	class CTester {
	public:
		CTester();
		static void	MakeNoteTable(LPCTSTR pszOutPath);
		static bool	TestKeys();
	};

protected:
	static const BYTE	m_arrNoteIdx[TONALITIES][KEYS][NOTES];
	static const LPCTSTR	m_arrNoteName[NOTE_NAMES];
	static const LPCTSTR	m_arrLilyNoteName[NOTE_NAMES];
	static const int	m_arrKeyIdx[TONALITIES][NOTE_NAMES];
	static const LPCTSTR	m_arrTonalityName[TONALITIES];
	static const int	m_arrKeyShift[TONALITIES];
	static int	GetKeyNote(int nKey, int nTonality);
};

inline LPCTSTR CNote::GetNoteNameByIdx(int iNoteName)
{
	ASSERT(IsValidNoteName(iNoteName));
	return m_arrNoteName[iNoteName];
}

inline LPCTSTR CNote::GetLilyNoteNameByIdx(int iNoteName)
{
	ASSERT(IsValidNoteName(iNoteName));
	return m_arrLilyNoteName[iNoteName];
}

inline bool CNote::IsValidNote(int nNote)
{
	return nNote >= 0 && nNote < NOTES;
}

inline bool	CNote::IsValidNoteName(int nNoteName)
{
	return nNoteName >= 0 && nNoteName < NOTE_NAMES;
}

inline bool CNote::IsValidKey(int nKey)
{
	return nKey >= -DIATONES && nKey <= DIATONES;
}

inline bool CNote::IsValidTonality(int nTonality)
{
	return nTonality >= 0 && nTonality < TONALITIES;
}

inline int CNote::Wrap(int Val, int Modulo)
{
	Val %= Modulo;
	if (Val < 0)
		Val = Val + Modulo;
	return(Val);
}
