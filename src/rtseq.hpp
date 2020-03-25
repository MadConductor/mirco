#pragma once
#include <cstdint>
#include <rtmidi/RtMidi.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>

using namespace std;
// language fwd declarations

class SequenceNode;
class SequenceParentNode;
class Definition;
class Sequence;
class Chord;
class Note;
class Tone;
class Identifier;
class RtResource;

//-----------------------------------------------------------------

using Context = unordered_map<string, SequenceNode *>;

//MIDI Status bytes
//remember: status bytes are 128+
//data bytes are 0-127

#define MIDI_NOTE_OFF_BYTE   0x80
#define MIDI_NOTE_ON_BYTE    0x90
#define MIDI_POLY_AFTER_BYTE 0xA0
//byte | channel, Note Number, pressure / velocity

#define MIDI_CHAN_PITCH_BYTE 0xE0
//byte | channel, LSB (0-127), MSB (0-127)

#define MIDI_CHAN_AFTER_BYTE 0xD0
//byte | channel, Pressure (0-127)

#define MIDI_POS_PTR_BYTE    0xF2
//byte, LSB (0-127), MSB (0-127)

#define MIDI_TIME_CLOCK_BYTE 0xF8
#define MIDI_START_BYTE      0xFA
#define MIDI_CONTINUE_BYTE   0xFB
#define MIDI_STOP_BYTE       0xFC
//no data bytes

#define MIDI_PULSES_PQN  940

//-----------------------------------------------------------------
class RtNoteOffEvent;

class RtEvent {
  private:
    RtEvent *next;

  public:
    virtual uint_fast32_t getPausePulses() = 0;
    virtual void setNext(RtEvent * n) = 0;
    virtual RtEvent *getNext() = 0;
    virtual struct RtEventResult run(RtMidiOut *m, Context r, uint_fast32_t triggerKey) = 0;
    virtual void append(RtEvent * next);
};

struct RtEventResult {
    RtEvent *next;
    uint_fast32_t pausepulses;
};

/* 
  Helper class that does nothing but host the next event
  Used for building event chains
*/
class RtNopEvent : public RtEvent {
  private:
    RtEvent *next;
    uint_fast32_t pausepulses;

  public:
    RtNopEvent(uint_fast32_t pulses);
    uint_fast32_t getPausePulses() override { return pausepulses; };
    void setNext(RtEvent *n) override { next = n; };
    RtEvent *getNext() override { return next; };
    struct RtEventResult run(RtMidiOut *m, Context r, uint_fast32_t triggerKey) override;

};

class RtNoteOnEvent : public RtEvent {
  private:
    typedef RtEvent super;
    RtEvent *next;
    uint_fast32_t pausepulses;
    vector<unsigned char> message;
    //std::vector<unsigned char> message;

  public:
    RtNoteOnEvent(unsigned char channel, unsigned char byte2, unsigned char byte3, uint_fast32_t pulses);
    uint_fast32_t getPausePulses() override { return pausepulses; };
    void setNext(RtEvent *n) override { next = n; };
    RtEvent *getNext() override { return next; };
    unsigned char getChannel() { return message[0] & 0x0f; }
    unsigned char getKey() { return message[1]; }
    unsigned char getVelocity() { return message[2]; }
    bool isKey(unsigned char key);
    struct RtEventResult run(RtMidiOut *m, Context r, uint_fast32_t triggerKey) override;
};

class RtNoteOffEvent : public RtEvent {
  private:
    typedef RtEvent super;
    RtEvent *next;
    uint_fast32_t pausepulses;
    vector<unsigned char> message;
    vector<unsigned char> legacyMessage;
    //std::vector<unsigned char> message;

  public:
    RtNoteOffEvent(unsigned char channel, unsigned char byte2, uint_fast32_t pulses);
    RtNoteOffEvent(RtNoteOnEvent *on);
    uint_fast32_t getPausePulses() override { return pausepulses; };
    void setNext(RtEvent *n) override { next = n; };
    RtEvent *getNext() override { return next; };
    unsigned char getKey() { return message[1]; }
    struct RtEventResult run(RtMidiOut *m, Context r, uint_fast32_t triggerKey) override;
};
