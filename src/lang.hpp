#pragma once 
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "rtseq.hpp"

using namespace std; 

extern void yyerror(const char *s);

class Definition;
class Note;

class SequenceNode {
  public:
    vector<SequenceNode *> children;

    virtual vector<SequenceNode *> getChildren();
    virtual bool isReducible();
    virtual string toString();
    virtual RtEvent *renderRtEvents();
    virtual void resolve(map<string, SequenceNode *> argMap);

    virtual SequenceNode *add(SequenceNode *o);
    virtual SequenceNode *subtract(SequenceNode *o);
    virtual SequenceNode *divide(SequenceNode *o);
    virtual SequenceNode *multiply(SequenceNode *o);
};

class Note : public SequenceNode {
  public:
    int key;
    int velocity;
    static std::map<string, int> noteToValueMap;
    static std::map<int, string> valueToNoteMap;

    Note() = default;
    Note(string s);

    string toString() override;
    RtEvent *renderRtEvents() override;

    SequenceNode *add(SequenceNode *o) override;
    SequenceNode *subtract(SequenceNode *o) override;
    SequenceNode *divide(SequenceNode *o) override;
    SequenceNode *multiply(SequenceNode *o) override;
};

class Tone : public Note {
  public:
    int key;
    int velocity;
    int denominator = 16;
  
    Tone() = default;
    Tone(string s);

    string toString() override;
};

class Chord : public SequenceNode {
  public:
    vector<SequenceNode *> children;
    int velocity;

    Chord() = default;
    Chord(string s);
};

class Identifier : public SequenceNode {
  public:
    string id;
    SequenceNode *resolvedValue;

    Identifier() = default;
    Identifier(string i);

    bool isReducible() override;
    string toString() override;
    RtEvent *renderRtEvents() override;
    void resolve(map<string, SequenceNode *> argMap) override;
};

class Instantiation : public SequenceNode {
  public:
    Definition *definition;
    vector<SequenceNode *> arguments;
    vector<SequenceNode *> children;

    virtual vector<SequenceNode *> getChildren();
    Instantiation() = default;
    Instantiation(Definition *d, vector<SequenceNode *> *a);
};

class Operation : public SequenceNode {
  public:
    enum Operator {
      PLUS = '+',
      MINUS = '-',
      TIMES = '*',
      DIVIDED = '/'  
    };

    Operator op;
    vector<SequenceNode *> operands;
    vector<SequenceNode *> children;
    SequenceNode *reducedValue;

    Operation() = default;
    Operation(string o, vector<SequenceNode *> *os);

    bool isReducible() override;
    string toString() override;
    RtEvent *renderRtEvents() override;
  
    void reduce();
};

class RtResource : public SequenceNode {
  public:
    RtResource() = default;

    bool isReducible() override;
};

class Definition {
  public:
    string id;
    vector<Identifier *> arguments;
    vector<SequenceNode *> body;

    Definition() = default;
    Definition(string i, vector<Identifier *> *a, vector<SequenceNode *> *b);
};
