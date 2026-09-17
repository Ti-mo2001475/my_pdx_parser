#ifndef PDX_EFFECT
#define PDX_EFFECT
#include<string>
#include<functional>
#include "paradox_type.h"
#include<map>
#include<vector>
#include<bitset>
#include<memory>
#include<iostream>
#include"scope.h"
#include<cassert>
#include<cstdint>

struct Trigger;
struct Effect;
struct ComplexEffect;
struct ScriptedEffect;
enum class EffectType{
	COMMON,CHANGE_SCOPE,CONDITIONAL,HIDDEN,RANDOM,RANDOM_LIST,TOOLTIP,SPECIAL
};



struct Effect{
	virtual std::string toString(int depth = 1) = 0;
	virtual EffectType getType() = 0;
	uint8_t extra_data[8];
	ComplexEffect* getAsComplexEffect(){
		if(this->getType() != EffectType::COMMON) return (ComplexEffect*)this;
		return nullptr;
	}
	virtual ~Effect() noexcept = default;
};
template<typename... types>
struct NativeCommonEffect;

template<typename... types>
struct _NativeCommonEffect;

template<typename... types>
struct _EffectItem;

template<ParadoxType... types>
using _EffectItem2 = _EffectItem<rawType<types>...>;

template<typename head,typename... tails>
typename std::tuple_element<0, _NativeCommonEffect<head,tails...>>::type& get(_NativeCommonEffect<head,tails...> effect);

template< class F, class Tuple >
constexpr decltype(auto) apply_effect( F&& f, Tuple&& t );

template<class F,class Tuple, std::size_t... I>
constexpr decltype(auto)
    apply_effect_impl(F&& f, Tuple&& t, std::index_sequence<I...>);

template<std::size_t I,typename head,typename... tails>
struct std::tuple_element<I,_NativeCommonEffect<head,tails...>> : std::tuple_element<I - 1,_NativeCommonEffect<tails...>>{
	
};
template<typename head,typename... tails>
struct std::tuple_element<0,_NativeCommonEffect<head,tails...>>{
	using type = head;
};
template<typename... type>
struct std::tuple_size<_NativeCommonEffect<type...>>{
	constexpr static size_t value = sizeof...(type);
};

struct EffectItem{
	const std::string& name;
	ScopeType usable_scope;
	int attribute;
	virtual ParadoxType getType(int index) = 0;
	virtual int getParameterAmount() = 0;
	EffectItem(std::string _name,ScopeType _usable_scope) : name(_name), usable_scope(_usable_scope){} 
	virtual std::unique_ptr<Effect> createInstance(const std::vector<ParadoxBase*>& base) = 0;
	virtual std::unique_ptr<Effect> createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>& base) = 0;
};


template<>
struct _EffectItem<>{
	int getParameterAmount() {
		return 0;
	}
	inline ParadoxType getType(int index){ return ParadoxType::BASE; }

};

template<typename head, typename...tail>
struct _EffectItem<head,tail...> : private _EffectItem<tail...>{
	public:
	int getParameterAmount(){
		return sizeof...(tail) + 1;
	}
	inline ParadoxType getType(size_t index){
		if (index > sizeof...(tail)) { 
			return ParadoxType::BASE;
		}
		if (index == 0) return getParadoxType<head>();
		return this->getTail().getType(index - 1);
	}

	private:
	_EffectItem<tail...>& getTail(){ return static_cast<_EffectItem<tail...>&>(*this); }
};
template<typename... types>
struct NativeEffectItem : EffectItem{
	std::function<std::string(types...)> localizeFunction;
	_EffectItem<types...> content;
	virtual constexpr ParadoxType getType(int index){
		return content.getType(index);
	}
	virtual int getParameterAmount(){
		return content.getParameterAmount();
	}

	NativeEffectItem(std::string _name,ScopeType _usable_scope,std::function<std::string(types...)> _localizeFunction) : EffectItem(_name,_usable_scope),localizeFunction(_localizeFunction){}
	virtual std::unique_ptr<Effect> createInstance(const std::vector<ParadoxBase*>& base){
		if(base.size() < sizeof...(types)) return nullptr;
		NativeCommonEffect<types...>* _instance = new NativeCommonEffect<types...>();
		std::unique_ptr<Effect> instance(_instance);
		_instance->item = this;
		for(size_t i = 0;i < sizeof...(types);i++){
			ParadoxBase* arg = base[i];
			if(base[i] == nullptr) return nullptr;
			if(content.getType(i) != base[i]->getType()) {
				if(isCastable(base[i],content.getType(i))){
					arg = castTo(base[i],content.getType(i));
					if(arg == nullptr) return nullptr;
				}
				else return nullptr;	
			}
			_instance->body.unsafe_set(i,arg);
			
		}
		return instance;
	};
	virtual std::unique_ptr<Effect> createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>& base){
		return nullptr;
	}
};

