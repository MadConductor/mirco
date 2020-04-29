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

  Sequence Definition Language (c) 2020 Mircosoft
  Licensed GPLv3 (see LICENSE)

  Lukas Hannen, Max Wolschlager
*/

//SECTION includes -------------------------------


#include <map>
#include <queue>
#include <vector>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstdio>

#include <csignal>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <rtmidi/RtMidi.h>

#include "lang.hpp"
#include "rtseq.hpp"
#include "param.hpp"
#include "error.hpp"
#include "yytype.hpp"


using clk = chrono::high_resolution_clock;

//SECTION global defines and constants -----------

#define INTERNAL_PPQN MIDI_PULSES_PQN
#define NS_MIN (1000ULL * 1000 * 1000 * 60)
#define KERNEL_MS 20

//SECTION global value structures ----------------

struct global_settings GLOBAL_SETTINGS;

class globalAtomics {
  atomic<int> running;
  std::condition_variable statechange;
  std::mutex waitmtx;

  public:
  enum {
        PLAY,//"normal" runstate
        START,//MIDI_START_BYTE -> START (resets the sequence) -> PLAY
        CONTINUE,//MIDI_CONTINUE_BYTE -> CONTINUE (no sequence reset) -> PLAY
        SLEEP,//output loop skipping pulses
        STOP,//MIDI_STOP_BYTE -> STOP (finish current notes) -> HALT
        HALT,//"halted" runstate
        PREABORT,//unused but hey
        ABORT//all notes off immediately, abort program
  };

  globalAtomics(int runstate){
    running.store(runstate);
  }

  int getRunState(){
    return running.load();
  }

  void setRunState(int s){
    running.store(s);//may be invalid, don't store invalid runstates!
    std::unique_lock<std::mutex> lck (waitmtx);
    statechange.notify_all();
  }

  int waitForStateChange(){
    std::unique_lock<std::mutex> lck (waitmtx);
    statechange.wait(lck);
    return getRunState();
  }

  int waitMsForStateChange(chrono::microseconds time){
    std::unique_lock<std::mutex> lck (waitmtx);
    statechange.wait_for(lck, time);
    return getRunState();
  }

} GLOBAL_ATOMICS(globalAtomics::PLAY);



//SECTION global handlers -------------------------

void yyparse(void*);
void yylex_init(void**);

void yyset_extra(scanner_extra_data *d, void*);
void yylex_destroy(void*);

extern unordered_map<int, RtEvent *> eventMap;
unordered_map<uint_fast32_t, RtEvent *> playMap;
unordered_map<int, int> nextPulseMap;
unordered_map<int, vector<RtNoteOnEvent *>*> openNotes;
int lastTriggerKey = 60; // MIDI C4

mutex playMutex;
mutex deltaMutex;
mutex noteTerminationMutex;

auto now = clk::now();
auto lastPulse = now;
deque<chrono::nanoseconds> deltas;
uint_fast32_t numPulses = 0;

//SECTION function definitions ---s-----------------

void deleteEvents(RtEvent *start) {
  RtEvent *next = start->getNext();
  if (next != nullptr) {
    deleteEvents(next);
  }
  free(start);
}

/*
  Sets up autoplay if "auto" was mapped.
*/
void autoplay() {
  RtEvent *event;
  try {
      event = eventMap.at(-1); // default mapping
  } catch (const std::out_of_range& oor) {
    return;
  }
  
  vector<RtNoteOnEvent *> *vec = new vector<RtNoteOnEvent *>({});
  lock_guard<mutex> guard(playMutex);
  playMap[-1] = event->clone();
  openNotes[-1] = vec;
}

/*
  Averages the last bar of clock message deltas
  in order to get a steady bpm value.
*/
float estimateBpm() {
  if(!GLOBAL_SETTINGS.FOLLOW_INPUT_CLOCK.val) return GLOBAL_SETTINGS.DEFAULT_BPM.val;
  chrono::nanoseconds avgDelta(0);

  lock_guard<mutex> guard(deltaMutex);
  int size = deltas.size();
  for (int i=0; i<size; i++) {
    auto delta = deltas[i];
    avgDelta += delta / size;
  }
  if (avgDelta.count() == 0) {return 0;}
  float estimatedBpm = (((float)NS_MIN) / avgDelta.count()) / GLOBAL_SETTINGS.INPUT_PPQN.val;
  return estimatedBpm;
};

