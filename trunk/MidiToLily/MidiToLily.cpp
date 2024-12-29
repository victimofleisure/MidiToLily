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
		02		29dec24	add logging of note overlaps
 
*/

// MidiToLily.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MidiToLily.h"
#include "StdioFileUTF.h"
#include "Note.h"
#include "ParamParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only application object

CWinApp theApp;

BOOL CMidiToLily::CMyException::GetErrorMessage(LPTSTR lpszError, UINT nMaxError, PUINT pnHelpContext) const
{
	UNREFERENCED_PARAMETER(pnHelpContext);
	_tcsncpy_s(lpszError, nMaxError, m_sErrMsg.GetString(), m_sErrMsg.GetLength());
	return TRUE;
}

void CMidiToLily::OnError(CString sErrMsg)
{
	THROW(new CMyException(sErrMsg));
}

inline void CMidiToLily::CItem::Reset()
{
	m_nDur = 0;
	m_bIsTied = false;
	m_bIsTuplet = false;
	m_nDots = 0;
	m_nLilyDur = 0;
	m_arrNote.RemoveAll();
}

inline CMidiToLily::CItem::CItem()
{
	Reset();
}

inline CMidiToLily::CItem::CItem(int nDur, int nNote, bool bIsTied)
{
	Reset();
	m_nDur = nDur;
	m_arrNote.Add(nNote);
	m_bIsTied = bIsTied;
}

inline CMidiToLily::CItem::CItem(int nDur, const CNoteArray& arrNote, bool bIsTied)
{
	Reset();
	m_nDur = nDur;
	m_arrNote = arrNote;
	m_bIsTied = bIsTied;
}

bool CMidiToLily::CItem::IsRest() const
{
	return m_arrNote[0] < 0;
}

inline CMidiToLily::CTrack::CTrack()
{
}

CMidiToLily::CParams::CParams()
{
	m_bFrenched = false;
	m_bVerify = false;
	m_nOffset = 0;
	m_nQuantDenom = 0;
	m_nTripletQuantDenom = 0;
	m_nLoggingMask = 0;
}

void CMidiToLily::CParams::Log() const
{
	_tprintf(_T("parameters:\n"));
	_tprintf(_T("Output = \"%s\"\n"), m_sOutput.GetString());
	_tprintf(_T("Title = \"%s\"\n"), m_sTitle.GetString());
	_tprintf(_T("Subtitle = \"%s\"\n"), m_sSubtitle.GetString());
	_tprintf(_T("Opus = \"%s\"\n"), m_sOpus.GetString());
	_tprintf(_T("Piece = \"%s\"\n"), m_sPiece.GetString());
	_tprintf(_T("Composer = \"%s\"\n"), m_sComposer.GetString());
	_tprintf(_T("Copyright = \"%s\"\n"), m_sCopyright.GetString());
	_tprintf(_T("Frenched = %d\n"), m_bFrenched);
	_tprintf(_T("Offset = %d\n"), m_nOffset);
	_tprintf(_T("Quant = %d\n"), m_nQuantDenom);
	_tprintf(_T("Triplet = %d\n"), m_nTripletQuantDenom);
	_tprintf(_T("Sections = %d\n"), m_arrSection.GetSize());
	for (int iSection = 0; iSection < m_arrSection.GetSize(); iSection++) {
		_tprintf(_T("Section %d = %d\n"), iSection, m_arrSection[iSection] + 1);
	}
	for (int iClef = 0; iClef < m_arrClef.GetSize(); iClef++) {
		_tprintf(_T("Track %d clef = \"%s\"\n"), iClef, m_arrClef[iClef].GetString());
	}
	for (int iTrack = 0; iTrack < m_arrOttavaArray.GetSize(); iTrack++) {
		const COttavaArray& arrOttava = m_arrOttavaArray[iTrack];
		_tprintf(_T("Track %d Ottavas = %d\n"), iTrack, arrOttava.GetSize());
		for (int iOttava = 0; iOttava < arrOttava.GetSize(); iOttava++) {
			const COttava& ot = arrOttava[iOttava];
			_tprintf(_T("Ottava %d = %d %d\n"), iOttava, ot.m_nTick, ot.m_nShift);
		}
	}
	for (int iStave = 0; iStave < m_arrStave.GetSize(); iStave++) {
		_tprintf(_T("Stave %d track = %d\n"), iStave, m_arrStave[iStave]);
	}
}

void CMidiToLily::CParams::Finalize(WORD nTimebase, int nMeter)
{
	ASSERT(nTimebase);
	ASSERT(nMeter);
	m_arrSection.Sort();	// sort sections in ascending order by bar number
	int	nTracks = m_arrOttavaArray.GetSize();
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each track
		CParams::COttavaArray&	arrOttava = m_arrOttavaArray[iTrack];
		int	nOttavas = arrOttava.GetSize();
		for (int iOttava = 0; iOttava < nOttavas; iOttava++) {	// for each ottava
			CParams::COttava&	ot = arrOttava[iOttava];
			// convert MBT time to scalar time in ticks
			ASSERT(ot.m_nMeasure > 0);	// meter is one-based
			ASSERT(ot.m_nBeat > 0);		// beat is one-based
			ASSERT(ot.m_nTick >= 0);	// tick is zero-based
			int	nTimeTicks = (ot.m_nMeasure - 1) * nTimebase * nMeter
				+ (ot.m_nBeat - 1) * nTimebase + ot.m_nTick;
			ot.m_nTick = nTimeTicks;	// overwrite m_nTick with time in ticks
		}
		arrOttava.Sort();	// sort ottavas in ascending order by time in ticks
	}
}

const LPCTSTR CMidiToLily::m_arrClefName[CLEFS] = {
	_T("F"),
	_T("G"),
};

const LPCTSTR CMidiToLily::m_arrLoggingTypeName[LOGGING_TYPES] = {
	#define LOGGINGTYPEDEF(name) _T(#name),
	#include "ParamDef.h"	// use preprocessor to generate array init
};

static const bool m_bWriteFine = true;	// in case we make it optional

CMidiToLily::CMidiToLily()
{
	m_nTimebase = 0;
	m_nTempo = 0;
	m_nKeySig = 0;
	m_nMeter = 0;
	m_nEndTime = 0;
	m_iTrack = 0;
	m_iMeasure = 0;
	m_iBeat = 0;
	m_iSection = 0;
	m_iOttava = 0;
	m_nQuantDur = 0;
	m_nTripletQuantDur = 0;
}

