#include "lang.hpp"

Definition::Definition(string t, string i) 
    : type(t), identifier(i) {
};

void Definition::finalizeToScope(map<string, ScopeItem> &scope) {
    // only one definition type rn
    Sequence *sequence = new Sequence(*this);
    ScopeItem *scopeItem = new ScopeItem(*sequence);
    scope[this->identifier] = *scopeItem;
}

Call::Call(string i) // Not sure if this will be a struct in the end, lets see.
    : identifier(i) {
};

ScopeItem::ScopeItem(Sequence s)
    : sequence(s) {
};

Engine::Engine() {
};

void Engine::startCall(string id){
    Call *call = new Call(id);
    currentCall = call;
    callStack.push_back(currentCall);
};

void Engine::endCall() {
    Call call = *(callStack.back());
    callStack.pop_back();

    // only one call type rn
    // TODO: Prevent unknown seq call
    Sequence sequence = scope[call.identifier].sequence;
    sequence.resolveArguments(call);
    Sequence *seqItem = new Sequence(sequence); 

    // add to next call?
    if (!callStack.empty()) {
        currentCall = callStack.back();
        currentCall->arguments->push_back(seqItem);
    // add to open definition?
    } else if (!definitionStack.empty()) {
        currentDefinition->body->push_back(seqItem);
    // add to open mapping?
    } else if (currentMapping > -3) {
        addMappingTarget(sequence);
    }
    // lost in the void of nothing being done with it.
};

void Engine::addCallArgument(Sequence *arg){
    currentCall->arguments->push_back(arg);
};


void Engine::startDefinition(string type, string id){
    Definition *def = new Definition(type, id);
    currentDefinition = def;
    definitionStack.push_back(currentDefinition);
};

void Engine::addDefinitionArgument(string id){
    vector<string> *arguments = currentDefinition->arguments;
    for (int i = 0; i < arguments->size(); i++) {
        if(id == (*arguments)[i]) {
            yyerror("Arguments require unique identifiers.");
        }
    }
    arguments->push_back(id);
};

void Engine::addDefinitionListItem(Sequence *item) {
    currentDefinition->body->push_back(item);
};

void Engine::endDefinition() {
    Definition *definition = definitionStack.back();
    definition->finalizeToScope(scope);
    definitionStack.pop_back();

    if (!definitionStack.empty()) {
        currentDefinition = definitionStack.back();
    }
};

void Engine::startMapping(int n) {
    currentMapping = n;
};

void Engine::addMappingTarget(Sequence s){
    mapping[currentMapping] = s;
};

void Engine::endMapping(){
    string seqString = mapping[currentMapping].toString();
    printf("Found Mapping to Sequence:\n");
    printf(seqString.c_str());
    printf("\n");
    currentMapping = -3;
};

std::map<string, int> Note::noteToValueMap = {
  {"C", 0},
  {"D", 2},
  {"E", 4},
  {"F", 5},
  {"G", 7},
  {"A", 9},
  {"B", 11},
  {"H", 11}
};

std::map<int, string> Note::valueToNoteMap = {
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

Note::Note(uint8_t k, uint8_t v) 
    : key(k), vel(v) {
    resolved = true;
};

Note::Note(string s) {
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
    vel = 127;
    if (s.length() > i) {
        i++;
        if (s[i] == ':') {
            vel = stoi(s.substr(i + 1));
            if (vel > 127) {
                yyerror("Note velocity may not exceed 127.");
            }
        }
    }

    resolved = true;
}

string Note::toString() {
    int noteVal = key % 12;
    int octave = ((key - noteVal) / 12) - 1;
    string note = valueToNoteMap[noteVal];
    note.append(to_string(octave));
    note.append(":");
    note.append(to_string(vel));
    return note;
}
	
Identifier::Identifier(string id) 
    : identifier(id) {
    resolved = false;
};

Sequence *Identifier::resolveSelf(map<string, Sequence *> argMap) {
    Sequence *res = argMap[identifier];
    return res;
}

Sequence::Sequence(Definition d) 
    : children(*d.body), args(*d.arguments) {
    resolved = true;
}

Sequence::Sequence(Call c) {
    resolved = true;
}

void Sequence::resolveSelf(map<string, Sequence *> argMap) {
    yyerror("Already resolved.");
}

void Sequence::resolveArguments(Call call) {
    if (call.arguments->size() != args.size()) {
        yyerror(("Wrong number of arguments supplied to: " + call.identifier).c_str());
    }
    map<string, Sequence *> argMap;
    for  (int i = 0; i < args.size(); i++) {
        argMap[args[i]] = (*call.arguments)[i];
    }
    for (int i = 0; i < children.size(); i++) {
        Sequence *item = children.at(i);
        if (!item->resolved) {
            Identifier *id = dynamic_cast<Identifier*>(item); // TODO: Use Interfaces for this shit is fucky.
            children[i] = id->resolveSelf(argMap);
        }
    }
}

string Sequence::toString() {
    string res = "";
    for (int i=0; i<children.size(); i++) {
        Note *note = dynamic_cast<Note*>(children[i]); // ditto
        if(note != nullptr) {
            res.append(note->toString());
        } else {
            Identifier *id = dynamic_cast<Identifier*>(children[i]);
            if(id != nullptr) {
                res.append(id->toString());
            } else {
                res.append(children[i]->toString());
            }
        }
        res.append(" ");
    }
    return res;
}