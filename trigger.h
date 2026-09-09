#ifndef PDX_TRIGGER
#define PDX_TRIGGER

#include<iostream>
#include<map>
#include<bitset>
#include "paradox_type.h"
#include "scope.h"
#include <functional>
#include "utils/functional_util.h"

enum class TriggerType{
	COMMON,LOGIC,CHANGE_SCOPE,CONDITIONAL,NUM,HIDDEN,CUSTOM_TT,SPECIAL
};
enum class LogicType{
	AND,OR,NOT
};
const int SINGLE_SCOPE_MERGABLE = 0x1;
struct ComplexTrigger;
struct LogicTrigger;
struct CommonTrigger;
struct ScriptedTrigger;

struct TriggerItem{
	std::string pattern;
	std::string reversePattern;
	const std::string& name;
	std::map<std::string,int> parameterName;
	std::vector<ParadoxType> parameterType;
	std::vector<int> usedParameter;
	std::bitset<64> requiredParameter = flipedBitset<64>();
	std::function<std::string(std::vector<ParadoxBase*>,bool)> overrideLocalization;
	ScopeType usable_scope;
	int attribue;
	std::string toString(std::vector<ParadoxBase*> base,bool reversed);
	std::string toHtml(std::vector<ParadoxBase*> base,bool reversed);
	TriggerItem(const std::string& _name) : name(_name){}
	TriggerItem(const std::string& _name,std::pair<std::string,std::string>&& patterns,std::vector<std::string>&& parameterName,std::vector<ParadoxType>&& parameterType,std::vector<int>&& usedParameter,std::bitset<64> _requiredParameter = flipedBitset<64>(),std::function<std::string(std::vector<ParadoxBase*>,bool)> _overrideLocalization = std::function<std::string(std::vector<ParadoxBase*>,bool)>(),ScopeType scope_type = ScopeType::COUNTRY);
};


struct Trigger{
	virtual TriggerType getType() const = 0;
	virtual std::string toString(bool reversed,int depth = 1) const = 0;
	virtual std::string toHtml(bool reversed,int depth = 1) = 0;
	virtual void takeOverLifeCycle() = 0;
	virtual bool hasAnyTrigger(bool (*predicate)(Trigger* trigger)) = 0;
	virtual bool foreach(std::function<bool(Trigger*)>) = 0;
	ComplexTrigger* getAsComplexTrigger();
	LogicTrigger* getAsLogicTrigger();
	CommonTrigger* getAsCommonTrigger();
	Trigger(){
		*(uint64_t*)extra_data = 0ll;
	}
	uint8_t extra_data[8];
	virtual ~Trigger() noexcept = default;
};
struct ComplexTrigger : Trigger{
	virtual void takeOverLifeCycle();
	virtual bool hasAnyTrigger(bool (*predicate)(Trigger* trigger));
	virtual bool foreach(std::function<bool(Trigger*)>);
	std::vector<Trigger*> subTriggers;
	ComplexTrigger() : Trigger(){};
	virtual ~ComplexTrigger(){
		for(Trigger* trigger : subTriggers){
			delete trigger;
		}
	}
	void putTrigger(Trigger* trigger);

};
struct CommonTrigger : Trigger{
	virtual TriggerType getType() const{
		return TriggerType::COMMON;
	}
	virtual std::string toString(bool reversed,int depth = 1) const;
	virtual std::string toHtml(bool reversed,int depth = 1);
	virtual bool foreach(std::function<bool(Trigger*)>);	
	virtual void takeOverLifeCycle();
	virtual bool hasAnyTrigger(bool (*predicate)(Trigger* trigger));
	CommonTrigger(TriggerItem* item);
	void pushObject(ParadoxBase* base);
	TriggerItem* item;
	bool reversed;
	std::vector<ParadoxBase*> base;
	virtual ~CommonTrigger(){
		if(this->extra_data[0] == 1){
			for(ParadoxBase* base1 : base) delete base1;
		}
	}
};


struct SpecialTrigger : Trigger {
	virtual TriggerType getType() const{
		return TriggerType::SPECIAL;
	}
	ScriptedTrigger * const prototype;
	mutable Trigger* instance;
	std::map<const std::string*,ParadoxBase*> args;

