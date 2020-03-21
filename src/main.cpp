//SECTION license --------------------------------

//TODO license



//SECTION includes -------------------------------

#include "stdio.h"
#include <map>
#include <vector>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>

#include <rtmidi/RtMidi.h>
#include "lang.hpp"
#include "rtseq.hpp"


//SECTION global defines and constants -----------

#define INTERNAL_PPQN MIDI_PULSES_PQN
#define DEFAULT_EXTERNAL_PPQN 24



//SECTION global value structures ----------------

struct global_settings {
  bool FOLLOW_INPUT_CLOCK;
  uint_fast16_t INPUT_PPQN;
  bool FOLLOW_INPUT_STARTSTOP;
} GLOBAL_SETTINGS { false, DEFAULT_EXTERNAL_PPQN, false };

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

extern map<int, SequenceNode *> eventMap;



//SECTION function definitions --------------------
auto now = chrono::steady_clock::now();
auto lastPulse = now;
auto delta = now - lastPulse;

void handleClockPulse(vector<unsigned char> *message) {
  printf("Received Midi Clock Pulse Message \n");
  now = chrono::steady_clock::now();
  delta = now - lastPulse;
  int bpm = (((1000. * 1000 * 1000 * 60) / delta.count())) / GLOBAL_SETTINGS.INPUT_PPQN;
  printf("Estimated BPM: %d\n", bpm);

  lastPulse = now;
}

void onmessage( double deltatime, vector<unsigned char> *message, void *userData )
{
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
    case MIDI_NOTE_ON_BYTE: 
      printf("Received Midi Note On Message \n");
      break;
    case MIDI_NOTE_OFF_BYTE: 
      printf("Received Midi Note Off Message \n");
      break;
    case MIDI_POLY_AFTER_BYTE: 
      printf("Received Midi Note Aftertouch Message \n");
      break;
    default:
      fprintf(stderr, "Unknown Midi Message: 0x%x \n", status);
      break;
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

  RtEvent *start = eventMap[-2]->renderRtEvents(1, 1);
  Context rtContext({});
  start->run(rtContext); 

  RtMidiIn *midiin = new RtMidiIn();
  RtMidiOut *midiout = new RtMidiOut();

  // Check available ports.
  unsigned int inPorts = midiin->getPortCount();
  unsigned int onPorts = midiout->getPortCount();
  if (inPorts == 0 || onPorts == 0) {
    printf("No ports available!\n");
    goto cleanup;
  }

  midiin->openPort( 0 );
  midiout->openPort( 0 );
  // Set our callback function.  This should be done immediately after
  // opening the port to avoid having incoming messages written to the
  // queue.
  // TODO: port selection via cmdline parameter
  midiin->setCallback(&onmessage);
  // Don't ignore sysex, timing, or active sensing messages.
  midiin->ignoreTypes( false, false, false );
  printf("\nReading MIDI input ... press <enter> to quit.\n");
  char input;
  std::cin.get(input);
  // Clean up
 cleanup:
  delete midiin;
  return 0;
}
