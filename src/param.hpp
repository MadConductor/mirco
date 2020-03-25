//TODO: license


#include <cstdint>
#include <string>
#include "rtmidi/RtMidi.h"

template <typename T>
struct param{
  T val;
  bool changed;
};

struct global_settings {
  param<uint_fast16_t> INPUT_PORT;
  param<uint_fast16_t> OUTPUT_PORT;

  param<bool> FOLLOW_INPUT_CLOCK;
  param<uint_fast16_t> INPUT_PPQN;

  param<uint_fast16_t> DEFAULT_BPM;

  param<bool> FOLLOW_INPUT_STARTSTOP;

  param<RtMidi::Api> BACKEND;
};

void init_settings(struct global_settings &settings);

void unpack_cmdline(struct global_settings *settings, char *&filename, int argc, char * argv[]);

void string_to_param(struct global_settings *settings, const std::string *str);
