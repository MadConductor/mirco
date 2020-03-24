// TODO: license

#include <cstdint>
#include <string>
#include <getopt.h>

#include "param.hpp"

#define DEFAULT_EXTERNAL_PPQN 24

static const struct option LONG_OPTIONS[] = {
  {"follow-clk",  optional_argument, 0, 'c'},
  {"bpm",         required_argument, 0, 'b'},
0};

void init_settings(struct global_settings &settings){
  settings.FOLLOW_INPUT_CLOCK =     {false, false};
  settings.INPUT_PPQN =             {DEFAULT_EXTERNAL_PPQN, false};
  settings.FOLLOW_INPUT_STARTSTOP = {false, false};
  settings.DEFAULT_BPM =            {120, false};
}

void unpack_cmdline(struct global_settings *settings, int argc, char *argv[]){
    while (1)
    {
        int index = -1;
        struct option * opt = 0;
        int result = getopt_long(argc, argv,
            "c::b:",
            LONG_OPTIONS, &index);
        if (result == -1) break; /* end of list */
        switch (result)
        {
            case 'b': /* same as index==1 */
              settings->DEFAULT_BPM = {(uint_fast16_t)atol(optarg), true};
              //TODO error checking
              break;
            case 'c': /* same as index==2 */
              settings->FOLLOW_INPUT_CLOCK = {true, true};
              settings->INPUT_PPQN = {(uint_fast16_t)atol(optarg), true};
              //TODO error checking
              break;
            case 0:
            case ':':
              switch(optopt){
                case 'c':
                  settings->FOLLOW_INPUT_CLOCK = {true, true};
                  settings->INPUT_PPQN = {DEFAULT_EXTERNAL_PPQN, true};
                  break;
                default:
                  //TODO error handling
                  break;
              }
            case '?':
            default: /* unknown */
              //TODO error handling
                break;
        }
    }
    /* print all other parameters */
    while (optind < argc) {
      printf("other parameter: <%s>\n", argv[optind++]);
    }
}














void string_to_param(struct global_settings *settings, const std::string *str){}
