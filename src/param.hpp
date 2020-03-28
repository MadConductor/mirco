#include <cstdint>
#include <string>
#include <getopt.h>

#include "rtmidi/RtMidi.h"

template <typename T>
class param{
public:

  T val;
  bool changed;
  struct option OPT;

  param(T v, option opt){
    changed = false;
    val = v;
    OPT = opt;
  }

  void operator=(T v){
    changed = true;
    val = v;
  }

};

using puft = param<uint_fast16_t>;
using pbol = param<bool>;
using papi = param<RtMidi::Api>;

RtMidi::Api rmau = RtMidi::Api::UNSPECIFIED;

#define rarg required_argument
#define oarg required_argumemt
#define narg no_argument

struct global_settings {
public:
  puft INPUT_PORT         = puft(0,    {"input",      rarg, 0,'i'});
  puft OUTPUT_PORT        = puft(0,    {"output",     rarg, 0,'o'});
  pbol FOLLOW_INPUT_CLOCK = pbol(false,{"follow-clk", oarg, 0,'c'});
  puft INPUT_PPQN         = puft(24,   {"",narg, 0,'?'});

  puft DEFAULT_BPM        = puft(120,  {"bpm",        rarg, 0,'b'});

  papi BACKEND            = papi(rmau, {"api",        rarg, 0,'a'});


  pbol INPUT_CONTROLS     = pbol(false,{"",narg, 0,'?'});

  const struct option LONG_OPTS[6] = {
                                                INPUT_PORT.OPT,
                                                OUTPUT_PORT.OPT,
                                                FOLLOW_INPUT_CLOCK.OPT,
                                                DEFAULT_BPM.OPT,
                                                BACKEND.OPT,
                                                0
  };
};

void unpack_cmdline(struct global_settings *settings, char *&filename, int argc, char * argv[]);

void string_to_param(struct global_settings *settings, const std::string *str);
