//SECTION license --------------------------------

//TODO license



//SECTION includes -------------------------------

#include "stdio.h"

#include <map>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>
#include <thread>
#include <queue>

#include <rtmidi/RtMidi.h>

#include "lang.hpp"
#include "rtseq.hpp"

#include "param.hpp"


using clk = chrono::high_resolution_clock;

//SECTION global defines and constants -----------

#define INTERNAL_PPQN MIDI_PULSES_PQN
#define DEFAULT_EXTERNAL_PPQN 24
#define NS_MIN (1000ULL * 1000 * 1000 * 60)

//SECTION global value structures ----------------

struct global_settings  GLOBAL_SETTINGS {
       {false,                     false},
       {DEFAULT_EXTERNAL_PPQN,     false},
       {false,                     false}
};

class globalAtomics {
  atomic<bool> running;
  atomic<uint_fast16_t> internal_counter; // internal 960 PPQN counter
  atomic<unsigned char> external_counter;

  public:
  globalAtomics(bool runstate){
    running.store(runstate);
    internal_counter.store(0);
    external_counter.store(0);

  }

} GLOBAL_ATOMICS(true);



//SECTION global handlers -------------------------

void yyparse();
extern FILE *yyin;

extern unordered_map<int, RtEvent *> eventMap;
unordered_map<int, RtEvent *> playMap;
unordered_map<int, int> lastPulseMap;
mutex playMutex;

auto now = clk::now();
auto lastPulse = now;
deque<chrono::nanoseconds> deltas;
uint_fast32_t numPulses = 0;

//SECTION function definitions --------------------

float estimateBpm() {
  int size = deltas.size();
  chrono::nanoseconds avgDelta(0);

  for (int i=0; i<size; i++) {
    auto delta = deltas[i];
    avgDelta += delta / size;
  }

  float estimatedBpm = (((float)NS_MIN) / avgDelta.count()) / GLOBAL_SETTINGS.INPUT_PPQN.val;
  return estimatedBpm;
};

void handleClockPulse(vector<unsigned char> *message) {
  // printf("Received Midi Clock Pulse Message \n");
  now = clk::now();
  auto delta = now - lastPulse;
  deltas.push_back(delta);
  if (deltas.size() > (GLOBAL_SETTINGS.INPUT_PPQN.val * 4)) {
    deltas.pop_front();
  }
  if (numPulses % (GLOBAL_SETTINGS.INPUT_PPQN.val * 4) == 0) { 
    printf("Estimated BPM: %f\n", estimateBpm()); 
  }

  lastPulse = now;
  numPulses++;
}

void handleOnMsg(vector<unsigned char> *message) {
  unsigned char key = message->at(1);
  RtEvent *event;
  try {
    event = eventMap.at(key);
  } catch (const std::out_of_range& oor) {
    try {
      event = eventMap.at(-2); // default mapping 
    } catch (const std::out_of_range& oor) {
      return;
    }
  }
  lock_guard<mutex> guard(playMutex);
  playMap[key] = event;
}

void handleOffMsg(vector<unsigned char> *message) {
  unsigned char key = message->at(1);
  lock_guard<mutex> guard(playMutex);
  playMap.erase(key);
  lastPulseMap.erase(key);
}


void onmessage(double deltatime, vector<unsigned char> *message, void *userData) {
  unsigned char status = message->at(0);
  switch(status) {
    case MIDI_TIME_CLOCK_BYTE:
      handleClockPulse(message);
      break;
    case MIDI_START_BYTE: 
      printf("Received Midi Clock Start Message \n");
      break;
    case MIDI_CONTINUE_BYTE: 
      printf("Received Midi Clock Continue Message \n");
      break;
    case MIDI_STOP_BYTE: 
      printf("Received Midi Clock Stop Message \n");
      break;
    case MIDI_POS_PTR_BYTE: 
      printf("Received Midi Position Pointer Message \n");
      break;
    case MIDI_NOTE_ON_BYTE: 
      handleOnMsg(message);
      break;
    case MIDI_NOTE_OFF_BYTE:
      handleOffMsg(message);
      break;
    case MIDI_POLY_AFTER_BYTE: 
      printf("Received Midi Note Aftertouch Message \n");
      break;
    default:
      fprintf(stderr, "Unknown Midi Message: 0x%x \n", status);
      break;
  }
}

