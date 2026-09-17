#include "effect.h"
#include "trigger.h"
#include "localization.h"
#include "pattern.h"
#include "utils/functional_util.h"
#include "paradox_type.h"
#include "paradox_macro.h"
#include <array>
#include <string_view>
#include <iostream>
#include <source_location>
#include <charconv>

using OverrideHandler = bool(*)(std::vector<std::pair<std::string,ParadoxBase*>>&);

extern std::map<const std::string*,ScriptedEffect*> loadedSEs;
extern ParadoxString* createString(std::string);
std::map<const std::string*,EffectItem*> effectItems;
std::set<const std::string*> simpleEffectItems;
std::map<const EffectItem*,OverrideHandler> effectHandlers;

inline void preInit(std::string& str,int depth){
    for(int i = 0;i < depth;i++) str.push_back(' ');
}

bool isSimpleItem(std::string key){
    return simpleEffectItems.find(getStringPtr(key)) != simpleEffectItems.end();
}
EffectItem* getEffectItem(std::string key){
    return effectItems[getStringPtr(key)];
}   
template<ParadoxType types>
void registerSingleArgEffect(std::string name,ScopeType usable_scope, std::function<std::string(rawType<types>)> localize){
    const std::string* name_ptr = getStringPtr(name.append(std::to_string(static_cast<int>(types))));
    EffectItem* u = new NativeEffectItem<rawType<types>>(*name_ptr,usable_scope,localize);
    
    effectItems[name_ptr] = u;
}

template<ParadoxType... types>
void registerSimpleEffect(std::string name,ScopeType usable_scope, std::function<std::string(rawType<types>...)> localize){
    const std::string* name_ptr = getStringPtr(name);
    EffectItem* u = new NativeEffectItem<rawType<types>...>(*name_ptr,usable_scope,localize);
    simpleEffectItems.insert(name_ptr);
    effectItems[name_ptr] = u;
}

template<ParadoxType...types>
void registerSimpleClausedEffect(std::string name,ScopeType usable_scope,std::function<std::string(rawType<types>...)> localize,std::array<std::string,sizeof...(types)> names,std::bitset<sizeof...(types)> required = flipedBitset<sizeof...(types)>()){
    const std::string* name_ptr = getStringPtr(name);
    if(!localize) log_error(current_location(),"An empty localize function was provided for effect ",name);
    EffectItem* u = new ParameteredNativeEffectItem<rawType<types>...>(*name_ptr,usable_scope,localize,names,required);
    simpleEffectItems.insert(name_ptr);
    effectItems[name_ptr] = u;
}

template<ParadoxType...types>
void regiserArrayEffect(std::string name,ScopeType usable_scope,std::function<std::string(rawType<types>...)> localize,std::array<std::string,sizeof...(types)> names){
    name.append("_array");
    const std::string* name_ptr = getStringPtr(name);
    EffectItem* u = new NativeEffectItem<rawType<types>...>(*name_ptr,usable_scope,localize,names);
    simpleEffectItems.insert(name_ptr);
    effectItems[name_ptr] = u;
}

template<ParadoxType...types>
void registerClausedEffect(std::string name,ScopeType usable_scope,std::function<std::string(rawType<types>...)> localize,std::array<std::string,sizeof...(types)> names,std::bitset<sizeof...(types)> required,OverrideHandler handler){
    const std::string* name_ptr = getStringPtr(name);
    EffectItem* u = new ParameteredNativeEffectItem<rawType<types>...>(*name_ptr,usable_scope,localize,names,required);
    simpleEffectItems.insert(name_ptr);
    effectItems[name_ptr] = u;    
    effectHandlers[u] = handler;
}//

