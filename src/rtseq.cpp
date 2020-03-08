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

//-----------------------------------------------------------------













class RtEvent{
public:
  struct RtEventResult run();
  void append(RtEvent * next);
};





struct RtEventResult{
  RtEvent * next;
  uint_fast32_t sleeptime;
};





class RtNoteEvent : RtEvent{
private:
  RtEvent * next;
  uint_fast32_t sleeptime;
  const unsigned char message[3];
  //std::vector<unsigned char> message;

public:
  struct RtEventResult run(){
    //send midieventblafoo
    return (RtEventResult){.next = next, .sleeptime = sleeptime};
    //this is apparently valid in C99 !
  }

  void append(RtNoteEvent * next){
    this->next = next;
  }



};