/*
  Stores a time delta value.
*/

void storeDelta(chrono::nanoseconds delta) {
  lock_guard<mutex> guard(deltaMutex);
  deltas.push_back(delta);
  if (deltas.size() > (GLOBAL_SETTINGS.INPUT_PPQN.val * 4)) {
    deltas.pop_front();
  }
}

/*
  Handles an incoming clock pulse by computing
  the time since the last clock pulse and
  storing it.
*/
void handleClockPulse(vector<unsigned char> *message) {
  // printf("Received Midi Clock Pulse Message \n");
  now = clk::now();
  auto delta = now - lastPulse;

  storeDelta(delta);
  if (numPulses % (GLOBAL_SETTINGS.INPUT_PPQN.val * 4) == 0) {
    printf("\rEstimated BPM: %f", estimateBpm());
  }

  lastPulse = now;
  numPulses++;
}

/*
  Handles "Note Off" message by shutting
  all currently active notes off or replacing
  the playing sequence with an RtNopEvent.
*/
void turnKeyOff(unsigned char key) {
  vector<RtNoteOnEvent *> *on;
  lock_guard<mutex> tGuard(noteTerminationMutex);
  try {
    on = openNotes.at(key);
  } catch (const std::out_of_range &oor) {
    printf("Input device sent unprecedented Note Off Message\n");
    return;
  }
  RtNopEvent *start = new RtNopEvent(0);
  for (int i = 0; i < (on->size()); i++) {
    RtNoteOffEvent *off = new RtNoteOffEvent(on->at(i));
    start->append(off);
  }
  lock_guard<mutex> pGuard(playMutex);
  playMap[key] = start;
}

void handleOffMsg(vector<unsigned char> *message) {
  unsigned char key = message->at(1);
  turnKeyOff(key);
}

/*
  Handles "Note On" message by looking up
  the mapped sequence for the trigger note
  and storing it in the playMap.
*/
void handleOnMsg(vector<unsigned char> *message) {
  unsigned char key = message->at(1);
  lastTriggerKey = key;
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
  vector<RtNoteOnEvent *> *vec = new vector<RtNoteOnEvent *>({});
  lock_guard<mutex> guard(playMutex);
  playMap[key] = event->clone();
  openNotes[key] = vec;
}

/*
  Invokes midi message handlers.
*/
void onmessage(double deltatime, vector<unsigned char> *message, void *userData) {
  unsigned char status = message->at(0);
  switch(status) {//TODO channelify
    case MIDI_TIME_CLOCK_BYTE:
      if(GLOBAL_SETTINGS.FOLLOW_INPUT_CLOCK.val)handleClockPulse(message);
      break;
    case MIDI_START_BYTE:
      debug("Received Midi Clock Start Message \n");
      GLOBAL_ATOMICS.setRunState(globalAtomics::START);
      break;
    case MIDI_CONTINUE_BYTE:
      debug("Received Midi Clock Continue Message \n");
      GLOBAL_ATOMICS.setRunState(globalAtomics::CONTINUE);
      break;
    case MIDI_STOP_BYTE:
      debug("Received Midi Clock Stop Message \n");
      GLOBAL_ATOMICS.setRunState(globalAtomics::STOP);
      break;
    case MIDI_POS_PTR_BYTE:
      debug("Received Midi Position Pointer Message \n");
      break;
    case MIDI_NOTE_ON_BYTE:
      if(GLOBAL_ATOMICS.getRunState() != globalAtomics::HALT) handleOnMsg(message);
      if(GLOBAL_ATOMICS.getRunState() == globalAtomics::SLEEP) GLOBAL_ATOMICS.setRunState(globalAtomics::PLAY);
      break;
    case MIDI_NOTE_OFF_BYTE://TODO: add "channel notes off" handler
      handleOffMsg(message);
      break;
    case MIDI_POLY_AFTER_BYTE:
      debug("Received Midi Note Aftertouch Message \n");
      break;
    default:
      fprintf(stderr, "Unknown Midi Message: 0x%x \n", status);
      break;//TODO move this to debug(...) ?
  }
}

