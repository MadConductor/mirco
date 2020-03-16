#include "rtseq.hpp"

struct RtEventResult RtNoteEvent::run() {
  return (RtEventResult){.next = next, .pausepulses = pausepulses};
  //this is apparently valid in C99 !
}

void RtNoteEvent::append(RtEvent *next) {
  if(this->next != nullptr) {
    this->next->append(next);
  } else {
    this->next = next;
  }
}

RtNoteEvent::RtNoteEvent(unsigned char status, unsigned char byte2, unsigned char byte3) {
  message[0] = status;
  message[1] = byte2;
  message[2] = byte3;
  next = nullptr;
  //pausepulses not initialized !
}

RtNoteEvent::RtNoteEvent(unsigned char status,
                         unsigned char byte2,
                         unsigned char byte3,
                         uint_fast32_t pulses) {
  message[0] = status;
  message[1] = byte2;
  message[2] = byte3;
  next = nullptr;
  pausepulses = pulses;
}

RtNoteEvent * MakeNoteMonophonic(unsigned char channel,
                               unsigned char note,
                               unsigned char on_velocity,
                               unsigned char off_velocity,
                               uint_fast32_t length,
                               uint_fast32_t afterpause){
  RtNoteEvent * retval = new RtNoteEvent( MIDI_NOTE_ON_BYTE | channel, note, on_velocity, length);
  retval->append(new RtNoteEvent(MIDI_NOTE_OFF_BYTE | channel, note, off_velocity, 1) );
  retval->append(new RtNoteEvent(MIDI_NOTE_ON_BYTE | channel, note, 0, afterpause -1));
  return retval;
}