void CMidiToLily::SetParams(const CParams& params)
{
	m_params = params;
	if (IsLogging()) {
		CParamParser::ShowAppVersion();
	}
}

CString	CMidiToLily::GetTrackVarName(int iTrack)
{
	CString	sTrackNum;
	sTrackNum.Format(_T("%d"), iTrack);
	return _T("\"track") + sTrackNum + '\"';
}

void CMidiToLily::ReadMidiFile(LPCTSTR pszMidiFilePath)
{
	CMidiFile	fMidi(pszMidiFilePath, CFile::modeRead);
	CMidiFile::CMidiTrackArray	arrTrack;
	UINT	nTempo;
	CMidiFile::TIME_SIGNATURE	sigTime;
	CMidiFile::KEY_SIGNATURE	sigKey;
	if (IsLogging()) {
		_tprintf(_T("reading MIDI file \"%s\"\n"), pszMidiFilePath);
	}
	fMidi.ReadTracks(arrTrack, m_arrTrackName, m_nTimebase, &nTempo, &sigTime, &sigKey);
	m_nTempo = Round(CMidiFile::MICROS_PER_MINUTE / double(nTempo));	// LilyPond tempo is integer
	m_nKeySig = CNote::Mod(sigKey.SharpsOrFlats * 7, NOTES);
	m_nMeter = sigTime.Numerator;
	if (IsLogging(LOG_MIDI_FILE_INFO)) {
		_tprintf(_T("Tracks = %d\nTimebase = %d\nTempo = %d\nKeySig = %d\nMeter = %d\n"), 
			arrTrack.GetSize(), m_nTimebase, m_nTempo, m_nKeySig, m_nMeter);
	}
	OnMidiFileRead(arrTrack);
	int	nTracks = arrTrack.GetSize();
	m_arrTrack.SetSize(nTracks);
	m_nEndTime = 0;
	if (IsLogging(LOG_MIDI_MESSAGES)) {
		_tprintf(_T("input MIDI messages:\n"));
	}
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each input track
		PrepareMidiEvents(arrTrack[iTrack], iTrack);
		const CMidiEventArray&	arrEvent = m_arrTrack[iTrack].m_arrEvent;
		int	nEvents = arrEvent.GetSize();
		if (nEvents) {
			const CMidiEvent&	evt = arrEvent[nEvents - 1];
			int	nEndTime = evt.m_dwTime + evt.m_nDur;
			if (nEndTime > m_nEndTime) {
				m_nEndTime = nEndTime;
			}
		}
	}
	if (IsLogging(LOG_MIDI_FILE_INFO)) {
		_tprintf(_T("total duration = %d ticks\n"), m_nEndTime);
	}
}

inline int CMidiToLily::CalcQuantError(int nTime, int nQuant)
{
	return (nTime + nQuant / 2) % nQuant - nQuant / 2;
}

void CMidiToLily::PrepareMidiEvents(const CMidiFile::CMidiEventArray& arrInEvent, int iTrack)
{
	CMidiEventArray& arrOutEvent = m_arrTrack[iTrack].m_arrEvent;
	int	nEvents = arrInEvent.GetSize();
	int	arrNoteOnIdx[MIDI_NOTES];
	memset(arrNoteOnIdx, -1, MIDI_NOTES * sizeof(int));
	int	nCurTime = m_params.m_nOffset;
	int	iOutChan = -1;
	int	arrClefHits[CLEFS] = {0};
	int	nQuantDur = m_nQuantDur;	// invariant throughout event loop
	int	nTripletQuantDur = m_nTripletQuantDur;
	if (IsLogging(LOG_MIDI_MESSAGES)) {
		_tprintf(_T("track %d\n"), iTrack);
	}
	for (int iEvent = 0; iEvent < nEvents; iEvent++) {	// for each of track's events
		const CMidiFile::MIDI_EVENT&	evtIn = arrInEvent[iEvent];
		nCurTime += evtIn.DeltaT;	// bump current time by event's delta time
		if (IsLogging(LOG_MIDI_MESSAGES)) {
			_tprintf(_T("%d %d %x\n"), iEvent, nCurTime, evtIn.Msg);
		}
		DWORD	nMsg = evtIn.Msg;
		int iChan = MIDI_CHAN(nMsg);
		int	nCmd = MIDI_CMD(nMsg);
		switch (nCmd) {
		case NOTE_ON:
		case NOTE_OFF:
			{	// note events
				int nQuantError = 0;	// default is no quantization
				if (nQuantDur) {	// if quantizing to regular grid
					nQuantError = CalcQuantError(nCurTime, nQuantDur);
				}
				if (nTripletQuantDur) {	// if quantizing to triplet grid
					int nTripletQuantError = CalcQuantError(nCurTime, nTripletQuantDur);
					if (nQuantDur) {	// if also quantizing to regular grid
						if (abs(nTripletQuantError) < abs(nQuantError)) {	// if closer to triplet grid
							nQuantError = nTripletQuantError;	// quantize to triplet grid instead
						}
					} else {	// not quantizing to regular grid
						nQuantError = nTripletQuantError;	// quantize to triplet grid only
					}
				}
				int nNoteTime = nCurTime - nQuantError;	// apply quantization if any
				int iNote = MIDI_P1(nMsg);
				int	nVelo = MIDI_P2(nMsg);
				if (nCmd == NOTE_ON && nVelo) {	// if note on
					if (iOutChan < 0) {	// if output channel not set
						iOutChan = iChan;
					} else {	// output channel set
						if (iChan != iOutChan) {	// if output channel differs
							CString	sErrMsg;
							sErrMsg.Format(IDS_ERR_MULTIPLE_MIDI_CHANNELS, iTrack, iEvent);
							OnError(sErrMsg);
						}
					}
					if (arrNoteOnIdx[iNote] >= 0) {	// if note is already in progress
						CString	sErrMsg;
						sErrMsg.Format(IDS_ERR_OVERLAPPING_NOTE, iTrack, iEvent, iNote);
						OnError(sErrMsg);
					}
					if (nNoteTime < 0) {	// if negative time
						CString	sErrMsg;
						sErrMsg.Format(IDS_ERR_NEGATIVE_TIME, iTrack, iEvent, nNoteTime);
						OnError(sErrMsg);
					}
					CMidiEvent	evtOut;
					evtOut.m_dwTime = nNoteTime;
					evtOut.m_dwMsg = evtIn.Msg;
					evtOut.m_nDur = 0;	// duration is unknown until note off is located
					int	iNoteOnEvent = INT64TO32(arrOutEvent.Add(evtOut));
					arrNoteOnIdx[iNote] = iNoteOnEvent;	// store index of note on
					int	iClef = iNote < 60 ? CLEF_BASS : CLEF_TREBLE;
					arrClefHits[iClef]++;
				} else {	// note off
					if (arrNoteOnIdx[iNote] < 0) {	// if note isn't in progress
						CString	sErrMsg;
						sErrMsg.Format(IDS_ERR_UNEXPECTED_NOTE_OFF, iTrack, iEvent, iNote);
						OnError(sErrMsg);
					}
					int	iNoteOnEvent = arrNoteOnIdx[iNote];
					int	nDur = nNoteTime - arrOutEvent[iNoteOnEvent].m_dwTime;
					arrOutEvent[iNoteOnEvent].m_nDur = nDur;
					arrNoteOnIdx[iNote] = -1;	// reset note on index
				}
			}
			break;
		}
	}
	int	nMaxHits = 0;
	int	iBestClef = CLEF_TREBLE;
	for (int iClef = 0; iClef < CLEFS; iClef++) {	// for each clef
		if (arrClefHits[iClef] > nMaxHits) {
			nMaxHits = arrClefHits[iClef];
			iBestClef = iClef;
		}
	}
	m_arrTrack[iTrack].m_sClef = m_arrClefName[iBestClef];
}

