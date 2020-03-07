#include <cstdint>




class RtEvent{
public:
  struct RtEventResult run();
};

struct RtEventResult{
  RtEvent * next;
  uint_fast32_t sleeptime;
};

class RtNoteEvent : RtEvent{
private:
  RtEvent * next;
  uint_fast32_t sleeptime;
  //compiled midi Message

public:
  struct RtEventResult run(){
    //send midieventblafoo
    return (RtEventResult){.next = next, .sleeptime = sleeptime};
    //this is apparently valid in C99 !
  }



};