void registerEffectItems(){
    using namespace std::literals;
    static const ParadoxType pInt = ParadoxType::INTEGER;
    static const ParadoxType pStr = ParadoxType::STRING;
    static const ParadoxType pBool = ParadoxType::BOOLEAN;
    static const ParadoxType pScope = ParadoxType::SCOPE;

    registerSimpleEffect<pInt>("add_treasury"s,ScopeType::COUNTRY,
        signedPattern<0,pInt>("获得%d克朗"s,"失去-%d克朗"s));
    registerSimpleClausedEffect<pScope,pInt>("add_trust"s,ScopeType::COUNTRY,
        signedOrderPattern<1,pScope,pInt>("对%s的信任提高了%d"s,"对%s的信任降低了-%d"s),{"who","value"});
    registerSimpleEffect<pStr>("set_global_flag"s,ScopeType::ANY,
        orderedPattern<pStr>("设置全局标签'%s'"s));
    registerSimpleEffect<pStr>("clr_global_flag"s,ScopeType::ANY,
        orderedPattern<pStr>("清除全局标签'%s'"s));
    registerSimpleEffect<pStr>("custom_tooltip"s,ScopeType::ANY,
    [](std::string str){return getLocalization(str);});
    registerSimpleEffect<pStr>("save_event_target_as",ScopeType::ANY,
        orderedPattern<pStr>("将当前作用域保存为事件目标%s"s));
    registerSimpleEffect<pStr>("save_global_event_target_as",ScopeType::ANY,
        orderedPattern<pStr>("将当前作用域保存为全局事件目标%s"s));
    registerSimpleEffect<pStr>("clear_global_event_target",ScopeType::ANY,
        orderedPattern<pStr>("清除全局事件目标%s"s));
    registerSimpleEffect<pBool>("clear_global_event_target",ScopeType::ANY,
        [](bool b){return "清除所有全局事件目标";});
    registerSimpleEffect<pInt>("add_years_of_income",ScopeType::COUNTRY,
        signedPattern<0,pInt>("获得与%d年年收入数量的克朗","失去%d年年收入数量的克朗"));
    registerSimpleEffect<pInt>("add_treasury",ScopeType::COUNTRY,
        signedPattern<0,pInt>("获得%d克朗","失去%d克朗"));
    registerSimpleClausedEffect<pStr,pInt,pInt,pStr>("country_event",ScopeType::COUNTRY,
        [](std::string id,long long days,long long random,std::string tt){
            if(days == 0) {
                return applyPattern("触发事件%s",id);
            }
            else if(random == 0){
                return applyPattern("事件%s将在%d天后发生",id,days);
            }
            else return applyPattern("事件%s将在%d天到%d天内发生",id,days,days+random);
        },
        {"id","days","random","tooltip"},
        fromArray<4>(
            {'T','F','F','F'}
        )
    );
    auto variable_handler = [](std::vector<std::pair<std::string,ParadoxBase*>>& map)-> bool {
		std::string src_string = "";
		std::string tar_string = "";
		if(map.size() != 1){
			// 2 参数:第一个必须为 which
			if(map[0].first != "which") return false;
			src_string = map[0].second->getAsString()->getStringContent();
			ParadoxBase* mv_src = map[0].second;
			// 第二个参数适用于 value 或 which(原 which@2 以 which 代替)
			if(map[1].first == "value"){
				tar_string = std::to_string(map[1].second->getAsInteger()->getIntegerContent() / 1000);
			}
			else if(map[1].first == "which"){
				if(isCastable(map[1].second,ParadoxType::SCOPE)){
					std::string tar1 = "";
					if(map[1].second->getType() == ParadoxType::INTEGER) {
						tar1 = std::to_string(map[1].second->getAsInteger()->getIntegerContent() / 1000);
					}
					else tar1 = map[1].second->getAsString()->getStringContent();
					Scope* scope = createScopeFromString(tar1);
					if(scope == nullptr) return false;
					else tar_string = scope->toString();
				}
				else tar_string = map[1].second->getAsString()->getStringContent();
			}
			else return false;
			map.clear();
			map.push_back({"src", mv_src});
			map.push_back({"tar", createString(tar_string)});
		}
		else {
			auto [k,v] = map[0];
			map.clear();
			map.push_back({"src", createString(k)});
			ParadoxString* str = v->getAsString();
			if(str != nullptr){
				if(Scope *scope = createScopeFromString(str->getStringContent());scope != nullptr){
					map.push_back({"tar", createString(scope->toString())});
				}
				else map.push_back({"tar", str});
			}
			else if(ParadoxInteger* pi = v->getAsInteger();pi != nullptr){
				map.push_back({"tar", createString(std::to_string(pi->getIntegerContent() / 1000))});
			}
			else return false;
		}
        return true;
    };
    registerClausedEffect<pStr,pStr>("set_variable",ScopeType::ANY,
        orderedPattern<pStr,pStr>("将变量%s的值设置为%s"),
        {"src","tar"},
        flipedBitset<2>(),
        variable_handler
    );
    registerClausedEffect<pStr,pStr>("change_variable",ScopeType::ANY,
        orderedPattern<pStr,pStr>("将变量%s的值增加%s"),
        {"src","tar"},
        flipedBitset<2>(),
        variable_handler
    );
    registerClausedEffect<pStr,pStr>("subtract_variable",ScopeType::ANY,
        orderedPattern<pStr,pStr>("将变量%s的值减少%s"),
        {"src","tar"},
        flipedBitset<2>(),
        variable_handler
    );
    registerClausedEffect<pStr,pStr>("multiply_variable",ScopeType::ANY,
        orderedPattern<pStr,pStr>("将变量%s的值乘以%s"),
        {"src","tar"},
        flipedBitset<2>(),
        variable_handler
    );



}

