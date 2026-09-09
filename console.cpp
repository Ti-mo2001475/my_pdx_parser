#include "modifier.h"
#include "trigger.h"
#include "db_object.h"
#include "localization.h"
#include "utils/parser_util.h"
#include "national_idea.h"
#include "effect.h"
#include "paradox_macro.h"
#include "pattern.h"
#include "map_data.h"
#include <iostream>
#include <map>
#include <bitset>
#include <sstream>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <charconv>

using json = nlohmann::json;
//
typedef void(*CommandHandler)(std::vector<std::string>);

std::map<std::string,CommandHandler> handlers;
extern std::set<std::string> shortStringSet;
extern std::set<std::string> registeredTriggers;
extern std::map<std::string,ScriptedTrigger*> loadedSTs;
extern std::map<const std::string*,ScriptedEffect*> loadedSEs;
extern std::map<const std::string*,ProvinceGroup> provinceGroups;

std::vector<ProvinceGroup> tempGroups;
void printModifier(std::vector<std::string> vec){
    ParadoxTag* root = parseFile(vec[0]);
	std::vector<Modifier> modifiers;
	ParseModifier(root,modifiers);
	for(int i = 0;i < modifiers.size();i++){
		std::cout << modifiers[i].localize();
		std::cout << std::endl; 
		std::cout << std::endl; 
	}	
    clearParserDatas();
}
void printModifierHtml(std::vector<std::string> vec){
    ParadoxTag* root = parseFile(vec[0]);
	std::vector<Modifier> modifiers;
	ParseModifier(root,modifiers);
	for(int i = 0;i < modifiers.size();i++){
		std::cout << modifiers[i].localizeHtml();
		std::cout << std::endl; 
		std::cout << std::endl; 
	}	
    clearParserDatas();
}
void printEffect(std::vector<std::string> vec){
	ParadoxTag* root = parseFile(vec[0]);
 	std::unique_ptr<ComplexEffect> effect = createBaseEffect();
	parseEffect(root,effect.get());
	clearParserDatas();
	std::cout << effect->toString() << std::endl;
}

