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
#include "Note.h"

const int CNote::m_NoteNameTbl[TONALITIES][NOTES][NOTES] = {
	{	// major
		{	C,	Db, D,	Eb,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	// C
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// Db
		{	C,	Cs, D,	Eb,	E,	F,	Fs,	G,	Gs,	A,	Bb,	B	},	// D
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// Eb
		{	C,	Cs, D,	Ds,	E,	F,	Fs,	G,	Gs,	A,	As,	B	},	// E
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// F
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	Cf	},	// Gb
		{	C,	Cs, D,	Eb,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	// G
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// Ab
		{	C,	Cs, D,	Ds,	E,	F,	Fs,	G,	Gs,	A,	Bb,	B	},	// A
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// Bb
		{	C,	Cs, D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	// B
	},
	{	// minor
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// C-
		{	Bs,	Cs, D,	Ds,	E,	F,	Fs,	G,	Gs,	A,	As,	B	},	// C#-
		{	C,	Cs, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// D-
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	Cf	},	// Eb-
		{	C,	Cs, D,	Ds,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	// E-
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// F-
		{	C,	Cs, D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	Bb,	B	},	// F#-
		{	C,	Db, D,	Eb,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	// G-
		{	C,	Db, D,	Eb,	Ff,	F,	Gb,	G,	Ab,	A,	Bb,	Cf	},	// Ab-
		{	C,	Db, D,	Eb,	E,	F,	Fs,	G,	Gs,	A,	Bb,	B	},	// A-
		{	C,	Db, D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// Bb-
		{	C,	Cs, D,	Eb,	E,	F,	Fs,	G,	Gs,	A,	As,	B	},	// B-
	},
};

const LPCTSTR CNote::m_NoteNameStr[NOTE_NAMES] = {
	_T("C"),  _T("Db"), _T("D"),  _T("Eb"), _T("E"),  _T("F"), 
	_T("Gb"), _T("G"),  _T("Ab"), _T("A"),  _T("Bb"), _T("B"), 
	_T("F#"), _T("C#"), _T("G#"), _T("D#"), _T("A#"), _T("E#"), _T("B#"),
	_T("Fb"), _T("Cb")
};

const LPCTSTR CNote::m_LilyNoteNameStr[NOTE_NAMES] = {
	_T("c"),  _T("df"), _T("d"),  _T("ef"), _T("e"),  _T("f"), 
	_T("gf"), _T("g"),  _T("af"), _T("a"),  _T("bf"), _T("b"), 
	_T("fs"), _T("cs"), _T("gs"), _T("ds"), _T("as"), _T("es"), _T("bs"),
	_T("ff"), _T("cf")
};

LPCTSTR CNote::GetNoteName(int nNote, int nKey, int nTonality)
{
	ASSERT(nNote >= 0 && nNote < NOTES);
	ASSERT(nKey >= 0 && nKey < NOTES);
	ASSERT(nTonality >= 0 && nTonality < TONALITIES);
	int	iName = m_NoteNameTbl[nTonality][nKey][nNote];
	return m_NoteNameStr[iName];
}

LPCTSTR CNote::GetLilyNoteName(int nNote, int nKey, int nTonality)
{
	ASSERT(nNote >= 0 && nNote < NOTES);
	ASSERT(nKey >= 0 && nKey < NOTES);
	ASSERT(nTonality >= 0 && nTonality < TONALITIES);
	int	iName = m_NoteNameTbl[nTonality][nKey][nNote];
	return m_LilyNoteNameStr[iName];
}