	virtual std::string toString(bool reversed,int depth = 1) const;
	virtual std::string toHtml(bool reversed,int depth = 1) { return toString(reversed,depth); }
	virtual void takeOverLifeCycle();
	virtual bool foreach(std::function<bool(Trigger*)>);
	virtual bool hasAnyTrigger(bool (*predicate)(Trigger* trigger));
	SpecialTrigger(ScriptedTrigger* _prototype,Trigger* _instance) : Trigger(), prototype(_prototype), instance(_instance){}

	virtual ~SpecialTrigger() noexcept {
		if(this->extra_data[0] == 1){
			for(auto[u,v] : args) delete v;
		}
	}
};

struct LogicTrigger : ComplexTrigger {
	virtual TriggerType getType() const{
		return TriggerType::LOGIC;
	}
	virtual std::string toHtml(bool reversed,int depth = 1){
		return this->toString(reversed);
	}
	LogicTrigger(LogicType type);
	virtual std::string toString(bool reversed,int depth = 1) const;
	LogicType type;
};
struct ChangeScopeTrigger : ComplexTrigger{
	virtual TriggerType getType() const{
		return TriggerType::CHANGE_SCOPE;
	}
	virtual std::string toHtml(bool reversed,int depth = 1){
		return this->toString(reversed);
	}
	ChangeScopeTrigger(Scope* scope);
	virtual std::string toString(bool reversed,int depth = 1) const;
	Scope* changedScope;
	bool hasType() const{
		return this->extra_data[2] != 0;
	}
	bool isAnyType() const{
		return this->extra_data[2] == 1;
	}
	bool isAllType() const{
		return this->extra_data[2] == 2;
	}
	void setAnyType(){
		this->extra_data[2] = 1;
	}
	void setAllType(){
		this->extra_data[2] = 2;
	}
};
struct ConditionalTrigger : ComplexTrigger{
	virtual TriggerType getType() const{
		return TriggerType::CONDITIONAL;
	}
	virtual std::string toHtml(bool reversed,int depth = 1){
		return this->toString(reversed);
	}
	virtual std::string toString(bool reversed,int depth = 1) const;
	ComplexTrigger* condition;
	ConditionalTrigger() : ComplexTrigger(){};
	bool isElse() const {
		return this->extra_data[3] != 0;
	}
	void setElseState(){
		this->extra_data[3] = 1;
	}

};
//for calc_true_if
struct NumberRequiredTrigger : ComplexTrigger{
	virtual TriggerType getType() const{
		return TriggerType::NUM;
	}
	virtual std::string toHtml(bool reversed,int depth = 1){
		return this->toString(reversed,depth);
	}
	virtual std::string toString(bool reversed,int depth = 1) const;
	void setAmount(int amount){
		*(int*)(extra_data + 4) = amount;
	}
	int getAmount() const{
		return *(int*)(extra_data + 4); 
	}
	NumberRequiredTrigger() : ComplexTrigger(){};
	TriggerItem* item;
};
struct CustomTooltipTrigger : ComplexTrigger{
	virtual TriggerType getType() const{
		return TriggerType::CUSTOM_TT;
	}
	virtual std::string toHtml(bool reversed,int depth = 1){
		return this->toString(reversed,depth);
	}
	virtual std::string toString(bool reversed,int depth = 1) const;
	bool isShowOrigin() const{
		return this->extra_data[3] != 0;
	}
	void setShowOrigin(){
		this->extra_data[3] = 1;
	}
	std::string tooltip;
	CustomTooltipTrigger() : ComplexTrigger(){};
};

struct HiddenTrigger : ComplexTrigger{
	virtual TriggerType getType() const{
		return TriggerType::HIDDEN;
	}
	virtual std::string toHtml(bool reversed,int depth = 1){
		return this->toString(reversed,depth);
	}
	virtual std::string toString(bool reversed,int depth = 1) const;
	void isCurrentHidden(bool hidden){
		this->extra_data[3] = hidden;
	}
	bool isCurrentHidden() const{
		return this->extra_data[3] != 0;
	}
	HiddenTrigger() : ComplexTrigger(){};
};


void registerTriggerItems();
ComplexTrigger* createBaseTrigger(); 
void parseTrigger(ParadoxTag*,ComplexTrigger* trigger);

#endif