template<typename... types>
struct ParameteredNativeEffectItem : NativeEffectItem<types...>{
	std::array<std::string,sizeof...(types)> parameterName;
	std::bitset<sizeof...(types)> parameterRequired;
	ParameteredNativeEffectItem(std::string _name,
								ScopeType _usable_scope,
								std::function<std::string(types...)> _localizeFunction,
								std::array<std::string,sizeof...(types)> _parameterName,
								std::bitset<sizeof...(types)> required = std::bitset<sizeof...(types)>().flip())
								:
								NativeEffectItem<types...>(_name,_usable_scope,_localizeFunction),
								parameterName(_parameterName), parameterRequired(required) {};
	virtual std::unique_ptr<Effect> createInstance(const std::vector<std::pair<std::string,ParadoxBase*>>& base){
		if(base.size() > sizeof...(types)) return nullptr; //Only extra Parameter Not supported.
		NativeCommonEffect<types...>* _instance = new NativeCommonEffect<types...>();
		_instance->body.set_default_values();
		_instance->item = this;
		std::unique_ptr<Effect> instance(_instance);
		std::bitset<sizeof...(types)> required = parameterRequired; // 追踪必要参数是否均已提供
		std::bitset<sizeof...(types)> provided;                    // 追踪已提供的参数,重复提供时忽略后提供的
		for(const auto& [key, value] : base){
			size_t idx = 0;
			for(;idx < sizeof...(types);idx++){
				if(parameterName[idx] == key) break;
			}
			if(idx == sizeof...(types)) continue; // 未知参数,忽略
			if(provided[idx]) continue;           // 重复参数,忽略后续提供的
			provided[idx] = true;
			required[idx] = false;
			ParadoxBase* arg = value;
			if(arg == nullptr) return nullptr;
			if(arg->getType() != this->content.getType(idx)) {
				if(isCastable(arg,this->content.getType(idx))){
					arg = castTo(arg,this->content.getType(idx));
					if(arg == nullptr) return nullptr;
				}
				else return nullptr;				
			}
			_instance->body.unsafe_set(idx,arg);
		}
		if(required.any()) return nullptr; // 存在未提供的必要参数
		return instance;
	}
};

template<typename... tails>
struct NativeCommonEffect : Effect
{
	NativeEffectItem<tails...>* item;
	_NativeCommonEffect<tails...> body;
	
	virtual std::string toString(int depth = 1){
		std::string str("");
		for(int i = 0;i < depth;i++) str.push_back(' ');
		str.append(apply_effect(item->localizeFunction,this->body));
		str.push_back('\n');
		return str;
	}
	virtual EffectType getType(){
		return EffectType::COMMON;
	}
};

template<>
struct _NativeCommonEffect<>{
    inline std::unique_ptr<ParadoxBase> getContent(int i){
		return std::unique_ptr<ParadoxBase>(nullptr);
	}
	void set(int i, ParadoxBase* val){}
	void unsafe_set(int i, ParadoxBase* val){}
	void set_default_values(){}
};
template<typename head,typename... tails>
struct _NativeCommonEffect<head,tails...> : _NativeCommonEffect<tails...>{
    head value;
	inline std::unique_ptr<ParadoxBase> getContent(int i){
		if(i > sizeof...(tails)) return std::unique_ptr<ParadoxBase>(nullptr);
		if(i == 0) return std::make_unique<pdxTypeFromRaw<head>>(value);
		return getTails().getContent(i - 1);
	}
	void set(int i, ParadoxBase* val){
		if(val == nullptr) return;
		if(i > sizeof...(tails)) return;
		if(i == 0){
			if constexpr (std::is_same_v<rawType<ParadoxType::INTEGER>,head>) {
				if(val->getType() == ParadoxType::INTEGER){
					value = val->getAsInteger()->getIntegerContent();
				} 
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::STRING>,head>) {
				if(val->getType() == ParadoxType::STRING){
					value = val->getAsString()->getStringContent();
				} 
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::SCOPE>,head>) {
				if(val->getType() == ParadoxType::SCOPE){
					value = val->getAsScope()->getValue();
				} 
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::BOOLEAN>,head>) {
				if(val->getType() == ParadoxType::BOOLEAN){
					value = val->getAsBoolean()->getValue();
				} 
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::DATE>,head>) {
				if(val->getType() == ParadoxType::DATE){
					value = val->getAsDate()->getDateContent();
				} 
			}
			return;			
		}
		getTails().set(i - 1, val);
	}
	void set_default_values(){
			if constexpr (std::is_same_v<rawType<ParadoxType::INTEGER>,head>) {
				value = 0;
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::STRING>,head>) {
				value = "";
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::SCOPE>,head>) {
				value = nullptr;
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::BOOLEAN>,head>) {
				value = false;
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::DATE>,head>) {
				value = Date();
			}		
		getTails().set_default_values();
	}
	void unsafe_set(size_t i, ParadoxBase* val){
		if(val == nullptr) return;
		if(i > sizeof...(tails)) return;
		if(i == 0){
			if constexpr (std::is_same_v<rawType<ParadoxType::INTEGER>,head>) {
				value = val->getAsInteger()->getIntegerContent();
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::STRING>,head>) {
				value = val->getAsString()->getStringContent(); 
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::SCOPE>,head>) {
				value = val->getAsScope()->getValue();
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::BOOLEAN>,head>) {
				value = val->getAsBoolean()->getValue();
			}
			if constexpr (std::is_same_v<rawType<ParadoxType::DATE>,head>) {
				value = val->getAsDate()->getDateContent();
			}
			return;			
		}
		getTails().unsafe_set(i - 1, val);
	}
	constexpr _NativeCommonEffect<tails...>& getTails() { return static_cast<_NativeCommonEffect<tails...>&>(*this);}
};
template<size_t I,typename head,typename... tails>
typename std::tuple_element<I, _NativeCommonEffect<head,tails...>>::type& get(_NativeCommonEffect<head,tails...>& effect){
	if constexpr(I == 0) return effect.value;
	if constexpr(I != 0) return get<I - 1,tails...>(effect.getTails());
}

