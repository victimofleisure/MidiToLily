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
		03		06jan25	add time and key signature params
		04		08jan25	add scan MBT time
		05		09jan25	add tempo param
		06		16sep25	fix comment only
		07		17sep25	handle type 0 MIDI files
		08		17sep25	add combine param
 
*/

// MidiToLily.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MidiToLily.h"
#include "StdioFileUTF.h"
#include "Note.h"

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

inline void CMidiToLily::CItem::Reset()
{
	m_nDur = 0;
	m_bIsTied = false;
	m_bIsTuplet = false;
	m_nDots = 0;
	m_nDenom = 0;
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
	m_nEndTime = 0;
	m_nQuantDur = 0;
	m_nTripletQuantDur = 0;
	m_iFirstNoteTrack = 0;
	ResetTrackData();
}

void CMidiToLily::ResetTrackData()
{
	m_nCurTime = 0;
	m_iTrack = 0;
	m_iMeasure = 0;
	m_iBeat = 0;
	m_iSection = 0;
	m_iOttava = 0;
	m_iTimeSig = 0;
	m_iKeySig = 0;
	m_iTempo = 0;
}

void CMidiToLily::OnError(CString sErrMsg)
{
	THROW(new CMyException(sErrMsg));
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
	// add key signature to parameters, unless first measure is already specified
	CKeySigChange	chgKeySig(1, sigKey.SharpsOrFlats, sigKey.IsMinor != 0);
	if (m_params.m_arrKeySig.IsEmpty() || m_params.m_arrKeySig[0].m_nMeasure > 1) {
		m_params.m_arrKeySig.InsertAt(0, chgKeySig);
	}
	// add time signature to parameters, unless first measure is already specified
	CTimeSigChange	chgTimeSig(1, sigTime.Numerator, 1 << sigTime.Denominator);
	if (m_params.m_arrTimeSig.IsEmpty() || m_params.m_arrTimeSig[0].m_nMeasure > 1) {
		m_params.m_arrTimeSig.InsertAt(0, chgTimeSig);
	}
	// add tempo to parameters, unless first measure is already specified
	int	nLilyTempo = Round(CMidiFile::MICROS_PER_MINUTE / double(nTempo));	// LilyPond tempo is integer
	CTempoChange	chgTempo(1, 4, 0, nLilyTempo);	// quarter notes per minute
	if (m_params.m_arrTempo.IsEmpty() || m_params.m_arrTempo[0].m_nMeasure > 1) {
		m_params.m_arrTempo.InsertAt(0, chgTempo);
	}
	if (IsLogging(LOG_MIDI_FILE_INFO)) {
		_tprintf(_T("Tracks = %d\nTimebase = %d\nTempo = %d\nKeySig = %s\nTimeSig = %s\n"), 
			arrTrack.GetSize(), m_nTimebase, nLilyTempo, 
			chgKeySig.Format().GetString(), chgTimeSig.Format().GetString());
	}
	OnMidiFileRead(arrTrack);
	int	nTracks = arrTrack.GetSize();
	m_arrTrack.SetSize(nTracks);
	m_nEndTime = 0;
	if (IsLogging(LOG_MIDI_MESSAGES)) {
		_tprintf(_T("input MIDI messages:\n"));
	}
	for (int iTrack = 0; iTrack < nTracks; iTrack++) {	// for each input track
		PrepareMidiEvents(iTrack, arrTrack[iTrack]);
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

void CMidiToLily::PrepareMidiEvents(int iTrack, const CMidiFile::CMidiEventArray& arrInEvent)
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
	int	nTracks = arrTrack.GetSize();
	if (nTracks < 1) {	// at least one track required
		OnError(LDS(IDS_CLA_TOO_FEW_TRACKS));
	}
	// if MIDI file has multiple tracks, assume track is tempo map and skip it
	m_iFirstNoteTrack = nTracks > 1;
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
		if (iTrack < m_iFirstNoteTrack || iTrack >= nTracks) {	// if stave's track index is out of range
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
	m_params.Finalize();	// finalize parameters, a crucial last step
	if (IsLogging(LOG_PARAMETERS)) {
		m_params.Log();
	}
	int	nCombines = m_params.m_arrCombine.GetSize();
	if (!nStaves)	// if staves aren't specified
		nStaves = nTracks - m_iFirstNoteTrack;	// use track count
	for (int iComb = 0; iComb < nCombines; iComb++) {	// for each combine
		int	iMate = m_params.m_arrCombine[iComb];	// get mate's stave index
		if (iMate >= nStaves) {	// if index is out of range
			LPCTSTR	pszFlagName = CParamParser::GetFlagName(CParamParser::F_combine);
			CString	sErrMsg;
			sErrMsg.Format(IDS_CLA_STAVE_NUMBER_RANGE, iMate + 1, pszFlagName);
			OnError(sErrMsg);
		}
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
	_tprintf(_T("%d %x %s %d\n"), evt.m_dwTime, evt.m_dwMsg, GetMidiName(evt.m_dwMsg).GetString(), evt.m_nDur);
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
		nTracks = m_params.m_arrStave.GetSize() + m_iFirstNoteTrack;	// account for tempo track
	} else {	// no staves
		nTracks = m_arrTrack.GetSize();
	}
	CString	sLine;
	for (int iTrack = m_iFirstNoteTrack; iTrack < nTracks; iTrack++) {	// for each note track
		int	iSrcTrack;
		if (bUseStaves) {	// if iterating staves
			iSrcTrack = m_params.m_arrStave[iTrack - m_iFirstNoteTrack];	// map stave to track; account for tempo track
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
	if (IsLogging(LOG_NOTE_OVERLAPS)) {
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
					if (IsLogging(LOG_NOTE_OVERLAPS)) {
						// if times and durations match, it will notate correctly as a chord, else log it
						if (evt.m_dwTime != evtNext.m_dwTime || evt.m_nDur != evtNext.m_nDur) {
							_tprintf(_T("track %d: %d %s %d overlaps %d %s %d\n"), iTrack, 
								evt.m_dwTime, GetMidiName(evt.m_dwMsg).GetString(), evt.m_nDur,
								evtNext.m_dwTime, GetMidiName(evtNext.m_dwMsg).GetString(), evtNext.m_nDur);
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
			for (int iNote = 0; iNote < nNotes; iNote++) {	// for each of item's notes
				int	nNote = item.m_arrNote[iNote];
				ASSERT(nNote < MIDI_NOTES);
				int	nOctave = nNote / OCTAVE - 4;	// account for middle C
				int	nNoteNorm = nNote % OCTAVE;
				int	iNoteName = CNote::GetNoteNameIdx(nNoteNorm, m_keySig.m_nAccs, m_keySig.m_bIsMinor);
				CString	sNote(CNote::GetLilyNoteNameByIdx(iNoteName));
				if (iNoteName == CNote::Cb)	// if C flat
					nOctave++;	// octave up
				else if (iNoteName == CNote::Bs)	// if B sharp
					nOctave--;	// octave down
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
				sItem += sNote + sOctave;
			}
			if (nNotes > 1) {
				sItem += '>';
			}
		}
	}
	CString	sTie(item.m_bIsTied ? _T("~") : _T(""));
	CString	sDur;
	sDur.Format(_T("%d"), item.m_nDenom);
	for (int iDot = 0; iDot < item.m_nDots; iDot++) {
		sDur += '.';
	}
	sItem += sDur + sTie + _T(" ");
	return sItem;
}

bool CMidiToLily::GetLilyDuration(CItem& item, bool bNoThrow) const
{
	ASSERT(item.m_nDur);
	int	nWhole = m_nTimebase * 4;	// start with whole note
	if (!(nWhole % item.m_nDur)) {	// if duration divides evenly
		item.m_nDenom = static_cast<WORD>(nWhole / item.m_nDur);	// most likely case
		return true;
	}
	// duration not found above; try dotted values
	const int nMaxDots = 2;	// maximum number of dots we allow
	for (int iDot = 0; iDot <= nMaxDots; iDot++) {
		int	nDotWhole = nWhole - (m_nTimebase >> iDot);
		if (!(nDotWhole % item.m_nDur)) {	// if duration divides evenly
			item.m_nDots = static_cast<BYTE>(iDot + 1);
			item.m_nDenom = static_cast<WORD>(nDotWhole / item.m_nDur * 2);
			return true;
		}
	}
	if (!bNoThrow) {
		CString	sErrMsg;
		sErrMsg.Format(IDS_ERR_BAD_DURATION, m_iTrack, m_iMeasure + 1, item.m_nDur);
		OnError(sErrMsg);
		ASSERT(0);	// zero duration is an error in LilyPond
	}
	return false;
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
			GetLilyDuration(item);
			if (!(item.m_nDenom % 3)) {	// if triplet duration
				if (item.m_nDots) {
					CString	sErrMsg;
					sErrMsg.Format(IDS_ERR_DOT_WITHIN_TRIPLET, m_iTrack, m_iMeasure + 1);
					OnError(sErrMsg);
				}
				item.m_nDenom = static_cast<WORD>(Round(item.m_nDenom * (2.0 / 3.0)));
				item.m_bIsTuplet = true;
			}
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
			|| (nTiedDur == m_nTimebase * 3 && m_timeSig == CTimeSig(4, 4)))) {
				int	nTiedItems = nTiedDur / item.m_nDur;
				arrMeasure.RemoveAt(iItem, nTiedItems - 1);	// remove all but last item in group
				item.m_nDur = nTiedDur;	// update duration
				GetLilyDuration(item);
			} else {	// didn't consolidate
				if (m_timeSig == CTimeSig(4, 4)) {	// apply this correction to 4/4 only
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
								GetLilyDuration(item);
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
		const COttavaArray&	arrOttava = m_params.m_arrOttavaArray[iTrack];
		if (m_iOttava < arrOttava.GetSize()) {	// if ottava index is within track's ottava array
			const COttava&	ot = arrOttava[m_iOttava];
			if (m_iMeasure >= ot.m_nMeasure - 1) {	// if ottava's measure is due
				// compute ottava's time in ticks relative to start of measure
				int	nOttavaTime = (ot.m_nBeat - 1) * m_nTimebase + ot.m_nTick;
				if (nTime >= nOttavaTime) {	// if ottava's beat and tick are also due
					CString	sOttava;
					sOttava.Format(_T("%d"), ot.m_nShift);
					sMeasure += _T("\\ottava #") + sOttava + ' ';
					if (IsLogging(LOG_SCHEDULED_ITEMS)) {
						_tprintf(_T("%s ottava %d\n"), 
							ot.Format().GetString(), ot.m_nShift);
					}
					m_iOttava++;
				}
			}
		}
	}
}

void CMidiToLily::FormatMeasure(CString& sMeasure, const CItemArray& arrMeasure, bool bPrevMeasureTied)
{
	sMeasure = _T("  ");
	if (m_iSection < m_params.m_arrSection.GetSize() && m_iMeasure >= m_params.m_arrSection[m_iSection] - 1) {
		sMeasure += _T("\\section ");
		if (IsLogging(LOG_SCHEDULED_ITEMS)) {
			_tprintf(_T("%d: section\n"), m_params.m_arrSection[m_iSection]);	// one-based measure number
		}
		m_iSection++;
	}
	int	nTime = 0;
	if (IsWholeMeasureRest(arrMeasure)) {	// if whole measure rest
		AddScheduledItems(sMeasure, nTime);
		CString	sBarMult;
		if (m_timeSig != CTimeSig(4, 4)) {
			sBarMult = _T("*") + m_timeSig.Format();
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

bool CMidiToLily::OnNewMeasure(CStdioFile& fLily)
{
	bool	bIsTimeChange = false;
	if (m_iKeySig < m_params.m_arrKeySig.GetSize()) {	// if key signatures remain
		const CKeySigChange& ksc = m_params.m_arrKeySig[m_iKeySig];
		if (m_iMeasure >= ksc.m_nMeasure - 1) {	// if next key change is due
			const CKeySig&	keySig = ksc;	// upcast
			m_keySig = keySig;	// update member var
			m_iKeySig++;	// bump index
			fLily.WriteString(_T("  \\key ") + ksc.FormatLily() + '\n');
			if (IsLogging(LOG_SCHEDULED_ITEMS)) {
				_tprintf(_T("%d: key %s\n"), ksc.m_nMeasure, ksc.Format().GetString());
			}
		}
	}
	if (m_iTimeSig < m_params.m_arrTimeSig.GetSize()) {	// if time signatures remain
		const CTimeSigChange& tsc = m_params.m_arrTimeSig[m_iTimeSig];
		if (m_iMeasure >= tsc.m_nMeasure - 1) {	// if next time change is due
			const CTimeSig&	timeSig = tsc;	// upcast
			m_timeSig = timeSig;	// update member var
			m_iTimeSig++;	// bump index
			CString	sTimeSig(tsc.Format());
			fLily.WriteString(_T("  \\time ") + sTimeSig + '\n');
			if (IsLogging(LOG_SCHEDULED_ITEMS)) {
				_tprintf(_T("%d: time %s\n"), tsc.m_nMeasure, sTimeSig.GetString());
			}
			bIsTimeChange = true;
		}
	}
	if (m_iTempo < m_params.m_arrTempo.GetSize()) {	// if tempo changes remain
		const CTempoChange& tc = m_params.m_arrTempo[m_iTempo];
		if (m_iMeasure >= tc.m_nMeasure - 1) {	// if next tempo change is due
			const CTempo&	tempo = tc;	// upcast
			m_tempo = tempo;	// update member var
			m_iTempo++;	// bump index
			CString	sTempo(tc.Format());
			fLily.WriteString(_T("  \\tempo ") + sTempo + '\n');
			if (IsLogging(LOG_SCHEDULED_ITEMS)) {
				_tprintf(_T("%d: key %s\n"), tc.m_nMeasure, sTempo.GetString());
			}
		}
	}
	return bIsTimeChange;
}

void CMidiToLily::WriteTrack(CStdioFile& fLily, int iTrack)
{
	if (IsLogging(LOG_MEASURE_EVENTS)) {
		_tprintf(_T("track %d\n"), iTrack);
	}
	CItem	itemRemain;	// remaining portion of subdivided item
	CItemArray	arrMeasure;	// array of items for an entire measure
	bool	bPrevMeasureTied = false;	// true if previous measure's last note was tied
	const CMidiEventArray& arrEvent = m_arrTrack[iTrack].m_arrEvent;
	int	nEvents = arrEvent.GetSize();
	int	iEvent = 0;	// index of current event
	OnNewMeasure(fLily);
	CNoteArray	arrNote;
	CString	sMeasure;
	int	nBeatLen = m_nTimebase * 4 / m_timeSig.m_nDenom;
	while (m_nCurTime < m_nEndTime) {	// while measures remain
		int	nBeatDur = 0;	// accumulated duration of beat, in ticks
		CItemArray	arrItem;	// beat starts with empty item array
		if (itemRemain.m_nDur > 0) {	// if duration remaining from previous beat
			int	nItemDur = min(itemRemain.m_nDur, nBeatLen);
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
		int	nBeatStartTime = m_nCurTime + m_iBeat * nBeatLen;	// start of this beat in absolute ticks
		while (nBeatDur < nBeatLen) {	// while beat isn't full
			if (iEvent >= nEvents) {	// if no more events
				int	nBeatRemain = nBeatLen - nBeatDur;	// remaining beat duration
				CItem	item(nBeatRemain);
				arrItem.Add(item);	// add trailing rest to beat's item array
				nBeatDur += nBeatRemain;	// fill out beat duration
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
			if (nEvtStartTime >= nBeatLen) {	// if note starts after this beat
				int	nBeatRemain = nBeatLen - nBeatDur;	// remaining beat duration
				CItem	item(nBeatRemain);
				arrItem.Add(item);	// add intermediate rest to beat's item array
				nBeatDur += nBeatRemain;	// fill out beat duration
				break;
			}
			iEvent++;	// increment past this note
			if (nEvtStartTime > nBeatDur) {	// if note starts later than beat duration
				CItem	item(nEvtStartTime - nBeatDur);	// duration of pre-note rest
				arrItem.Add(item);	// add pre-note rest to beat's item array
				nBeatDur += item.m_nDur;	// add item duration to beat duration
			}
			int	nBeatRemain = nBeatLen - nBeatDur;	// remaining beat duration
			// compute portion of note duration that fits within remaining beat duration
			int	nItemDur = min(evt.m_nDur, nBeatRemain);
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
		ASSERT(nBeatDur == nBeatLen);	// beat should be exactly full, else logic error
		arrMeasure.Append(arrItem);	// add item array to measure
		m_iBeat++;	// increment beat count
		if (m_iBeat >= m_timeSig.m_nNumer) {	// if measure is full
			CalcDurations(arrMeasure);
			if (IsLogging(LOG_PRELIM_MEASURES)) {
				LogMeasure(arrMeasure, bPrevMeasureTied);
			}
			ConsolidateItems(arrMeasure);
			FormatMeasure(sMeasure, arrMeasure, bPrevMeasureTied);
			if (IsLogging(LOG_FINAL_MEASURES)) {
				_tprintf(_T("%d: %s"), m_iMeasure + 1, sMeasure.GetString());
			}
			fLily.WriteString(sMeasure);
			bPrevMeasureTied = arrMeasure[arrMeasure.GetSize() - 1].m_bIsTied;
			arrMeasure.FastRemoveAll();
			m_nCurTime += nBeatLen * m_timeSig.m_nNumer;
			m_iMeasure++;
			m_iBeat = 0;
			bool	bIsTimeChange = OnNewMeasure(fLily);
			if (bIsTimeChange) {
				nBeatLen = m_nTimebase * 4 / m_timeSig.m_nDenom;
			}
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
	return CNote::GetMidiName(MIDI_P1(dwMsg), m_keySig.m_nAccs, m_keySig.m_bIsMinor);
}

void CMidiToLily::WriteTrackHeader(CStdioFile& fLily, int iTrack)
{
	CString	sHeader(GetTrackVarName(iTrack) + _T(" = \\absolute {\n"));	// absolute pitch
	fLily.WriteString(sHeader);
	OnNewMeasure(fLily);	// write initial scheduled items
	CString	sClef(_T("  \\clef \"") + GetClefString(iTrack) + _T("\"\n"));
	fLily.WriteString(sClef);
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

inline int CMidiToLily::GetStaveTrack(int iStave) const
{
	if (!m_params.m_arrStave.IsEmpty()) {	// if staves specified
		return m_params.m_arrStave[iStave];
	} else {	// no staves
		return iStave + m_iFirstNoteTrack;	// account for tempo track
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
	for (int iTrack = m_iFirstNoteTrack; iTrack < nTracks; iTrack++) {	// for each note track
		ResetTrackData();	// do first; order matters
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
		nStaves = m_arrTrack.GetSize() - m_iFirstNoteTrack;
	}
	for (int iStave = 0; iStave < nStaves; iStave++) {	// for each stave
		int	iTrack = GetStaveTrack(iStave);
		if (iStave < m_params.m_arrCombine.GetSize()) {
			int	iMate = m_params.m_arrCombine[iStave];
			if (iMate >= 0) {	// if stave is combined with another
				ASSERT(iMate != iStave);	// can't self-combine
				if (iMate > iStave) {	// if first stave of pair
					int	iMateTrack = GetStaveTrack(iMate);
					fLily.WriteString(_T("  \\new Staff \\partCombine \\") 
						+ GetTrackVarName(iTrack) + _T(" \\") 
						+ GetTrackVarName(iMateTrack) + '\n');
				}
				continue;	// done with this stave
			}
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
	CParams&	params = lily.m_params;	// upcast
	params = m_params;	// copy parameters
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

#define WRITE_HELP_MARKDOWN_FILE 0	// set non-zero to write help Markdown file

int CMidiToLily::Process(LPCTSTR pszInPath)
{
	if (!m_params.ParseCommandLine()) {	// if parser wants us to exit
		return 0;
	}
	if (pszInPath == NULL) {	// if not enough arguments
#if WRITE_HELP_MARKDOWN_FILE
		m_params.WriteHelpMarkdown(_T("help.txt"));
#endif
		m_params.ShowHelp();	// show help and exit
		return 0;
	}
	if (IsLogging()) {
		CParamParser::ShowAppVersion();
	}
	CString	sLilyFilePath;
	if (!m_params.m_sOutput.IsEmpty()) {	// if output file specified
		if (!PathsDiffer(pszInPath, m_params.m_sOutput)) {	// if input and output paths collide
			// LilyPond-generated MIDI file would overwrite our input MIDI file
			CMidiToLily::OnError(LDS(IDS_ERR_PATH_COLLISION));	// so prevent that
		}
		sLilyFilePath = m_params.m_sOutput;
	} else {	// output file not specified
		// use input MIDI file path, but replace extension with LilyPond's;
		// append suffix to file name to avoid overwriting input MIDI file
		sLilyFilePath = pszInPath;	
		RenameExtension(sLilyFilePath, _T(" [lily].ly"));	
	}
	ReadMidiFile(pszInPath);
	RemoveOverlaps();
//	DumpEvents();
	if (m_params.m_bVerify) {	// if verifying
		CString	sLilyMidiFilePath(sLilyFilePath);
		RenameExtension(sLilyMidiFilePath, _T(".mid"));
		VerifyLilyMidi(sLilyMidiFilePath);
	} else {	// normal operation
		WriteLily(sLilyFilePath);
	}
	_tprintf(LDS(IDS_MSG_ALL_GOOD) + '\n');
	return 0;	// success
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
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
		CMidiToLily	lily;
		bRetVal = lily.Process(argv[1]);
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
	return bRetVal;
}
