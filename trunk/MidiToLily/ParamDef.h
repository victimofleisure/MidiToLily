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

#ifdef PARAMDEF

PARAMDEF(output)    // The path of the output LilyPond file.
PARAMDEF(title)     // The title of the composition.
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
                    // 4=quarter etc.); can be combined with regular quant
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
PARAMDEF(help)      // Display the help.
PARAMDEF(license)   // Display the license.
PARAMDEF(logging)   // Enables various types of logging; specify * to enable all
                    // logging, otherwise specify a bitmask in hexadecimal.

#undef PARAMDEF
#endif

#ifdef LOGGINGTYPEDEF

LOGGINGTYPEDEF(PARAMETERS)
LOGGINGTYPEDEF(MIDI_FILE_INFO)
LOGGINGTYPEDEF(MIDI_MESSAGES)
LOGGINGTYPEDEF(MEASURE_EVENTS)
LOGGINGTYPEDEF(PRELIM_MEASURES)
LOGGINGTYPEDEF(FINAL_MEASURES)

#undef LOGGINGTYPEDEF
#endif
