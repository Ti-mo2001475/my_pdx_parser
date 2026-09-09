#ifndef PARADOX_TYPE
#define PARADOX_TYPE 2


#include<map>
#include<string>
#include<vector>
#include<stdio.h>
#include<cstdint>
#include<iostream>
#include<fstream>
#include<functional>
#include<concepts>
#include<source_location>

#include "pattern.h"
consteval std::source_location current_location(std::source_location current = std::source_location::current()){
    return current;
}

enum class ParadoxType : uint8_t{
	BASE = 255,
	STRING = 0,
	INTEGER = 1,
	TAG = 2,
	ARRAY = 3,
	DATE = 4,
	BOOLEAN = 5,
	SCOPE = 129,
	GOOD = 130
};

struct Date{
	short year;
	unsigned char month;
	unsigned char day;
	std::string toString() const;
};

struct Scope;
struct ParadoxBase;
struct ParadoxString;
struct ParadoxInteger;
struct ParadoxTag;
struct ParadoxArray;
struct ParadoxDate;
struct ParadoxBoolean;
struct ParadoxScope;

struct ParadoxBase{
	virtual void* getContent() = 0;
	virtual ParadoxType getType() const = 0;
	virtual std::string toString() const = 0;
	virtual void traverse(std::function<void(ParadoxBase*)> visitor) = 0;
	ParadoxString* getAsString();
	ParadoxInteger* getAsInteger();
	ParadoxTag* getAsTag();
	ParadoxArray* getAsArray();
	ParadoxDate* getAsDate();
	ParadoxBoolean* getAsBoolean();
	ParadoxScope* getAsScope();
	virtual ~ParadoxBase() noexcept = default;
};
template<typename T>
concept ParadoxObject = std::convertible_to<T,ParadoxBase*>;
struct ParadoxString : public ParadoxBase{
	private:
		std::string content;
	public:
	virtual std::string toString() const { return content; }
	ParadoxString(std::string str){ 
		content = str;
	}
	virtual void* getContent(){
		return (void*)&content;
	}
	virtual ParadoxType getType() const{
		return ParadoxType::STRING;
	}
	virtual void traverse(std::function<void(ParadoxBase*)> visitor) override{
		visitor(this);
	}
	std::string getStringContent() const{
		return content;
	}
};
struct ParadoxInteger : public ParadoxBase{
	private:
		long long content;
	public:
		virtual std::string toString() const { 
			if(content % 1000 == 0) return std::to_string(content / 1000);
			else {
				Pattern p("%d");
				p.setNextInteger(content);
				return p.getOutput();
			}
		}
		ParadoxInteger(long long val){
			content = val;
		}
		virtual void* getContent(){
			return (void*)&content;
		}
		virtual ParadoxType getType() const{
			return ParadoxType::INTEGER;
		}
		virtual void traverse(std::function<void(ParadoxBase*)> visitor) override{
			visitor(this);
		}
		long long getIntegerContent(){
			return content;
		}
};

struct ParadoxTag : public ParadoxBase{
	public:
		std::vector<std::pair<std::string,ParadoxBase*>> contents;
		
	public:
		virtual std::string toString() const { return "[TAG]"; }
		virtual void* getContent(){
			return (void*)&contents;
		}
		virtual ParadoxType getType() const {
			return ParadoxType::TAG;
		}
		ParadoxBase* get(std::string name,int index = 1);
		ParadoxBase* get(int index);
		std::pair<std::string,ParadoxBase*> getEntry(int i);
		std::pair<std::string,ParadoxBase*> operator[](int i);
		ParadoxTag* getAsTag(std::string name);
		ParadoxTag* getAsTag(std::string name,int index);
		ParadoxTag* getAsTag(int index);
		int size();
		void add(std::string name,ParadoxBase* base);
		void remove(std::string name,int index);
		virtual void traverse(std::function<void(ParadoxBase*)> visitor) override{
			visitor(this);
			for(auto& [key, child] : contents){
				if(child) child->traverse(visitor);
			}
		}
};

struct ParadoxArray : public ParadoxBase{
	std::vector<ParadoxBase*> contents;
	virtual std::string toString() const { return "[ARRAY]"; }
	virtual ParadoxType getType() const{
		return ParadoxType::ARRAY;
	}
	virtual void* getContent(){
		return (void*)&contents;
	}
	ParadoxType getContentType() const{
		if(contents.empty()) return ParadoxType::BASE;
		else return contents[0]->getType();
	}
	ParadoxBase* get(int index) const{
		if(index < 0 || contents.size() <= (size_t)index) return nullptr;
		else return contents[(size_t)index];
	}
	bool append(ParadoxBase* base){
		if(contents.empty()){
			contents.push_back(base);
			return true;
		}
		else if(contents[0]->getType() == base->getType()){
			contents.push_back(base);
			return true;
		}
		return false;
	}
	virtual void traverse(std::function<void(ParadoxBase*)> visitor) override{
		visitor(this);
		for(ParadoxBase* child : contents){
			if(child) child->traverse(visitor);
		}
	}
};

