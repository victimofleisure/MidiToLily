# LilyToMidi

## Convert a MIDI file to LilyPond format

MidiToLily is a Windows console application that converts a MIDI file to LilyPond format. You can create sheet music from a MIDI file by using MidiToLily as a preprocessor for LilyPond. MidiToLily is an alternative to the midi2ly script that ships with LilyPond.

# Getting started

Download the latest release, unzip it, and run MidiToLily.exe. MidiToLily doesn't have an installer, doesn't have dependencies, and doesn't store things in the registry or anywhere else.

# Command line

### Usage: MidiToLily MIDI_FILE [options]

|Option|Description|
|---|---|
|MIDI_FILE|The path of the input MIDI file; if it contains spaces, enclose it in double quotes.|
|&#8209;output&nbsp;PATH|The path of the output LilyPond file; if it contains spaces, enclose it in double quotes.|
|&#8209;title&nbsp;STRING|The title of the composition; if it contains spaces, enclose it in double quotes.|
|&#8209;composer&nbsp;STRING|The name of the composer; if it contains spaces, enclose it in double quotes.|
|&#8209;copyright&nbsp;STRING|The copyright notice; if it contains spaces, enclose it in double quotes.|
|&#8209;frenched|Hide empty staves.|
|&#8209;verify|Compare the input MIDI file to a LilyPond-generated MIDI file; MidiToLilyIn.txt and LilyPondMidi.txt are created.|
|&#8209;quant&nbsp;DURATION|Quantize note start and end times to the specified duration (1=whole, 2=half, 4=quarter, etc.); if combined with triplet quantization, notes snap to the nearest grid.|
|&#8209;triplet&nbsp;DURATION|Quantize note start and end times to the specified TRIPLET duration (1=whole, 2=half, 4=quarter, etc.); if combined with regular quantization, notes snap to the nearest grid.|
|&#8209;offset&nbsp;OFFSET|Signed offset in ticks added to all MIDI event times.|
|&#8209;clef&nbsp;LIST|Comma-separated list of clef overrides, each consisting of t=c, where t is a track index and c is a clef name defined in LilyPond. By default, a track is automatically assigned treble or bass clef, depending on what pitches it uses.|
|&#8209;section&nbsp;LIST|Comma-separated list of measure numbers, each of which specifies the start of a section.|
|&#8209;ottava&nbsp;LIST|Comma-separated list of octave shifts, each consisting of t_M:B:T=n where t is a track index, M:B:T is a time in Measure:Beat:Tick format, and n is a signed number of octaves to transpose the staff by.|
|&#8209;help|Display the help.|
|&#8209;license|Display the license.|
|&#8209;logging&nbsp;TYPE|Enables various types of logging; specify a bitmask in hexadecimal, ? to list the types, or * to enable all.|

### Examples

|Example|Description|
|---|---|
|&#8209;quant&nbsp;16|Quantize to the sixteenth note grid.|
|&#8209;section&nbsp;33,65|Start sections at measures 33 and 65.|
|&#8209;clef&nbsp;1=C,2=C|Use alto clef for tracks 1 and 2.|
|&#8209;ottava&nbsp;1_5:3:0=2|Shift track 1 up two octaves starting at MBT time 5:3:0. The longer example 1_5:3:0=2,1_8:1:45=0 also restores the same track to its unshifted state at 8:1:45.|


# Development

MidiToLily is written in C++ using MFC. It compiles cleanly in Visual Studio 2012 and 2019. MidiToLily does what I need it to do, but undoubtably it could be improved.

# Links

* [LilyPond](https://lilypond.org/)
* [Chris Korda software](https://victimofleisure.github.io/software)
* [Scores created using MidiToLily](https://victimofleisure.github.io/misc/scores.html)

