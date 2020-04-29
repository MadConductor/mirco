#pragma once
#include <cstdint>
#include <rtmidi/RtMidi.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <mutex>

#include "object.hpp"

using namespace std;
// language fwd declarations

class SequenceNode;
class SequenceParentNode;
class Definition;
class Sequence;
class Chord;
class Note;
class Pause;
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

#define MIDI_CHAN_MODE_BYTE 0xB0
#define MIDI_CHAN_OFF_BYTE 0x7B
//chan_mode | channel, chan_off, 00H

#define MIDI_POS_PTR_BYTE    0xF2
// byte, LSB (0-127), MSB (0-127)

#define MIDI_TIME_CLOCK_BYTE 0xF8
#define MIDI_START_BYTE      0xFA
#define MIDI_CONTINUE_BYTE   0xFB
#define MIDI_STOP_BYTE       0xFC
// no data bytes

#define MIDI_PULSES_PQN  960

//-----------------------------------------------------------------
class RtNoteOffEvent;
class RtEvent;
class RtEvent : public object::cloneable<RtEvent> {
  private:
    RtEvent *next = nullptr;
    bool cloneNext = true;

  public:
    virtual uint_fast32_t getPausePulses() = 0;
    void setCloneNext(bool b) { cloneNext = b; };
    bool getCloneNext() const { return cloneNext; };
    static RtEvent *decideCloneNext(const RtEvent &obj); 
    virtual void setNext(RtEvent * n) = 0;
    virtual RtEvent *getNext() const = 0;
    virtual struct RtEventResult run(RtMidiOut *m, Context r, int_fast32_t triggerKey) = 0;
    virtual void append(RtEvent * next);
};

struct RtEventResult {
    RtEvent *next;
    uint_fast32_t pausepulses;
};

/* 
  Helper class that does nothing but host the next event.
  Used for building event chains.
*/
class RtNopEvent : public RtEvent, public object::cloneable<RtNopEvent>  {
  private:
    RtEvent *next = nullptr;
    uint_fast32_t pausepulses;

  public:
    RtNopEvent(uint_fast32_t pulses);
    RtNopEvent(const RtNopEvent &obj) 
    : pausepulses(obj.pausepulses),        
      next(RtEvent::decideCloneNext(obj)) {};

    uint_fast32_t getPausePulses() override { return pausepulses; };
    void setNext(RtEvent *n) override { next = n; };
    RtEvent *getNext() const override { return next; };
    struct RtEventResult run(RtMidiOut *m, Context r, int_fast32_t triggerKey) override;

    virtual RtNopEvent *clone() const override {
      return new RtNopEvent(*this); 
    };
};

class RtNoteOnEvent : public RtEvent, public object::cloneable<RtNoteOnEvent> {
  private:
    typedef RtEvent super;
    RtEvent *next = nullptr;
    uint_fast32_t pausepulses;
    vector<unsigned char> message;
    //std::vector<unsigned char> message;

  public:
    RtNoteOnEvent(unsigned char channel, unsigned char byte2, unsigned char byte3, uint_fast32_t pulses);
    RtNoteOnEvent(const RtNoteOnEvent &obj)
      : pausepulses(obj.pausepulses), message(obj.message), 
        next(RtEvent::decideCloneNext(obj)) {};

    uint_fast32_t getPausePulses() override { return pausepulses; };
    void setNext(RtEvent *n) override { next = n; };
    RtEvent *getNext() const override { return next; };
    unsigned char getChannel() const { return message[0] & 0x0f; }
    unsigned char getKey() const { return message[1]; }
    unsigned char getVelocity() const { return message[2]; }
    bool isKey(unsigned char key);
    struct RtEventResult run(RtMidiOut *m, Context r, int_fast32_t triggerKey) override;

    RtNoteOnEvent *clone() const override {
      return new RtNoteOnEvent(*this); 
    };
};

class RtNoteOffEvent : public RtEvent, public object::cloneable<RtNoteOffEvent> {
  private:
    typedef RtEvent super;
    RtEvent *next = nullptr;
    uint_fast32_t pausepulses;
    vector<unsigned char> message;
    vector<unsigned char> legacyMessage;
    //std::vector<unsigned char> message;

  public:
    RtNoteOffEvent(unsigned char channel, unsigned char byte2, uint_fast32_t pulses);
    RtNoteOffEvent(RtNoteOnEvent *on);
    RtNoteOffEvent(const RtNoteOffEvent &obj) 
      : pausepulses(obj.pausepulses), 
        message(obj.message), 
        legacyMessage(obj.legacyMessage), 
        next(RtEvent::decideCloneNext(obj)) {};

    uint_fast32_t getPausePulses() override { return pausepulses; };
    void setNext(RtEvent *n) override { next = n; };
    RtEvent *getNext() const override { return next; };
    unsigned char getChannel() const { return message[0] & 0x0f; }
    unsigned char getKey() const { return message[1]; }
    unsigned char getVelocity() const { return message[2]; }
    struct RtEventResult run(RtMidiOut *m, Context r, int_fast32_t triggerKey) override;

    RtNoteOffEvent *clone() const override {
      return new RtNoteOffEvent(*this); 
    };
};
