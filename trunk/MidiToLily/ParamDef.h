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
		04		09jan25	add tempo param
		05		17sep25	add combine param
		06		17oct25	add cmd and block params

*/

#ifdef PARAMDEF

PARAMDEF(output)    // The path of the output LilyPond file.
PARAMDEF(title)     // The title of the composition.
PARAMDEF(subtitle)	// The subtitle of the composition.
PARAMDEF(opus)		// The title of the opus.
PARAMDEF(piece)		// The title of the piece.
PARAMDEF(composer)  // The name of the composer.
PARAMDEF(copyright) // The copyright notice.
PARAMDEF(frenched)  // Hide empty staves.
PARAMDEF(verify)    // Compare the input MIDI file to a LilyPond-generated MIDI
                    // file; MidiToLilyIn.txt and LilyPondMidi.txt are created.
PARAMDEF(quant)     // Quantize note start and end times to the specified duration
                    // (1=whole, 2=half, 4=quarter, etc.); if combined with
                    // triplet quantization, notes snap to the nearest grid.
PARAMDEF(triplet)   // Quantize note start and end times to the specified TRIPLET
                    // duration (1=whole, 2=half, 4=quarter, etc.); if combined
                    // with regular quantization, notes snap to the nearest grid.
                    // 4=quarter etc.); can be combined with regular quant.
PARAMDEF(offset)    // Signed offset in ticks added to all MIDI event times.
PARAMDEF(clef)      // Comma-separated list of clef overrides, each consisting of
                    // t=c, where t is a track index and c is a clef name defined
                    // in LilyPond. By default, a track is automatically assigned
                    // treble or bass clef, depending on what pitches it uses.
PARAMDEF(section)   // Comma-separated list of section boundaries, each consisting
                    // of a one-based measure number.
PARAMDEF(ottava)    // Comma-separated list of octave shifts, each consisting of
                    // t_M:B:T=n where t is a track index, M:B:T is a time in
                    // Measure:Beat:Tick format, and n is a signed number of
                    // octaves to transpose the staff by.
PARAMDEF(staves)	// Comma-separated list of track indices specifying which
					// tracks are assigned to staves, from top to bottom.
PARAMDEF(combine)	// comma-separated list of pairs of staves to combine, each
					// consisting of N_N where N is a one-based stave number.
PARAMDEF(key)		// Comma-separated list of key signatures, each consisting of
					// M=k where M is a one-based measure number, and k is the
					// key signature in LilyPond note format, optionally followed
					// by the letter 'm' to indicate a minor key.
PARAMDEF(time)		// Comma-separated list of time signatures, each consisting of
					// M=n/d where M is a one-based measure number, and n and d
					// are the the time signature's numerator and denominator.
PARAMDEF(tempo)		// Comma-separated list of tempos, each consisting of M=T
					// where M is a one-based measure number and T is a tempo.
					// The tempo can be a metronome mark, a description, or both.
					// If the tempo list contains spaces, enclose it in quotes.
PARAMDEF(cmd)		// Comma-separated list of LilyPond commands, each consisting
					// of t_M:B:T=s where t is a track index, M:B:T is a time in
                    // Measure:Beat:Tick format, and s is a LilyPond command.
PARAMDEF(block)		// LilyPond block definition, which is a toplevel expression.
PARAMDEF(params)	// Read parameters from the specified text file.
PARAMDEF(help)      // Display the help.
PARAMDEF(license)   // Display the license.
PARAMDEF(logging)   // Enables various types of logging; specify * to enable all
                    // logging, otherwise specify a bitmask in hexadecimal.

#undef PARAMDEF
#endif

#ifdef HELPEXAMPLEDEF

HELPEXAMPLEDEF(QUANT)
HELPEXAMPLEDEF(SECTION)
HELPEXAMPLEDEF(CLEF)
HELPEXAMPLEDEF(OTTAVA)
HELPEXAMPLEDEF(STAVES)
HELPEXAMPLEDEF(COMBINE)
HELPEXAMPLEDEF(TIME_SIG)
HELPEXAMPLEDEF(KEY_SIG)
HELPEXAMPLEDEF(TEMPO)
HELPEXAMPLEDEF(COMMAND)
					
#undef HELPEXAMPLEDEF
#endif

#ifdef LOGGINGTYPEDEF

LOGGINGTYPEDEF(PARAMETERS)
LOGGINGTYPEDEF(MIDI_FILE_INFO)
LOGGINGTYPEDEF(MIDI_MESSAGES)
LOGGINGTYPEDEF(MEASURE_EVENTS)
LOGGINGTYPEDEF(PRELIM_MEASURES)
LOGGINGTYPEDEF(FINAL_MEASURES)
LOGGINGTYPEDEF(NOTE_OVERLAPS)
LOGGINGTYPEDEF(SCHEDULED_ITEMS)

#undef LOGGINGTYPEDEF
#endif
