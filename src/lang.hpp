#pragma once 
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>
#include <vector>
#include "rtseq.hpp"

using namespace std; 

extern void yyerror(const char *s);

class SequenceNode {
  public:
    enum Type {
      NOTE,
      TONE,
      CHORD,
      SEQUENCE,
      OPERATION,
      IDENTIFIER,
      RTRESOURCE,
    };

    virtual string toString() = 0;
    virtual SequenceNode::Type getType() = 0;
    virtual RtEvent *renderRtEvents(unsigned char channel, uint_fast32_t multiplier) = 0;
    
    virtual bool isAmbiguous() {return false;};
    virtual SequenceNode *disambiguate(Context e) {return this;};

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Sequence *o);
    SequenceNode *operator+(Chord *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
};

class SequenceParentNode: public SequenceNode {
  public:
    vector<SequenceNode *> children;
    SequenceParentNode() = default;
    SequenceParentNode(const SequenceParentNode *pn);

    virtual string toString() override;
    virtual vector<SequenceNode *> getChildren() = 0;
    virtual RtEvent *renderRtEvents(unsigned char channel, uint_fast32_t multiplier) override;
    
    virtual bool isAmbiguous() override; 
    virtual SequenceNode *disambiguate(Context e) override;    
};

/*
  Single Note, very boring.
*/
class Note : public SequenceNode {
  public:
    uint_fast32_t key;
    uint_fast32_t velocity;
    uint_fast32_t denominator;
    unsigned char dutycycle; // staccato / legato
    static map<string, int> noteToValueMap;
    static map<int, string> valueToNoteMap;

    Note() = default;
    Note(string s);
    Note(uint_fast32_t k, uint_fast32_t v, uint_fast32_t d = 1, uint_fast32_t du = 1);

    virtual string toString() override;
    virtual SequenceNode::Type getType() override { return SequenceNode::NOTE; };

    virtual uint_fast32_t getVelocity() const; 
    virtual uint_fast32_t getKey() const; 
    virtual void setKey(uint_fast32_t k); 
    virtual void setVelocity(uint_fast32_t v); 
    RtEvent *renderRtEvents(unsigned char channel, uint_fast32_t multiplier) override;

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Sequence *o);
    SequenceNode *operator+(Chord *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
    SequenceNode *operator+(SequenceNode *o);
};

/*
  Velocity and denominator agnostic version of "Note".
  Only used for arithmetic, not to be played directly.
*/
class Tone : public Note { // kind of a bad name ToneLiteral?
  public:
    uint_fast32_t key;
  
    Tone() = default;
    Tone(string s);
    Tone(uint_fast32_t k);

    string toString() override;
    virtual SequenceNode::Type getType() override { return SequenceNode::TONE; };

    uint_fast32_t getKey() const override; 
    void setKey(uint_fast32_t k) override;

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Sequence *o);
    SequenceNode *operator+(Chord *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
    SequenceNode *operator+(SequenceNode *o);
};

/*
  Represents the simultaneous triggering of 
  multiple notes.
*/
class Chord : public SequenceParentNode {
  public:
    vector<SequenceNode *> children;
    uint_fast32_t velocity = 0;
    uint_fast32_t denominator;
    unsigned char dutycycle;

    Chord() = default;
    Chord(string s);
    Chord(vector<SequenceNode *> c, uint_fast32_t v = 127, uint_fast32_t d = 1, uint_fast32_t du = 1);

    string toString() override;
    vector<SequenceNode *> getChildren() override { return children; };
    virtual SequenceNode::Type getType() override { return SequenceNode::CHORD; };

    RtEvent *renderRtEvents(unsigned char channel, uint_fast32_t multiplier) override;

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
    SequenceNode *operator+(SequenceNode *o);
};

/*
  Represents a playable sequence.
  Holds children and renders them sequentially.
*/
class Sequence : public SequenceParentNode {
  public:
    vector<SequenceNode *> children;

    Sequence() = default;
    Sequence(vector<SequenceNode *> c);
    Sequence(Definition *d, vector<SequenceNode *> *a);

    vector<SequenceNode *> getChildren() override { return children; };
    virtual SequenceNode::Type getType() override { return SequenceNode::SEQUENCE; };

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
    SequenceNode *operator+(SequenceNode *o);
};

