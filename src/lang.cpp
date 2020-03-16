
#include "lang.hpp"

using namespace std;

/*
    SequenceNode
*/

vector<SequenceNode *> SequenceNode::getChildren() {
  return children;
};

bool SequenceNode::isReducible() {
  vector<SequenceNode *> children = getChildren();
  bool res = true;
  for(int i=0; i<children.size(); i++) {
    res &= children[i]->isReducible();
  }
  return res;
}

string SequenceNode::toString() {
  vector<SequenceNode *> children = getChildren();
  string res = "";
  for(int i=0; i<children.size(); i++) {
    res += children[i]->toString();
    res += " ";
  }
  return res;
}

RtEvent *SequenceNode::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  vector<SequenceNode *> children = getChildren();
  RtEvent *first, *cur, *last = nullptr;
  for(int i=0; i<children.size(); i++) {
    cur = children[i]->renderRtEvents(channel, multiplier);
    if (last != nullptr) {
      last->append(cur);
    } else {
      first = cur;
    }
    last = cur;
  }
  return first;
}

void SequenceNode::resolve(map<string, SequenceNode *> argMap) {
  vector<SequenceNode *> children = getChildren();
  for(int i=0; i<children.size(); i++) {
    children[i]->resolve(argMap);
  }
}


SequenceNode *SequenceNode::add(SequenceNode *o) {
  throw logic_error("Not implemented");
};
SequenceNode *SequenceNode::subtract(SequenceNode *o) {
  throw logic_error("Not implemented");
};
SequenceNode *SequenceNode::divide(SequenceNode *o) {
  throw logic_error("Not implemented");
};
SequenceNode *SequenceNode::multiply(SequenceNode *o) {
  throw logic_error("Not implemented");
};


// --- SequenceNode 

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

  length = 1; // dirty temporary hack
  dutycycle = 128;

  if (s.length() > i) {
    i++;
    if (s[i] == ':') {
      velocity = stoi(s.substr(i + 1));
      if (velocity > 127) {
        yyerror("Note velocity may not exceed 127.");
      }
      if (velocity == 0) {
        velocity =1;//velocity of 0 turns notes off again
      }
    }
  }
}

string Note::toString() {
  int noteVal = key % 12;
  int octave = ((key - noteVal) / 12) - 1;
  string note = valueToNoteMap[noteVal];
  note.append(to_string(octave));
  note.append(":");
  note.append(to_string(velocity));
  return note;
} 

RtEvent *Note::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  //should include note lengths!
  //needs channel byte !
  RtNoteEvent *on = new RtNoteEvent(
    MIDI_NOTE_ON_BYTE | channel,
    key,
    velocity,
    (length * multiplier * dutycycle) / 256
  );
  RtNoteEvent *off = new RtNoteEvent(
    MIDI_NOTE_OFF_BYTE | channel,
    key,
    velocity,
    0
  );
  RtNoteEvent *off2 = new RtNoteEvent(//backup "0 velocity pseudo note off" for older devices
    MIDI_NOTE_ON_BYTE | channel,
    key,
    0,
    (length * multiplier * (256-dutycycle)) / 256
  );
  on->append((RtEvent *)off);
  return (RtEvent *)on;
}

SequenceNode *Note::add(SequenceNode *o) {
  Note *note= dynamic_cast<Note *>(o);
  if (note != nullptr) {
      key += note->key;
      return this;
  }
  yyerror("Wrong operand type.");
  return this;
};
SequenceNode *Note::subtract(SequenceNode *o) {
  if(Note *note= dynamic_cast<Note *>(o)) {
      key -= note->key;
      return this;
  }
  yyerror("Wrong operand type.");
  return this;
};
SequenceNode *Note::divide(SequenceNode *o) {
  if(Note *note= dynamic_cast<Note *>(o)) {
      key /= note->key;
      return this;
  }
  yyerror("Wrong operand type.");
  return this;
};
SequenceNode *Note::multiply(SequenceNode *o) {
  Note *note = dynamic_cast<Note *>(o);
  if (note != nullptr) {
    key *= note->key;
    return this;
  }
  yyerror("Wrong operand type.");
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

string Tone::toString() {
  int noteVal = key % 12;
  int octave = ((key - noteVal) / 12) - 1;

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


// --- Tone 

/*
    Chord (tbd)
*/

// --- Chord 

/*
    Identifier
*/

Identifier::Identifier(string i)
  : id(i) {
}

bool Identifier::isReducible() {  
  if (resolvedValue == nullptr) {
      return false;
  }
  return resolvedValue->isReducible();
}

string Identifier::toString() {
  if (resolvedValue == nullptr) {
      return id;
  }
  return resolvedValue->toString();
}

RtEvent *Identifier::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  if (resolvedValue == nullptr) {
      yyerror("Cannot render unresolved identifier.");
  }
  return resolvedValue->renderRtEvents(channel, multiplier);
}


void Identifier::resolve(map<string, SequenceNode *> argMap) {
  resolvedValue = argMap[id];
  printf(resolvedValue->toString().c_str());
  printf("\n");
}

// --- Identifier 

/*
    Instantiation
*/

Instantiation::Instantiation(Definition *d, vector<SequenceNode *> *a)
  : definition(d), arguments(*a) {
  if (definition->arguments.size() != arguments.size()) {
    yyerror(("Wrong number of arguments supplied to: " + d->id).c_str());
  }
  map<string, SequenceNode *> argMap;
  for  (int i = 0; i < definition->arguments.size(); i++) {
    argMap[definition->arguments[i]->id] = arguments[i];
  }
  for (int i = 0; i < definition->body.size(); i++) {
    SequenceNode *node = definition->body[i];
    node->resolve(argMap);
    children.push_back(node);
  }
}

vector<SequenceNode *> Instantiation::getChildren() {
  return children;
};

// --- Instantiation 

/*
    Operation
*/

Operation::Operation(string o, vector<SequenceNode *> *os)
  : operands(*os) {
      op = (Operator)o[0];
}

bool Operation::isReducible() {  
  bool res = true;
  for(int i=0; i<operands.size(); i++) {
    res &= operands[i]->isReducible();
  }
  return res;
}

RtEvent *Operation::renderRtEvents(unsigned char channel, uint_fast32_t multiplier) {
  if (isReducible()) {
    reduce();
    return reducedValue->renderRtEvents(channel, multiplier);
  }
  return (RtEvent *)new RtOperationEvent(/* Realtime operation representation*/);
}

string Operation::toString() {
  return ("(" + operands[0]->toString() + " "
    + string(1, (unsigned char)op) + " "
      + operands[1]->toString()) + ")";
}

void Operation::reduce() {  
  SequenceNode *lhs = operands[0];
  SequenceNode *rhs = operands[1];
  switch (op) {
    case PLUS:
      reducedValue = (lhs->add(rhs));
      break;
    case MINUS:
      reducedValue = (lhs->subtract(rhs));
      break;
    case TIMES:
      reducedValue = (lhs->multiply(rhs));
      break;
    case DIVIDED:
      reducedValue = (lhs->divide(rhs));
      break;
    default:
      yyerror("Unkown Operator");
      break;
  }
}
    
// --- Operation 

/*
    RtResource (tbd)
*/

bool RtResource::isReducible() {  
  return false;
}

// --- RtResource 

/*
    Definition
*/

Definition::Definition(string i, vector<Identifier *> *a, vector<SequenceNode *> *b)
  : id(i), arguments(*a), body(*b) {
  // Check if all used identifiers are declared arguments
}

// --- Definition 
