//TODO: license


#include <cstdint>
#include <string>
#include "rtmidi/RtMidi.h"

#define DEFAULT_EXTERNAL_PPQN 24


template <typename T>
class param{
public:

  T val;
  bool changed;

  param(T v){
    changed = false;
    val = v;
  }

  void operator=(T v){
    changed = true;
    val = v;
  }

};

struct global_settings {
  param<uint_fast16_t> INPUT_PORT = param<uint_fast16_t>(0);
  param<uint_fast16_t> OUTPUT_PORT = param<uint_fast16_t>(0);

  param<bool> FOLLOW_INPUT_CLOCK = param<bool>(0);
  param<uint_fast16_t> INPUT_PPQN = param<uint_fast16_t>(DEFAULT_EXTERNAL_PPQN);

  param<uint_fast16_t> DEFAULT_BPM = param<uint_fast16_t>(120);

  param<bool> FOLLOW_INPUT_STARTSTOP = param<bool>(false);

  param<RtMidi::Api> BACKEND = param<RtMidi::Api>(RtMidi::Api::UNSPECIFIED);
};

void unpack_cmdline(struct global_settings *settings, char *&filename, int argc, char * argv[]);

void string_to_param(struct global_settings *settings, const std::string *str);
