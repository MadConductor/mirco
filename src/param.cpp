// TODO: license

#include <cstdint>
#include <string>
#include <getopt.h>

#include "param.hpp"
#include "error.hpp"


static const struct option LONG_OPTIONS[] = {
    {"input", required_argument, 0, 'i'},
    {"output", required_argument, 0, 'o'},
    {"follow-clk", optional_argument, 0, 'c'},
    {"bpm", required_argument, 0, 'b'},
    {"api", required_argument, 0, 'a'},
    0};

void unpack_cmdline(struct global_settings *settings, char *&filename, int argc, char *argv[]){
    while (1)
    {
        int index = 0;
        int result = getopt_long(argc, argv,
            "c::b:i:o:a:",
            LONG_OPTIONS, &index);
        if (result == -1) break; /* end of list */
        switch (result)
          {
            case 'a':
              switch(optarg[0]){
                case 'a':
                case 'A':
                  settings->BACKEND = RtMidi::Api::LINUX_ALSA;
                  break;
                case 'j':
                case 'J':
                  settings->BACKEND = RtMidi::Api::UNIX_JACK;
                  break;
                case 'm':
                case 'M':
                  settings->BACKEND = RtMidi::Api::MACOSX_CORE;
                  break;
                default:
                  settings->BACKEND = RtMidi::Api::UNSPECIFIED;
                  break;
              }
              break;
            case 'b':
              settings->DEFAULT_BPM = (uint_fast16_t)atol(optarg);
              //TODO error checking
              break;
            case 'c':
              settings->FOLLOW_INPUT_CLOCK = true;
              settings->INPUT_PPQN = (uint_fast16_t)atol(optarg);
              //TODO error checking
              break;
            case 'i':
              settings->INPUT_PORT = (uint_fast16_t)atol(optarg);
              break;
            case 'o':
              settings->OUTPUT_PORT = (uint_fast16_t)atol(optarg);
              break;
            case 0:
            case ':':
              switch(optopt){
                case 'c':
                  settings->FOLLOW_INPUT_CLOCK = true;
                  settings->INPUT_PPQN = DEFAULT_EXTERNAL_PPQN;
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
    filename = argv[optind];
    optind++;
    while (optind < argc) {//DEBUG
      debug("other parameter: <%s>\n", argv[optind++]);
    }
}














void string_to_param(struct global_settings *settings, const std::string *str){}
