#include "paradox_macro.h"
#include "utils/filesystem_util.h"
#include "utils/string_util.h"
#include "utils/parser_util.h"
#include "localization.h"
#include "effect.h"
#include "trigger.h"
#include <stack>
#include <fstream>
#include <algorithm>
#include <string_view>



extern std::set<std::string> simpleTriggers;
extern std::set<std::string> registeredTriggers;
extern std::map<std::string,TriggerItem*> items;
extern std::map<const std::string*,EffectItem*> effectItems;
std::set<std::string> fixedSTs;
std::set<const std::string*> fixedSEs;
std::map<std::string,ScriptedTrigger*> loadedSTs;
std::map<const std::string*,ScriptedEffect*> loadedSEs;

std::vector<std::pair<std::string,std::string>> allowedSTSuffix = {
    {"_yes","_no"},
    {"_more","_less"},
    {"_higher","_lower"}
};

std::string ScriptedTrigger::getLocalizationPattern(bool reversed){
        std::string u = "";
        if(suffix_index == -1) return u;
        u.append(name);
        if(reversed) {
            u.append(allowedSTSuffix[suffix_index].second);
            return getLocalization(u);
        }
        else {
            u.append(allowedSTSuffix[suffix_index].first);
            return getLocalization(u);
        }
}

Trigger* ComplicateScriptedTrigger::createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>& datas){
    std::vector<std::unique_ptr<MacroHolder>>* current_holder = &this->MacroHolders;
    int pos = 0;
    std::stack<int> sPos;
    std::stack<std::vector<std::unique_ptr<MacroHolder>>*> holders;
    std::string result;
    while(!holders.empty() || pos < current_holder->size()){
        MacroHolder* holder = (*current_holder)[pos].get();
        if(holder->getType() == HolderType::STRING) result.append(static_cast<StringHolder*>(holder)->data);
        else if(holder->getType() == HolderType::PARAMETER) {
            std::string& name = this->parameterName[static_cast<ParameterHolder*>(holder)->parameterIndex];
            auto it = std::find_if(datas.begin(), datas.end(), [&name](const auto& p){ return p.first == name; });
            if(it == datas.end()) return nullptr;
            result.append(it->second->toString());
        }
        else{
            std::string& name = this->parameterName[static_cast<ConditionalHolder*>(holder)->parameterIndex];
            auto it = std::find_if(datas.begin(), datas.end(), [&name](const auto& p){ return p.first == name; });
            if(it != datas.end()){
                sPos.push(pos);
                pos = -1;
                holders.push(current_holder);
                current_holder = &static_cast<ConditionalHolder*>(holder)->subHolders;
            }
        }
        pos++;
        while(!holders.empty() && pos >= current_holder->size()){
            current_holder = holders.top();
            pos = sPos.top();
            holders.pop();
            sPos.pop();
            pos++;
        }
    }
    
    ParadoxTag* root = parseString(result);
    if(root == nullptr) {
        return nullptr;
    }
    ComplexTrigger* ct = createBaseTrigger();
    parseTrigger(root,ct);    
    ct->takeOverLifeCycle();
    return ct;
}
Trigger* FixedScriptedTrigger::createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>& datas){
    if(this->instance == nullptr) return nullptr;
    auto it = std::find_if(datas.begin(), datas.end(), [](const auto& p){ return p.first == "__REVERSED__"; });
    if(it != datas.end()) {
        return this->instance;
    }
    return this->instance->getAsComplexTrigger()->subTriggers[0];
}
FixedScriptedTrigger::~FixedScriptedTrigger(){
    delete this->instance;
}
Effect* ComplicateScriptedEffect::createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>& datas){
    std::vector<std::unique_ptr<MacroHolder>>* current_holder = &this->MacroHolders;
    int pos = 0;
    std::stack<int> sPos;
    std::stack<std::vector<std::unique_ptr<MacroHolder>>*> holders;
    std::string result;
    while(!holders.empty() || pos < current_holder->size()){
        MacroHolder* holder = (*current_holder)[pos].get();
        if(holder->getType() == HolderType::STRING) result.append(static_cast<StringHolder*>(holder)->data);
        else if(holder->getType() == HolderType::PARAMETER) {
            std::string& name = this->parameterName[static_cast<ParameterHolder*>(holder)->parameterIndex];
            auto it = std::find_if(datas.begin(), datas.end(), [&](const auto& p){ return p.first == name; });
            if(it == datas.end()) return nullptr;
            result.append(it->second->toString());
        }
        else{
            std::string& name = this->parameterName[static_cast<ConditionalHolder*>(holder)->parameterIndex];
            auto it = std::find_if(datas.begin(), datas.end(), [&](const auto& p){ return p.first == name; });
            if(it != datas.end()){
                sPos.push(pos);
                pos = -1;
                holders.push(current_holder);
                current_holder = &static_cast<ConditionalHolder*>(holder)->subHolders;
            }
        }
        pos++;
        while(!holders.empty() && pos >= current_holder->size()){
            current_holder = holders.top();
            pos = sPos.top();
            holders.pop();
            sPos.pop();
            pos++;
        }
    }
    ParadoxTag* root = parseString(result);
    if(root == nullptr) {
        return nullptr;
    }
    std::unique_ptr<ComplexEffect> ce = createBaseEffect();
    parseEffect(root,ce.get());    

    return ce.release();
}