void CMidiToLily::OnMidiFileRead(CMidiFile::CMidiTrackArray& arrTrack)
{
	int	nTracks = arrTrack.GetSize();	// only type 0 MIDI file is supported
	if (nTracks < 2) {	// at least two tracks, with tempo map in first track
		OnError(LDS(IDS_CLA_TOO_FEW_TRACKS));
	}
	int	nClefs = m_params.m_arrClef.GetSize();
	if (nClefs > nTracks) {	// if we have clefs for non-existent tracks
		LPCTSTR	pszFlagName = CParamParser::GetFlagName(CParamParser::F_clef);
		CString	sErrMsg;
		sErrMsg.Format(IDS_CLA_TRACK_INDEX_RANGE, nClefs - 1, pszFlagName);
		OnError(sErrMsg);
	}
	m_params.m_arrClef.SetSize(nTracks);	// force one clef per track
	int	nOttavas = m_params.m_arrOttavaArray.GetSize();
	if (nOttavas > nTracks) {	// if we have ottava arrays for non-existent tracks
		LPCTSTR	pszFlagName = CParamParser::GetFlagName(CParamParser::F_ottava);
		CString	sErrMsg;
		sErrMsg.Format(IDS_CLA_TRACK_INDEX_RANGE, nOttavas - 1, pszFlagName);
		OnError(sErrMsg);
	}
	int	nStaves = m_params.m_arrStave.GetSize();
	for (int iStave = 0; iStave < nStaves; iStave++) {	// for each stave
		int	iTrack = m_params.m_arrStave[iStave];
		if (iTrack < 1 || iTrack >= nTracks) {	// if stave's track index is out of range
			LPCTSTR	pszFlagName = CParamParser::GetFlagName(CParamParser::F_staves);
			CString	sErrMsg;
			sErrMsg.Format(IDS_CLA_TRACK_INDEX_RANGE, iTrack, pszFlagName);
			OnError(sErrMsg);
		}
	}
	m_params.m_arrOttavaArray.SetSize(nTracks);	// force one ottava array per track
	// convert quantization from power of two denominator to duration in ticks
	int	nWholeNoteDur = m_nTimebase * 4;
	m_nQuantDur = GetQuantDuration(m_params.m_nQuantDenom, nWholeNoteDur);
	int	nTripletWholeNoteDur = Round(nWholeNoteDur * (2.0 / 3.0));	// scale whole note for triplets
	m_nTripletQuantDur = GetQuantDuration(m_params.m_nTripletQuantDenom, nTripletWholeNoteDur); 
	m_params.Finalize(m_nTimebase, m_nMeter);	// finalize parameters, a crucial last step
	if (IsLogging(LOG_PARAMETERS)) {
		m_params.Log();
	}
}

int CMidiToLily::GetQuantDuration(int nQuantDenom, int nWholeNoteDur)
{
	int	nQuantDur = 0;
	if (nQuantDenom) {	// if denominator is specified
		if (nWholeNoteDur % nQuantDenom) {	// if whole note isn't evenly divisible by denominator
			CString	sErrMsg;
			sErrMsg.Format(IDS_ERR_QUANT_TOO_SMALL, nQuantDenom, m_nTimebase);
			OnError(sErrMsg);
		} else {	// no remainder, whole note is evenly divisible by denominator
			nQuantDur = nWholeNoteDur / nQuantDenom;	// quotient is quantization duration in ticks
		}
	}
	return nQuantDur;
}

void CMidiToLily::LogEvent(const CMidiEvent& evt) const
{
	_tprintf(_T("%d %x %s %d\n"), evt.m_dwTime, evt.m_dwMsg, GetMidiName(evt.m_dwMsg), evt.m_nDur);
}

void CMidiToLily::LogEvents() const
{
	int	nTracks = m_arrTrack.GetSize();
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each track
		const CTrack&	track = m_arrTrack[iTrack];
		int	nEvents = track.m_arrEvent.GetSize();
		for (int iEvent = 0; iEvent < nEvents; iEvent++) {	// for each of input track's events
			LogEvent(track.m_arrEvent[iEvent]);
		}
	}
}

