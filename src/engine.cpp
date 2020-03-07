#include "engine.hpp"

Definition::Definition(string t, string i) 
    : type(t), identifier(i) {
};

Call::Call(string i) 
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
    ;
    Sequence sequence = scope[call.identifier].sequence;
    sequence.resolveArguments(call);
    Sequence *seqItem = new Sequence(sequence); 
    // add to next call?
    if (!callStack.empty()) {
        currentCall = callStack.back();
        currentCall->arguments->push_back(seqItem);
    } else if (!definitionStack.empty()) {
        currentDefinition->body->push_back(seqItem);
    } else if (currentMapping > -3) {
        addMappingTarget(sequence);
    }
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
    Definition definition = *(definitionStack.back());

    // only one definition type rn
    Sequence *sequence = new Sequence(definition);
    ScopeItem *scopeItem = new ScopeItem(*sequence);
    scope[definition.identifier] = *scopeItem;
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

Note::Note(uint8_t k, uint8_t v) 
    : key(k), vel(v) {
    resolved = true;
};


Note::Note(string s) {
    int noteVal = noteToValueMap[s.substr(0,1)];
    int secIdx = 1;
    string secString = s.substr(secIdx,1);

    if (secString == "#") {
      noteVal += 1;
      secIdx++;
    }
    if (secString == "b") {
      noteVal -= 1;
      secIdx++;
    }

    int octave = 1 + std::stoi(s.substr(secIdx, 1));
    int octaveOffset = 12 * octave;
    noteVal += octaveOffset;
    resolved = true;
    key = noteVal;
    vel = 120;
}

string Note::toString() {
    int noteVal = key % 12;
    int octave = ((key - noteVal) / 12) - 1;
    string note = valueToNoteMap[noteVal];
    note.append(to_string(octave));
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
}    void toString();

void Sequence::resolveArguments(Call call) {
    if (call.arguments->size() != args.size()) {
        yyerror(("Wrong number of arguments supplied to:" + call.identifier).c_str());
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
        Note *note = dynamic_cast<Note*>(children[i]);
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