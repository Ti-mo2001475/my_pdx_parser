#include "paradox_type.h"
#include "utils/string_util.h"
#include "scope.h"
#include <iostream>
#include <algorithm>

extern ParadoxTag* createTag();
extern ParadoxArray* createArray();
extern ParadoxString* createString(std::string);
extern ParadoxInteger* createInteger(long long);
extern ParadoxDate* createDate(Date);
extern ParadoxScope* createScope(Scope*);

std::ofstream fout("./log.txt");

std::ofstream& logStream(){
	return fout;
}

ParadoxBoolean* getBooleanInstance(bool value){
	static ParadoxBoolean BOOLEAN_YES(true);
	static ParadoxBoolean BOOLEAN_NO(false);
	return (value ? &BOOLEAN_YES : &BOOLEAN_NO);
}

ParadoxString* ParadoxBase::getAsString(){
	if(getType() != ParadoxType::STRING) return nullptr;
	return static_cast<ParadoxString*>(this); 
}
int ParadoxTag::size(){
	return contents.size();
}
ParadoxInteger* ParadoxBase::getAsInteger(){
	if(getType() != ParadoxType::INTEGER) return nullptr;
	return static_cast<ParadoxInteger*>(this); 
}
ParadoxTag* ParadoxBase::getAsTag(){
	if(getType() != ParadoxType::TAG) return nullptr;
	return static_cast<ParadoxTag*>(this); 
}
ParadoxArray* ParadoxBase::getAsArray(){
	if(getType() != ParadoxType::ARRAY) return nullptr;
	return static_cast<ParadoxArray*>(this); 
}
ParadoxDate* ParadoxBase::getAsDate(){
	if(getType() != ParadoxType::DATE) return nullptr;
	return static_cast<ParadoxDate*>(this); 
}
ParadoxBoolean* ParadoxBase::getAsBoolean(){
	if(getType() != ParadoxType::BOOLEAN) return nullptr;
	return static_cast<ParadoxBoolean*>(this);
}
ParadoxScope* ParadoxBase::getAsScope(){
	if(getType() != ParadoxType::SCOPE) return nullptr;
	return static_cast<ParadoxScope*>(this);
}

ParadoxBase* ParadoxTag::get(std::string name,int index){
	if(index < 1) index = 1;
	int count = 0;
	for(auto& [key, child] : contents){
		if(key == name){
			if(++count == index) return child;
		}
	}
	return nullptr;
}
ParadoxBase* ParadoxTag::get(int index){
	if(index < 0 || index >= (int)contents.size()) return nullptr;
	return contents[index].second;
}
std::pair<std::string,ParadoxBase*> ParadoxTag::getEntry(int i){
	if(i < 0 || i >= (int)contents.size()) return {"",nullptr};
	return contents[i];
}
std::pair<std::string,ParadoxBase*> ParadoxTag::operator[](int i){
	return contents[i];
}
ParadoxTag* ParadoxTag::getAsTag(std::string name,int index){
	ParadoxBase* tag = get(name,index);
	return tag == nullptr ? nullptr : tag->getAsTag();
}
ParadoxTag* ParadoxTag::getAsTag(int index){
	ParadoxBase* tag = get(index);
	return tag == nullptr ? nullptr : tag->getAsTag();
}
ParadoxTag* ParadoxTag::getAsTag(std::string name){
	ParadoxBase* tag = get(name,1);
	return tag == nullptr ? nullptr : tag->getAsTag();
}
void ParadoxTag::add(std::string name,ParadoxBase* base){
	contents.push_back({name,base});
}
void ParadoxTag::remove(std::string name,int index){
	if(index < 1) index = 1;
	for(auto it = contents.begin();it != contents.end();it++){
		if(it->first == name){
			if(--index == 0){
				contents.erase(it);
				return;
			}
		}
	}
}
std::string Date::toString() const {
	std::string str = "";
	str.append(std::to_string((int)this->year));
	str.append(".");
	str.append(std::to_string((int)this->month));
	str.append(".");
	str.append(std::to_string((int)this->day));
	return str;
}

std::string ParadoxScope::toString() const { return scope->toString(); }

bool Xor(bool a,bool b){
	return (a || b) && !(a && b);
} 