void CMidiToLily::DumpEvents(LPCTSTR pszPath, int nVelocityOverride, bool bUseStaves) const
{
	CStdioFile	fText(pszPath, CFile::modeCreate | CFile::modeWrite);
	if (m_params.m_arrStave.IsEmpty()) {	// if no staves
		bUseStaves = false;	// override argument
	}
	int	nTracks;
	if (bUseStaves) {	// if iterating staves instead of tracks
		nTracks = m_params.m_arrStave.GetSize() + 1;	// account for tempo track
	} else {	// no staves
		nTracks = m_arrTrack.GetSize();
	}
	CString	sLine;
	for (int iTrack = 1; iTrack < nTracks; iTrack++) {	// for each track, excluding tempo track
		int	iSrcTrack;
		if (bUseStaves) {	// if iterating staves
			iSrcTrack = m_params.m_arrStave[iTrack - 1];	// map stave to track; account for tempo track
		} else {	// no staves
			iSrcTrack = iTrack;	// no mapping
		}
		const CTrack&	track = m_arrTrack[iSrcTrack];
		int	nEvents = track.m_arrEvent.GetSize();
		sLine.Format(_T("track %d events %d\n"), iTrack, nEvents);
		fText.WriteString(sLine);
		for (int iEvent = 0; iEvent < nEvents; iEvent++) {	// for each of input track's events
			const CMidiEvent& evt = track.m_arrEvent[iEvent];
			DWORD	nMsg = evt.m_dwMsg;
			if (nVelocityOverride) {
				if (MIDI_CMD(nMsg) == NOTE_ON && MIDI_P2(nMsg)) {
					nMsg &= 0xffff;	// zero velocity
					nMsg |= nVelocityOverride << 16;	// update velocity
				}
			}
			if (bUseStaves) {	// if iterating staves
				nMsg &= ~0xf;	// zero channel
				nMsg |= iTrack - 1;	// update channel to match sequential track order
			}
			int	nDur = evt.m_nDur;
			if (!nDur) {
				for (int iEvtDur = iEvent + 1; iEvtDur < nEvents; iEvtDur++) {
					const CMidiEvent& evtDur = track.m_arrEvent[iEvtDur];
					if (evtDur.m_dwTime != evt.m_dwTime) {
						break;
					}
					if (evtDur.m_nDur) {
						nDur = evtDur.m_nDur;
						break;
					}
				}
			}
			sLine.Format(_T("%5d %02x %5d\n"), evt.m_dwTime, nMsg, nDur);
			fText.WriteString(sLine);
		}
	}
}

void CMidiToLily::RemoveOverlaps()
{
	if (IsLogging()) {
		_tprintf(_T("removing note overlaps\n"));
	}
	int	nTracks = m_arrTrack.GetSize();
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each track
		CTrack&	track = m_arrTrack[iTrack];
		int	nEvents = track.m_arrEvent.GetSize();
		CMidiEventArray&	arrEvt = track.m_arrEvent;
		for (int iEvent = 0; iEvent < nEvents; iEvent++) {	// for each of input track's events
			CMidiEvent&	evt = arrEvt[iEvent];
			int	iNextEvent = iEvent + 1;
			if (iNextEvent < nEvents) {
				CMidiEvent&	evtNext = arrEvt[iNextEvent];
				DWORD	nNextEvtTime = evtNext.m_dwTime;
				if (evt.m_dwTime + evt.m_nDur > nNextEvtTime) {
					if (IsLogging()) {
						// if times and durations match, it will notate correctly as a chord, else log it
						if (evt.m_dwTime != evtNext.m_dwTime || evt.m_nDur != evtNext.m_nDur) {
							_tprintf(_T("track %d: %d %s %d overlaps %d %s %d\n"), iTrack, 
								evt.m_dwTime, GetMidiName(evt.m_dwMsg), evt.m_nDur,
								evtNext.m_dwTime, GetMidiName(evtNext.m_dwMsg), evtNext.m_nDur);
						}
					}
					evt.m_nDur = nNextEvtTime - evt.m_dwTime;
				}
			}
		}
	}
}

void CMidiToLily::LogNotes(int iTrack) const
{
	const CTrack&	track = m_arrTrack[iTrack];
	int	nEvents = track.m_arrEvent.GetSize();
	for (int iEvent = 0; iEvent < nEvents; iEvent++) {	// for each of input track's events
		const CMidiEvent& evt = track.m_arrEvent[iEvent];
		_tprintf(_T("%d "), MIDI_P1(evt.m_dwMsg));
	}
	_tprintf(_T("\n"));
}

CString	CMidiToLily::FormatItem(const CItem& item, bool bPrevItemTied) const
{
	ASSERT(!item.m_arrNote.IsEmpty());
	CString	sItem;
	if (!bPrevItemTied) {
		if (item.IsRest()) {
			sItem = _T("r");
		} else {
			int	nNotes = item.m_arrNote.GetSize();
			if (nNotes > 1) {
				sItem = '<';
			}
			for (int iNote = 0; iNote < nNotes; iNote++) {
				int	nNote = item.m_arrNote[iNote];
				ASSERT(nNote < MIDI_NOTES);
				int	nOctave = nNote / OCTAVE - 4;	// account for middle C
				int	nNoteNorm = nNote % OCTAVE;
				CString	sOctave;
				TCHAR	cOctave;
				if (nOctave > 0) {
					cOctave = '\'';
				} else {
					cOctave = ',';
				}
				int	nShifts = abs(nOctave);
				for (int iOctave = 0; iOctave < nShifts; iOctave++) {
					sOctave += cOctave;
				}
				if (iNote)
					sItem += ' ';
				sItem += CNote::GetLilyNoteName(nNoteNorm, m_nKeySig) + sOctave;
			}
			if (nNotes > 1) {
				sItem += '>';
			}
		}
	}
	CString	sTie(item.m_bIsTied ? _T("~") : _T(""));
	CString	sDur;
	sDur.Format(_T("%d"), item.m_nLilyDur);
	for (int iDot = 0; iDot < item.m_nDots; iDot++) {
		sDur += '.';
	}
	sItem += sDur + sTie + _T(" ");
	return sItem;
}

int CMidiToLily::GetDuration(int nDur, int& nDots) const
{
	ASSERT(nDur);
	nDots = 0;
	int	nDiv = 0; 
	int	nWhole = m_nTimebase * 4;	// start with whole note
	if (!(nWhole % nDur)) {	// if duration divides evenly
		return nWhole / nDur;	// most likely case
	}
	// duration not found above; try dotted values
	const int nMaxDots = 2;	// maximum number of dots we allow
	for (int iDot = 0; iDot <= nMaxDots; iDot++) {
		int	nDotWhole = nWhole - (m_nTimebase >> iDot);
		if (!(nDotWhole % nDur)) {	// if duration divides evenly
			nDots = iDot + 1;
			return nDiv = nDotWhole / nDur * 2;
		}
	}
	CString	sErrMsg;
	sErrMsg.Format(IDS_ERR_BAD_DURATION, m_iTrack, m_iMeasure + 1, nDur);
	OnError(sErrMsg);
	ASSERT(0);	// zero duration is an error in LilyPond
	return 0;	// duration not found; failure
}