/*
  Opens RtMidiIn
*/
void openMidiIn() {
  RtMidiIn *midiin = new RtMidiIn(
    GLOBAL_SETTINGS.BACKEND.val,
    "Mirco Sequencer"
  );
  unsigned int inPorts = midiin->getPortCount();

  midiin->setCallback(&onmessage);
  midiin->ignoreTypes(false, false, false);

  if(GLOBAL_SETTINGS.INPUT_PORT.changed){
    debug("Opening midi input port: %lu\n", GLOBAL_SETTINGS.INPUT_PORT.val);
    while(inPorts <= GLOBAL_SETTINGS.INPUT_PORT.val){
      inPorts = midiin->getPortCount();
      //if port is specified but not there, wait until port is available
    }
    midiin->openPort(
        GLOBAL_SETTINGS.INPUT_PORT.val);
    debug("Opened midi input port: %s\n", midiin->getPortName(GLOBAL_SETTINGS.INPUT_PORT.val).c_str());

  } else {
    if (inPorts == 0) {
      debug("No input ports available! Connect one manually!\n");
    } else {
      midiin->openPort(GLOBAL_SETTINGS.INPUT_PORT.val);
      debug("Opened midi input port: %s\n", midiin->getPortName(GLOBAL_SETTINGS.INPUT_PORT.val).c_str());
    }
  }

  while (GLOBAL_ATOMICS.waitForStateChange() != globalAtomics::ABORT) {}

  midiin->closePort();
  midiin->cancelCallback();
  delete midiin;

}

/*
  Opens RtMidiOut
*/
RtMidiOut *openMidiOut() {
  RtMidiOut *midiout = new RtMidiOut(
    GLOBAL_SETTINGS.BACKEND.val,
    "Mirco Sequencer"
  );
  unsigned int outPorts = midiout->getPortCount();

  if (GLOBAL_SETTINGS.OUTPUT_PORT.changed) {
    debug("Opening midi output port: %lu\n", GLOBAL_SETTINGS.OUTPUT_PORT.val);
    while (outPorts <= GLOBAL_SETTINGS.OUTPUT_PORT.val) {
      outPorts = midiout->getPortCount();
      // if port is specified but not there, wait until port is available
    }
    midiout->openPort(
        GLOBAL_SETTINGS.OUTPUT_PORT.val);
    debug("Opened midi output port: %s\n", midiout->getPortName(GLOBAL_SETTINGS.OUTPUT_PORT.val).c_str());
  } else {
    if (outPorts == 0) {
      fprintf(stderr,"No input ports available! Connect one manually!\n");
    } else {
      midiout->openPort(
          GLOBAL_SETTINGS.OUTPUT_PORT.val);
      debug("Opened midi output port: %s\n", midiout->getPortName(GLOBAL_SETTINGS.OUTPUT_PORT.val).c_str());
    }
  }
  return midiout;
}

/*
  Runs a realtime event and stores the next event
  in the playMap or runs the next event immediately.
  Erases event from playMap if it has no successor. 
*/
RtEventResult runEvent(RtEvent *event, RtMidiOut *midiout, Context *rtContext, uint_fast32_t key, uint_fast32_t totalPulses) {
  RtEventResult res = event->run(midiout, *rtContext, key);

  bool end = res.next == nullptr;

  if (res.pausepulses == 0 && !end) {
    res = runEvent(res.next, midiout, rtContext, key, totalPulses);
  } else if (!end) {
    lock_guard<mutex> guard(playMutex);
    nextPulseMap[key] = totalPulses + res.pausepulses;
    playMap[key] = res.next;
  } else {
    lock_guard<mutex> guard(playMutex);
    deleteEvents(playMap[key]);
    playMap.erase(key);
    nextPulseMap.erase(key);
  }

  return res;
}

void allNotesHardOff(RtMidiOut *midiout){
  std::vector<unsigned char> msg{MIDI_CHAN_MODE_BYTE, MIDI_CHAN_OFF_BYTE, 0x00};
  midiout->sendMessage(&msg);
  for (int n = 0; n < 128; n++) {
    RtNoteOffEvent *off = new RtNoteOffEvent(0, n, 0); // channel hardcoded to 0
    Context rtContext;
    RtEventResult res = off->run(midiout, rtContext, -1);
  }
  return;
}



