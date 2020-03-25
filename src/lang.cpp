
#include "lang.hpp"

using namespace std;

/*
    SequenceNode
*/

SequenceNode *SequenceNode::operator+(Note *o) {
  return doOperationRhs("+", this, o);
};

SequenceNode *SequenceNode::operator+(Tone *o) {
  return doOperationRhs("+", this, o);
};

SequenceNode *SequenceNode::operator+(Sequence *o) {
  return doOperationRhs("+", this, o);
};

SequenceNode *SequenceNode::operator+(Chord *o) {
  return doOperationRhs("+", this, o);
};

SequenceNode *SequenceNode::operator+(Identifier *o) {
  return doOperationRhs("+", this, o);
};

SequenceNode *SequenceNode::operator+(RtResource *o) {
  return doOperationRhs("+", this, o);
};


// --- SequenceNode 

/*
    SequenceParentNode
*/

string SequenceParentNode::toString() {
  vector<SequenceNode *> children = getChildren();
  string res = "";
  for(int i=0; i<children.size(); i++) {
    res += children[i]->toString();
    res += " ";
  }
  return res;
}

RtEvent *SequenceParentNode::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  vector<SequenceNode *> children = getChildren();
  RtNopEvent *start = new RtNopEvent(0);

  for(int i=0; i<children.size(); i++) {
    RtEvent *cur = children[i]->renderRtEvents(channel, multiplier);
    start->append(cur);
  }
  return start;
}

SequenceNode *SequenceParentNode::disambiguate(Context extraContext) {
  vector<SequenceNode *> children = getChildren();
  for(int i=0; i<children.size(); i++) {
    SequenceNode *resolvedValue = children[i]->disambiguate(extraContext);
    children[i] = resolvedValue;
  }
  return this;
}

bool SequenceParentNode::isAmbiguous() {
  vector<SequenceNode *> children = getChildren();
  for(int i=0; i<children.size(); i++) {
    if(children[i]->isAmbiguous()) {return true;}
  }
  return false;
}

// --- SequenceParentNode 

/*
    Note
*/

map<string, int> Note::noteToValueMap = {
  {"C", 0},
  {"D", 2},
  {"E", 4},
  {"F", 5},
  {"G", 7},
  {"A", 9},
  {"B", 11},
  {"H", 11}
};

map<int, string> Note::valueToNoteMap = {
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

Note::Note(string s) {
  //TODO: add note length from string
  //TODO: add duty cycle (staccato / legato) from string
  int noteVal = noteToValueMap[s.substr(0, 1)];
  int i = 1;

  if (s[i] == '#') {
    noteVal += 1;
    i++;
  }
  if (s[i] == 'b') {
    noteVal -= 1;
    i++;
  }

  int octave = 1 + stoi(s.substr(i, 1));
  int octaveOffset = 12 * octave;
  noteVal += octaveOffset;

  key = noteVal;
  velocity = 127;

  denominator = 16; // default denominator
  dutycycle = 1; // default dutycycle
  int velIdx = s.find(":");
  int denIdx = s.find("<");
  int denEndIdx = s.find(">");

  if (velIdx > -1) {
    velocity = stoi(s.substr(velIdx + 1, denIdx));
    if (velocity > 127) {
      yyerror("Note velocity may not exceed 127.");
    }
    if (velocity == 0) {
      velocity = 1; //velocity of 0 turns notes off again
    }
  }

  if (denIdx > -1) {
    int dutyIdx = s.find("|");
    denominator = stoi(s.substr(denIdx + 1, max(denEndIdx, dutyIdx) - 1));
    if (dutyIdx > -1) {
      dutycycle = stoi(s.substr(dutyIdx + 1, denEndIdx - 1));
    }
  }
};

Note::Note(uint_fast32_t k, uint_fast32_t v, uint_fast32_t d, uint_fast32_t du) 
  : key(k), velocity(v), denominator(d), dutycycle(du) {
}

string Note::toString() {
  int noteVal = key % 12;
  int octave = ((key - noteVal) / 12) - 1;
  string note = valueToNoteMap[noteVal];
  note.append(to_string(octave));
  note.append(":");
  note.append(to_string(velocity));
  return note;
};

uint_fast32_t Note::getVelocity() const {
  return velocity;
};

void Note::setVelocity(uint_fast32_t v) {
  velocity = v;
};

uint_fast32_t Note::getKey() const {
  return key;
};

void Note::setKey(uint_fast32_t k) {
  if (k < 0 ) k = 0;
  key = k;
};


RtEvent *Note::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  uint_fast32_t pulses = (MIDI_PULSES_PQN * 4) / denominator;
  uint_fast32_t dutyPulses = pulses / dutycycle;
  uint_fast32_t offPulses = pulses - dutyPulses;

  RtNoteOnEvent *on = new RtNoteOnEvent(
    channel,
    key,
    velocity,
    pulses - offPulses
  );

  RtNoteOffEvent *off = new RtNoteOffEvent(
    channel,
    key,
    offPulses
  );

  on->append((RtEvent *)off);
  return (RtEvent *)on;
};