std::string ChangeScopeEffect::toString(int depth){
    std::string str("");
    if(this->target != nullptr){
        preInit(str,depth);
        if(this->subEffects.empty()) return str;
        str.append(this->target->toString());
        str.append(":\n");
        if(this->condition != nullptr){
            preInit(str,depth + 1);
            str.append("若满足以下条件:");
            str.append(this->condition->toString(false,depth + 2));
            preInit(str,depth + 1);
            str.append("则:\n");
        }
    }
    for(Effect* effect : this->subEffects){
        str.append(effect->toString(depth + 2));
    }
    return str;
}

std::string HiddenEffect::toString(int depth){
    if(this->isHidden()) return "";
    else{
        std::string str("");
        preInit(str,depth);
        str.append("隐藏效果:\n");
        for(Effect* effect : this->subEffects){
            str.append(effect->toString(depth + 1));
        } 
        return str;
    }
}

std::string ConditionalEffect::toString(int depth){
    std::string str("");
    if(this->condition == nullptr && !this->isElse()){
        log_warning(current_location(),"unexcepted nullptr trigger in ConditionalEffect.");
        depth--;
    }
    else if(this->isElse()){
        preInit(str,depth);
        str.append("否则:\n");
    }
    else if(this->isElseIf()){
        preInit(str,depth);
        str.append("否则若满足以下条件:\n");
        preInit(str,depth + 1);
        str.append(this->condition->toString(false));
        preInit(str,depth);
        str.append("则:\n");
    }
    else if(this->isLoop()){
        preInit(str,depth);
        str.append("若满足以下条件:\n");
        preInit(str,depth + 1);
        str.append(this->condition->toString(false));
        preInit(str,depth);
        str.append("则循环执行:\n");        
    }
    else {
        preInit(str,depth);
        str.append("若满足以下条件:\n");
        preInit(str,depth + 1);
        str.append(this->condition->toString(false));
        preInit(str,depth);
        str.append("则:\n");
    }

    for(Effect* effect : this->subEffects){
        str.append(effect->toString(depth + 1));
    }
    return str;
}
std::string RandomEffect::toString(int depth){
    std::string str("");
    preInit(str,depth);
    str.append(applyPattern("%d%%概率发生以下效果:",this->getChance()));
    for(Effect* effect : this->subEffects){
        str.append(effect->toString(depth + 1));
    }
    return str;
}

std::string SpecialEffect::toString(int depth){
    if(this->instance == nullptr && this->prototype->isFixed()){
 		std::vector<std::pair<std::string,ParadoxBase*>> data;
		this->instance = this->prototype->createInstance(data);       
    }
    return this->instance->toString(depth);
}

FixedRandomListEffect* RandomListEffect::simplify(){
    for(RandomItem& item : this->items){
        if(item.trigger == nullptr && item.modifier.empty()) return nullptr;
    }
    FixedRandomListEffect* effect = new FixedRandomListEffect();  
    *((uint64_t*)effect->extra_data) = 0;
    for(RandomItem& item : this->items){
        effect->items.emplace_back(item.baseWeight,item.effect);
        *((uint64_t*)effect->extra_data) += item.baseWeight;
    }
    return effect;
}