/*
  Midi output loop
*/
void outputLoop() {
  RtMidiOut *midiout = openMidiOut();
  // stores how many internal clock pulses have passed
  uint_fast32_t totalPulses = 0;
  float bpm = estimateBpm();
  // local copy of the playmap (concurrency)
  unordered_map<uint_fast32_t, RtEvent *> playMapCopy(playMap);

  while (true) {
    switch(GLOBAL_ATOMICS.getRunState()){
      case globalAtomics::ABORT:
        allNotesHardOff(midiout);
        return;//exit outputLoop
      case globalAtomics::STOP:
        allNotesHardOff(midiout);
        GLOBAL_ATOMICS.setRunState(globalAtomics::HALT);
        //break;//immediate fallthrough
      case globalAtomics::HALT:
        while(GLOBAL_ATOMICS.waitForStateChange() == globalAtomics::HALT){}
        continue;//go right into the runState switch again
        break;
      case globalAtomics::START:
        for(unsigned char i = 0; i<128; i++){
          turnKeyOff(i);
        }
        autoplay();
        //nobreak;
      case globalAtomics::CONTINUE:
        GLOBAL_ATOMICS.setRunState(globalAtomics::PLAY);
        break;
      case globalAtomics::PLAY:
      default:
        break;
    }

    if (totalPulses % (INTERNAL_PPQN*4) == 0) {
      // reestimate bpm every bar (lo-pass bitcrusher :P)
      bpm = estimateBpm();
    }
    if (true) { // just a scope for the lock_guard
      lock_guard<mutex> guard(playMutex);
      playMapCopy = unordered_map<uint_fast32_t, RtEvent *>(playMap);
    }

    Context rtContext;

    // calculate when the next internal clock
    // pulse will happen in nanoseconds
    uint_fast32_t pulseDelay = (NS_MIN / (bpm * INTERNAL_PPQN));
    auto nextPulseNs = clk::now() + chrono::nanoseconds(pulseDelay);

    // iterate over playMap
    auto it = playMapCopy.begin();
    while (it != playMapCopy.end()) {
      uint_fast32_t key = it->first;
      uint_fast32_t trigger = key;
       // use last trigger key with autoplay
      if (key == -1) trigger = lastTriggerKey;
      RtEvent *event = it->second;

      // update realtime context
      rtContext["$trigger"] = new Tone(trigger);
      // next clock pulse for current key
      int nextPulse;
      try {
        nextPulse = nextPulseMap.at(key);
      } catch (const std::out_of_range& oor) {
        nextPulse = totalPulses;
      }

      // run event if it is scheduled
      if (nextPulse <= totalPulses) {
        runEvent(event, midiout, &rtContext, key, totalPulses);
      }
      it++;
    }
    totalPulses++;

    //if permitted, sleep for a while
    // busy wait until next internal pulse
    // TODO: determine wait method according to kernel features

    if((clk::now() - nextPulseNs) > std::chrono::milliseconds(KERNEL_MS)){
      this_thread::sleep_for((clk::now() - nextPulseNs) - std::chrono::milliseconds(KERNEL_MS));
    }


    while (clk::now() < nextPulseNs) {}
    // this_thread::sleep_until(nextPulseNs);
  }
}

void handleAbortSignal(int parameter){//to be installed by signal()
  GLOBAL_ATOMICS.setRunState(globalAtomics::ABORT);
}



//SECTION main -----------------------------------
int main(int argc, char* argv[]) {
  char *filename;

  unpack_cmdline(&GLOBAL_SETTINGS, filename, argc, argv);

  // open mirco file or MCODE argument

  istream *is;

  if(GLOBAL_SETTINGS.MCODE.changed){
    istringstream *iss = new istringstream(*GLOBAL_SETTINGS.MCODE.val);
    is = iss;
  } else {
    ifstream *ifs = new ifstream();
    ifs->open(filename, ifstream::in);
    //TODO handle file error
    is = ifs;
  }

  struct scanner_extra_data scdat {.stream = is};

  void* scanner;
  yylex_init(&scanner);

  yyset_extra(&scdat, scanner);
  // parse mirco file (generated by parser.ypp)
  yyparse(scanner);
  yylex_destroy(scanner);

  // start input and output threads
  thread inputThread(openMidiIn);
  thread outputThread(outputLoop);

  if (signal(SIGINT, handleAbortSignal) == SIG_ERR) {
    // TODO: error handling, could not register signal handler
  }//TODO use sigaction if available
  autoplay();//TODO initial play-state according to cmdline

  inputThread.join();
  outputThread.join();
}