// Operators

SequenceNode *Note::operator+(Note *o) {
  return new Note(
    getKey() + o->getKey(),
    std::max(getVelocity(), o->getVelocity()),
    denominator,
    dutycycle
  );
};

SequenceNode *Note::operator+(Tone *o) {
  return new Note(
    getKey() + o->getKey(),
    getVelocity(),
    denominator,
    dutycycle
  );
};

SequenceNode *Note::operator+(Sequence *o) {  
  return (*o) + this;
};

SequenceNode *Note::operator+(Chord *o) {  
  return (*o) + this;
};

SequenceNode *Note::operator+(Identifier *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Note::operator+(RtResource *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Note::operator+(SequenceNode *o) {
  return doOperationLhs("+", this, o);
};

// --- Note 

/*
    Tone
*/

Tone::Tone(string s) {
  key = 0;
  int octIdx = s.find("o");
  if (octIdx > -1) {
    string octave = s.substr(0, octIdx);
    key += 12 * stoi(octave);
    s.erase(0, octIdx+1);
  }
  int semIdx = s.find("s");
  if (semIdx > -1) {
    string semitones = s.substr(0, octIdx);
    key += stoi(semitones);
  }
}

Tone::Tone(uint_fast32_t k) : key(k) {}

string Tone::toString() {
  if (key == 0) return "0s";
  int noteVal = key % 12;
  int octave = ((key - noteVal) / 12);

  string res = "";
  if (octave > 0) {
      res.append(to_string(octave));
      res.append("o");
  }
  if (noteVal > 0) {
      res.append(to_string(noteVal));
      res.append("s");
  }

  return res;
}

uint_fast32_t Tone::getKey() const {
  return key;
};

void Tone::setKey(uint_fast32_t k) {
  if (k < 0) k = 0;
  key = k;
};

// Operators

SequenceNode *Tone::operator+(Note *o) {
  return new Note(
    getKey() + o->getKey(),
    o->getVelocity(),
    o->denominator,
    o->dutycycle
  );
};

SequenceNode *Tone::operator+(Tone *o) {
  return new Tone(
    getKey() + o->getKey()
  );
};

SequenceNode *Tone::operator+(Sequence *o) {  
  return (*o) + this;
};

SequenceNode *Tone::operator+(Chord *o) {  
  return (*o) + this;
};

SequenceNode *Tone::operator+(Identifier *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Tone::operator+(RtResource *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Tone::operator+(SequenceNode *o) {
  return doOperationLhs("+", this, o);
};


// --- Tone 

/*
    Chord
*/

Chord::Chord(string s) {
  // TODO: Parse chord notation like "C#msus"
}

Chord::Chord(vector<SequenceNode *> c, uint_fast32_t v, uint_fast32_t d, uint_fast32_t du) 
  : children(c), velocity(v), denominator(d), dutycycle(du) {
}

RtEvent *Chord::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  uint_fast32_t pulses = (MIDI_PULSES_PQN * 4) / denominator;
  uint_fast32_t dutyPulses = pulses / dutycycle;
  uint_fast32_t offPulses = pulses - dutyPulses;

  uint_fast32_t size = children.size();
  RtNopEvent *start = new RtNopEvent(0);

  for (int i=0; i<size; i++) {
    Note *child = static_cast<Note *>(children[i]);
    uint_fast32_t evtPulses = 0;
    if(i == size - 1) evtPulses = dutyPulses;
    RtNoteOnEvent *on = new RtNoteOnEvent(
      channel,
      child->key,
      velocity,
      evtPulses
    );
      
    start->append(on);
  }

  for (int i=0; i<size; i++) {
    Note *child = static_cast<Note *>(children[i]);
    uint_fast32_t evtPulses = 0;
    if(i == size - 1) evtPulses = offPulses;

    RtNoteOffEvent *off = new RtNoteOffEvent(
      channel,
      child->key,
      evtPulses
    );
    start->append(off);
  }
  return (RtEvent *)start;
}

string Chord::toString() {
  vector<SequenceNode *> children = getChildren();
  int size = children.size();
  string res = "(";

  for(int i=0; i<size; i++) {
    res += children[i]->toString();
    if (i != size - 1) res += ", ";
  }
  res += ")";
  return res;
};

// Operators

SequenceNode *Chord::operator+(Note *o) {
  vector<SequenceNode *> c({});
  for (int i=0; i<children.size(); i++) {
    c.push_back(*(children[i]) + o);
  }
  return new Chord(c, velocity, denominator, dutycycle);
};

SequenceNode *Chord::operator+(Tone *o) {  
  vector<SequenceNode *> c({});
  for (int i=0; i<children.size(); i++) {
    c.push_back(*(children[i]) + o);
  }
  return new Chord(c, velocity, denominator, dutycycle);
};

SequenceNode *Chord::operator+(Identifier *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Chord::operator+(RtResource *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Chord::operator+(SequenceNode *o) {
  return doOperationLhs("+", this, o);
};

// --- Chord 

/*
    Identifier
*/

Identifier::Identifier(string i)
  : id(i) {
}

string Identifier::toString() {
  return id;
}

RtEvent *Identifier::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  throw logic_error("Cannot render unresolved identifier.");
}

SequenceNode *Identifier::disambiguate(Context extraContext) {
  if (extraContext.count(id) > 0) {
    return extraContext[id];
  } else {
    return this;
  }
}

// Operators

SequenceNode *Identifier::operator+(Note *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Identifier::operator+(Tone *o) {  
  return makeOperation("+", this, o);
};

SequenceNode *Identifier::operator+(Sequence *o) {  
  return makeOperation("+", this, o);
};

SequenceNode *Identifier::operator+(Chord *o) {  
  return makeOperation("+", this, o);
};

SequenceNode *Identifier::operator+(Identifier *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Identifier::operator+(RtResource *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Identifier::operator+(SequenceNode *o) {
  return makeOperation("+", this, o);
};


// --- Identifier 

/*
    Sequence
*/

Sequence::Sequence(Definition *d, vector<SequenceNode *> *a) {
  if (d->arguments.size() != a->size()) {
    yyerror(("Wrong number of arguments supplied to: " + d->id).c_str());
  }
  Context argMap;
  for  (int i = 0; i < d->arguments.size(); i++) {
    argMap[d->arguments[i]->id] = (*a)[i];
  }
  for (int i = 0; i < d->body.size(); i++) {
    SequenceNode *node = d->body[i];
    node = node->disambiguate(argMap);
    children.push_back(node);
  }
}

Sequence::Sequence(vector<SequenceNode *> c) : children(c) {}

// Operators

SequenceNode *Sequence::operator+(Note *o) {
  vector<SequenceNode *> c({});
  for (int i=0; i<children.size(); i++) {
    c.push_back(*(children[i]) + o);
  }
  return new Sequence(c);
};

SequenceNode *Sequence::operator+(Tone *o) {  
  vector<SequenceNode *> c({});
  for (int i=0; i<children.size(); i++) {
    c.push_back(*(children[i]) + o);
  }
  return new Sequence(c);
};

SequenceNode *Sequence::operator+(Identifier *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Sequence::operator+(RtResource *o) {
  return makeOperation("+", this, o);
};

SequenceNode *Sequence::operator+(SequenceNode *o) {
  return doOperationLhs("+", this, o);
};

// --- Sequence 


/*
    RtResource (tbd)
*/


// Operators

SequenceNode *RtResource::operator+(Note *o) {
  return makeOperation("+", this, o);
};

SequenceNode *RtResource::operator+(Tone *o) {  
  return makeOperation("+", this, o);
};

SequenceNode *RtResource::operator+(Sequence *o) {  
  return makeOperation("+", this, o);
};

SequenceNode *RtResource::operator+(Chord *o) {  
  return makeOperation("+", this, o);
};

SequenceNode *RtResource::operator+(Identifier *o) {
  return makeOperation("+", this, o);
};

SequenceNode *RtResource::operator+(RtResource *o) {
  return makeOperation("+", this, o);
};

SequenceNode *RtResource::operator+(SequenceNode *o) {
  return makeOperation("+", this, o);
};

// --- RtResource 

/*
    Definition
*/

Definition::Definition(string i, vector<Identifier *> *a, vector<SequenceNode *> *b)
  : id(i), arguments(*a), body(*b) {
  for (int i=0; i<body.size(); i++) {
    Context ec({});
    body[i] = body[i]->disambiguate(ec);
  }
  // TODO: Check if all used identifiers are declared arguments
}

// --- Definition 
