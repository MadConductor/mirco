
#include "stdio.h"
#include <map>
#include <vector>
#include <cstdlib>
#include <rtmidi/RtMidi.h>
#include "lang.hpp"
#include "rtseq.hpp"

void yyparse();
extern FILE *yyin;

extern map<int, SequenceNode *> eventMap;

void onmessage( double deltatime, std::vector< unsigned char > *message, void *userData )
{
  unsigned int nBytes = message->size();
  for ( unsigned int i=0; i<nBytes; i++ )
    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
  if ( nBytes > 0 )
    std::cout << "stamp = " << deltatime << std::endl;
}


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

  RtEvent *start = eventMap[-2]->renderRtEvents();
  start->run();

  RtMidiIn *midiin = new RtMidiIn();
  // Check available ports.
  unsigned int nPorts = midiin->getPortCount();
  if ( nPorts == 0 ) {
    printf("No ports available!\n");
    goto cleanup;
  }
  midiin->openPort( 0 );
  // Set our callback function.  This should be done immediately after
  // opening the port to avoid having incoming messages written to the
  // queue.
  midiin->setCallback( &onmessage );
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