class AmbiguousSequenceNode : public SequenceNode {
  public:
    virtual bool isAmbiguous() override {return true;};
};

/*
  Represents an unresolved identifier. 
  Stays ambiguous until sequence instantiation 
  ("compile" time).
*/
class Identifier : public AmbiguousSequenceNode {
  public:
    string id;
    SequenceNode *resolvedValue;

    Identifier() = default;
    Identifier(string i);

    string toString() override;
    virtual SequenceNode::Type getType() override { return SequenceNode::IDENTIFIER; };

    RtEvent *renderRtEvents(unsigned char channel, uint_fast32_t multiplier) override;
    SequenceNode *disambiguate(Context e) override;

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Sequence *o);
    SequenceNode *operator+(Chord *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
    SequenceNode *operator+(SequenceNode *o);
};

/*
  Represents a realtime resource (eg. trigger note).
  Is resolved at runtime of the mirco program.
*/
class RtResource : public Identifier {
  public:
    RtResource() = default;

    virtual SequenceNode::Type getType() override { return SequenceNode::RTRESOURCE; };
    SequenceNode *disambiguate(Context e) override;

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Sequence *o);
    SequenceNode *operator+(Chord *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
    SequenceNode *operator+(SequenceNode *o);
};

/*
  Represents an unevaluated operation. 
  Stays ambiguous until all identifiers or
  realtime resources are resolved.
*/
template<typename LhsT, typename RhsT>
class Operation : public AmbiguousSequenceNode, public RtEvent {
  public:
    string op;
    LhsT *lhs;
    RhsT *rhs;
    RtEvent *next;

    unsigned char channel;
    uint_fast32_t multiplier;
    
    Operation() = default;
    Operation(string o, LhsT *l, RhsT *r);

    string toString() override;
    virtual SequenceNode::Type getType() override { return SequenceNode::OPERATION; };

    RtEvent *renderRtEvents(unsigned char channel, uint_fast32_t multiplier) override;
    SequenceNode *disambiguate(Context e) override;

    uint_fast32_t getPausePulses() override { return 0; };
    void setNext(RtEvent * n) override { next = n; };
    RtEvent *getNext() override { return next; };
    struct RtEventResult run(RtMidiOut *m, Context r, uint_fast32_t key) override;

    SequenceNode *operator+(Note *o);
    SequenceNode *operator+(Tone *o);
    SequenceNode *operator+(Sequence *o);
    SequenceNode *operator+(Chord *o);
    SequenceNode *operator+(Identifier *o);
    SequenceNode *operator+(RtResource *o);
    SequenceNode *operator+(SequenceNode *o);
};

/*
  Represents a sequence definition.
  Has to be constructed into a Sequence 
  with a list of parameters.
*/
class Definition {
  public:
    string id;
    vector<Identifier *> arguments;
    vector<SequenceNode *> body;

    Definition() = default;
    Definition(string i, vector<Identifier *> *a, vector<SequenceNode *> *b);
};


/*
  Operation

  Definition has to be kept in header file
  because Operation is a template class. 
*/

enum Operator {
  PLUS = '+',
  MINUS = '-',
  TIMES = '*',
  DIVIDED = '/' 
};

template<typename LhsT, typename RhsT>
SequenceNode *makeOperation(string ops, LhsT *lhs, RhsT *rhs) {
  return new Operation<LhsT, RhsT>(ops, lhs, rhs);
};

template<typename LhsT>
SequenceNode *doOperationLhs(string opstr, LhsT *lhs, SequenceNode *rhs) {
  SequenceNode::Type type = rhs->getType();
  switch (type) {
    case SequenceNode::NOTE:
      return doOperation(opstr, lhs, static_cast<Note *>(rhs));
      break;
    case SequenceNode::TONE:
      return doOperation(opstr, lhs, static_cast<Tone *>(rhs));
      break;
    case SequenceNode::CHORD:
      return doOperation(opstr, lhs, static_cast<Chord *>(rhs));
      break;
    case SequenceNode::SEQUENCE:
      return doOperation(opstr, lhs, static_cast<Sequence *>(rhs));
      break;
    case SequenceNode::OPERATION:
      return makeOperation(opstr, lhs, rhs);
      break;
    case SequenceNode::IDENTIFIER:
      return doOperation(opstr, lhs, static_cast<Identifier *>(rhs));
      break;
    case SequenceNode::RTRESOURCE:
      return doOperation(opstr, lhs, static_cast<RtResource *>(rhs));
      break;
    default:
      throw logic_error("Unknown Operator");
      break;
  }
};

