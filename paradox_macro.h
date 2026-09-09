#ifndef PDX_Macro
#define PDX_Macro


#include "paradox_type.h"
#include <memory>



struct ScriptedTrigger;
struct Effect;
struct Trigger;

enum class HolderType{
    CONDITIONAL,
    STRING,
    PARAMETER,
};

struct MacroHolder {
    virtual HolderType getType() = 0;
    virtual ~MacroHolder() = default;
};

struct ConditionalHolder : MacroHolder{
    virtual HolderType getType(){
        return HolderType::CONDITIONAL;
    }
    virtual ~ConditionalHolder() = default;    
    int parameterIndex;
    std::vector<std::unique_ptr<MacroHolder>> subHolders;
};

struct StringHolder : MacroHolder{
    virtual HolderType getType(){
        return HolderType::STRING;
    }
    virtual ~StringHolder() = default;
    std::string data;
    StringHolder(const std::string& _data) : data(_data){}
    StringHolder(std::string&& _data) : data(_data){}
};

struct ParameterHolder : MacroHolder{
    virtual HolderType getType(){
        return HolderType::PARAMETER;
    }
    virtual ~ParameterHolder() = default;
    int parameterIndex;
};


//
struct ScriptedTrigger {
    virtual Trigger* createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>&) = 0;
    virtual bool isFixed() const = 0;
    size_t suffix_index = -1;
    std::string name;
    std::string getLocalizationPattern(bool reversed);
    virtual ~ScriptedTrigger() noexcept = default;
};

struct FixedScriptedTrigger : ScriptedTrigger {
    Trigger* instance;
    virtual Trigger* createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>&);
    virtual bool isFixed() const{ return true; }
    virtual ~FixedScriptedTrigger();
};

struct ComplicateScriptedTrigger : ScriptedTrigger {
    virtual Trigger* createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>&);
    std::vector<std::string> parameterName;
    std::vector<std::unique_ptr<MacroHolder>> MacroHolders;
    virtual bool isFixed() const { return false; }
};

struct ScriptedEffect{
    std::string name;
    virtual Effect* createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>&) = 0;
    virtual bool isFixed() const = 0;
    virtual ~ScriptedEffect() noexcept = default;
};

struct FixedScriptedEffect : ScriptedEffect{
    Effect* instance;
    virtual Effect* createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>&);
    virtual bool isFixed() const { return true; }

};

struct ComplicateScriptedEffect : ScriptedEffect{
    std::string pattern;
    std::vector<std::string> parameterName;
    std::vector<std::unique_ptr<MacroHolder>> MacroHolders;
    virtual Effect* createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>&);
    virtual bool isFixed() const { return false; }

};



void loadScriptedTrigger(std::string rootPath = ".");
//void printAllScriptedTrigger();
void loadScriptedTrigger_POST();
void loadScriptedEffect(std::string rootPath = ".");



#endif