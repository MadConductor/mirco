#pragma once 
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace std; 

extern void yyerror(const char *s);

class Definition; 
class Call; 

class Sequence {
  public:
    vector<string> args;
    vector<Sequence *> children;
    bool resolved;

    Sequence(Definition d);
    Sequence(Call c);
    Sequence() = default;
    virtual ~Sequence() = default;

    void resolveSelf(map<string, Sequence *> argMap);
    void resolveArguments(Call call);
    string toString();
};

class Note : public Sequence {
	public:
		uint8_t key;
		uint8_t vel;
    std::map<string, int> noteToValueMap = {
      {"C", 0},
      {"D", 2},
      {"E", 4},
      {"F", 5},
      {"G", 7},
      {"A", 9},
      {"B", 11},
      {"H", 11}
    };
    std::map<int, string> valueToNoteMap = {
      {0, "C"},
      {1, "C#"},
      {2, "D"},
      {3, "D#"},
      {4, "E"},
      {5, "F"},
      {6, "F#"},
      {7, "G"},
      {8, "G#"},
      {9, "A"},
      {10, "A#"},
      {11, "B"},
    };

    Note(uint8_t k, uint8_t v);
    Note(string s);
    Note() = default;

    string toString();
};

class Identifier : public Sequence {
	public:
    string identifier;
    Identifier(string id);
    Identifier() = default;

    Sequence *resolveSelf(map<string, Sequence *> argMap);
};

class ScopeItem {
  public:
    Sequence sequence;

    ScopeItem(Sequence s);
    ScopeItem() = default;
};

class Definition { 
  public: 
    string type;
    string identifier;
    vector<string> *arguments = new vector<string>{};
    vector<Sequence *> *body = new vector<Sequence *>{};
  
  Definition(string t, string i);
};

class Call { 
  public: 
    string identifier;
    vector<Sequence *> *arguments = new vector<Sequence *>{};
  
  Call(string i);
};

class Engine { // Sequencer Definition Language Engine
	public:
    vector<Definition*> definitionStack;
    Definition *currentDefinition; 
    vector<Call*> callStack;
    Call *currentCall;
    map<int, Sequence> mapping;
    int currentMapping;

    map<string, ScopeItem> scope;


    Engine();
    void startDefinition(string type, string id);
    void endDefinition();
    void addDefinitionArgument(string id);
    void addDefinitionListItem(Sequence *item);

    void startCall(string id);
    void endCall();
    void addCallArgument(Sequence *arg);

    void startMapping(int n);
    void addMappingTarget(Sequence s);
    void endMapping();
};