std::string FixedRandomListEffect::toString(int depth){
    std::string str("");
    preInit(str,depth);
    long long totalWeight = this->getTotalWeight();
    str.append("随机发生下列效果之一:\n");
    for(auto[weight,effect] : this->items){
        preInit(str,depth + 1);
        long long chance = weight * 100'000 / totalWeight;
        if(weight != 0 && chance == 0){
            str.append(applyPattern("小于0.001%的概率发生下列效果:\n"));
        }
        else {
            str.append(applyPattern("%d%%的概率发生下列效果:\n",chance));
        }
        str.append(effect->toString(depth + 2));
    }
    return str;
}

std::string RandomListEffect::toString(int depth){
    std::string str("");
    preInit(str,depth);
    str.append("随机发生下列效果之一:\n");
    for(auto& item : this->items){
        preInit(str,depth + 1);
        if(item.trigger != nullptr) {
            str.append("若满足下列条件:\n");
            str.append(item.trigger->toString(depth+2));
            preInit(str,depth + 1);
        }
        str.append(applyPattern("有%d的基础权重发生下列效果:\n",item.baseWeight));
        str.append(item.effect->toString(depth+2));
        for(auto[trigger,factor] : item.modifier){
            preInit(str,depth+1);
            str.append(applyPattern("若满足以下条件,权重将会变为原来的%d倍:\n",factor));
            str.append(item.effect->toString(depth+2));
        }
    }    
    return str;
}


