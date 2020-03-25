/*
      ___                       ___           ___           ___     
     /__/\        ___          /  /\         /  /\         /  /\    
    |  |::\      /  /\        /  /::\       /  /:/        /  /::\   
    |  |:|:\    /  /:/       /  /:/\:\     /  /:/        /  /:/\:\  
  __|__|:|\:\  /__/::\      /  /:/~/:/    /  /:/  ___   /  /:/  \:\ 
 /__/::::| \:\ \__\/\:\__  /__/:/ /:/___ /__/:/  /  /\ /__/:/ \__\:\
 \  \:\~~\__\/    \  \:\/\ \  \:\/:::::/ \  \:\ /  /:/ \  \:\ /  /:/
  \  \:\           \__\::/  \  \::/~~~~   \  \:\  /:/   \  \:\  /:/ 
   \  \:\          /__/:/    \  \:\        \  \:\/:/     \  \:\/:/  
    \  \:\         \__\/      \  \:\        \  \::/       \  \::/   
     \__\/                     \__\/         \__\/         \__\/    
  
  Sequence Definition Language

  Lukas Hannen, Max Wolschlager
*/

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
#define NS_MIN (1000ULL * 1000 * 1000 * 60)

//SECTION global value structures ----------------

struct global_settings GLOBAL_SETTINGS;

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
unordered_map<int, int> nextPulseMap;
unordered_map<int, vector<RtNoteOnEvent *>*> openNotes;
mutex playMutex;

auto now = clk::now();
auto lastPulse = now;
deque<chrono::nanoseconds> deltas;
uint_fast32_t numPulses = 0;

//SECTION function definitions --------------------

/*
  Averages the last bar of clock message deltas
  in order to get a steady bpm value.
*/
float estimateBpm() { 
  if(!GLOBAL_SETTINGS.FOLLOW_INPUT_CLOCK.val) return GLOBAL_SETTINGS.DEFAULT_BPM.val;
  int size = deltas.size();
  chrono::nanoseconds avgDelta(0);

  for (int i=0; i<size; i++) {
    auto delta = deltas[i];
    avgDelta += delta / size;
  }

  float estimatedBpm = (((float)NS_MIN) / avgDelta.count()) / GLOBAL_SETTINGS.INPUT_PPQN.val;
  return estimatedBpm;
};

/*
  Handles an incoming clock pulse by computing
  the time since the last clock pulse and 
  storing it.
*/
void handleClockPulse(vector<unsigned char> *message) {
  // printf("Received Midi Clock Pulse Message \n");
  now = clk::now();
  auto delta = now - lastPulse;

  //TODO thread-safety
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

/*
  Handles "Note On" message by looking up
  the mapped sequence for the trigger note
  and storing it in the playMap.
*/
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
  openNotes[key] = new vector<RtNoteOnEvent *>({});
}

/*
  Handles "Note Off" message by shutting
  all currently active notes off or replacing
  the playing sequence with an RtNopEvent.
*/
void handleOffMsg(vector<unsigned char> *message) {
  unsigned char key = message->at(1);
  vector<RtNoteOnEvent *> *on;
  try {
    on = openNotes.at(key);
  } catch (const std::out_of_range& oor) {
    printf("Input device sent unprecedented Note Off Message\n");
    return;
  }
  RtNopEvent *start = new RtNopEvent(0);
  for(int i=0; i<(on->size()); i++) {
    RtNoteOffEvent *off = new RtNoteOffEvent(on->at(i));
    start->append(off);
  }
  lock_guard<mutex> guard(playMutex);
  playMap[key] = start;
}