RtMidiIn *openMidiIn() {
  RtMidiIn *midiin = new RtMidiIn(
    RtMidi::Api::UNSPECIFIED, // TODO: Command line parameter
    "Mirco Sequencer"
  );
  unsigned int inPorts = midiin->getPortCount();

  midiin->setCallback(&onmessage);
  midiin->ignoreTypes( false, false, false );

  if (inPorts == 0) {
    printf("No input ports available! Connect one manually!\n");
  } else {
    midiin->openPort(0);
  }
  return midiin;
}

RtMidiOut *openMidiOut() {
  RtMidiOut *midiout = new RtMidiOut(
    RtMidi::Api::UNSPECIFIED, // TODO: Command line parameter
    "Mirco Sequencer"
  );  
  unsigned int outPorts = midiout->getPortCount();
  if (outPorts == 0) {
    printf("No output ports available! Connect one manually!\n");
  } else {
    midiout->openPort(0);
  }
  return midiout;
}

void runEvent(RtEvent *event, RtMidiOut *midiout, Context *rtContext, uint_fast32_t key, uint_fast32_t totalPulses) {
  int lastPulse;
  try {
    lastPulse = lastPulseMap.at(key);
  } catch (const std::out_of_range& oor) {
    lastPulse = totalPulses;
  }

  if ((lastPulse + (event->getPausePulses())) <= totalPulses) {
    RtEventResult res = event->run(midiout, *rtContext);
    
    if (res.next != nullptr) {  
      lock_guard<mutex> guard(playMutex);
      playMap[key] = res.next;
      lastPulseMap[key] = totalPulses;
      if (res.pausepulses == 0) {
        runEvent(res.next, midiout, rtContext, key, totalPulses);
      }
    } else {
      printf("Sequence Ended\n");
      lock_guard<mutex> guard(playMutex);
      playMap.erase(key);
      lastPulseMap.erase(key);
    }
  }
}      

void outputLoop() {
  RtMidiOut *midiout = openMidiOut();
  uint_fast32_t totalPulses = 0;
  Context *rtContext = new Context({});
  float bpm = estimateBpm();
  unordered_map<int, RtEvent *> playMapCopy(playMap);

  while (true) {
    if (totalPulses % (INTERNAL_PPQN*4) == 0) {
      printf("Bar completed in output thread \n");
      bpm = estimateBpm();
    }
    if (true) { 
      lock_guard<mutex> guard(playMutex);
      playMapCopy = unordered_map<int, RtEvent *>(playMap);
    }

    uint_fast32_t pulseDelay = (NS_MIN / (bpm * INTERNAL_PPQN));
    auto nextPulse = clk::now() + chrono::nanoseconds(pulseDelay);

    auto it = playMapCopy.begin();
    while (it != playMapCopy.end()) {
      int key = it->first;
      RtEvent *event = it->second;
      runEvent(event, midiout, rtContext, key, totalPulses);
      it++;
    }
    totalPulses++;
    this_thread::sleep_until(nextPulse);
  }
}

//SECTION main -----------------------------------

int main(int argc, char* argv[]) {
  char *filename = argv[1];

  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Cannot open: ");
    printf(filename);
    printf("\n");
    return -1;
  }
  yyin = file;

  yyparse();
  thread inputThread(openMidiIn);
  thread outputThread(outputLoop);

  printf("\nReading MIDI input ... press <enter> to quit.\n");
  char input;
  std::cin.get(input);
  inputThread.join();
  outputThread.join();
}
