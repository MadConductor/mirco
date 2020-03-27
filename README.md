# MIRCO
Mirco is a MIDI sequence definition language, it takes midi input and maps simple input notes to more complex patterns.
Defined sequences can be parameterised and reused, allowing the easy creation of complex, variative patterns.

## Setup

Install `bison` and `flex`.

### Arch:

`
pacman -S bison flex
`

### Compiling:

Run `make`.

### Running:

Run `./mirco syntax.mirco <args>`.

Edit `syntax.mirco`!

## Command Line Arguments:

## --follow-clk

If set mirco will try to match the speed
of the incoming midi clock. 
(Mirco assumes 24 midi clock pulses per quarter note)

## --bpm

Sets beats per minute of the internal output loop. 
If --follow-clk is specified it will be overridden by 
the input clock BPM.

Defaults to 120.

## --api

Options: 
    - `alsa`
    - `jack`
    - `macosx_core`

Specifies the midi api to be used by RtMidi.
Following apis are available to be selected explicitly:

Linux provides ALSA and JACK apis (assuming they are installed).
Note: JACK seems to introduce alot of latency (500 - 1000ms),
use ALSA for the best performance.

MacOs allows for the installation of JACK. If JACK is installed
select either `jack` or `macosx_core`.

On Windows only one api is available, rendering this
argument unnecessary.

WARNING: Currently only linux apis are tested, 
since both contributors use linux.

## --input & --output

Specify the input and output indexes for RtMidi.
If the specified index does not exist mirco will
wait until it is available.
For more control we recommend something like
[Catia](https://kx.studio/Applications:Catia) or
[Claudia](https://kx.studio/Applications:Claudia) 

## Syntax

### Note:

Notes can be defined with this syntax:

```
C#3
D3|50
G5:8:2
G#5|100:16
```

The integer after `|` specifies the note velocity.
Note length and playtime are set after a colon  (`:`).
The first integer specifies the note length denominator (`:16` for 16ths)
and the second integer describes the note play length denominator
(`:8:2` will play a 16th and wait another 16th after that).
Both integers default to `1` if not specified.

### Tone Literals:

Tone Literals are only used for arithmetic.
They can be defined with the syntax: `<octaves>o<semitones>s`

```
1o
2s
3o2s
```

### Chords:

Chords are simultaneously played notes.

```
(C3, D#3, G3)
(C3, D#3, G3)|100
(C3, D#3, G3):16
(C3, D#3, G3)|50:8:2
```

Velocity and note length of the individual notes 
will be ignored and overriden by the chord's parameters.

### Realtime Resources:

Realtime resources are sequence nodes which are only 
known at runtime (for example the note that triggered the
playing sequence). They are prefixed with the `$` character. 

Currently these realtime resources exist:

- $trigger: the note which triggered the sequence.

### Sequences:

Sequences can be defined and parameterized as follows:

```
sequence seq(arg1, arg2) {
    arg1, C4, D4, arg2
}
```

They can also be defined anonymously:

```

sequence seq() {
    { F4, C4, D4, G4 } + $trigger
}
```

### Operations:

Sequences and Notes can be subjects to arithmetic:

```
C2|100 + 2s
(C3, D#3, G3) + 1o
seq(C3, D#3) + 7s
```

### Mappings:

Sequences can be mapped to midi input notes, 
`default` (fallback sequences if input note is not mapped) and
`auto` (autoplay).

```
C4 : seq(F4, D4)
default: seq(G4, D4)
auto: seq(C3, C3)
```