template<typename RhsT>
SequenceNode *doOperationRhs(string opstr, SequenceNode *lhs, RhsT *rhs) {
  SequenceNode::Type type = lhs->getType();
  switch (type) {
    case SequenceNode::NOTE:
      return doOperation(opstr, static_cast<Note *>(lhs), rhs);
      break;
    case SequenceNode::TONE:
      return doOperation(opstr, static_cast<Tone *>(lhs), rhs);
      break;
    case SequenceNode::CHORD:
      return doOperation(opstr, static_cast<Chord *>(lhs), rhs);
      break;
    case SequenceNode::SEQUENCE:
      return doOperation(opstr, static_cast<Sequence *>(lhs), rhs);
      break;
    case SequenceNode::OPERATION:
      return makeOperation(opstr, lhs, rhs);
      break;
    case SequenceNode::IDENTIFIER:
      return doOperation(opstr, static_cast<Identifier *>(lhs), rhs);
      break;
    case SequenceNode::RTRESOURCE:
      return doOperation(opstr, static_cast<RtResource *>(lhs), rhs);
      break;
    default:
      throw logic_error("Unknown Operator");
      break;
  }
};

template<typename LhsT, typename RhsT>
SequenceNode *doOperation(string opstr, LhsT *lhs, RhsT *rhs) {
  char op = (Operator)opstr[0];
  
  switch (op) {
    case PLUS:
      return (*lhs) + rhs;
      break;
    case MINUS:
      return (*lhs) + rhs;
      break;
    case DIVIDED:
      return (*lhs) + rhs;
      break;
    case TIMES:
      return (*lhs) + rhs;
      break;
    default:
      throw logic_error("Unknown Operator");
      break;
  }
};

template<typename LhsT, typename RhsT>
Operation<LhsT, RhsT>::Operation(string o, LhsT *l, RhsT *r) {
  lhs = l;
  rhs = r;
  op = o;
};

template<typename LhsT, typename RhsT>
RtEvent *Operation<LhsT, RhsT>::renderRtEvents(unsigned char c, uint_fast32_t m) {
  channel = c;
  multiplier = m;
  return this;
};

template<typename LhsT, typename RhsT>
struct RtEventResult Operation<LhsT, RhsT>::run(RtMidiOut *m, Context rtContext, uint_fast32_t key) {
  SequenceNode *value = this->disambiguate(rtContext);
  RtEvent *event = value->renderRtEvents(channel, multiplier);
  event->append(next);
  return event->run(m, rtContext, key);
};

template<typename LhsT, typename RhsT>
string Operation<LhsT, RhsT>::toString() {
  return ("(" + lhs->toString() + " "
    + op + " "
      + rhs->toString()) + ")";
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::disambiguate(Context extraContext) {
  SequenceNode *l = lhs->disambiguate(extraContext);
  SequenceNode *r = rhs->disambiguate(extraContext);
  bool ambiguous = l->isAmbiguous() | r->isAmbiguous();

  if (ambiguous) {
    return this;
  } else {
    return doOperationLhs(op, l, r);
  }
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::operator+(Note *o) {
  return makeOperation("+", this, o);
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::operator+(Tone *o) {  
  return makeOperation("+", this, o);
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::operator+(Sequence *o) { 
  return makeOperation("+", this, o);
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::operator+(Chord *o) {  
  return makeOperation("+", this, o);
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::operator+(Identifier *o) {  
  return makeOperation("+", this, o);
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::operator+(RtResource *o) {  
  return makeOperation("+", this, o);
};

template<typename LhsT, typename RhsT>
SequenceNode *Operation<LhsT, RhsT>::operator+(SequenceNode *o) {
  return makeOperation("+", this, o);
};

// --- Operation
