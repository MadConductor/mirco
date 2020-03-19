//SECTION license --------------------------------

//TODO license



//SECTION includes -------------------------------

#include "stdio.h"
#include <map>
#include <vector>
#include <cstdlib>
#include <rtmidi/RtMidi.h>
#include "lang.hpp"
#include "rtseq.hpp"

#include <thread>
#include <atomic>
#include <chrono>



//SECTION global defines and constants -----------

#define INTERNAL_PPQN 960
#define DEFAULT_EXTERNAL_PPQN 24



//SECTION global value structures ----------------

struct global_settings {
  bool FOLLOW_INPUT_CLOCK;
  uint_fast16_t INPUT_PPQN;
  bool FOLLOW_INPUT_STARTSTOP;
}GLOBAL_SETTINGS{false,DEFAULT_EXTERNAL_PPQN,false};

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

void onmessage( double deltatime, std::vector< unsigned char > *message, void *userData )
{
  unsigned int nBytes = message->size();
  for ( unsigned int i=0; i<nBytes; i++ )
    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
  if ( nBytes > 0 )
    std::cout << "stamp = " << deltatime << std::endl;
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

  RtEvent *start = eventMap[-2]->renderRtEvents(1,1);
  start->run();

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