/*
  Invokes midi message handlers.
*/
void onmessage(double deltatime, vector<unsigned char> *message, void *userData) {
  unsigned char status = message->at(0);
  switch(status) {
    case MIDI_TIME_CLOCK_BYTE:
      if(GLOBAL_SETTINGS.FOLLOW_INPUT_CLOCK.val)handleClockPulse(message);
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

/*
  Opens RtMidiIn
*/
RtMidiIn *openMidiIn() {
  RtMidiIn *midiin = new RtMidiIn(
    RtMidi::Api::UNSPECIFIED, // TODO: Command line parameter
    "Mirco Sequencer"
  );
  unsigned int inPorts = midiin->getPortCount();

  midiin->setCallback(&onmessage);
  midiin->ignoreTypes(false, false, false);

  if (inPorts == 0) {
    printf("No input ports available! Connect one manually!\n");
  } else {
    midiin->openPort(0); // TODO: Command line parameter
  }
  return midiin;
}

/*
  Opens RtMidiOut
*/
RtMidiOut *openMidiOut() {
  RtMidiOut *midiout = new RtMidiOut(
    RtMidi::Api::UNSPECIFIED, // TODO: Command line parameter
    "Mirco Sequencer"
  );  
  unsigned int outPorts = midiout->getPortCount();
  if (outPorts == 0) {
    printf("No output ports available! Connect one manually!\n");
  } else {
    midiout->openPort(0); // TODO: Command line parameter
  }
  return midiout;
}

/*
  Runs a realtime event and stores the next event
  in the playMap or runs the next event immediately.
  Erases event from playMap if it has no successor. 
*/
RtEventResult runEvent(RtEvent *event, RtMidiOut *midiout, Context *rtContext, uint_fast32_t key, uint_fast32_t totalPulses) {
  printf("Executing RtEvent at %d pulses\n", totalPulses);
  RtEventResult res = event->run(midiout, *rtContext, key);
  bool end = res.next == nullptr;

  if (res.pausepulses == 0 && !end) {
    res = runEvent(res.next, midiout, rtContext, key, totalPulses);
  } else if (!end) {  
    lock_guard<mutex> guard(playMutex);
    nextPulseMap[key] = totalPulses + res.pausepulses;
    playMap[key] = res.next;
  } else {
    printf("Sequence Ended\n");
    lock_guard<mutex> guard(playMutex);
    playMap.erase(key);
    nextPulseMap.erase(key);
  }
  return res; 
}

/*
  Midi output loop
*/
void outputLoop() {
  RtMidiOut *midiout = openMidiOut();
  // stores how many internal clock pulses have passed
  uint_fast32_t totalPulses = 0; 
  // TODO: add realtime context (trigger note eg.)
  Context *rtContext = new Context({});
  float bpm = estimateBpm();
  // local copy of the playmap (concurrency)
  unordered_map<int, RtEvent *> playMapCopy(playMap);

  while (true) {
    if (totalPulses % (INTERNAL_PPQN*4) == 0) {
      printf("Bar completed in output thread \n");
      // reestimate bpm every bar (lo-pass bitcrusher :P)
      bpm = estimateBpm();
    }
    if (true) { // just a scope for the lock_guard
      lock_guard<mutex> guard(playMutex);
      playMapCopy = unordered_map<int, RtEvent *>(playMap);
    }

    // calculate when the next internal clock 
    // pulse will happen in nanoseconds
    uint_fast32_t pulseDelay = (NS_MIN / (bpm * INTERNAL_PPQN));
    auto nextPulseNs = clk::now() + chrono::nanoseconds(pulseDelay);

    // iterate over playMap
    auto it = playMapCopy.begin();
    while (it != playMapCopy.end()) {
      int key = it->first;
      RtEvent *event = it->second;

      // next clock pulse for current key
      int nextPulse;
      try {
        nextPulse = nextPulseMap.at(key);
      } catch (const std::out_of_range& oor) {
        nextPulse = totalPulses;
      }

      // run event if it is scheduled
      if (nextPulse <= totalPulses) {
        runEvent(event, midiout, rtContext, key, totalPulses);
      }
      it++;
    }
    totalPulses++;
    
    // sleep thread until next internal pulse
    // TODO: support high bpm (will only work
    // on real time kernels as of now)
    this_thread::sleep_until(nextPulseNs);
  }
}

//SECTION main -----------------------------------
int main(int argc, char* argv[]) {
  init_settings(GLOBAL_SETTINGS);
  unpack_cmdline(&GLOBAL_SETTINGS, argc, argv);

  // open mirco file
  char *filename = argv[1];

  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Cannot open: ");
    printf(filename);
    printf("\n");
    return -1;
  }
  yyin = file;

  // parse mirco file (generated by parser.ypp)
  yyparse();

  // start input and output threads
  thread inputThread(openMidiIn);
  thread outputThread(outputLoop);

  printf("\nReading MIDI input ... press <enter> to quit.\n");
  char input;
  std::cin.get(input);
  inputThread.join();
  outputThread.join();
}
