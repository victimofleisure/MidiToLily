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

#pragma once

enum {	// chromatic scale
	C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B, NOTES
};

class CNote {
public:
	enum {	// tonalities
		MAJOR,
		MINOR,
		TONALITIES
	};
	enum {	// special sharps and flats for note name table
		Fs = NOTES, Cs, Gs, Ds, As, Es, Bs, Ff, Cf, NOTE_NAMES
	};
	static LPCTSTR GetNoteName(int nNote, int nKey = 0, int nTonality = MAJOR);
	static LPCTSTR GetLilyNoteName(int nNote, int nKey = 0, int nTonality = MAJOR);
	static int Mod(int Val, int Modulo);

// protected
	static	const int	m_NoteNameTbl[TONALITIES][NOTES][NOTES];
	static	const LPCTSTR	m_NoteNameStr[NOTE_NAMES];
	static	const LPCTSTR	m_LilyNoteNameStr[NOTE_NAMES];
};

inline int CNote::Mod(int Val, int Modulo)
{
	Val %= Modulo;
	if (Val < 0)
		Val = Val + Modulo;
	return(Val);
}
