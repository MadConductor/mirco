#pragma once 
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace std; 

extern void yyerror(const char *s);

class SequenceItem; 
class Definition; 
class Call; 

class Note {
	public:
		uint8_t key;
		uint8_t vel;
    Note(uint8_t k, uint8_t v);
    Note() = default;
};

class Sequence {
  public:
    vector<string> args;
    vector<SequenceItem> items;

    Sequence(Definition d);
    Sequence(Call c);
    Sequence() = default;

    void resolveArguments(Call call);
};

class SequenceItem {
	public:
		Sequence sequence;
		Note note;
		string identifier;
	
    SequenceItem(Sequence s); 
    SequenceItem(Note n);
    SequenceItem(string i);
    SequenceItem() = default;
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
    vector<SequenceItem> *body = new vector<SequenceItem>{};
  
  Definition(string t, string i);
};

class Call { 
  public: 
    string identifier;
    vector<SequenceItem> *arguments = new vector<SequenceItem>{};
  
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
    void addDefinitionListItem(SequenceItem item);

    void startCall(string id);
    void endCall();
    void addCallArgument(SequenceItem arg);

    void startMapping(int n);
    void addMappingTarget(Sequence s);
    void endMapping();
};