void CMidiToLily::LogMeasure(const CItemArray& arrMeasure, bool bPrevMeasureTied) const
{
	int	nItems = arrMeasure.GetSize();
	_tprintf(_T("%d: "), m_iMeasure + 1);
	for (int iItem = 0; iItem < nItems; iItem++) {
		const CItem&	item = arrMeasure[iItem];
		ASSERT(!(item.IsRest() && item.m_bIsTied));	// rests can't be tied
		bool	bPrevItemTied;
		if (iItem > 0) {	// if not first item
			bPrevItemTied = arrMeasure[iItem - 1].m_bIsTied;
		} else {	// first item
			bPrevItemTied = bPrevMeasureTied;	// true if previous measure's last item was tied
		}
		if (item.m_bIsTuplet) {
			_fputtc('T', stdout);
		}
		_tprintf(_T("%s"), FormatItem(item, bPrevItemTied).GetString());
	}
	_fputtc('\n', stdout);
}

bool CMidiToLily::IsWholeMeasureRest(const CItemArray& arrMeasure) const
{
	int	nItems = arrMeasure.GetSize();
	for (int iItem = 0; iItem < nItems; iItem++) {
		if (!arrMeasure[iItem].IsRest())
			return false;
	}
	return true;
}

int CMidiToLily::GetTiedDuration(const CItemArray& arrMeasure, int iStartItem) const
{
	// this method works on rests as well as tied notes
	int	nItems = arrMeasure.GetSize();
	CItem	itemInit = arrMeasure[iStartItem];
	bool	bInitItemIsNote = !arrMeasure[iStartItem].IsRest();
	int	nTotalDur = itemInit.m_nDur;
	for (int iItem = iStartItem + 1; iItem < nItems; iItem++) {	// for each item
		const CItem&	item = arrMeasure[iItem];
		// if item's duration or note differs from initial item's values
		if (item.m_nDur != itemInit.m_nDur || item.m_arrNote != itemInit.m_arrNote)
			break;
		nTotalDur += item.m_nDur;
		if (bInitItemIsNote && !item.m_bIsTied)	// if we're iterating notes, and item isn't tied
			break;
	}
	return min(nTotalDur, m_nTimebase * 4);	// limit to whole note, LilyPond's maximum duration
}

void CMidiToLily::CalcDurations(CItemArray& arrMeasure) const
{
	int	nItems = arrMeasure.GetSize();
	for (int iItem = 0; iItem < nItems; iItem++) {	// for each item in measure
		CItem&	item = arrMeasure[iItem];
		if (item.m_nDur) {
			int	nDots;
			int	nDur = GetDuration(item.m_nDur, nDots);
			if (!(nDur % 3)) {	// if triplet duration
				if (nDots) {
					CString	sErrMsg;
					sErrMsg.Format(IDS_ERR_DOT_WITHIN_TRIPLET, m_iTrack, m_iMeasure + 1);
					OnError(sErrMsg);
				}
				nDur = Round(nDur * (2.0 / 3.0));
				item.m_bIsTuplet = true;
			}
			item.m_nLilyDur = static_cast<WORD>(nDur);
			item.m_nDots = static_cast<BYTE>(nDots);
		}
	}
}

void CMidiToLily::ConsolidateItems(CItemArray& arrMeasure) const
{
	int	nPos = 0;
	int	iItem = 0;
	while (iItem < arrMeasure.GetSize()) {	// for each item in measure
		CItem&	item = arrMeasure[iItem];
		// if item isn't a tuplet, and is tied or a rest
		if (!item.m_bIsTuplet && (item.m_bIsTied || item.IsRest())) {
			int	nTiedDur = GetTiedDuration(arrMeasure, iItem);
			// if item can be consolidated, and its new duration is on a suitable boundary
			if (nTiedDur > item.m_nDur && (!(nPos % nTiedDur) || nTiedDur == m_nTimebase * 2
			|| (nTiedDur == m_nTimebase * 3 && m_nMeter == 4))) {
				int	nTiedItems = nTiedDur / item.m_nDur;
				arrMeasure.RemoveAt(iItem, nTiedItems - 1);	// remove all but last item in group
				item.m_nDur = nTiedDur;	// update duration
				int	nDots;	// update LilyPond duration too
				item.m_nLilyDur = static_cast<WORD>(GetDuration(item.m_nDur, nDots));
				item.m_nDots = static_cast<BYTE>(nDots);
			} else {	// didn't consolidate
				if (m_nMeter == 4) {	// apply this correction to 4/4 only
					if (iItem < arrMeasure.GetSize() - 1) {	// if not on last item
						CItem&	itemNext = arrMeasure[iItem + 1];
						if (item.m_arrNote == itemNext.m_arrNote) {	// if notes match
							// if alignment is suitable, merge tied quarter and eighth into dotted quarter
							int	d4 = m_nTimebase;	// quarter note or rest
							int	d8 = m_nTimebase / 2;	// eighth note or rest
							if ((item.m_nDur == d4 && itemNext.m_nDur == d8 && (nPos == 0 || nPos == d4 * 2))
							|| (item.m_nDur == d8 && itemNext.m_nDur == d4 && (nPos + d8 == d4 || nPos + d8 == d4 * 3))) {
								arrMeasure.RemoveAt(iItem);
								item.m_nDur = d4 + d8;	// replace pair with dotted quarter
								int	nDots;	// update LilyPond duration too
								item.m_nLilyDur = static_cast<WORD>(GetDuration(item.m_nDur, nDots));
								item.m_nDots = static_cast<BYTE>(nDots);
							}
						}
					}
				}
			}
		}
		nPos += item.m_nDur;
		iItem++;
	}
}

