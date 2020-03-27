#include "rtseq.hpp"

extern unordered_map<int, vector<RtNoteOnEvent *>*> openNotes;

void RtEvent::append(RtEvent *next) {
  if(this->getNext() != nullptr) {
    this->getNext()->append(next);
  } else {
    this->setNext(next);
  }
}

struct RtEventResult RtNopEvent::run(RtMidiOut *m, Context rtContext, uint_fast32_t triggerKey) {
  return (RtEventResult){.next = getNext(), .pausepulses = getPausePulses()};
}

RtNopEvent::RtNopEvent(uint_fast32_t pulses) {
  pausepulses = pulses;
  next = nullptr;
}

RtNoteOnEvent::RtNoteOnEvent(unsigned char channel,
                           unsigned char byte2,
                           unsigned char byte3,
                           uint_fast32_t pulses) {
  message.push_back(MIDI_NOTE_ON_BYTE | channel);
  message.push_back(byte2);
  message.push_back(byte3);
  next = nullptr;
  pausepulses = pulses;
}

struct RtEventResult RtNoteOnEvent::run(RtMidiOut *m, Context rtContext, uint_fast32_t triggerKey) {
  openNotes[triggerKey]->push_back(this);
  m->sendMessage(&message);
  return (RtEventResult){.next = getNext(), .pausepulses = getPausePulses()};
}

bool RtNoteOnEvent::isKey(unsigned char key) {
  return this->getKey() == key;
};

RtNoteOffEvent::RtNoteOffEvent(unsigned char channel,
                           unsigned char byte2,
                           uint_fast32_t pulses) {
  message.push_back(MIDI_NOTE_OFF_BYTE | channel);
  message.push_back(byte2);
  message.push_back(0);

  // Legacy "Note Off" Message for older devices
  legacyMessage.push_back(MIDI_NOTE_ON_BYTE | channel); 
  legacyMessage.push_back(byte2);
  legacyMessage.push_back(0);

  next = nullptr;
  pausepulses = pulses;
}

RtNoteOffEvent::RtNoteOffEvent(RtNoteOnEvent *on) {
  unsigned char channel = on->getChannel();
  unsigned char byte2 = on->getKey();

  message.push_back(MIDI_NOTE_OFF_BYTE | channel);
  message.push_back(byte2);
  message.push_back(0);

  // Legacy "Note Off" Message for older devices
  legacyMessage.push_back(MIDI_NOTE_ON_BYTE | channel); 
  legacyMessage.push_back(byte2);
  legacyMessage.push_back(0);
}
struct RtEventResult RtNoteOffEvent::run(RtMidiOut *m, Context rtContext, uint_fast32_t triggerKey) {
  m->sendMessage(&message);
  m->sendMessage(&legacyMessage);
  vector<RtNoteOnEvent *> *on = openNotes[triggerKey];
  for(int i=0; i<(on->size()); i++) {
    if (on->at(i)->isKey(this->getKey())) on->erase(on->begin() + i);
  }
  return (RtEventResult){.next = getNext(), .pausepulses = getPausePulses()};
}

