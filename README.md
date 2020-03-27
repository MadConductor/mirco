# MIRCO
Mirco is a MIDI sequence definition language, it takes midi input and maps simple input notes to more complex patterns.
Defined sequences can be parameterised and reused, allowing the easy creation of complex, variative patterns.

## Setup

Install `bison` and `flex`.

### Arch:

`
pacman -S bison flex
`

Run `make`.

Run `./mirco syntax.mirco <args>`.

Edit `syntax.mirco`!

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