template<class F,class Tuple, std::size_t... I>
constexpr decltype(auto)
    apply_effect_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return std::invoke(std::forward<F>(f), get<I>(std::forward<Tuple>(t))...);
}

template<class F,class Tuple>
constexpr decltype(auto)
    apply_effect(F&& f, Tuple&& t)
{
    return apply_effect_impl(std::forward<F>(f), std::forward<Tuple>(t),std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
}

struct ComplexEffect : Effect{
	virtual std::string toString(int depth = 1) = 0;
	std::vector<Effect*> subEffects;
	void addEffect(Effect* effect){
		subEffects.push_back(effect);
	}
};

struct ChangeScopeEffect : ComplexEffect{
	virtual std::string toString(int depth = 1);
	Scope* target;
	Trigger* condition;
	virtual EffectType getType(){
		return EffectType::CHANGE_SCOPE;
	}
	ChangeScopeEffect(Scope* _target, Trigger* _condition = nullptr) : target(_target), condition(_condition){}
};
struct ConditionalEffect : ComplexEffect{
	virtual std::string toString(int depth = 1);
	Trigger* condition;
	void setElseIfState(){ this->extra_data[0] = 1; };
	void setElseState() { this->extra_data[0] = 2; }
	void setLoopState() { this->extra_data[0] = 4;}
	bool isElseIf() { return this->extra_data[0] == 1; }
	bool isElse() { return this->extra_data[0] == 2; }
	bool isLoop() { return this->extra_data[0] == 4;}
	ConditionalEffect() noexcept{
		this->extra_data[0] = 0;
	}
	ConditionalEffect(ConditionalEffect&& ce) = default;
	ConditionalEffect(ConditionalEffect& ce) = default;
	virtual EffectType getType(){
		return EffectType::CONDITIONAL;
	}
};
struct HiddenEffect : ComplexEffect{
	virtual std::string toString(int depth = 1);
	void setHidden(bool value) { this->extra_data[0] = value; }
	bool isHidden() { return this->extra_data[0]; }
	virtual EffectType getType(){
		return EffectType::HIDDEN;
	}
};
struct RandomEffect : ComplexEffect {
	virtual std::string toString(int depth = 1);
	int getChance(){
		return *(int*)extra_data;
	}
	void setChance(int chance){
		*(int*)extra_data = chance;
	}
	virtual EffectType getType(){
		return EffectType::RANDOM;
	}
};
//sizeof(SpecialEffect) = 72...
struct SpecialEffect : Effect {
	virtual std::string toString(int depth = 1);
	virtual EffectType getType() {
		return EffectType::SPECIAL;
	}
	ScriptedEffect* prototype;
	Effect* instance;
	std::map<std::string,ParadoxBase*> items;
	virtual ~SpecialEffect(){
		for(auto[u,v]:items) delete v;
	}
};

struct RandomItem{
	long long baseWeight;
	Trigger* trigger;
	std::vector<std::pair<Trigger*,long long>> modifier;
	Effect* effect;
};
struct RandomListEffect : Effect {
	std::vector<RandomItem> items;
	virtual std::string toString(int depth = 1);
	virtual EffectType getType() {
		return EffectType::RANDOM_LIST;
	}
	virtual ~RandomListEffect() = default;
	FixedRandomListEffect* simplify();

};
struct FixedRandomListEffect : Effect {
	std::vector<std::pair<long long, Effect*>> items;
	virtual std::string toString(int depth = 1);
	virtual EffectType getType() {
		return EffectType::RANDOM_LIST;
	}
	virtual ~FixedRandomListEffect() = default;
	FixedRandomListEffect(){
		*(uint64_t*)this->extra_data = 0;
	}
	long long getTotalWeight(){
		return *(uint64_t*)this->extra_data;
	}
};

void parseEffect(ParadoxTag* root,ComplexEffect* from);
std::unique_ptr<ComplexEffect> createBaseEffect();
void registerEffectItems();
#endif