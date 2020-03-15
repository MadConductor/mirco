#include "rtseq.hpp"

struct RtEventResult RtEvent::run() {
  return (RtEventResult){.next = next, .pausepulses = pausepulses};
  //this is apparently valid in C99 !
}

void RtEvent::append(RtEvent *next) {
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
}

struct RtEventResult RtNoteEvent::run() {
  return super::run();
}