Effect* FixedScriptedEffect::createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>& datas){
    if(this->instance == nullptr) return nullptr;
    return this->instance;
}

enum class MacroToken{
    IDENT,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    EQUAL,
    DOLLAR,
    SQUARE_OPEN,
    SQUARE_CLOSE,
    SPACE,
    END
};
struct TokenStruct {
    MacroToken token;
};


struct MacroTokenizer{
    std::string str;
    std::string current_info;
    bool inQuote;
    MacroTokenizer(const std::string && _str) : str(_str) {
        std::reverse(str.begin(),str.end());
        inQuote = false;
    }
    MacroToken nextToken(){
        std::string cache = "";
        while(!str.empty()){
            char c = str.back();
            if(cache.empty()){
                str.pop_back();
                if(c == '{') return MacroToken::BRACKET_OPEN;
                else if(c == '}') return MacroToken::BRACKET_CLOSE;
                else if(c == '[') return MacroToken::SQUARE_OPEN;
                else if(c == ']') return MacroToken::SQUARE_CLOSE;
                else if(c == '$') return MacroToken::DOLLAR;
                else if(c == '=') return MacroToken::EQUAL;
                else if(c == ' ') return MacroToken::SPACE;
                else {
                    if(c == '"') inQuote = !inQuote;
                    cache.push_back(c);
                    continue;
                }
            }
            else {
                if(inQuote || (c != '{' && c != '}'
                && c != '[' && c != ']'
                && c != '$' && c != '='))
                {
                    if(c == '"') inQuote = !inQuote;
                    cache.push_back(c);
                }
                else{
                    current_info = cache;
                    return MacroToken::IDENT;
                }
            }
            str.pop_back();
        }
        if(cache.empty()) {
            return MacroToken::END;
        }
        else {
            current_info = cache;
            return MacroToken::IDENT;
        }
    }
};

bool ParseScriptedTrigger(MacroTokenizer &tokenizer,std::map<std::string,std::string>& fixedSTString);
bool ParseScriptedEffect(MacroTokenizer &tokenizer,std::map<std::string,std::string>& fixedSEString);

void loadScriptedTrigger(std::string rootPath){
    std::map<std::string,std::string> tempString;
    std::vector<std::string> paths;
    rootPath.append("/scripted_triggers");
    getAllFiles(rootPath,paths);
    //std::ofstream fout("./out.txt");
    for(std::string path : paths){
        std::map<std::string,std::string> simpleMacro;
        std::string content = readAllLinesWithoutComment(path,'@',[&simpleMacro](std::string str){
            auto [key,value] = splitWith(str,"=");
            trim(key);
            trim(value);
            simpleMacro[key] = value;
            return false;
        });
        
        for(auto [key,value] : simpleMacro){
            
            replaceWith(content,key,value);
        }
        //std::ofstream fout("./out.txt");
        //fout << content << std::endl;
        MacroTokenizer tokenizer(std::move(content));
        if(!ParseScriptedTrigger(tokenizer,tempString)) std::cout << "#Failed in File" << path << std::endl;
    }
    for(auto[k,v]:tempString){
        if(v.empty()) {
            loadedSTs.erase(k);
        }
        ParadoxTag* root = parseString(v);
        ComplexTrigger* ct = createBaseTrigger();
        LogicTrigger* lt = new LogicTrigger(LogicType::NOT);
        lt->putTrigger(ct);
        if(root == nullptr) {
            //log_error(current_location(),"cannot parse st ",k);
            loadedSTs.erase(k);
            delete lt;
            continue;
        }
        parseTrigger(root,ct);

        ct->takeOverLifeCycle();
        clearParserDatas();
        static_cast<FixedScriptedTrigger*>(loadedSTs[k])->instance = lt;
    }
}

