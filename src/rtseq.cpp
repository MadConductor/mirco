#include "rtseq.hpp"

void RtEvent::append(RtEvent *next) {
  if(this->next != nullptr) {
    this->next->append(next);
  } else {
    this->next = next;
  }
}

struct RtEventResult RtNoteEvent::run() {
  return (RtEventResult){.next = next, .pausepulses = pausepulses};
  //this is apparently valid in C99 !
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

struct RtEventResult RtOperationEvent::run() {
  // Run Operation
}
