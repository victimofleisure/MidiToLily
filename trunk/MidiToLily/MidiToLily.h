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

#include "resource.h"
#include "ArrayEx.h"
#include "Midi.h"
#include "MidiFile.h"
#include "BoundArray.h"

class CMidiToLily {
public:
// Construction
	CMidiToLily();

// Types
	class CParams {
	public:
		CParams();
		class COttava {
		public:
			int		m_nShift;		// signed number of octaves to shift, or zero to reset
			int		m_nMeasure;		// measure number; used during command line parsing
			int		m_nBeat;		// beat number; used during command line parsing
			int		m_nTick;		// time in ticks at which octave shift should occur 
			bool	operator>(const COttava& ot) const;
			bool	operator<(const COttava& ot) const;
		};
		typedef CArrayEx<COttava, COttava&> COttavaArray;
		typedef CArrayEx<COttavaArray, COttavaArray&> COttavaArrayArray;
		CString	m_sOutput;		// path of output file
		CString	m_sTitle;		// optional title string
		CString	m_sComposer;	// optional composer string
		CString	m_sCopyright;	// optional copyright string
		bool	m_bFrenched;	// true if hiding empty staves
		bool	m_bVerify;		// true if verifying MIDI file
		int		m_nOffset;		// offset in ticks added to all events
		int		m_nQuantDenom;	// regular quantization denominator as a power of two, or zero if none
		int		m_nTripletQuantDenom;	// triplet quantization denominator as a power of two, or zero if none
		UINT	m_nLoggingMask;	// bitmask of enabled logging types; see enum below
		CStringArrayEx	m_arrClef;	// array of clef overrides; one item per track, omitting trailing empty clefs
		CIntArrayEx	m_arrSection;	// array of sections; each item is a zero-based measure index
		COttavaArrayArray	m_arrOttavaArray;	// one array of ottava arrays per track
		void	Finalize(WORD nTimebase, int nMeter);
		void	Log() const;
	};

// Constants
	enum {	// define logging types
		#define LOGGINGTYPEDEF(name) LOG_##name,
		#include "ParamDef.h"	// use preprocessor to generate enum
	};
	enum {	// define a unique bit for each logging type
		#define LOGGINGTYPEDEF(name) LOGBIT_##name = 1 << LOG_##name,
		#include "ParamDef.h"	// use preprocessor to generate enum
	};

// Attributes
	int		GetTrackCount() const;
	WORD	GetTimebase() const;
	void	GetParams(CParams& params) const;
	void	SetParams(const CParams& params);

// Operations
	static	void	OnError(CString sErrMsg);
	void	ReadMidiFile(LPCTSTR pszMidiFilePath);
	void	RemoveOverlaps();
	void	LogEvents() const;
	void	DumpEvents(LPCTSTR pszPath, int nVelocityOverride = 0) const;
	void	WriteLily(LPCTSTR pszLilyFilePath);
	void	VerifyLilyMidi(LPCTSTR pszLilyMidiFilePath);

protected:
// Types
	class CMyException : public CException {
	public:
		CMyException(CString sErrMsg) : m_sErrMsg(sErrMsg) {}
		CString	m_sErrMsg;
		virtual BOOL GetErrorMessage(LPTSTR lpszError, UINT nMaxError, PUINT pnHelpContext = NULL) const;
	};
	class CMidiEvent {
	public:
		DWORD	m_dwTime;	// absolute time in ticks
		DWORD	m_dwMsg;	// MIDI message
		int		m_nDur;		// duration in ticks, for note events only
	};
	typedef CArrayEx<CMidiEvent, CMidiEvent&> CMidiEventArray;
	class CTrack {
	public:
		CTrack();
		CMidiEventArray	m_arrEvent;	// array of MIDI events
		CString	m_sClef;
	};
	typedef CArrayEx<CTrack, CTrack&> CTrackArray;
	enum {
		OCTAVE = 12		// semitones
	};
	typedef CBoundArray<int, OCTAVE> CNoteArray;
	class CItem {
	public:
		CItem();
		CItem(int nDur, int nNote = -1, bool bIsTied = false);
		CItem(int nDur, const CNoteArray& arrNote, bool bIsTied = false);
		int		m_nDur;			// duration in ticks
		CNoteArray	m_arrNote;	// array of MIDI notes, or -1 for rest
		bool	m_bIsTied;		// true if tied
		bool	m_bIsTuplet;	// true if tuplet member
		BYTE	m_nDots;		// number of dots
		WORD	m_nLilyDur;		// LilyPond duration; 1 = whole, 2 = half, 4 = quarter, etc.
		bool	IsRest() const;
		void	Reset();
	};
	typedef CArrayEx<CItem, CItem&> CItemArray;

// Constants
	enum {	// define clefs
		CLEF_BASS,
		CLEF_TREBLE,
		CLEFS
	};
	static const LPCTSTR	m_arrClefName[CLEFS];

// Member data
	CTrackArray	m_arrTrack;	// array of tracks
	CStringArrayEx	m_arrTrackName;	// array of track names
	WORD	m_nTimebase;	// timebase in ticks
	int		m_nTempo;		// tempo in beats per minute
	int		m_nKeySig;		// key signature, as note ranging from 0 to 11
	int		m_nMeter;		// time signature's numerator; denominator is 4
	int		m_nEndTime;		// end time of longest track
	int		m_iTrack;		// index of current track
	int		m_iMeasure;		// index of current measure
	int		m_iBeat;		// index of current beat
	int		m_iSection;		// index of next item in section array
	int		m_iOttava;		// index of next item in ottava array
	int		m_nQuantDur;	// regular quantization duration in ticks, or zero if none
	int		m_nTripletQuantDur;	// triplet quantization duration in ticks, or zero if none
	CParams	m_params;		// command-line parameters

// Helpers
	bool	IsLogging() const;
	bool	IsLogging(int iLoggingType) const;
	void	OnMidiFileRead(CMidiFile::CMidiTrackArray& arrTrack);
	static	int		CalcQuantError(int nTime, int nQuant);
	void	PrepareMidiEvents(const CMidiFile::CMidiEventArray& arrInEvent, int iTrack);
	int		GetQuantDuration(int nQuantDenom, int nWholeNoteDur);
	static	CString	GetTrackVarName(int iTrack);
	CString	FormatItem(const CItem& item, bool bPrevItemTied = false) const;
	int		GetDuration(int nDur, int& nDots) const;
	void	LogNotes(int iTrack) const;
	void	LogMeasure(const CItemArray& arrMeasure, bool bPrevMeasureTied) const;
	bool	IsWholeMeasureRest(const CItemArray& arrMeasure) const;
	int		GetTiedDuration(const CItemArray& arrMeasure, int iStartItem) const;
	void	CalcDurations(CItemArray& arrMeasure) const;
	void	ConsolidateItems(CItemArray& arrMeasure) const;
	void	AddScheduledItems(CString& sMeasure, int nTime);
	void	FormatMeasure(CString& sMeasure, const CItemArray& arrMeasure, bool bPrevMeasureTied);
	void	WriteTrack(CStdioFile& fLily, int iTrack);
	void	WriteBookHeader(CStdioFile& fLily);
	void	WriteTrackHeader(CStdioFile& fLily, int iTrack);
	CString	GetClefString(int iTrack) const;
};

inline bool CMidiToLily::CParams::COttava::operator>(const COttava& ot) const
{
	return m_nTick > ot.m_nTick;
}

inline bool CMidiToLily::CParams::COttava::operator<(const COttava& ot) const
{
	return m_nTick < ot.m_nTick;
}

inline bool CMidiToLily::IsLogging() const
{
	return m_params.m_nLoggingMask != 0;
}

inline bool CMidiToLily::IsLogging(int iLoggingType) const
{
	return (m_params.m_nLoggingMask & (1 << iLoggingType)) != 0;
}

inline int CMidiToLily::GetTrackCount() const
{
	return m_arrTrack.GetSize();
}

inline WORD CMidiToLily::GetTimebase() const
{
	return m_nTimebase;
}

inline void CMidiToLily::GetParams(CParams& params) const
{
	params = m_params;
}