void loadScriptedEffect(std::string rootPath){
    std::map<std::string,std::string> tempString;
    std::vector<std::string> paths;
    rootPath.append("/scripted_effects");
    getAllFiles(rootPath,paths);
    //std::ofstream fout("./out.txt");
    for(std::string path : paths){
        std::map<std::string,std::string> simpleMacro;
        std::string content = readAllLinesWithoutComment(path,'@',[&simpleMacro](std::string str){
            auto [key,value] = splitWith(str,"=");
            trim(key);
            trim(value);
            simpleMacro[key] = value;
            return false;
        });
        
        for(auto [key,value] : simpleMacro){
           
            replaceWith(content,key,value);
        }
        //std::ofstream fout("./out.txt");
        //fout << content << std::endl;
        MacroTokenizer tokenizer(std::move(content));
        if(!ParseScriptedEffect(tokenizer,tempString)) std::cout << "#Failed in File" << path << std::endl;
    }
    	
    for(auto[k,v]:tempString){
 
        auto ptr = getStringPtr(k);
        if(v.empty()) {
            loadedSEs.erase(ptr);
            removeString(k);
        }
        ParadoxTag* root = parseString(v);
        std::unique_ptr<ComplexEffect> ce = createBaseEffect();

        if(root == nullptr) {
            loadedSEs.erase(ptr);
            removeString(k);
            continue;
        }

        parseEffect(root,ce.get());
        clearParserDatas();
        static_cast<FixedScriptedEffect*>(loadedSEs[ptr])->instance = ce.release();
    }
   
}


size_t getSuffixIndex(std::string str){
    for(size_t i = 0;i < allowedSTSuffix.size();i++){
        std::string first = str;
        
        first.append(allowedSTSuffix[i].first);
        //if(str == "is_or_was_tag") std::cout << first << std::endl; 
        if(!hasLocalization(first)) continue;
        std::string second = str;
        second.append(allowedSTSuffix[i].second);
        //if(str == "is_or_was_tag") std::cout << second << std::endl; 
        if(hasLocalization(second)) return i;
    }
    return -1;
}

