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

#include "stdafx.h"
#include "Note.h"

const BYTE CNote::m_arrNoteIdx[TONALITIES][KEYS][NOTES] = {	// generated via MakeNoteTable
	{	// major
		{	C,	Db,	D,	Eb,	Fb,	F,	Gb,	G,	Ab,	A,	Bb,	Cb	},	// -7 Cb
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	Cb	},	// -6 Gb
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -5 Db
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -4 Ab
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -3 Eb
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -2 Bb
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -1 F
		{	C,	Db,	D,	Eb,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	//  0 C
		{	C,	Cs,	D,	Eb,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	//  1 G
		{	C,	Cs,	D,	Eb,	E,	F,	Fs,	G,	Gs,	A,	Bb,	B	},	//  2 D
		{	C,	Cs,	D,	Ds,	E,	F,	Fs,	G,	Gs,	A,	Bb,	B	},	//  3 A
		{	C,	Cs,	D,	Ds,	E,	F,	Fs,	G,	Gs,	A,	As,	B	},	//  4 E
		{	C,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  5 B
		{	Bs,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  6 F#
		{	Bs,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  7 C#
	},
	{	// minor
		{	C,	Db,	D,	Eb,	Fb,	F,	Gb,	G,	Ab,	A,	Bb,	Cb	},	// -7 Ab
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	Cb	},	// -6 Eb
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -5 Bb
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -4 F
		{	C,	Db,	D,	Eb,	E,	F,	Gb,	G,	Ab,	A,	Bb,	B	},	// -3 C
		{	C,	Db,	D,	Eb,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	// -2 G
		{	C,	Cs,	D,	Eb,	E,	F,	Fs,	G,	Ab,	A,	Bb,	B	},	// -1 D
		{	C,	Cs,	D,	Eb,	E,	F,	Fs,	G,	Gs,	A,	Bb,	B	},	//  0 A
		{	C,	Cs,	D,	Ds,	E,	F,	Fs,	G,	Gs,	A,	Bb,	B	},	//  1 E
		{	C,	Cs,	D,	Ds,	E,	F,	Fs,	G,	Gs,	A,	As,	B	},	//  2 B
		{	C,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  3 F#
		{	Bs,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  4 C#
		{	Bs,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  5 G#
		{	Bs,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  6 D#
		{	Bs,	Cs,	D,	Ds,	E,	Es,	Fs,	G,	Gs,	A,	As,	B	},	//  7 A#
	},
};

const LPCTSTR CNote::m_arrNoteName[NOTE_NAMES] = {
	_T("C"),  _T("Db"), _T("D"),  _T("Eb"), _T("E"),  _T("F"), 
	_T("Gb"), _T("G"),  _T("Ab"), _T("A"),  _T("Bb"), _T("B"), 
	_T("F#"), _T("C#"), _T("G#"), _T("D#"), _T("A#"), _T("E#"), _T("B#"),	// sharps
	_T("Cb"), _T("Fb")	// extra flats
};

const LPCTSTR CNote::m_arrLilyNoteName[NOTE_NAMES] = {
	_T("c"),  _T("df"), _T("d"),  _T("ef"), _T("e"),  _T("f"), 
	_T("gf"), _T("g"),  _T("af"), _T("a"),  _T("bf"), _T("b"), 
	_T("fs"), _T("cs"), _T("gs"), _T("ds"), _T("as"), _T("es"), _T("bs"),	// sharps
	_T("cf"), _T("ff")	// extra flats
};

const int CNote::m_arrKeyIdx[TONALITIES][NOTE_NAMES] = {
// major
	//	C	Db	D	Eb	E	F	Gb	G	Ab	A	Bb	B	F#	C#	G#	D#	A#	E#	B#	Cb	Fb
	{	0,	-5,	2,	-3,	4,	-1,	-6,	1,	-4,	3,	-2,	5,	6,	7,	-4,	-3,	-2,	-1,	0,	-7,	4	},
// minor
	//	C	Db	D	Eb	E	F	Gb	G	Ab	A	Bb	B	F#	C#	G#	D#	A#	E#	B#	Cb	Fb
	{	-3,	4,	-1,	-6,	1,	-4,	3,	-2,	-7,	0,	-5,	2,	3,	4,	5,	6,	7,	-4,	-3,	2,	1	},
};

const LPCTSTR CNote::m_arrTonalityName[TONALITIES] = {_T("major"), _T("minor")};

const int CNote::m_arrKeyShift[TONALITIES] = {0, 3};

LPCTSTR CNote::GetNoteName(int nNote, int nKey, int nTonality)
{
	ASSERT(IsValidNote(nNote));
	ASSERT(IsValidKey(nKey));
	ASSERT(IsValidTonality(nTonality));
	int	iName = m_arrNoteIdx[nTonality][nKey + DIATONES][nNote];
	return m_arrNoteName[iName];
}

LPCTSTR CNote::GetLilyNoteName(int nNote, int nKey, int nTonality)
{
	ASSERT(IsValidNote(nNote));
	ASSERT(IsValidKey(nKey));
	ASSERT(IsValidTonality(nTonality));
	int	iName = m_arrNoteIdx[nTonality][nKey + DIATONES][nNote];
	return m_arrLilyNoteName[iName];
}

CString CNote::GetMidiName(int nNote, int nKey, int nTonality)
{
	ASSERT(IsValidKey(nKey));
	ASSERT(IsValidTonality(nTonality));
	// per MIDI 1.0 specification, middle C == 60 == C4
	CString	s;
	int	iName = m_arrNoteIdx[nTonality][nKey + DIATONES][nNote % 12];
	s.Format(_T("%s%d"), m_arrNoteName[iName], nNote / 12 - 1);
	return(s);
}

LPCTSTR CNote::GetKeyName(int nKey, int nTonality)
{
	ASSERT(IsValidTonality(nTonality));
	int	iNote = GetKeyNote(nKey, nTonality);
	int	iName = m_arrNoteIdx[nTonality][nKey + DIATONES][iNote];
	return m_arrNoteName[iName];
}

LPCTSTR CNote::GetLilyKeyName(int nKey, int nTonality)
{
	ASSERT(IsValidTonality(nTonality));
	int	iNote = GetKeyNote(nKey, nTonality);
	int	iName = m_arrNoteIdx[nTonality][nKey + DIATONES][iNote];
	return m_arrLilyNoteName[iName];
}

int CNote::GetNoteNameIdx(int nNote, int nKey, int nTonality)
{
	ASSERT(IsValidNote(nNote));
	ASSERT(IsValidKey(nKey));
	ASSERT(IsValidTonality(nTonality));
	return m_arrNoteIdx[nTonality][nKey + DIATONES][nNote];
}

inline int CNote::GetKeyNote(int nKey, int nTonality)
{
	return Wrap((nKey + m_arrKeyShift[nTonality]) * DIATONES, NOTES);
}

int	CNote::GetKeyIdx(LPCTSTR pszNote, int nTonality)
{
	ASSERT(IsValidTonality(nTonality));
	for (int iNote = 0; iNote < NOTE_NAMES; iNote++) {
		if (!_tcscmp(pszNote, m_arrNoteName[iNote])) {
			return m_arrKeyIdx[nTonality][iNote];
		}
	}
	return INT_MAX;	// key signature not found
}

int	CNote::GetLilyKeyIdx(LPCTSTR pszLilyNote, int nTonality)
{
	ASSERT(IsValidTonality(nTonality));
	for (int iNote = 0; iNote < NOTE_NAMES; iNote++) {
		if (!_tcscmp(pszLilyNote, m_arrLilyNoteName[iNote])) {
			return m_arrKeyIdx[nTonality][iNote];
		}
	}
	return INT_MAX;	// key signature not found
}

//static CNote::CTester	CNoteTester;	// uncomment this line to run CNote tests

CNote::CTester::CTester()
{
	MakeNoteTable(_T("NoteTable.txt"));	// write note table initialization code to text file
	TestKeys();	// check key signatures for consistency, via round trip test
}

void CNote::CTester::MakeNoteTable(LPCTSTR pszOutPath)
{
	static const int	arrFirstSharp[TONALITIES] = {0, -2};	// F# in keys C major and G minor
	CStdioFile	fOut(pszOutPath, CFile::modeCreate | CFile::modeWrite);	// create output text file
	for (int iTonality = 0; iTonality < TONALITIES; iTonality++) {	// for each tonality
		fOut.WriteString(_T("\t{\t//\t")	// write tonality's opening bracket and comment
			+ CString(m_arrTonalityName[iTonality]) + '\n');
		BYTE	arrNote[NOTES];	// receives each semitone's note name index (0 to NOTE_NAMES - 1)
		for (int iKey = -DIATONES; iKey <= DIATONES; iKey++) {	// for each key signature
			for (BYTE iSemi = 0; iSemi < NOTES; iSemi++) {	// for each semitone
				arrNote[iSemi] = iSemi;	// initialize to enharmonic note (no sharps, only flats)
			}
			int	iFirstSharp = arrFirstSharp[iTonality];	// first sharp differs for major versus minor
			if (iKey >= iFirstSharp) {	// if sharp key signature
				int	nSharps = min(iKey - iFirstSharp + 1, DIATONES);	// one extra for sharp four
				for (int iSharp = 0; iSharp < nSharps; iSharp++) {	// for each sharp
					int	iNote = Wrap(Gb + iSharp * DIATONES, NOTES);	// cycle of fifths from F#
					arrNote[iNote] = static_cast<BYTE>(NOTES + iSharp);	// see m_arrNoteName
				}
			} else {	// flat key signature
				const int	iFirstFlat = 5;	// skip first five flat keys as they're already correct
				int	nFlats = min(-iKey, DIATONES);	// flat keys have negative indices
				for (int iFlat = iFirstFlat; iFlat < nFlats; iFlat++) {	// for each flat
					int	iNote = Wrap(Bb - iFlat * DIATONES, NOTES);	// cycle of fourths from Bb
					// in m_arrNoteName, extra flats appear after enharmonic notes and sharps
					arrNote[iNote] = static_cast<BYTE>(NOTES + DIATONES + iFlat - iFirstFlat);
				}
			}
			fOut.WriteString(_T("\t\t{\t"));	// write key's opening bracket
			for (int iSemi = 0; iSemi < NOTES; iSemi++) {	// for each semitone
				int	iNoteName = arrNote[iSemi];	// get semitone's note name index
				ASSERT(iNoteName >= 0 && iNoteName < NOTE_NAMES);	// range-check note name index
				CString	sNote(m_arrNoteName[iNoteName]);	// get corresponding note name string
				sNote.Replace('#', 's');	// identifier can't contain # so replace with letter
				if (iSemi < NOTES - 1)	// for all but last semitone
					sNote += ',';	// append separator
				fOut.WriteString(sNote + _T("\t"));	// write semitone's note name
			}
			CString	sKey;
			sKey.Format(_T("%2d "), iKey);	// convert key index to string for comment
			fOut.WriteString(_T("},\t// ") + sKey	// write key's closing bracket and comment
				+ CString(GetKeyName(iKey, iTonality)) + '\n');
		}
		fOut.WriteString(_T("\t},\n"));		// write tonality's closing bracket
	}
}

bool CNote::CTester::TestKeys()
{
	for (int iTonality = 0; iTonality < TONALITIES; iTonality++) {	// for each tonality
		for (int iKey = -DIATONES; iKey <= DIATONES; iKey++) {	// for each key signature
			{
				// test regular key names
				LPCTSTR	pszKey = GetKeyName(iKey, iTonality);
				int	nKey2 = GetKeyIdx(pszKey, iTonality);
				if (nKey2 != iKey) {	// if key indices differ
					ASSERT(0);	// round trip test failed for regular key
					_tprintf(_T("key %s: %d != %d\n"), pszKey, nKey2, iKey);
					return false;
				}
			}
			{
				// test LilyPond key names too
				LPCTSTR	pszKey = GetLilyKeyName(iKey, iTonality);
				int	nKey2 = GetLilyKeyIdx(pszKey, iTonality);
				if (nKey2 != iKey) {	// if key indices differ
					ASSERT(0);	// round trip test failed for LilyPond key
					_tprintf(_T("lily key %s: %d != %d\n"), pszKey, nKey2, iKey);
					return false;
				}
			}
		}
	}
	return true;
}