bool isCastable(ParadoxBase* base,ParadoxType type){
	if(base->getType() == type){
		return true;
	}
	else if(type == ParadoxType::SCOPE){
		if(base->getType() == ParadoxType::INTEGER){
			return base->getAsInteger()->getIntegerContent() <= 8000 * 1000L;
		}
		if(base->getType() == ParadoxType::STRING){
			return true;
		}
	}
	return false;
}
//!WARNING!
//the returned pointer is created by 'new' operator and the caller has the responsibility to manage memory
//this function is used to create a ParadoxBase Object which do not managed by global object manager.
ParadoxBase* deep_copy(ParadoxBase* base){
	if(base == nullptr) return nullptr;
	if(base->getType() == ParadoxType::INTEGER){
		ParadoxInteger* pInt = base->getAsInteger();
		return new ParadoxInteger(pInt->getIntegerContent());
	}
	if(base->getType() == ParadoxType::STRING){
		ParadoxString* pStr = base->getAsString();
		return new ParadoxString(pStr->getStringContent());
	}
	if(base->getType() == ParadoxType::DATE){
		ParadoxDate* pDate = base->getAsDate();
		return new ParadoxDate(pDate->getDateContent());
	}
	if(base->getType() == ParadoxType::ARRAY){
		ParadoxArray* pArray = base->getAsArray();
		ParadoxArray* nArray = new ParadoxArray();
		for(ParadoxBase* base:pArray->contents){
			ParadoxBase* base_copy = deep_copy(base);
			nArray->append(base_copy);
		}
		return nArray;
	}
	if(base->getType() == ParadoxType::BOOLEAN){
		ParadoxBoolean* pBoolean = base->getAsBoolean();
		ParadoxBoolean* nBoolean = new ParadoxBoolean(pBoolean->getValue());
		return nBoolean;
	}
	if(base->getType() == ParadoxType::SCOPE){
		ParadoxScope* pScope = base->getAsScope();
		ParadoxScope* nScope = new ParadoxScope(pScope->getValue());
		return nScope;
	}
	else {
		ParadoxTag* pTag = base->getAsTag();
		ParadoxTag* nTag = new ParadoxTag();
		for(int i = 0;i < pTag->size();i++){
			auto[key, childNode] = (*pTag)[i];
			ParadoxBase* base_copy = deep_copy(childNode);
			nTag->add(key,base_copy);
		}
		return nTag;
	}
}
//this function is used to deep copy a ParadoxBase Object safely.
ParadoxBase* deep_copy_safe(ParadoxBase* base){
	if(base == nullptr) return nullptr;
	if(base->getType() == ParadoxType::INTEGER){
		ParadoxInteger* pInt = base->getAsInteger();
		return createInteger(pInt->getIntegerContent());
	}
	if(base->getType() == ParadoxType::STRING){
		ParadoxString* pStr = base->getAsString();
		return createString(pStr->getStringContent());
	}
	if(base->getType() == ParadoxType::DATE){
		ParadoxDate* pDate = base->getAsDate();
		return createDate(pDate->getDateContent());
	}
	if(base->getType() == ParadoxType::ARRAY){
		ParadoxArray* pArray = base->getAsArray();
		ParadoxArray* nArray = createArray();
		for(ParadoxBase* base:pArray->contents){
			ParadoxBase* base_copy = deep_copy_safe(base);
			nArray->append(base_copy);
		}
		return nArray;
	}
	if(base->getType() == ParadoxType::BOOLEAN){
		ParadoxBoolean* pBoolean = base->getAsBoolean();
		ParadoxBoolean* nBoolean = new ParadoxBoolean(pBoolean->getValue());
		extern std::vector<ParadoxBase*> parsedObject;
		parsedObject.push_back(nBoolean);
		return nBoolean;
	}
	if(base->getType() == ParadoxType::SCOPE){
		ParadoxScope* pScope = base->getAsScope();
		ParadoxScope* nScope = createScope(pScope->getValue());
		return nScope;
	}	
	else {
		ParadoxTag* pTag = base->getAsTag();
		ParadoxTag* nTag = createTag();
		for(int i = 0;i < pTag->size();i++){
			auto[key, childNode] = (*pTag)[i];
			ParadoxBase* base_copy = deep_copy_safe(childNode);
			nTag->add(key,base_copy);
		}
	
		return nTag;
	}

}
template<ParadoxObject T>
ParadoxBase* castTo(T base,ParadoxType type){
	if(base == nullptr) return nullptr;
	ParadoxBase* aBase = base;
	if(aBase->getType() == type) return base;
	else return nullptr;
}

template<>
ParadoxBase* castTo(ParadoxBase* base,ParadoxType type){
	if(base == nullptr) return nullptr;
	ParadoxType baseType = base->getType();
	if(baseType == ParadoxType::INTEGER){
		return castTo(static_cast<ParadoxInteger*>(base),type);
	}
	if(baseType == ParadoxType::STRING){
		return castTo(static_cast<ParadoxString*>(base),type);
	}
	if(baseType == type) return base;
	else return nullptr;
}

template<>
ParadoxBase* castTo(ParadoxString* string,ParadoxType type){
	if(string == nullptr) return nullptr;
	if(type == ParadoxType::STRING) return string;
	if(type == ParadoxType::SCOPE){
		Scope* scope = createScopeFromString(string->getStringContent());
		if(scope == nullptr) return nullptr;
		return createScope(scope);
	}
	return nullptr;
}
//
template<>
ParadoxBase* castTo(ParadoxInteger* number, ParadoxType type){
	if(number == nullptr) return nullptr;
	if(type == ParadoxType::INTEGER) return number;
	if(type == ParadoxType::SCOPE) {
		Scope* scope = getProvinceScope(number->getIntegerContent());
		if(scope == nullptr) return nullptr;
		return new ParadoxScope(scope);
	}
	return nullptr;
}