void CMidiToLily::AddScheduledItems(CString& sMeasure, int nTime)
{
	int	iTrack = m_iTrack;
	int	nOttavaTracks = m_params.m_arrOttavaArray.GetSize();
	if (iTrack < nOttavaTracks) {	// if track within ottava track range
		const CParams::COttavaArray&	arrOttava = m_params.m_arrOttavaArray[iTrack];
		// if ottava index is within track's ottava array and ottava is due
		if (m_iOttava < arrOttava.GetSize() && nTime >= arrOttava[m_iOttava].m_nTick) {
			CString	sOttava;
			sOttava.Format(_T("%d"), arrOttava[m_iOttava].m_nShift);
			sMeasure += _T("\\ottava #") + sOttava + ' ';
			m_iOttava++;
		}
	}
}

void CMidiToLily::FormatMeasure(CString& sMeasure, const CItemArray& arrMeasure, bool bPrevMeasureTied)
{
	sMeasure = _T("  ");
	if (m_iSection < m_params.m_arrSection.GetSize() && m_iMeasure >= m_params.m_arrSection[m_iSection]) {
		sMeasure += _T("\\section ");
		m_iSection++;
	}
	int	nTime = m_iMeasure * m_nMeter * m_nTimebase;
	if (IsWholeMeasureRest(arrMeasure)) {	// if whole measure rest
		AddScheduledItems(sMeasure, nTime);
		CString	sBarMult;
		if (m_nMeter != 4) {
			sBarMult.Format(_T("*%d/4"), m_nMeter);
		}
		sMeasure += _T("R1") + sBarMult;
	} else {	// measure contains at least one note
		bool	bIsTuplet = false;
		int	nItems = arrMeasure.GetSize();
		for (int iItem = 0; iItem < nItems; iItem++) {	// for each item in measure
			AddScheduledItems(sMeasure, nTime);
			const CItem&	item = arrMeasure[iItem];
			ASSERT(!(item.IsRest() && item.m_bIsTied));	// rests can't be tied
			bool	bPrevItemTied;
			if (iItem > 0) {	// if not first item
				bPrevItemTied = arrMeasure[iItem - 1].m_bIsTied;
			} else {	// first item
				bPrevItemTied = bPrevMeasureTied;	// true if previous measure's last item was tied
			}
			if (item.m_bIsTuplet != bIsTuplet) {
				if (item.m_bIsTuplet) {
					const int nTupletLength = 4;	// complex issue; constant will do for now
					CString	sTuplet;
					sTuplet.Format(_T("\\tuplet 3/2 %d { "), nTupletLength);
					sMeasure += sTuplet;
				} else {
					sMeasure += _T("} ");
				}
				bIsTuplet ^= 1;
			}
			sMeasure += FormatItem(item, bPrevItemTied);
			nTime += item.m_nDur;
		}
		if (bIsTuplet) {
			sMeasure += _T("} ");
		}
	}
	sMeasure += _T("|\n");
}

void CMidiToLily::WriteTrack(CStdioFile& fLily, int iTrack)
{
	if (IsLogging(LOG_MEASURE_EVENTS)) {
		_tprintf(_T("track %d\n"), iTrack);
	}
	m_iSection = 0;	// reset section index
	m_iOttava = 0;	// reset ottava index
	CItem	itemRemain;	// remaining portion of subdivided item
	CItemArray	arrMeasure;	// array of items for an entire measure
	bool	bPrevMeasureTied = false;	// true if previous measure's last note was tied
	const CMidiEventArray& arrEvent = m_arrTrack[iTrack].m_arrEvent;
	int	nEvents = arrEvent.GetSize();
	int	iEvent = 0;	// index of current event
	int	iBeat = 0;	// index of current beat
	int	iMeasure = 0;	// index of current measure
	int	nMeasureDur = m_nTimebase * m_nMeter;
	int	nMeasures = (m_nEndTime - 1) / nMeasureDur + 1;
	CNoteArray	arrNote;
	CString	sMeasure;
	while (iMeasure < nMeasures) {	// while measures remain
		int	nBeatDur = 0;	// accumulated duration of beat, in ticks
		m_iMeasure = iMeasure;	// for error reporting
		CItemArray	arrItem;	// beat starts with empty item array
		if (itemRemain.m_nDur > 0) {	// if duration remaining from previous beat
			int	nItemDur = min(itemRemain.m_nDur, m_nTimebase);
			CItem	item(nItemDur, itemRemain.m_arrNote);
			itemRemain.m_nDur -= nItemDur;	// subtract item duration from duration remaining
			// if duration still remaining and item isn't a rest
			if (itemRemain.m_nDur > 0 && !item.IsRest()) {
				item.m_bIsTied = true;	// make note tied
			} else {	// no duration remaining, or item is a rest
				itemRemain.m_bIsTied = false;	// reset remining item's tie flag
			}
			arrItem.Add(item);	// add item to beat's item array
			nBeatDur += nItemDur;	// add item duration to beat duration
		}
		int	nBeatStartTime = iBeat * m_nTimebase;	// start of this beat in absolute ticks
		while (nBeatDur < m_nTimebase) {	// while beat isn't full
			if (iEvent >= nEvents) {	// if no more events
				int	nRemainingBeatDur = m_nTimebase - nBeatDur;	// remaining beat duration
				CItem	item(nRemainingBeatDur);
				arrItem.Add(item);	// add trailing rest to beat's item array
				nBeatDur += nRemainingBeatDur;	// fill out beat duration
				break;
			}
			const CMidiEvent& evt = arrEvent[iEvent];	// peek at next note
			if (!evt.m_nDur) {
				if (IsLogging(LOG_MEASURE_EVENTS)) {
					LogEvent(evt);
				}
				arrNote.Add(MIDI_P1(evt.m_dwMsg));
				iEvent++;	// increment past this note
				continue;
			}
			int	nEvtStartTime = evt.m_dwTime - nBeatStartTime;	// make time beat-relative
			if (nEvtStartTime >= m_nTimebase) {	// if note starts after this beat
				int	nRemainingBeatDur = m_nTimebase - nBeatDur;	// remaining beat duration
				CItem	item(nRemainingBeatDur);
				arrItem.Add(item);	// add intermediate rest to beat's item array
				nBeatDur += nRemainingBeatDur;	// fill out beat duration
				break;
			}
			iEvent++;	// increment past this note
			if (nEvtStartTime > nBeatDur) {	// if note starts later than beat duration
				CItem	item(nEvtStartTime - nBeatDur);	// duration of pre-note rest
				arrItem.Add(item);	// add pre-note rest to beat's item array
				nBeatDur += item.m_nDur;	// add item duration to beat duration
			}
			int	nRemainingBeatDur = m_nTimebase - nBeatDur;	// remaining beat duration
			// compute portion of note duration that fits within remaining beat duration
			int	nItemDur = min(evt.m_nDur, nRemainingBeatDur);
			arrNote.Add(MIDI_P1(evt.m_dwMsg));
			CItem	item(nItemDur, arrNote);
			if (nItemDur < evt.m_nDur) {	// if entire note duration didn't fit
				itemRemain.m_nDur = evt.m_nDur - nItemDur;	// save note's remaining duration
				itemRemain.m_arrNote = arrNote;
				item.m_bIsTied = true;	// make note tied
			}
			if (IsLogging(LOG_MEASURE_EVENTS)) {
				LogEvent(evt);
			}
			arrItem.Add(item);	// add note to beat's item array
			arrNote.RemoveAll();
			nBeatDur += nItemDur;	// add item duration to beat duration
		}
		ASSERT(nBeatDur == m_nTimebase);	// beat should be exactly full, else logic error
		arrMeasure.Append(arrItem);	// add item array to measure
		iBeat++;	// increment beat count
		int	nModBeat = iBeat % m_nMeter;
		m_iBeat = nModBeat;	// for error reporting
		if (!nModBeat) {	// if on measure boundary
			CalcDurations(arrMeasure);
			if (IsLogging(LOG_PRELIM_MEASURES)) {
				LogMeasure(arrMeasure, bPrevMeasureTied);
			}
			ConsolidateItems(arrMeasure);
			FormatMeasure(sMeasure, arrMeasure, bPrevMeasureTied);
			if (IsLogging(LOG_FINAL_MEASURES)) {
				_fputts(sMeasure.GetString(), stdout);
			}
			fLily.WriteString(sMeasure);
			bPrevMeasureTied = arrMeasure[arrMeasure.GetSize() - 1].m_bIsTied;
			arrMeasure.FastRemoveAll();
			iMeasure++;
		}
	}
}

