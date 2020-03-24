#include "rtseq.hpp"

void RtEvent::append(RtEvent *next) {
  if(this->getNext() != nullptr) {
    this->getNext()->append(next);
  } else {
    this->setNext(next);
  }
}

struct RtEventResult RtNopEvent::run(RtMidiOut *m, Context rtContext) {
  return (RtEventResult){.next = getNext(), .pausepulses = getPausePulses()};
}

RtNopEvent::RtNopEvent(uint_fast32_t pulses) {
  pausepulses = pulses;
  next = nullptr;
}


struct RtEventResult RtNoteEvent::run(RtMidiOut *m, Context rtContext) {
  printf("0x%x, %d, %d\n", message[0], message[1], message[2]);
  return (RtEventResult){.next = getNext(), .pausepulses = getPausePulses()};
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
