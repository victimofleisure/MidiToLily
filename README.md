# LilyToMidi

## Convert a MIDI file to LilyPond format

MidiToLily is a console application that converts a MIDI file to LilyPond format. You can create sheet music from a MIDI file by using MidiToLily as a preprocessor for LilyPond. MidiToLily is an alternative to the midi2ly script that ships with LilyPond. MidiToLily is currently only available for Windows, though that may change.

# Getting started

Download the latest release, unzip it, and run MidiToLily.exe. MidiToLily doesn't have an installer, doesn't have dependencies, and doesn't store things in the registry or anywhere else.

If your MIDI file is fully quantized, try this simple command:
```
MidiToLily myfile.mid
```
If that succeeds, create your score with this command:
```
LilyPond "myfile [lily].ly"
```
If your MIDI file is not fully quantized, or you get an error, try this instead:
```
MidiToLily myfile.mid -quant 16 -triplet 16
```
If you're sure your file doesn't contain triplets, you can omit the triplet parameter. Depending on your file, other quantization values may work better.

# Command line

### Usage: MidiToLily MIDI_FILE [options]

|Option|Description|
|---|---|
|MIDI_FILE|The path of the input MIDI file; if it contains spaces, enclose it in double quotes.|
|&#8209;output&nbsp;PATH|The path of the output LilyPond file; if it contains spaces, enclose it in double quotes.|
|&#8209;title&nbsp;STRING|The title of the composition; if it contains spaces, enclose it in double quotes.|
|&#8209;subtitle&nbsp;STRING|The subtitle of the composition; if it contains spaces, enclose it in double quotes.|
|&#8209;opus&nbsp;STRING|The title of the opus; if it contains spaces, enclose it in double quotes.|
|&#8209;piece&nbsp;STRING|The title of the piece; if it contains spaces, enclose it in double quotes.|
|&#8209;composer&nbsp;STRING|The name of the composer; if it contains spaces, enclose it in double quotes.|
|&#8209;copyright&nbsp;STRING|The copyright notice; if it contains spaces, enclose it in double quotes.|
|&#8209;frenched|Hide empty staves.|
|&#8209;verify|Compare the input MIDI file to a LilyPond-generated MIDI file; MidiToLilyIn.txt and LilyPondMidi.txt are created.|
|&#8209;quant&nbsp;DURATION|Quantize note start and end times to the specified duration (1=whole, 2=half, 4=quarter, etc.); if combined with triplet quantization, notes snap to the nearest grid.|
|&#8209;triplet&nbsp;DURATION|Quantize note start and end times to the specified TRIPLET duration (1=whole, 2=half, 4=quarter, etc.); if combined with regular quantization, notes snap to the nearest grid.|
|&#8209;offset&nbsp;OFFSET|Signed offset in ticks added to all MIDI event times.|
|&#8209;clef&nbsp;LIST|Comma-separated list of clef overrides, each consisting of t=c, where t is a track index and c is a clef name defined in LilyPond. By default, a track is automatically assigned treble or bass clef, depending on what pitches it uses.|
|&#8209;section&nbsp;LIST|Comma-separated list of one-based measure numbers, each of which specifies the start of a section.|
|&#8209;ottava&nbsp;LIST|Comma-separated list of octave shifts, each consisting of t_M:B:T=n where t is a track index, M:B:T is a time in Measure:Beat:Tick format, and n is a signed number of octaves to transpose the staff by.|
|&#8209;staves&nbsp;LIST|Comma-separated list of track indices specifying which tracks are assigned to staves, from top to bottom.|
|&#8209;time&nbsp;LIST|Comma-separated list of time signatures, each consisting of M=n/d where M is a one-based measure number, and n and d are the the time signature's numerator and denominator.|
|&#8209;key&nbsp;LIST|Comma-separated list of key signatures, each consisting of M=k where M is a one-based measure number, and k is the key signature in LilyPond note format, optionally followed by the letter 'm' to indicate a minor key.|
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
|&#8209;staves&nbsp;3,1,2|Assign track 3 to the top staff, track 1 to the middle staff, and track 2 to the bottom staff.|
|&#8209;time&nbsp;1=3/4,9=4/4|Set the time signature to 3/4 at measure 1, and then change it to 4/4 at measure 9.|
|&#8209;key&nbsp;1=ef,9=csm|Set the key signature to E flat major at measure 1, and then change it to C sharp minor at measure 9.|

# Tips

Only type 0 MIDI files are supported, and the first track is reserved for the tempo map.

Track indices are zero-based, but since the tempo map is track zero, the first track with *note events* is track one. The valid range of a track index in a command line parameter is from one to the number of tracks in the MIDI file.

If your input MIDI file isn't quantized, use the quant and triplet parameters, otherwise you'll get errors, such as invalid duration.

If your tracks aren't shown in the desired order, you can use the staves parameter to reorder them. The staves parameter can also be used to hide specific tracks. The default behavior is tracks are assigned sequentially from top to bottom, i.e. track one gets the top stave, and the last track gets the bottom stave.

Here's an example of MIDI verification, by running MidiToLily again after LilyPond runs. For the verify pass, cosmetic parameters like title or composer can be omitted.

```
MidiToLily.exe "my song.mid" /quant 16 /triplet 16 /title "My Song" /composer "My Name"
lilypond.exe -dno-point-and-click "my song [lily].ly"
MidiToLily.exe "my song.mid" /quant 16 /triplet 16 /verify
```

# Development

MidiToLily is written in C++ using MFC. It compiles cleanly in Visual Studio 2012 and 2019. MidiToLily does what I need it to do, but undoubtedly it could be improved. Enhancements I'm considering include:

* Meter denominators other than 4
* Meter changes
* Tempo changes
* Better triplet handling; current implementation fails in various corner cases

# Links

* [LilyPond](https://lilypond.org/)
* [MidiToLily blog](https://miditolily.blogspot.com/)
* [Chris Korda software](https://victimofleisure.github.io/software)
* [Scores created using MidiToLily](https://victimofleisure.github.io/misc/scores.html)