CString	CMidiToLily::GetClefString(int iTrack) const
{
	if (iTrack < m_params.m_arrClef.GetSize() && !m_params.m_arrClef[iTrack].IsEmpty()) {
		return m_params.m_arrClef[iTrack];
	} else {
		return m_arrTrack[iTrack].m_sClef;
	}
}

CString	CMidiToLily::GetMidiName(DWORD dwMsg) const
{
	return CNote::GetMidiName(MIDI_P1(dwMsg), m_nKeySig);
}

void CMidiToLily::WriteTrackHeader(CStdioFile& fLily, int iTrack)
{
	CString	sTimeSig;
	sTimeSig.Format(_T("%d/4"), m_nMeter);
	CString	sKeySig(CNote::GetLilyNoteName(m_nKeySig));
	sKeySig += _T(" \\major");
	CString	sTempo;
	sTempo.Format(_T("4 = %d"), m_nTempo);
	CString	sClef(GetClefString(iTrack));
	CString	sHeader;
	sHeader = GetTrackVarName(iTrack) + _T(" = \\absolute {\n");	// absolute pitch
	sHeader += _T("  \\key ") + sKeySig + '\n';
	sHeader += _T("  \\time ") + sTimeSig + '\n';
	sHeader += _T("  \\tempo ") + sTempo + '\n';
	sHeader += _T("  \\clef \"") + sClef + _T("\"\n");
	fLily.WriteString(sHeader);
}

void CMidiToLily::WriteBookHeader(CStdioFile& fLily)
{
	CString	sHeader;
	if (!m_params.m_sTitle.IsEmpty())
		sHeader += _T("  title = \"") + m_params.m_sTitle + _T("\"\n");
	if (!m_params.m_sSubtitle.IsEmpty())
		sHeader += _T("  subtitle = \"") + m_params.m_sSubtitle + _T("\"\n");
	if (!m_params.m_sComposer.IsEmpty())
		sHeader += _T("  composer = \"") + m_params.m_sComposer + _T("\"\n");
	if (!m_params.m_sCopyright.IsEmpty())
		sHeader += _T("  copyright = \"") + m_params.m_sCopyright + _T("\"\n");
	if (!sHeader.IsEmpty()) {
		fLily.WriteString(_T("\\header {\n") + sHeader + _T("}\n"));
	}
}

void CMidiToLily::WriteScoreHeader(CStdioFile& fLily)
{
	CString	sHeader;
	if (!m_params.m_sOpus.IsEmpty())
		sHeader += _T("    opus = \"") + m_params.m_sOpus + _T("\"\n");
	if (!m_params.m_sPiece.IsEmpty())
		sHeader += _T("    piece = \"") + m_params.m_sPiece + _T("\"\n");
	fLily.WriteString(_T("\\score {\n"));
	if (!sHeader.IsEmpty()) {
		fLily.WriteString(_T("  \\header {\n") + sHeader + _T("  }\n"));
	}
}

void CMidiToLily::WriteLily(LPCTSTR pszOutputFilePath)
{
	if (IsLogging()) {
		_tprintf(_T("writing output Lily file \"%s\"\n"), pszOutputFilePath);
	}
	CStdioFileUTF	fLily(pszOutputFilePath, CFile::modeCreate | CFile::modeWrite);
	fLily.WriteString(_T("% created by ") + CString(theApp.m_pszAppName) + _T(" version ")
		+ CParamParser::GetAppVersionString() + '\n');
	fLily.WriteString(
		_T("\\version \"2.24.3\"\n")	// specify LilyPond version number
		_T("\\language \"english\"\n"));	// select English accidentals (s, f)
	WriteBookHeader(fLily);
	int	nTracks = m_arrTrack.GetSize();
	for (int iTrack = 1; iTrack < nTracks; iTrack++) {	// for each track
		m_iTrack = iTrack;	// for error reporting
//		DumpNotes(iTrack);
		WriteTrackHeader(fLily, iTrack);
		WriteTrack(fLily, iTrack);
		if (m_bWriteFine) {
			fLily.WriteString(_T("  \\fine\n"));	// end of song
		}
		fLily.WriteString(_T("}\n"));	// end music block
	}
	WriteScoreHeader(fLily);
	fLily.WriteString(
		_T("  <<\n"));	// start staves
	int	nStaves;
	if (!m_params.m_arrStave.IsEmpty()) {	// if staves specified
		nStaves = m_params.m_arrStave.GetSize();
	} else {	// no staves
		nStaves = m_arrTrack.GetSize() - 1;
	}
	for (int iStave = 0; iStave < nStaves; iStave++) {	// for each stave
		int	iTrack;
		if (!m_params.m_arrStave.IsEmpty()) {	// if staves specified
			iTrack = m_params.m_arrStave[iStave];
		} else {	// no staves
			iTrack = iStave + 1;
		}
		fLily.WriteString(_T("  \\new Staff \\") + GetTrackVarName(iTrack) + '\n');
	}
	CString	sLayout;
	if (m_params.m_bFrenched) {	// if removing empty staves
		sLayout =
			_T("  \\layout {\n")
			_T("    \\context {\n")
			_T("      \\Staff\n")
			_T("      \\RemoveAllEmptyStaves\n")
			_T("    }\n")
			_T("  }\n");
	} else {	// simple layout
		sLayout = 
			_T("  \\layout {}\n");
	}
	fLily.WriteString(
		_T("  >>\n")	// end staves
		+ sLayout +		// layout defined above
		_T("  \\midi {}\n")	// output MIDI file matching score
		_T("}\n"));	// end score block
}