void printTrigger(std::vector<std::string> vec){
    ParadoxTag* root = parseFile(vec[0]);
	if(root == nullptr) return;
	ComplexTrigger* ct = createBaseTrigger();
	parseTrigger(root,ct);
	std::cout << ct->toString(false) << std::endl;
	delete ct;
    clearParserDatas();
}
void runBench(std::vector<std::string> vec){
		auto start = std::chrono::system_clock::now();
		ParadoxTag* root = parseFile("./bench.txt");
		auto end = std::chrono::system_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::cout << "====== node parse phase ======" << std::endl;
		std::cout << "parsed node count: " << root->getAsTag(0)->size() + root->getAsTag(1)->size() + 2 << std::endl;
		std::cout << "time consumed: " << duration.count() << " microseconds" << std::endl;
		std::cout << "time per node: " << duration.count() / (1.0 * root->getAsTag(0)->size() + 1) << " mircoseconds" << std::endl;
		start = std::chrono::system_clock::now();
		ComplexTrigger* ct = createBaseTrigger();
		parseTrigger(root->getAsTag(0),ct);
		end = std::chrono::system_clock::now();
		duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::cout << "====== trigger create phase ======" << std::endl;
		std::cout << "parsed node count: " << root->getAsTag(0)->size() + 1 << std::endl;
		std::cout << "time consumed: " << duration.count() << " microseconds" << std::endl;
		std::cout << "time per node: " << duration.count() / (1.0 * root->getAsTag(0)->size() + 1) << " mircoseconds" << std::endl;
		start = std::chrono::system_clock::now();
		Modifier modifiers;
		ParseModifier(root->getAsTag(1),modifiers);
		end = std::chrono::system_clock::now();
		duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		
		std::cout << "====== modifier create phase ======" << std::endl;
		std::cout << "parsed node count: " << root->getAsTag(0)->size() + 1 << std::endl;
		std::cout << "time consumed: " << duration.count() << " microseconds" << std::endl;
		std::cout << "time per node: " << duration.count() / (1.0 * root->getAsTag(0)->size() + 1) << " mircoseconds" << std::endl;
		
		clearParserDatas();
		delete ct;
}
int main(){
    using namespace std;
	auto start = std::chrono::system_clock::now();
	registerInternalScopes();
    std::thread& th = readLocalizations();
    loadInternalModifier();
    registerGood();
    log_info(current_location(),"Modifier Loaded!");
	registerTriggerItems();
	log_info(current_location(),"Trigger Loaded!");
	loadScriptedTrigger();
	log_info(current_location(),"Scripted Trigger Phase 1 Loaded!");
	registerEffectItems();
	log_info(current_location(),"Effect Loaded!");
	loadScriptedEffect();
	log_info(current_location(),"Scripted Effect Loaded!");
	loadNationalIdea();
	log_info(current_location(),"Ni Loaded!");
	loadMapDatas();
	log_info(current_location(),"Map Data Loaded!");
	th.join();
	loadScriptedTrigger_POST();
	log_info(current_location(),"Scripted Trigger Phase 2 Loaded!");
	auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	log_info(current_location(),"Load Completed! ",duration.count(), " us consumed.");
	std::cout << "#load Completed!" << std::endl;
	handlers["credits"] = [](std::vector<std::string> vec){
		std::cout << "Paradox Data Parser \nV0.4.2-20260805\nAuthor: Mordd";
	};
	handlers["future_plan"] = [](std::vector<std::string> vec){
		std::cout << registeredTriggers.size() - loadedSTs.size() << " / 898 Internal Triggers" << std::endl;
		std::cout << "TODO Event Parser" << std::endl;
	};
	
    handlers["print_modifier"] = printModifier;
	handlers["print_modifier_html"] = printModifierHtml;
	handlers["debug_print"] = [](std::vector<std::string> vec){
		if(vec.size() != 0){
			if(vec[0] == "named_pattern_test") {
				NamedPattern np("Hello,%{name}!");
				np.fillName("name","mordd");
				std::cout << np.getOutput() << std::endl;
				np = NamedPattern("The IQ of Khet is %{IQ}");
				np.fillName("IQ",40000);
				std::cout << np.getOutput() << std::endl;
				np = NamedPattern("Oops,The IQ of Khet is %{IQ:percent} now!");
				np.fillName("IQ",399);
				std::cout << np.getOutput() << std::endl;
				np = NamedPattern("What Khet will do:\n%{KhetAction:effect_literal}");
				np.fillName("KhetAction","add_treasury = -300");
				std::cout << np.getOutput();
				np = NamedPattern("The Trigger of Khet being dumb:\n%{KhetCondition:trigger_literal}");
				np.fillName("KhetCondition","always = no");
				std::cout << np.getOutput();
				np = NamedPattern("Scope Test:%{lorent:scope},%{far_island:scope}");
				np.fillName("lorent","A01");
				np.fillName("far_island",1);	
				std::cout << np.getOutput();
			}
			else if(vec[0] == "se_test"){
				std::cout << loadedSEs.size() << std::endl;
			}
		}
		else {
			std::cout << "please enter debug option" << std::endl;
		}
		
	};

    handlers["print_trigger"] = printTrigger;
	handlers["print_effect"] = printEffect;
	handlers["run_bench"] = runBench;
    handlers["trade_good"] = [](std::vector<std::string> vec){
        if(vec.empty()){
            std::cout << "用法:trade_good <good_id>" << std::endl;
            return;
        }
        Good* good = getGood(vec[0]);
        if(good == nullptr){
            std::cout << "没有名为" << vec[0] << "的商品" << std::endl;
            return;
        }
        std::cout << getLocalization(*good->localizedNamePtr) << std::endl;
        good->provinceModifier->localize();
        std::cout << "基础价格:" << good->defaultPrice / 1000.0 << std::endl;
        std::cout << good->globalModifier->localize(); 
        std::cout << good->provinceModifier->localize();
    };
	handlers["export_group_with_color"] = [](std::vector<std::string> vec){
		if(vec.size() < 2) return;
		std::string path = "./";
		path.append(vec[0]);
		replaceWith(path,"/../","/./");
		std::cout << "Input Start X Y:";
		int x,y;
		std::cin >> x >> y;
		std::cout << "Input Width Height";
		int w,h;
		std::cin >> w >> h;
		json j;
		j["x"] = x;
		j["y"] = y;
		j["w"] = w;
		j["h"] = h;
		int num = 0;
		std::from_chars(vec[1].data(),vec[1].data()+vec[1].size(),num);
		for(int i = 0;i < num;i++){
			std::string group_name;
			std::string shaded;
			std::string colorString;
			uint32_t color = 0;
			std::cin >> group_name >> colorString >> shaded;
			auto ptr = getStringPtr(group_name);
			auto it = provinceGroups.find(ptr);
 			ProvinceGroup g;
			if(it == provinceGroups.end()) {
				auto it2 = std::find_if(tempGroups.begin(),tempGroups.end(),[ptr](ProvinceGroup g){return g.localize_key == ptr;});
				if(it2 == tempGroups.end()) return;
				g = *it2;
			}
			else g = it->second;
			std::from_chars(colorString.data(),colorString.data()+colorString.size(),color,16);
			bool shade = shaded == "y";
			j[group_name]["color"] = color;
			j[group_name]["shaded"] = shade;
			j[group_name]["provinces"] = g.provinces;
 		}
		std::ofstream fout(path);
		fout << j;
	};

	handlers["create_temp_group"] = [](std::vector<std::string> vec){
		if(vec.size() != 1) return;
		auto ptr = getStringPtr(vec[0]);
		ProvinceGroup g;
		g.localize_key = ptr;
		int id;
		std::cin >> id;
		while(id != -1){
			g.provinces.push_back(id);
			std::cin >> id;
		}
		tempGroups.push_back(std::move(g));
	};
	handlers["goods_list"] = [](std::vector<std::string> vec){
		std::map<std::string,std::string> map1;
		listGoods(map1);
		for(auto it : map1){
			std::cout << it.first << " " << it.second << std::endl;
		}
	};
	handlers["tag_idea"] = [](std::vector<std::string> vec){
		if(vec.empty()){
			std::cout << "用法:tag_idea <tag>" << std::endl;
            return;
		}
		const NationalIdea* idea = getTagIdea(vec[0]);
		if(idea == nullptr){
			std::cout << "没有找到" << vec[0] << "的国家理念" << std::endl;
			return;
		}
		std::cout << idea->toString() << std::endl;
	};
	handlers["ideas"] = [](std::vector<std::string> vec){
		if(vec.empty()){
			std::cout << "用法:ideas <tag>" << std::endl;
            return;
		}
		const NationalIdea* idea = getFromName(vec[0]);
		if(idea == nullptr){
			std::cout << "没有找到" << vec[0] << "的国家理念" << std::endl;
			return;
		}
		std::cout << idea->toString() << std::endl;
	};
	handlers["tag_idea_text"] = [](std::vector<std::string> vec){
				if(vec.empty()){
			std::cout << "用法:tag_idea_text <tag>" << std::endl;
            return;
		}
		const NationalIdea* idea = getTagIdea(vec[0]);
		if(idea == nullptr){
			std::cout << "没有找到" << vec[0] << "的国家理念" << std::endl;
			return;
		}
		for(int i = 0;i < 7;i++){
			std::cout << getLocalization(*idea->modifiers[i]->name) << std::endl;
			std::cout << getLocalization(*idea->modifiers[i]->name + "_desc") << std::endl;		
			if(i != 6) std::cout << "=========" << std::endl;	
		}
	};
	handlers["reload_loc"] = [](std::vector<std::string> vec){
		readLocalizations();
	};
	
	handlers["extract_mission"] = [](std::vector<std::string> vec){
		if(vec.empty()){
			std::cout << "usage: extract_mission <mission_file_name>";
			return;
		}
		ParadoxTag* root = parseFile(vec[0]);
		for(int i = 0;i < root->size();i++){
			auto[key, childNode] = (*root)[i];
			ParadoxTag* tag = childNode->getAsTag();
			if(tag == nullptr) continue;
			for(int j = 0;j < tag->size();j++){
				auto[key1, childNode1] = (*tag)[j];
				if(childNode1->getType() != ParadoxType::TAG) continue;
				std::cout << key1 << std::endl;
			}
		}
	};
	
	handlers["read_head"] = [](std::vector<std::string> vec){
		if(vec.empty()){
			std::cout << "usage: extract_mission <mission_file_name>";
			return;
		}
		ParadoxTag* root = parseFile(vec[0]);
		for(int i = 0;i < root->size();i++){
			std::cout << (*root)[i].first << std::endl;
		}
	};
	handlers["get_province_id"] = [](std::vector<std::string> vec){
		if(vec.size() == 0) return;
		for(auto str : vec){
			Scope* scope = findScopeByName(str,ScopeType::PROVINCE);
			if(scope == nullptr) {
				std::cout << "NOT FOUND!" << ' ';
				continue;
			}
			ProvinceScope* p = scope->getAsProvinceScope();
			if(p == nullptr){
				std::cout << "NOT FOUND!" << ' ';
				continue;
			} 
			std::cout << p->getId() << ' ';
		}
	};
	/*
	handlers["freespace"] = [](std::vector<std::string> vec){
		std::bitset<2600> tag1;
		for(int i = 0;i < 2600;i++) tag1.set(i,true);
		ParadoxTag* tag = parseFile("./datas/tags.txt")->getAsTag("tags");
		for(std::string str :tag->seq){
			
			if(str[1] >= '0' && str[1] <= '9' && str[2] >= '0' && str[2] <= '9'){
				int index = (str[0] - 'A') * 100;
				index += (str[1] - '0') * 10;
				index += (str[2] - '0');

				tag1.set(index,false);
			}
		}
		for(int i = 200;i <= 275;i++) tag1.set(i,false);
		for(int i = 300;i <= 375;i++) tag1.set(i,false);
		for(int i = 400;i <= 450;i++) tag1.set(i,false);
		for(int i = 500;i <= 520;i++) tag1.set(i,false);
		for(int i = 1000;i <= 1099;i++) tag1.set(i,false);
		for(int i = 1400;i <= 1410;i++) tag1.set(i,false);
		for(int i = 1800;i <= 1810;i++) tag1.set(i,false);
		for(int i = 1900;i <= 1975;i++) tag1.set(i,false);
		int start = -1;
		for(int i = 0;i < 26;i++){
			char t = ('A'+ i);
			for(int j = 0;j < 100;j++){
				if(tag1[i * 100 + j] && start == -1){
					start = j;
				}
				else if(start != -1 && !tag1[i * 100 + j]){
					if(start == j - 1) std::cout << t << start << std::endl;
					else {
						std::cout << t << start << "-" << t << j - 1 << std::endl; 
					}
					start = -1;
				}

			}
			if(start != -1){
				if(start == 99) std::cout << t << start << std::endl;
				else {
					std::cout << t << start << "-" << t << 99 << std::endl; 
				}
				start = -1;
			}

		}
	};

	*/
	
    string command,command1;
	vector<string> args;
	while(true){
		
		getline(cin,command);
		if(command.length() == 0) continue;
		stringstream ss;
		ss << command;
		string command1;
		string command2 = "";
		ss >> command >> command1;
		bool shouldMerge = command1[0] == '\"';
		bool broke = false;
		if(command1 == "\x0f\x03\x17") break;
		while(command1.length() != 0){
			if(!shouldMerge){
				args.push_back(command1);
				command1 = "";
				ss >> command1;
			} 
			else {
				ss >> command2;

				if(command1[command1.length() - 1] == '\"') {
					shouldMerge = false;
					command1 = command1.substr(1,command1.length() - 2);
					command2 = "";
					continue;
				}
				command1.append(" ");
				command1.append(command2);
				if(command2.length() == 0) {
					cout << "ERROR:Incomplete Quotation!" << endl; 
					broke = true;
					break;
				}
				
				command2 = "";
			}
			
		}
		if(broke) {
			args.clear();
			cout << endl;	
			continue;
		}	
		if(handlers.find(command) == handlers.end()) cout << "Command Not Found." << endl;
		else handlers[command](args);
		args.clear();
		cout << endl;
	}

}