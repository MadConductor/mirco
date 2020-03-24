//TODO: license


#include <cstdint>
#include <string>

template <typename T>
struct param{
  T val;
  bool changed;
};

struct global_settings {
  param<bool> FOLLOW_INPUT_CLOCK;
  param<uint_fast16_t> INPUT_PPQN;
  param<bool> FOLLOW_INPUT_STARTSTOP;
};


void init_settings(struct global_settings *settings);

void unpack_cmdline(struct global_settings *settings, int argc, char * argv[]);

void string_to_param(struct global_settings *settings, const std::string *str);