static bool FilesEqual(LPCTSTR pszFile1, LPCTSTR pszFile2)
{
	CFile f1(pszFile1, CFile::modeRead);
	CFile f2(pszFile2, CFile::modeRead);
	UINT nSize = (UINT)f1.GetLength();
	if (nSize != (UINT)f2.GetLength())
		return false;
	CByteArray b1, b2;
	b1.SetSize(nSize);
	b2.SetSize(nSize);
	f1.Read(b1.GetData(), nSize);
	f2.Read(b2.GetData(), nSize);
	return !memcmp(b1.GetData(), b2.GetData(), nSize);
}

void CMidiToLily::VerifyLilyMidi(LPCTSTR pszLilyMidiFilePath)
{
	static const LPCTSTR	pszInEventPath = _T("MidiToLilyIn.txt");
	static const LPCTSTR	pszOutEventPath = _T("LilyPondMidi.txt");
	if (IsLogging()) {
		_tprintf(_T("verifying Lily MIDI file \"%s\"\n"), pszLilyMidiFilePath);
	}
	DumpEvents(pszInEventPath, 90, true);	// override note velocities with LilyPond default velocity, and use staves
	CMidiToLily	lily;
	lily.SetParams(m_params);
	// if staves are specified, LilyPond MIDI file can have fewer tracks than input MIDI file, due to filtering
	lily.m_params.m_arrClef.FastRemoveAll();	// avoid track index range errors in OnMidiFileRead
	lily.m_params.m_arrOttavaArray.FastRemoveAll();
	lily.m_params.m_arrStave.FastRemoveAll();
	lily.m_params.m_nOffset = 0;	// LilyPond input file accounts for offset if any
	lily.ReadMidiFile(pszLilyMidiFilePath);
	double	fTimebaseScaling = m_nTimebase / double(lily.m_nTimebase);	// don't assume same timebase
	int	nTracks = lily.m_arrTrack.GetSize();
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each track
		CTrack&	track = lily.m_arrTrack[iTrack];
		int	nEvents = track.m_arrEvent.GetSize();
		for (int iEvent = 0; iEvent < nEvents; iEvent++) {	// for each of input track's events
			CMidiEvent& evt = track.m_arrEvent[iEvent];
			evt.m_dwTime = Round(evt.m_dwTime * fTimebaseScaling);	// scale start time
			evt.m_nDur = Round(evt.m_nDur * fTimebaseScaling);	// scale duration
		}
	}
	lily.DumpEvents(pszOutEventPath);
	if (!FilesEqual(pszInEventPath, pszOutEventPath)) {	// if event dump files don't match
		OnError(LDS(IDS_ERR_MIDI_VERIFY_FAIL));
	}
	_tprintf(LDS(IDS_MSG_MIDI_VERIFIED) + '\n');
}

static void RenameExtension(CString& sPath, CString sExtension)
{
	LPTSTR pszPath = sPath.GetBuffer(sPath.GetLength() + sExtension.GetLength());
	PathRemoveExtension(pszPath);
	PathAddExtension(pszPath, sExtension);
	sPath.ReleaseBuffer();
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	UNREFERENCED_PARAMETER(envp);
	HMODULE hModule = ::GetModuleHandle(NULL);
	if (hModule == NULL) {
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		return 1;
	}
	if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0)) {
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		return 1;
	}
	int	bRetVal = 0;
	TRY {
		CParamParser	parser;
		if (!parser.ParseCommandLine()) {	// if parser wants us to exit
			return 0;
		}
		if (argc < 2) {	// if not enough arguments
//			parser.WriteHelpMarkdown(_T("help.txt"));
			parser.ShowHelp();	// show help and exit
			return 0;
		}
		CString	sLilyFilePath;
		if (!parser.m_sOutput.IsEmpty()) {	// if output file specified
			sLilyFilePath = parser.m_sOutput;
		} else {	// output file not specified
			// use input MIDI file path, but replace extension with LilyPond's;
			// append suffix to file name to avoid overwriting input MIDI file
			sLilyFilePath = argv[1];	
			RenameExtension(sLilyFilePath, _T(" [lily].ly"));	
		}
		CMidiToLily	lily;
		lily.SetParams(parser);
		lily.ReadMidiFile(argv[1]);
		lily.RemoveOverlaps();
//		lily.DumpEvents();
		if (parser.m_bVerify) {	// if verifying
			CString	sLilyMidiFilePath(sLilyFilePath);
			RenameExtension(sLilyMidiFilePath, _T(".mid"));
			lily.VerifyLilyMidi(sLilyMidiFilePath);
		} else {	// normal operation
			lily.WriteLily(sLilyFilePath);
		}
	}
	CATCH (CException, e) {
		TCHAR	szMsg[MAX_PATH];
		e->GetErrorMessage(szMsg, _countof(szMsg));
		if (szMsg[0]) {	// if message isn't null string
			_tprintf(_T("%s\n"), szMsg);
		}
		bRetVal = 1;	// return failure
	}
	END_CATCH
	if (!bRetVal) {
		_tprintf(LDS(IDS_MSG_ALL_GOOD) + '\n');
	}
	return bRetVal;
}
