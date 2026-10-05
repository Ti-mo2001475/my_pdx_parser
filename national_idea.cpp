#include "national_idea.h"
#include "localization.h"
#include "utils/parser_util.h"
#include "paradox_type.h"
#include <map>
#include <functional>

std::map<std::string,const NationalIdea*> nationalIdeas;
std::map<std::string,const NationalIdea*> tagIdeas;


void loadNationalIdea(std::string rootPath){
    std::string filePath = rootPath;
    filePath.append("/datas/country_ideas.txt");
    ParadoxTag* tag = parseFile(filePath);
    for(int i = 0;i < tag->size();i++){
        auto[key, childNode] = (*tag)[i];
        ParadoxTag* ideaTag = childNode->getAsTag();
        if(ideaTag->get("free",1) == nullptr) continue;
        NationalIdea* idea = new NationalIdea();
        idea->start = std::make_unique<Modifier>();
        idea->bonus = std::make_unique<Modifier>();
        for(int k = 0;k < 7;k++){
            idea->modifiers[k] = std::make_unique<Modifier>();
        }
        idea->trigger = nullptr;
        int slot = 0;
        for(int j = 0;j < ideaTag->size();j++){
            auto[entry, entryNode] = (*ideaTag)[j];
            if(entry == "start"){
                idea->start->name = getStringPtr("传统");
                ParseModifier(ideaTag->getAsTag(entry),*(idea->start.get()));
            }
            else if(entry == "bonus"){
                idea->bonus->name = getStringPtr("野心");
                ParseModifier(ideaTag->getAsTag(entry),*(idea->bonus.get()));
            }
            else if(entry == "free") continue;
            else if(entry == "trigger"){
                idea->trigger = std::unique_ptr<Trigger>((Trigger*)createBaseTrigger());
                parseTrigger(ideaTag->getAsTag(entry),idea->trigger->getAsComplexTrigger());
                idea->trigger->takeOverLifeCycle();
            }
            else{
                idea->modifiers[slot]->name = getStringPtr(entry);
                ParseModifier(ideaTag->getAsTag(entry),*(idea->modifiers[slot]));
                slot++;
            }
        }
        nationalIdeas[key] = idea;
        if(idea->trigger != nullptr){
            idea->trigger->foreach([&idea](Trigger* trigger){
                CommonTrigger* commonTrigger = trigger->getAsCommonTrigger();
                if(commonTrigger == nullptr) return true;
                //std::cout << commonTrigger->item->name << std::endl;
                if(commonTrigger->item->name != "tag") return true;
                std::string tagName = commonTrigger->base[0]->getAsScope()->getValue()->getAsCountryScope()->getTag();

                if(tagIdeas.find(tagName) == tagIdeas.end()) tagIdeas[tagName] = idea;
                return true;
            });
        }
    }
    clearParserDatas();
}

void reloadNationalIdeas(std::string rootPath){
    for(std::pair<std::string, const NationalIdea*> p : nationalIdeas){
        delete p.second;
    }
    nationalIdeas.clear();
    tagIdeas.clear();
    loadNationalIdea(rootPath);
}

std::string NationalIdea::toString() const{
    std::string ret("");
    if(this->trigger != nullptr){
        ret.append("启用条件:\n");
        ret.append(this->trigger->toString(false));
    }
    ret.append(this->start->localize());
    for(int i = 0;i < 7;i++){
        ret.append(this->modifiers[i]->localize());
    }
    ret.append(this->bonus->localize());
    return ret;
}
std::string NationalIdea::toHtml() const{
    std::string ret("");
    if(this->trigger != nullptr){
        ret.append("启用条件:\n");
        ret.append(this->trigger->toHtml(false));
    }
    ret.append(this->start->localizeHtml());
    for(int i = 0;i < 7;i++){
        ret.append(this->modifiers[i]->localizeHtml());
    }
    ret.append(this->bonus->localizeHtml());
    return ret;
}
const NationalIdea* getFromName(std::string str){
    if(nationalIdeas.find(str) == nationalIdeas.end()) return nullptr;
    return nationalIdeas[str];
}
const NationalIdea* getTagIdea(std::string name){
    if(tagIdeas.find(name) == tagIdeas.end()) return nullptr;
    return tagIdeas[name];
}