struct ParadoxDate : public ParadoxBase{
	Date date;
	virtual std::string toString() const { return date.toString(); }
	ParadoxDate(std::string dateString){
		sscanf(dateString.c_str(),"%hd%*c%hhu%*c%hhu",&(date.year),&(date.month),&(date.day));
	}
	ParadoxDate(Date date){
		this->date = date;
	}
	virtual ParadoxType getType() const{
		return ParadoxType::DATE;
	}
	virtual void traverse(std::function<void(ParadoxBase*)> visitor) override{
		visitor(this);
	}
	virtual void* getContent(){
		return (void*)&date;
	}
	Date getDateContent(){
		return date;
	}
};

struct ParadoxBoolean : public ParadoxBase {
	private:
		bool value;
	public:
		virtual std::string toString() const { return value ? "yes" : "no"; }
		constexpr ParadoxBoolean(bool boolean) : value(boolean){}
		virtual void* getContent(){
			return (void*)&value;
		}
		bool getValue() const{
			return value;
		}
		virtual ParadoxType getType() const{
			return ParadoxType::BOOLEAN;
		}
		virtual void traverse(std::function<void(ParadoxBase*)> visitor) override{
			visitor(this);
		}
};
struct ParadoxScope : public ParadoxBase {
	private:
		Scope* scope;
	public:
		virtual std::string toString() const;
		ParadoxScope(Scope* scope) : scope(scope) {}
		virtual void* getContent(){
			return scope;
		}
		Scope* getValue() {
			return scope;
		}
		virtual ParadoxType getType() const {
			return ParadoxType::SCOPE;
		}
		virtual void traverse(std::function<void(ParadoxBase*)> visitor) override{
			visitor(this);
		}
};
ParadoxBoolean* getBooleanInstance(bool value);
bool isCastable(ParadoxBase* base,ParadoxType type);
template<ParadoxObject From>
ParadoxBase* castTo(From base,ParadoxType type);
template<>
ParadoxBase* castTo(ParadoxBase* base,ParadoxType type);
template<>
ParadoxBase* castTo(ParadoxInteger* base,ParadoxType type);
template<>
ParadoxBase* castTo(ParadoxString* base,ParadoxType type);
bool Xor(bool a,bool b);
ParadoxBase* deep_copy(ParadoxBase*);
template<typename T>
consteval ParadoxType getParadoxType(){
	return ParadoxType::BASE;
}
template<>
consteval ParadoxType getParadoxType<long long>(){
	return ParadoxType::INTEGER;
}
template<>
consteval ParadoxType getParadoxType<std::string>(){
	return ParadoxType::STRING;
}
template<>
consteval ParadoxType getParadoxType<Date>(){
	return ParadoxType::DATE;
}
template<>
consteval ParadoxType getParadoxType<bool>(){
	return ParadoxType::BOOLEAN;
}
template<>
consteval ParadoxType getParadoxType<Scope*>(){
	return ParadoxType::SCOPE;
}
template<ParadoxType enumType>
struct ParadoxTypeMap{
	using rawType = void*;
	using pdxType = ParadoxBase;
};
template<>
struct ParadoxTypeMap<ParadoxType::DATE>{
	using rawType = Date;
	using pdxType = ParadoxDate;
};
template<>
struct ParadoxTypeMap<ParadoxType::INTEGER>{
	using rawType = long long;
	using pdxType = ParadoxInteger;
};
template<>
struct ParadoxTypeMap<ParadoxType::STRING>{
	using rawType = std::string;
	using pdxType = ParadoxString;
};

template<>
struct ParadoxTypeMap<ParadoxType::SCOPE>{
	using rawType = Scope*;
	using pdxType = ParadoxScope;
};
template<>
struct ParadoxTypeMap<ParadoxType::BOOLEAN>{
	using rawType = bool;
	using pdxType = ParadoxBoolean;
};
template<ParadoxType type>
using rawType = typename ParadoxTypeMap<type>::rawType;
template<ParadoxType type>
using pdxType = typename ParadoxTypeMap<type>::pdxType;


template<typename T>
struct ParadoxTypeMap2{
	using pdxType = ParadoxBase;
};
template<>
struct ParadoxTypeMap2<long long>{
	using pdxType = ParadoxInteger;
};
template<>
struct ParadoxTypeMap2<std::string>{
	using pdxType = ParadoxString;
};
template<>
struct ParadoxTypeMap2<Date>{
	using pdxType = ParadoxDate;
};
template<>
struct ParadoxTypeMap2<bool>{
	using pdxType = ParadoxBoolean;
};
template<>
struct ParadoxTypeMap2<Scope*>{
	using pdxType = ParadoxScope;
};

template<typename type>
using pdxTypeFromRaw = typename ParadoxTypeMap2<type>::pdxType;


std::ofstream& logStream();

template<typename... types>
void log_error(const std::source_location location,types... args){
	std::ostream& stream = logStream();
	stream << "#[ERROR][" << location.file_name() << ":" << location.line() << "][" <<location.function_name() << "]: ";
	(stream << ... << args) << std::endl;
}
template<typename... types>
void log_warning(const std::source_location location,types... args){
	std::ostream& stream = logStream();
	stream << "#[WARNING][" << location.file_name() << ":" << location.line() << "][" <<location.function_name() << "]: ";
	(stream << ... << args) << std::endl;
}
template<typename... types>
void log_info(const std::source_location location,types... args){
	std::ostream& stream = logStream();
	stream << "#[INFO][" << location.file_name() << ":" << location.line() << "][" <<location.function_name() << "]: ";
	(stream << ... << args) << std::endl;
}
#endif