void loadScriptedTrigger_POST(){
    for(auto[name,st]: loadedSTs){
        st->suffix_index = getSuffixIndex(name);
    }
}
bool checkValidName(std::string& str2){
    for(size_t i = 0;i < str2.length();i++){
        char c = str2[i];
        if(c == '_' || (c > 'A' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <='9')) {}
        else return false;
    }
    return true;
}
bool ParseScriptedTrigger(MacroTokenizer &tokenizer,std::map<std::string,std::string>& fixedSTString){
    int state = 0;
    int depth = 0;
    std::string name;
    std::string content = "";
    std::string content2 = "";
    MacroToken nextToken;
    ComplicateScriptedTrigger* cst = nullptr;
    std::vector<std::unique_ptr<MacroHolder>>* currentHolder = nullptr;
    std::stack<std::vector<std::unique_ptr<MacroHolder>>*> holders;
    bool simple = true;
    while((nextToken = tokenizer.nextToken()) != MacroToken::END){
        if(state != 5 && nextToken == MacroToken::SPACE) continue;
        switch(state){
            case 0: {
                if(nextToken != MacroToken::IDENT) goto error;
                name = tokenizer.current_info;
                trim(name);
                state = 2;
                break;
            }
            case 2:{
                if(nextToken != MacroToken::EQUAL) goto error;
                state = 4;
                break;
            }
            case 3:{
                if(nextToken != MacroToken::DOLLAR) goto error;
                ParameterHolder* pHolder = new ParameterHolder(); 
                std::unique_ptr<MacroHolder> holder(pHolder);
                auto it = std::find(cst->parameterName.begin(),cst->parameterName.end(),content2);
                if(it == cst->parameterName.end()) {
                    pHolder->parameterIndex = cst->parameterName.size();
                    cst->parameterName.push_back(std::move(content2));
                }
                else {
                    pHolder->parameterIndex = it - cst->parameterName.begin();
                }
                currentHolder->push_back(std::move(holder));
                content2.clear();
                state = 5;
                break;
            }
            case 4:{
                if(nextToken != MacroToken::BRACKET_OPEN) goto error;
                depth++;
                state = 5;
                cst = new ComplicateScriptedTrigger();
                simple = true;
                currentHolder = &cst->MacroHolders;
                break;
            }
            case 5:{
                if(nextToken == MacroToken::IDENT){
                    content.append(tokenizer.current_info);
                }
                else if(nextToken == MacroToken::EQUAL){
                    content.push_back('=');
                }
                else if(nextToken == MacroToken::SPACE) {
                    content.push_back(' ');
                }
                else if(nextToken == MacroToken::DOLLAR){
                    simple = false;
                    if(!content.empty()){
                        currentHolder->push_back(std::make_unique<StringHolder>(content));
                        content.clear();
                    }
                    state = 6;
                }
                else if(nextToken == MacroToken::SQUARE_OPEN){
                    simple = false;
                    if(!content.empty()){
                        currentHolder->push_back(std::make_unique<StringHolder>(content));
                        content.clear();
                    }
                    state = 7;

                }
                else if(nextToken == MacroToken::SQUARE_CLOSE){
                    if(holders.empty()) goto error;
                    if(!content.empty()){
                        currentHolder->push_back(std::make_unique<StringHolder>(content));
                        content.clear();
                    }
                    currentHolder = holders.top();
                    holders.pop();
                }
                else if(nextToken == MacroToken::BRACKET_CLOSE){
                    if(depth == 1) {
                        state = 0;
                        if(simple){
                            delete cst;
                            FixedScriptedTrigger* fst = new FixedScriptedTrigger();
                            fst->name = name;
                            
                            //fst->suffix_index = getSuffixIndex(name);
                            fixedSTString[name] = content;
                            loadedSTs[name] = fst;
                            
                            registeredTriggers.insert(name);
                            content.clear();
                        }
                        else {
                            if(!content.empty()) currentHolder->push_back(std::make_unique<StringHolder>(content));
                            cst->name = name;
                            //cst->suffix_index = getSuffixIndex(name);
                            loadedSTs[name] = cst;
                            registeredTriggers.insert(name);
                            content.clear();
                        }
                        depth = 0;
                    }
                    else {
                        content.push_back('}');
                        depth--;
                    }
                }
                else if(nextToken == MacroToken::BRACKET_OPEN){
                    content.push_back('{');
                    depth++;
                }
                else goto error;
                break;
            }
            case 6:{
                if(nextToken != MacroToken::IDENT) goto error;
                content2 = tokenizer.current_info;
                state = 3;
                break;
            }
            case 7:{
                if(nextToken != MacroToken::SQUARE_OPEN) goto error;
                state = 8;
                break;
            }
            case 8:{
                if(nextToken != MacroToken::IDENT) goto error;
                content2 = tokenizer.current_info;
                state = 9;
                break;
            }
            case 9:{
                if(nextToken != MacroToken::SQUARE_CLOSE) goto error;
                ConditionalHolder* cHolder = new ConditionalHolder();
                std::unique_ptr<MacroHolder> holder(cHolder);
                auto it = std::find(cst->parameterName.begin(),cst->parameterName.end(),content2);
                if(it == cst->parameterName.end()) {
                    cHolder->parameterIndex = cst->parameterName.size();
                    cst->parameterName.push_back(std::move(content2));
                    
                }
                else {
                    cHolder->parameterIndex = it - cst->parameterName.begin();
                }
                content2.clear();
                currentHolder->push_back(std::move(holder));
                holders.push(currentHolder);
                currentHolder = &cHolder->subHolders;
                
                state = 5;
                break;
            }
        }
    }
    return true;
    error:
    return false;
}



