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
    SequenceItem seqItem(sequence); 
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

void Engine::addCallArgument(SequenceItem arg){
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

void Engine::addDefinitionListItem(SequenceItem item) {
    currentDefinition->body->push_back(item);
};

void Engine::endDefinition() {
    Definition definition = *(definitionStack.back());

    // only one definition type rn
    Sequence sequence(definition);
    ScopeItem scopeItem(sequence);
    scope[definition.identifier] = scopeItem;
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
    currentMapping = -3;
};

Note::Note(uint8_t k, uint8_t v) 
    : key(k), vel(v) {
};
	
SequenceItem::SequenceItem(string i) 
    : identifier(i) {
};

SequenceItem::SequenceItem(Sequence s)
    : sequence(s) {
};

SequenceItem::SequenceItem(Note n) 
    : note(n) {
};

Sequence::Sequence(Definition d) 
    : items(*d.body), args(*d.arguments) {
}

Sequence::Sequence(Call c) {
}

void Sequence::resolveArguments(Call call) {
    if (call.arguments->size() != args.size()) {
        yyerror(("Wrong number of arguments supplied to:" + call.identifier).c_str());
    }
    map<string, SequenceItem> argMap;
    for  (int i = 0; i < args.size(); i++) {
        argMap[args[i]] = (*call.arguments)[i];
    }
    for (int i = 0; i < items.size(); i++) {
        SequenceItem item = items[i];
        if (!item.identifier.empty()) {
            items[i] = argMap[item.identifier];
        }
    }
}