std::unique_ptr<ComplexEffect> createBaseEffect(){
    auto ptr = std::unique_ptr<ComplexEffect>(static_cast<ComplexEffect*>(new ChangeScopeEffect(nullptr)));
    return ptr;
}
//Final Part of the Paradox Parser Base!
void parseEffect(ParadoxTag* root,ComplexEffect* from){
    for(int i = 0;i < root->size();i++){
        auto[key, childNode] = (*root)[i];
        std::string name = key;
        ParadoxBase* base = childNode;
        ParadoxTag* tag = base->getAsTag();
        
        if(tag != nullptr){
            if(name == "if"){
                if(ParadoxTag* subTag = tag->getAsTag("limit");subTag != nullptr){
                    ConditionalEffect* effect = new ConditionalEffect();
                    from->addEffect(effect);
                    ComplexTrigger* trigger = createBaseTrigger();
                    parseTrigger(subTag,trigger);
                    trigger->takeOverLifeCycle();
                    tag->remove("limit",0);
                    effect->condition = trigger;
                    
                    parseEffect(tag,effect);
                }
                else {
                    log_error(current_location(),"cannot create a Conditional Effect without \"limit\" block.");
                }
            }
            else if(name == "else_if"){
                if(from->subEffects.empty()) continue;
                if (Effect* back = from->subEffects.back();back->getType() == EffectType::CONDITIONAL && back->extra_data[0] <= 1){
                    if(ParadoxTag* subTag = tag->getAsTag("limit");subTag != nullptr){
                        ConditionalEffect* effect = new ConditionalEffect();
                        from->addEffect(effect);
                        ComplexTrigger* trigger = createBaseTrigger();
                        parseTrigger(subTag,trigger);
                        tag->remove("limit",0);
                        effect->condition = trigger;
                        trigger->takeOverLifeCycle();
                        effect->setElseIfState();
                        parseEffect(tag,effect);
                    }
                    else {
                        log_error(current_location(),"cannot create a Conditional Effect without \"limit\" block.");
                    }
                }
                else log_error(current_location(),"cannot create \"else_if\" block before a \"if\" block.");
            }
            else if(name == "else"){
                if(from->subEffects.empty()) continue;
                if (Effect* back = from->subEffects.back();back->getType() == EffectType::CONDITIONAL && back->extra_data[0] <= 1){
                        ConditionalEffect* effect = new ConditionalEffect();
                        from->addEffect(effect);
                        effect->setElseState();
                        parseEffect(tag,effect);
                }
                else{
                    auto location = current_location();
                    log_error(current_location(),"cannot create \"else\" block before a \"if\" block.");
                    log_error(location,"Context:");
                    for(int j = 0;j < tag->size();j++){
                        auto[k, v] = (*tag)[j];
                        log_error(location,k,":",v->toString());
                    }
                    log_error(location,"effects:");
                    for(auto k : from->subEffects){
                        log_error(location,"effect:",(int)k->getType()," ",(int)k->extra_data[0]);
                    }
                } 
            }
            else if(name == "while"){
                if(ParadoxTag* subTag = tag->getAsTag("limit");subTag != nullptr){
                    ConditionalEffect* effect = new ConditionalEffect();
                    from->addEffect(effect);
                    ComplexTrigger* trigger = createBaseTrigger();
                    parseTrigger(subTag,trigger);
                    trigger->takeOverLifeCycle();
                    tag->remove("limit",0);
                    effect->condition = trigger;
                    effect->setLoopState();
                    parseEffect(tag,effect);
                }
                else {
                    log_error(current_location(),"cannot create a Conditional Effect without \"limit\" block.");
                }                
            }
            else if(name == "hidden_effect"){
                HiddenEffect* effect = new HiddenEffect();
                from->addEffect(effect);
                parseEffect(tag,effect);
            }
            else if(name == "random"){
                ParadoxBase* base = tag->get("chance",1);
                if(base == nullptr) log_error(current_location(),"cannot create \"random\" block without \"chance\"");
                if(ParadoxInteger* chance = base->getAsInteger();chance != nullptr){
                    RandomEffect* effect = new RandomEffect();
                    effect->setChance(chance->getIntegerContent());
                    tag->remove("chance",0);
                    from->addEffect(from);
                    parseEffect(tag,effect);
                }
                else {
                    log_error(current_location(),"cannot create \"random\" block with a non-number \"chance\"");
                }
            }
            else if(name == "random_list"){
                RandomListEffect* effect = new RandomListEffect();
                for(int i = 0;i < tag->size();i++){
                    RandomItem item;
                    auto[chance_str,content] = (*tag)[i];
                    double chance = 0;
                    std::from_chars(chance_str.data(),chance_str.data() + chance_str.size(),chance);
                    if(chance == 0.0) {
                        delete effect;
                        effect = nullptr;
                        log_error(current_location(),"cannot create \"random_list\" item with non-number or zero chance.");
                        break;
                    }
                    int iChance = chance * 1000;
                    item.baseWeight = iChance;
                    ComplexEffect* itemEffect = createBaseEffect().release(); 
                    if(content->getAsTag() == nullptr){
                        delete effect;
                        effect = nullptr;
                        log_error(current_location(),"cannot create \"random_list\" block without effect.");
                        break;
                    }
                    ParadoxTag* subTag = content->getAsTag();
                    parseEffect(subTag,itemEffect);
                    item.effect = itemEffect;
                    if(ParadoxTag* triggerTag = subTag->getAsTag("trigger");triggerTag != nullptr){
                        ComplexTrigger* trigger = createBaseTrigger();
                        parseTrigger(triggerTag,trigger);
                        item.trigger = trigger;
                    }
                    for(int i = 0;i < subTag->size();i++){
                        auto[k,v] = (*subTag)[i];
                        if(k != "modifier") continue;
                        if(ParadoxTag* modifierTag = v->getAsTag();modifierTag != nullptr){
                            long long factor = 0;
                            if(ParadoxBase* factorBase = modifierTag->get("factor");factorBase == nullptr && factorBase->getType() == ParadoxType::INTEGER) {
                                ParadoxInteger* pInt = (ParadoxInteger*)factorBase;
                                factor = pInt->getIntegerContent();
                            }
                            else continue;
                            ComplexTrigger* trigger = createBaseTrigger();
                            parseTrigger(modifierTag,trigger);
                            item.modifier.emplace_back(trigger,factor);
                        }
                    }
                }
                if(effect != nullptr) {
                    if(FixedRandomListEffect* simpleEffect = effect->simplify(); simpleEffect != nullptr) {
                        delete effect;
                        from->addEffect(simpleEffect);
                    }
                    else from->addEffect(effect);
                }
            }
            else if(Scope* scope = createScopeFromString(name);scope != nullptr){
                ParadoxTag* subTag = tag->getAsTag("limit");
                
                if(subTag != nullptr && !scope->isMultiScope()) log_error(current_location(),"\"limit\" block for single scope is Invalid.");
                else {
                    ChangeScopeEffect* effect = nullptr;
                    if(subTag != nullptr){
                        ComplexTrigger* trigger = createBaseTrigger();
                        parseTrigger(subTag,trigger);
                        trigger->takeOverLifeCycle();
                        effect = new ChangeScopeEffect(scope,trigger);   
                        from->addEffect(effect);
                        tag->remove("limit",0);
                        parseEffect(tag,effect);
                    }
                    else {
                        effect = new ChangeScopeEffect(scope);
                        from->addEffect(effect);
                        parseEffect(tag,effect);
                    }
                }
            }
            else {
                auto ptr = getStringPtr(name);
                if(loadedSEs.contains(ptr)){
                    SpecialEffect* se = new SpecialEffect();
                    se->prototype = loadedSEs[ptr];
                    se->instance = se->prototype->createInstance(tag->contents);       
                    if(se->instance == nullptr){
                        delete se;
                        continue;
                    }
                    for(int j = 0;j < tag->size();j++){
                        auto[k, v] = (*tag)[j];
                        se->items[k] = deep_copy(v);
                    }
                    from->addEffect(se);
                }
                else {
                    EffectItem* item = effectItems[ptr];
                    if(item == nullptr) continue;
                    std::vector<std::pair<std::string,ParadoxBase*>> parameters = tag->contents; // 拷贝,因 handler 会修改
                    if(effectHandlers.contains(item)){
                        bool success = effectHandlers[item](parameters);
                        if(!success) continue;
                    }
                    Effect* effect = item->createInstance(parameters).release();
                    if(effect != nullptr) from->addEffect(effect);
                }

            }
        }
        else if(base->getType() == ParadoxType::ARRAY){
            //arrays should not be 1 args...
            name.append("_array");
            EffectItem* item = getEffectItem(name);
            if(item == nullptr) continue;
            Effect* effect = item->createInstance(base->getAsArray()->contents).release();
            if(effect != nullptr) from->addEffect(effect);
        }
        else {
            auto ptr = getStringPtr(name);
            if(loadedSEs.contains(ptr)){
                SpecialEffect* se = new SpecialEffect();
                se->prototype = loadedSEs[ptr];
                std::vector<std::pair<std::string,ParadoxBase*>> placeholder;
                se->instance = se->prototype->createInstance(placeholder);
                if(se->instance != nullptr) from->addEffect(se);
                else delete se;
            }
            else if(simpleEffectItems.contains(ptr)){
                EffectItem* item = effectItems[ptr];
                Effect* effect = item->createInstance(std::vector<ParadoxBase*>{base}).release();
                if(effect != nullptr) from->addEffect(effect);
            }
            else{
                if(base->getType() == ParadoxType::INTEGER){
                    if(EffectItem* item = getEffectItem(name + std::to_string(static_cast<uint8_t>(ParadoxType::INTEGER)));item != nullptr){
                        Effect* effect = item->createInstance(std::vector<ParadoxBase*>{base}).release();
                        if(effect != nullptr) from->addEffect(effect);    
                    }
                    else if(EffectItem* item = getEffectItem(name + std::to_string(static_cast<uint8_t>(ParadoxType::SCOPE)));item != nullptr){
                        Effect* effect = item->createInstance(std::vector<ParadoxBase*>{castTo(base,ParadoxType::SCOPE)}).release();
                        if(effect != nullptr) from->addEffect(effect);    
                    }
                }
                else if(base->getType() == ParadoxType::STRING){
                    if(EffectItem* item = getEffectItem(name + std::to_string(static_cast<uint8_t>(ParadoxType::SCOPE)));item != nullptr){
                        Effect* effect = item->createInstance(std::vector<ParadoxBase*>{castTo(base,ParadoxType::SCOPE)}).release();
                        if(effect != nullptr) from->addEffect(effect);    
                    }
                    else if(EffectItem* item = getEffectItem(name + std::to_string(static_cast<uint8_t>(ParadoxType::STRING)));item != nullptr){
                        Effect* effect = item->createInstance(std::vector<ParadoxBase*>{base}).release();
                        if(effect != nullptr) from->addEffect(effect);    
                    }
                }
                else if(EffectItem* item = getEffectItem(name + std::to_string(static_cast<uint8_t>(base->getType())));item != nullptr){
                    Effect* effect = item->createInstance(std::vector<ParadoxBase*>{base}).release();
                    if(effect != nullptr) from->addEffect(effect);    
                }
            }

        }
    }
}