//duplicates for SE...
bool ParseScriptedEffect(MacroTokenizer &tokenizer,std::map<std::string,std::string>& fixedSEString){
    int state = 0;
    int depth = 0;
    std::string name;
    std::string content = "";
    std::string content2 = "";
    MacroToken nextToken;
    ComplicateScriptedEffect* cse = nullptr;
    std::vector<std::unique_ptr<MacroHolder>>* currentHolder = nullptr;
    std::stack<std::vector<std::unique_ptr<MacroHolder>>*> holders;
    bool simple = true;
    while((nextToken = tokenizer.nextToken()) != MacroToken::END){
        if(state != 5 && nextToken == MacroToken::SPACE) continue;
        switch(state){
            case 0: {
                if(nextToken != MacroToken::IDENT) goto error;
                name = tokenizer.current_info;
                trim(name);
                state = 2;
                break;
            }
            case 2:{
                if(nextToken != MacroToken::EQUAL) {
                    log_error(current_location(),"Non EQUAL token occured in state 2 in parsing ", name);
                    goto error;
                }
                state = 4;
                break;
            }
            case 3:{
                if(nextToken != MacroToken::DOLLAR) {
                    log_error(current_location(),"Non DOLLAR token occured in state 3 in parsing ", name);
                    goto error;
                }
                ParameterHolder* pHolder = new ParameterHolder(); 
                std::unique_ptr<MacroHolder> holder(pHolder);
                auto it = std::find(cse->parameterName.begin(),cse->parameterName.end(),content2);
                if(it == cse->parameterName.end()) {
                    pHolder->parameterIndex = cse->parameterName.size();
                    cse->parameterName.push_back(std::move(content2));
                }
                else {
                    pHolder->parameterIndex = it - cse->parameterName.begin();
                }
                currentHolder->push_back(std::move(holder));
                content2.clear();
                state = 5;
                break;
            }
            case 4:{
                if(nextToken != MacroToken::BRACKET_OPEN) {
                    log_error(current_location(),"Non BRACKET_OPEN token occured in state 4 in parsing ", name);
                    goto error;
                }
                depth++;
                state = 5;
                cse = new ComplicateScriptedEffect();
                simple = true;
                currentHolder = &cse->MacroHolders;
                break;
            }
            case 5:{
                if(nextToken == MacroToken::IDENT){
                    content.append(tokenizer.current_info);
                }
                else if(nextToken == MacroToken::EQUAL){
                    content.push_back('=');
                }
                else if(nextToken == MacroToken::SPACE) {
                    content.push_back(' ');
                }
                else if(nextToken == MacroToken::DOLLAR){
                    simple = false;
                    if(!content.empty()){
                        currentHolder->push_back(std::make_unique<StringHolder>(content));
                        content.clear();
                    }
                    state = 6;
                }
                else if(nextToken == MacroToken::SQUARE_OPEN){
                    simple = false;
                    if(!content.empty()){
                        currentHolder->push_back(std::make_unique<StringHolder>(content));
                        content.clear();
                    }
                    state = 7;

                }
                else if(nextToken == MacroToken::SQUARE_CLOSE){
                    if(holders.empty()) goto error;
                    if(!content.empty()){
                        currentHolder->push_back(std::make_unique<StringHolder>(content));
                        content.clear();
                    }
                    currentHolder = holders.top();
                    holders.pop();
                }
                else if(nextToken == MacroToken::BRACKET_CLOSE){
                    if(depth == 1) {
                        state = 0;
                        if(simple){
                            delete cse;
                            FixedScriptedEffect* fse = new FixedScriptedEffect();
                            fse->name = name;
                            
                            fixedSEString[name] = content;
                            loadedSEs[getStringPtr(name)] = fse;
                            content.clear();
                        }
                        else {
                            if(!content.empty()) currentHolder->push_back(std::make_unique<StringHolder>(content));
                            cse->name = name;
                            auto ptr = getStringPtr(name);
                            loadedSEs[ptr] = cse;
                            
                            content.clear();
                        }
                        depth = 0;
                    }
                    else {
                        content.push_back('}');
                        depth--;
                    }
                }
                else if(nextToken == MacroToken::BRACKET_OPEN){
                    content.push_back('{');
                    depth++;
                }
                else {
                    log_error(current_location(),"unexcepted token ", (int)nextToken, " occured in state 5 in parsing ", name);
                    goto error;
                }
                break;
            }
            case 6:{
                if(nextToken != MacroToken::IDENT) {
                    log_error(current_location(),"Non IDENT token occured in state 6 in parsing ", name);
                    goto error;
                }
                content2 = tokenizer.current_info;
                state = 3;
                break;
            }
            case 7:{
                if(nextToken != MacroToken::SQUARE_OPEN) {
                    log_error(current_location(),"Non Square OPEN token occured in state 7 in parsing ", name);
                    goto error;
                }
                state = 8;
                break;
            }
            case 8:{
                if(nextToken != MacroToken::IDENT) {
                    log_error(current_location(),"Non IDENT token occured in state 8 in parsing ", name);
                    goto error;
                }
                content2 = tokenizer.current_info;
                state = 9;
                break;
            }
            case 9:{
                if(nextToken != MacroToken::SQUARE_CLOSE) {
                    log_error(current_location(),"Non Square Close token occured in state 9 in parsing ", name);
                    goto error;
                }
                ConditionalHolder* cHolder = new ConditionalHolder();
                std::unique_ptr<MacroHolder> holder(cHolder);
                auto it = std::find(cse->parameterName.begin(),cse->parameterName.end(),content2);
                if(it == cse->parameterName.end()) {
                    cHolder->parameterIndex = cse->parameterName.size();
                    cse->parameterName.push_back(std::move(content2));
                }
                else {
                    cHolder->parameterIndex = it - cse->parameterName.begin();
                }
                content2 = "";
                currentHolder->push_back(std::move(holder));
                holders.push(currentHolder);
                currentHolder = &cHolder->subHolders;
                
                state = 5;
                break;
            }
        }
    }
    return true;
    error:
    return false;
}