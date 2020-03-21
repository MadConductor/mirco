#pragma once
#include <cstdint>
#include <rtmidi/RtMidi.h>
#include <vector>


//-----------------------------------------------------------------

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

#define MIDI_PULSES_PQN  240

//-----------------------------------------------------------------

class RtEvent {
  private:
    RtEvent *next;

  public:
    virtual struct RtEventResult run() = 0;
    virtual void append(RtEvent * next);
};

struct RtEventResult {
    RtEvent *next;
    uint_fast32_t pausepulses;
};

class RtNoteEvent : public RtEvent {
  private:
    typedef RtEvent super;
    RtEvent *next;
    uint_fast32_t pausepulses;
    unsigned char message[3];
    //std::vector<unsigned char> message;

  public:
    RtNoteEvent(unsigned char status, unsigned char byte2, unsigned char byte3);
    RtNoteEvent(unsigned char status, unsigned char byte2, unsigned char byte3, uint_fast32_t pulses);
    struct RtEventResult run() override;
};

RtNoteEvent * MakeNoteMonophonic(unsigned char channel,
                               unsigned char note,
                               unsigned char on_velocity,//shall not be 0 (velocity of 0 turns notes off)
                               unsigned char off_veocity,
                               uint_fast32_t length,
                               uint_fast32_t afterpause);//shall not be 0

class RtOperationEvent : public RtEvent {
  public:
    struct RtEventResult run() override;
};
