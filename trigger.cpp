#include<vector>
#include "paradox_type.h"
#include "trigger.h"
#include "pattern.h"
#include "scope.h"
#include "localization.h"
#include "paradox_macro.h"
#include "utils/functional_util.h"
#include<map>
#include<iostream>
#include<set>
//
using OverrideHandler = bool(*)(std::vector<std::pair<std::string,ParadoxBase*>>&);

std::map<std::string,std::string> numberRequiredItems;
std::set<std::string> simpleTriggers;
std::set<std::string> registeredTriggers;
std::map<std::string,OverrideHandler> overrideHandlers;
std::map<std::string,TriggerItem*> items;

extern std::map<std::string,ScriptedTrigger*> loadedSTs;

bool parseConditionalTrigger(ParadoxTag*,ConditionalTrigger*);

extern ParadoxString* createString(std::string str);

const ParadoxType INTEGER_MATCH_SEQUENCE[] = {ParadoxType::INTEGER,ParadoxType::SCOPE};
const ParadoxType STRING_MATCH_SEQUENCE[] = {ParadoxType::SCOPE,ParadoxType::STRING};
//No arg means only yes/no or even only allowing yes.
//those trigger will be registered as 1 BOOL type args.
//and no args will be used in localization text
void registerNoArgTrigger(std::string name,std::string pattern,std::string reversePattern,ScopeType scopeType = ScopeType::COUNTRY){
	TriggerItem* item = new TriggerItem(registerShortString(name));
	item->pattern = pattern;
	item->reversePattern = reversePattern;
	item->parameterType.push_back(ParadoxType::BOOLEAN);
	item->usable_scope = scopeType;
	items[name] = item;
	simpleTriggers.insert(name);
	registeredTriggers.insert(name);
}
//1 arg type.
//arg 0 will be used in patterns.
void registerSimpleTrigger(std::string name,std::string pattern,std::string reversePattern,ParadoxType type,ScopeType scopeType = ScopeType::COUNTRY){
	
	TriggerItem* item = new TriggerItem(registerShortString(name));
	item->pattern = pattern;
	item->reversePattern = reversePattern;
	item->parameterType.push_back(type);
	item->usedParameter.push_back(0);
	item->usable_scope = scopeType;
	items[name] = item;
	registeredTriggers.insert(name);
	simpleTriggers.insert(name);
}

void registerSimpleClauseTrigger(std::string name,TriggerItem* triggerItem){
	simpleTriggers.insert(registerShortString(name));
	items[name] = triggerItem;
	registeredTriggers.insert(name);
}
void registerNumberRequiredTrigger(std::string name,std::string amountKey,std::string pattern,std::string reversePattern,ScopeType scopeType = ScopeType::COUNTRY){
	TriggerItem* item = new TriggerItem(registerShortString(name));
	item->reversePattern = reversePattern;
	item->pattern = pattern;
	item->parameterType.push_back(ParadoxType::INTEGER);
	item->usedParameter.push_back(0);
	item->usable_scope = scopeType;
	numberRequiredItems[name] = amountKey;
	items[name] = item;
	registeredTriggers.insert(name);
}
void registerBooleanTrigger(std::string name,std::string pattern,std::string reversePattern,ScopeType scopeType = ScopeType::COUNTRY){
	std::string actual_name(name);
	actual_name.append("@");
	actual_name.append(std::to_string(static_cast<int>(ParadoxType::BOOLEAN)));
	TriggerItem* item = new TriggerItem(registerShortString(name));
	item->pattern = pattern;
	item->reversePattern = reversePattern;
	item->usable_scope = scopeType;
	item->parameterType.push_back(ParadoxType::BOOLEAN);
	items[actual_name] = item;
	registeredTriggers.insert(name);

}
void registerSingleArgTrigger(std::string name,std::string pattern,std::string reversePattern,ParadoxType type,ScopeType scopeType = ScopeType::COUNTRY){
	if(type == ParadoxType::BOOLEAN){
		registerBooleanTrigger(name,pattern,reversePattern);
		return;
	}
	
	std::string actual_name(name);
	actual_name.append("@");
	actual_name.append(std::to_string(static_cast<int>(type)));
	TriggerItem* item = new TriggerItem(registerShortString(name));
	item->pattern = pattern;
	item->reversePattern = reversePattern;
	item->parameterType.push_back(type);
	item->usedParameter.push_back(0);
	item->usable_scope = scopeType;
	items[actual_name] = item;
	registeredTriggers.insert(name);
}
void registerClausedTrigger(std::string name,TriggerItem* item,OverrideHandler handler){
	items[name] = item;
	overrideHandlers[name] = handler;
	registeredTriggers.insert(name);
}

//A clause trigger can only be rendered when every parameter needed by the text is present:
//- with an overrideLocalization, the "needed" set is dictated by requiredParameter;
//- without one, every pattern-used parameter (usedParameter) must be filled, since the
//  pattern placeholders cannot be left empty.
static bool missingRenderingParameter(const TriggerItem* item,const std::vector<ParadoxBase*>& base){
	if(item->overrideLocalization){
		for(size_t i = 0;i < item->parameterType.size();i++){
			if(item->requiredParameter.test(i) && (i >= base.size() || base[i] == nullptr)) return true;
		}
	}
	else{
		for(int index : item->usedParameter){
			if(index < 0 || index >= (int)base.size() || base[index] == nullptr) return true;
		}
	}
	return false;
}

void registerTriggerItems(){
	registerNoArgTrigger("ai","是AI","不是AI");
	registerNoArgTrigger("allows_female_emperor","允许女性皇帝","不允许女性皇帝");
	registerNoArgTrigger("always","总是为真","总是为假");
	registerNoArgTrigger("at_war_with_religious_enemy","与宗教敌人处于战争状态","没有与宗教敌人处于战争状态");
	registerNoArgTrigger("can_heir_be_child_of_consort","继承人可能是配偶的孩子","继承人不可能是配偶的孩子");
	registerNoArgTrigger("can_migrate","可以移民","不可以移民");
	registerNoArgTrigger("exist","存在","不存在");
	registerNoArgTrigger("has_active_debate","有正在进行中的辩论","没有正在进行中的辩论");
	registerBooleanTrigger("has_active_fervor","激活了一个热情效果","没有激活热情效果");
	registerSingleArgTrigger("has_active_fervor","拥有已激活的\"%s\"热情效果","没有已激活的\"%s\"热情效果",ParadoxType::STRING);
	registerNoArgTrigger("has_advisor","已经雇佣了一个顾问","尚未雇佣顾问");
	registerNoArgTrigger("has_any_active_estate_agenda","有进行中的阶层议程","没有进行中的阶层议程");
	registerNoArgTrigger("has_any_disaster","当前处于灾难状态","当前没有处于灾难状态");
	registerNoArgTrigger("has_cardinal","拥有在职的真知者","没有在职的真知者");
	registerNoArgTrigger("has_changed_nation","改变过游玩国家","从未改变过游玩国家");
	registerNoArgTrigger("has_colonist","有一个活跃的殖民队","没有活跃的殖民队");
	registerNoArgTrigger("has_commanding_three_star","有正在指挥的三星陆军或海军将领","没有正在指挥的三星陆军或海军将领");
	registerNoArgTrigger("has_consort","有配偶","没有配偶");
	registerNoArgTrigger("has_consort_regency","处于配偶摄政","没有处于配偶摄政");
	registerNoArgTrigger("has_custom_ideas","使用了自定义国家理念","没有使用自定义国家理念");
	registerNoArgTrigger("has_divert_trade","已经向宗主国转移贸易力量","没有向宗主国转移贸易力量");
	registerNoArgTrigger("has_embargo_rivals","已禁运宗主国的宿敌","没有禁运宗主国的宿敌");
	registerNoArgTrigger("has_estate_loans","有阶层贷款","没有阶层贷款");
	registerNoArgTrigger("has_factions","有派系","没有派系");
	registerNoArgTrigger("has_first_revolution_started","世界上已经爆发过革命","世界上没有爆发过革命");
	registerNoArgTrigger("has_female_consort","有女性配偶","没有女性配偶");
	registerNoArgTrigger("has_female_heir","有女性继承人","没有女性继承人");
	registerNoArgTrigger("has_flagship","拥有旗舰","没有拥有旗舰");
	registerNoArgTrigger("has_foreign_consort","有一个外国的配偶","没有一个外国的配偶");
	registerNoArgTrigger("has_foreign_heir","继承人是外国人","继承人不是外国人");
	registerNoArgTrigger("has_friendly_reformation_center","有一个当前宗教的宗教改革中心","没有一个当前宗教的宗教改革中心");
	registerNoArgTrigger("has_game_started","游戏已经开始","游戏尚未开始");
	registerNoArgTrigger("has_had_golden_age","曾经有过黄金时代","未曾有过黄金时代");
	registerNoArgTrigger("has_hostile_reformation_center","有一个敌对的宗教改革中心","没有一个敌对的宗教改革中心");
	registerNoArgTrigger("has_influencing_fort","拥有已激活的要塞","没有拥有已激活的要塞");
	registerNoArgTrigger("has_missionary","有正在进行的传教","没有正在进行的传教");
	registerNoArgTrigger("has_new_dynasty","有新王朝","没有新王朝");
	registerNoArgTrigger("has_or_building_flagship","拥有或正在建造旗舰","尚未拥有且没有建造旗舰");
	registerNoArgTrigger("has_owner_accepted_culture","省份文化是拥有者的相容文化","省份文化不是拥有者的相容文化");
	registerNoArgTrigger("has_owner_culture","省份文化是拥有者的主流文化","省份文化不是拥有者的主流文化");
	registerNoArgTrigger("has_owner_religion","省份宗教是拥有者的宗教","省份宗教不是拥有者的宗教");
	registerNoArgTrigger("has_owner_secondary_religion","省份宗教是拥有者的相容宗教","省份宗教不是拥有者的相容宗教");
	registerNoArgTrigger("has_parliament","有议会","没有议会");
	registerNoArgTrigger("has_pasha","有一个帕夏","没有一个帕夏");
	registerNoArgTrigger("has_port","拥有港口","没有港口");
	registerNoArgTrigger("has_privateers","在任意贸易节点拥有私掠者","在所有贸易节点都没有私掠者");
	registerNoArgTrigger("has_regency","有摄政议会","没有摄政议会");
	registerNoArgTrigger("has_removed_fow","战争迷雾已经消除","战争迷雾尚未消除");
	registerNoArgTrigger("has_revolution_in_province","革命已经传播至该省份","革命尚未传播至该省份");
	registerNoArgTrigger("has_scutage","已经实行免服兵役税","尚未实行免服兵役税");
	registerNoArgTrigger("has_seat_in_parliament","在议会中拥有席位","在议会中没有席位");
	registerNoArgTrigger("has_secondary_religion","拥有相容宗教","没有相容宗教");
	registerNoArgTrigger("has_send_officers","已经实行派遣军官","尚未实行派遣军官");
	registerNoArgTrigger("has_siege","有正在进行中的围城","没有正在进行中的围城");
	registerNoArgTrigger("has_state_patriach","已经开创当地教派","尚未开创当地教派");
	registerNoArgTrigger("has_subsidize_armies","已经实行资助军队","尚未实行资助军队");
	registerNoArgTrigger("has_support_loyalists","已经实行支持效忠派","尚未实行支持效忠派");
	registerNoArgTrigger("has_switched_tag","改变过游玩国家","从未改变过游玩国家");
	registerNoArgTrigger("has_truce","拥有停战协议","没有停战协议");
	registerNoArgTrigger("has_wartaxes","有战争税","没有战争税");
	registerNoArgTrigger("heir_has_consort_dynasty","继承人与配偶相同王朝","继承人与配偶不同王朝");
	registerNoArgTrigger("has_unified_culture_group","已统一文化组","未统一文化组");
	registerNoArgTrigger("heir_has_ruler_dynasty","继承人与统治者相同王朝","继承人与统治者不同王朝");
	registerNoArgTrigger("highest_value_trade_node","是世界上价值最高的贸易节点","不是世界上价值最高的贸易节点");
	registerNoArgTrigger("hre_leagues_enabled","宗教同盟已经启用","宗教同盟尚未启用");
	registerNoArgTrigger("hre_religion_locked","安本纳尔帝国有不可改变的官方信仰","安本纳尔帝国没有不可改变的官方信仰");
	registerNoArgTrigger("hre_religion_treaty","埃斯玛里雅和约已经签署","埃斯玛里雅和约没有签署");
	registerNoArgTrigger("in_golden_age","当前正处于黄金时代","当前不处于黄金时代");
	registerNoArgTrigger("ironman","是铁人模式","不是铁人模式");
	registerNoArgTrigger("is_all_concessions_in_council_taken","揭秘教辩论会已经结束","揭秘教辩论会尚未结束");
	registerNoArgTrigger("is_at_war","处于战争状态","处于和平状态");
	registerNoArgTrigger("is_backing_current_issue","正反对当前议会议程","正支持当前议会议程");
	registerNoArgTrigger("is_bankrupt","已经破产","没有破产");
	registerNoArgTrigger("is_blockaded","省份被封锁","省份未被封锁");
	registerNoArgTrigger("is_capital","是首都","不是首都");
	registerNoArgTrigger("is_city","是城市","不是城市");
	registerNoArgTrigger("is_client_nation","是仆从国","不是仆从国");
	registerNoArgTrigger("is_colonial_nation","是殖民领","不是殖民领");
	registerNoArgTrigger("is_colony","是殖民地","不是殖民地");
	registerNoArgTrigger("is_council_enabled","揭秘教辩论会已经开始","揭秘教辩论会尚未开始");
	registerNoArgTrigger("is_crusade_target","是十字军目标","不是十字军目标");
	registerNoArgTrigger("is_defender_of_faith","是信仰守护者","不是信仰守护者");
	registerNoArgTrigger("is_dynamic_tag","是动态标签","不是动态标签");
	registerNoArgTrigger("is_elector","是选帝侯","不是选帝侯");
	registerNoArgTrigger("is_emperor","是安本纳尔帝国皇帝","不是安本纳尔帝国皇帝");
	registerNoArgTrigger("is_emperor_of_china","是哀伤河管家","不是哀伤河管家");
	registerNoArgTrigger("is_empty","省份可以被殖民","省份无法被殖民");
	registerSimpleTrigger("is_enemy","是%s的敌人","不是%s的敌人",ParadoxType::SCOPE);
	registerNoArgTrigger("is_excommunicated","已被绝罚","未被绝罚");
	registerNoArgTrigger("is_federation_leader","是联盟领袖","不是联盟领袖");
	registerNoArgTrigger("is_federation_nation","是联盟成员","不是联盟成员");
	registerNoArgTrigger("is_female","统治者是女性","统治者不是女性");
	registerNoArgTrigger("is_force_converted","已被强制转换宗教","未被强制转换宗教");
	registerNoArgTrigger("is_former_colonial_nation","是前殖民领国家","不是前殖民领国家");
	registerNoArgTrigger("is_foreign_claim","是其他国家的宣称省份","不是其他国家的宣称省份");
	registerNoArgTrigger("is_great_power","是列强","不是列强");
	registerNoArgTrigger("is_heir_leader","继承人是陆军将领","继承人不是陆军将领");
	registerNoArgTrigger("is_hegemon","是霸权","不是霸权");
	registerNoArgTrigger("is_imperial_ban_allowed","帝国禁令宣战理由已被启用","帝国禁令宣战理由未被启用");
	registerNoArgTrigger("is_in_capital_area","与首都陆路相连","未与首都陆路相连");
	registerNoArgTrigger("is_in_coalition","在包围网中","不在包围网中");
	registerNoArgTrigger("is_in_coalition_war","在一场包围网战争中","不在一场包围网战争中");
	registerNoArgTrigger("is_in_deficit","处于赤字状态","未处于赤字状态");
	registerNoArgTrigger("is_in_extended_regency","在延长摄政中","未在延长摄政中");
	registerNoArgTrigger("is_in_league_war","在宗教联盟战争中","不在宗教联盟战争中");
	registerNoArgTrigger("is_in_trade_league","是贸易联盟的一员","不是贸易联盟的一员");
	registerNoArgTrigger("is_island","是岛屿","不是岛屿");
	registerNoArgTrigger("is_lacking_institutions","缺乏思潮","没有缺乏思潮");
	registerNoArgTrigger("is_league_leader","是宗教联盟领袖","不是宗教联盟领袖");
	registerNoArgTrigger("is_lesser_in_union","是被联统国","不是被联统国");
	registerNoArgTrigger("is_looted","省份已被劫掠","省份未被劫掠");
	registerNoArgTrigger("is_monarch_leader","统治者是陆军将领","统治者不是陆军将领");
	registerNoArgTrigger("is_march","是卫戍国","不是卫戍国");
	registerNoArgTrigger("is_node_in_trade_company_region","是贸易公司区域内的贸易节点","不是贸易公司区域内的贸易节点");
	registerNoArgTrigger("is_nomad","是游牧国家","不是游牧国家");
	registerNoArgTrigger("is_orangists_in_power","奥兰治派正掌权","奥兰治派未掌权");
	registerNoArgTrigger("is_overseas","是海外省份","不是海外省份");
	registerNoArgTrigger("is_overseas_subject","是海外属国","不是海外属国");
	registerNoArgTrigger("is_owned_by_trade_company","省份在贸易公司中","省份不在贸易公司中");
	registerNoArgTrigger("is_papal_controller","是教廷监护","不是教廷监护");
	registerNoArgTrigger("is_part_of_hre","是安本纳尔帝国的一部分","不是安本纳尔帝国的一部分");
	registerNoArgTrigger("is_playing_custom_nation","正在游玩自定义国家","不在游玩自定义国家");
	registerNoArgTrigger("is_previous_papal_controller","之前是教廷监护","之前不是教廷监护");
	registerNoArgTrigger("is_prosperous","省份处于繁荣中","省份不处于繁荣中");
	registerNoArgTrigger("is_protectorate","是受保护国","不是受保护国");
	registerNoArgTrigger("is_random_new_world","使用了随机新世界","没有使用随机新世界");
	registerNoArgTrigger("is_reformation_center","是一个宗教改革中心","不是一个宗教改革中心");
	registerNoArgTrigger("is_religion_reformed","已经改革宗教","尚未改革宗教");
	registerNoArgTrigger("is_revolution_target","是革命目标","不是革命目标");
	registerNoArgTrigger("is_revolutionary","是革命国家","不是革命国家");
	registerNoArgTrigger("is_ruler_commanding_unit","统治者正在指挥单位","统治者没有指挥单位");
	registerNoArgTrigger("is_sea","省份是海洋","省份不是海洋");
	registerNoArgTrigger("is_state","在直属州内","不在直属州内");
	registerNoArgTrigger("is_statists_in_power","议会派正掌权","议会派没有掌权");
	registerNoArgTrigger("is_subject","是属国","不是属国");
	registerNoArgTrigger("is_territory","是自治领","不是自治领");
	registerNoArgTrigger("is_trade_league_leader","是贸易联盟领袖","不是贸易联盟领袖");
	registerNoArgTrigger("is_tribal","是原住民","不是原住民");
	registerNoArgTrigger("is_vassal","是该国的附庸国","不是该国的附庸国");
	registerNoArgTrigger("is_wasteland","是荒凉之地","不是荒凉之地");
	registerNoArgTrigger("island","是岛屿","不是岛屿");
	registerNoArgTrigger("luck","是幸运国家","不是幸运国家");
	registerNoArgTrigger("normal_or_historical_nations","使用了普通或史实国家设置","没有使用普通或史实国家设置");
	registerNoArgTrigger("normal_province_values","使用正常省份价值","没有使用正常省份价值");
	registerNoArgTrigger("papacy_active","已启用教廷","未启用教廷");
	registerNoArgTrigger("primitives","是原始国家","不是原始国家");
	registerNoArgTrigger("province_is_on_an_island","省份在岛屿上","省份不在岛屿上");
	registerNoArgTrigger("province_getting_expelled_minority","省份正在驱逐少数族群","省份没有驱逐少数族群");
	registerNoArgTrigger("revolution_target_exists","革命目标存在","革命目标不存在");
	registerNoArgTrigger("ruler_is_foreigner","统治者是外国人","统治者不是外国人");
	registerNoArgTrigger("unit_in_battle","有单位在战斗中","没有单位在战斗中");
	registerNoArgTrigger("unit_in_siege","有正在进行的围城","没有正在进行的围城");
	registerNoArgTrigger("uses_authority","使用权威机制","没有使用权威机制");
	registerNoArgTrigger("uses_church_aspects","使用教会信条机制","没有使用教会信条机制");
	registerNoArgTrigger("uses_blessings","使用牧首神赐机制","没有使用牧首神赐机制");
	registerNoArgTrigger("uses_cults","使用崇拜物机制","没有使用崇拜物机制");
	registerNoArgTrigger("uses_devotion","使用奉献度机制","没有使用奉献度机制");
	registerNoArgTrigger("uses_doom","使用末日值机制","没有使用末日值机制");
	registerNoArgTrigger("uses_fervor","使用宗教热情机制","没有使用宗教热情机制");
	registerNoArgTrigger("uses_isolationism","使用孤立主义机制","没有使用孤立主义机制");
	registerNoArgTrigger("uses_karma","使用科琳典范值机制","没有使用科琳典范值机制");
	registerNoArgTrigger("uses_papacy","使用教廷机制","未使用教廷机制");
	registerNoArgTrigger("uses_patriarch_authority","使用恶魔力量机制","没有使用恶魔力量机制");
	registerNoArgTrigger("uses_personal_deities","使用个人神祇机制","没有使用个人神祇机制");
	registerNoArgTrigger("uses_piety","使用虔诚机制","没有使用虔诚机制");
	registerNoArgTrigger("uses_religious_icons","使用圣像机制","没有使用圣像机制");
	registerNoArgTrigger("uses_syncretic_faiths","使用相融信仰机制","没有使用相融信仰机制");
	registerNoArgTrigger("was_player","曾经是人类玩家","过去不是人类玩家");
	registerNoArgTrigger("will_back_next_reform","将反对下一项帝国改革","将同意下一项帝国改革");
	registerBooleanTrigger("is_incident_active","有任意事变处于活跃状态","没有任何事变处于活跃状态");
	registerSingleArgTrigger("is_incident_active","%s事变正处于活跃状态","%s事变不处于活跃状态",ParadoxType::STRING);
	registerBooleanTrigger("empire_of_china_has_active_decree","有生效中的圣旨","没有生效中的圣旨");
	registerSingleArgTrigger("empire_of_china_has_active_decree","圣旨%s处于生效状态","圣旨%s尚未处于生效状态",ParadoxType::STRING);
	registerSimpleTrigger("tag","是%s","不是%s",ParadoxType::SCOPE);
	registerSimpleTrigger("absolutism","专制度至少为%d","专制度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("accepted_culture","已经接纳%s的文化","尚未接纳%s的文化",ParadoxType::SCOPE);
	registerSingleArgTrigger("accepted_culture","已经接纳%s文化","尚未接纳%s文化",ParadoxType::STRING);
	registerSingleArgTrigger("adm","统治者的行政能力至少与%s相同","统治者的行政能力低于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("adm","统治者的行政能力至少为%d","统治者的行政能力低于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("adm_power","有至少与%s相同的行政点数","拥有有少于%s的行政点数",ParadoxType::SCOPE);
	registerSingleArgTrigger("adm_power","行政点数至少为%d","行政点数少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("adm_tech","行政科技至少为%d","行政科技低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("advisor","已经雇佣了%s","尚未雇佣%s",ParadoxType::STRING);
	registerSimpleTrigger("advisor_exists","id为%d的顾问存在","id为%d的顾问不存在",ParadoxType::INTEGER);
	registerSimpleClauseTrigger("ai_attitude",new TriggerItem(registerShortString("ai_attitude"),
		{"%s对该国的态度为%s","%s为该国的态度不为%s"},
		{"who","attitude"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleTrigger("army_professionalism","陆军职业度%p%%","陆军职业度少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("army_size","军队规模至少为%dK","军队规模小于%dK",ParadoxType::INTEGER);
	registerSingleArgTrigger("army_size","拥有至少和%s规模相同的军队","军队规模小于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("army_size_percentage","军队规模至少为上限的%p%%","军队规模小于上限的%p%%",ParadoxType::INTEGER);
	registerSimpleClauseTrigger("army_strength",new TriggerItem(registerShortString("army_strength"),
		{"陆军实力至少为%s的%p%%","陆军实力少于%s的%p%%"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSingleArgTrigger("army_tradition","陆军传统至少为%d","陆军传统少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("army_tradition","陆军传统不低于%s","陆军传统低于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("artillery_fraction","炮兵比例至少为%p%%","炮兵比例小于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("artillery_in_province","有至少%d队炮兵", "炮兵的数量小于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("artillery_in_province","有来自%s的炮兵", "没有来自%s的炮兵",ParadoxType::SCOPE);
	registerSingleArgTrigger("authority","权威值至少为%d", "权威值小于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("authority","拥有至少与%s相同的权威值", "权威值小于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("average_autonomy","平均自治度至少为%p%%","平均自治度低于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("average_home_autonomy","直属州核心省份的平均自治度至少为%p%%","直属州核心省份的平均自治度低于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("average_autonomy_above_min","最低限度以上的平均自治度至少为%p%%","最低限度以上的平均自治度低于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("base_production","基础生产至少为%d","基础生产少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("base_production","基础生产至少为variable:%s","基础生产少于variable:%s",ParadoxType::STRING);
	registerSingleArgTrigger("base_manpower","基础人力至少为%d","基础人力少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("base_manpower","基础人力至少为variable:%s","基础人力少于variable:%s",ParadoxType::STRING);

	registerSingleArgTrigger("base_tax","基础税收至少为%d","基础税收少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("base_tax","基础税收至少为variable:%s","基础税收少于variable:%s",ParadoxType::STRING);
	registerSimpleTrigger("blockade","被封锁的港口至少为%p%%","被封锁的港口的少于%p%%",ParadoxType::INTEGER);
	
	registerSimpleClauseTrigger("border_distance",new TriggerItem(registerShortString("border_distance"),
		{"与%s的边境距离至少为%d","与%s的边境距离少于%d"},
		{"who","distance"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerNumberRequiredTrigger("calc_true_if","amount","至少%d个","少于%d个");
	registerSimpleTrigger("can_be_overlord","可以作为%s的宗主国","无法作为%s的宗主国",ParadoxType::STRING);
	registerSimpleTrigger("can_build","可以修建%s","不能修建%s",ParadoxType::STRING);
	registerNoArgTrigger("can_create_vassals","可以创建附庸","不能创建附庸");
	registerSimpleTrigger("can_justify_trade_conflict","可以正当化与%s的贸易争端","无法正当化与%s的贸易争端",ParadoxType::SCOPE);
	registerSimpleTrigger("can_spawn_rebel","当地有效的叛军类型为%s","当地有效的叛军类型不是%s",ParadoxType::STRING);
	
	registerSimpleClauseTrigger("can_use_peace_treaty",new TriggerItem(registerShortString("can_use_peace_treaty"),
		{"%s可以使用%s条款","%s不可以使用%s条款"},
		{"who","treaty"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleTrigger("capital","首都位于%s","首都不位于%s",ParadoxType::SCOPE);
	registerSimpleClauseTrigger("capital_distance",new TriggerItem(registerShortString("capital_distance"),
		{"与%s首都之间的距离至少为%d","与%s首都之间的距离小于%d"},
		{"who","distance"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleTrigger("cavalry_fraction","骑兵占军队比例至少为%p%%","骑兵占军队比例少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("cavalry_in_province","有至少%d队骑兵", "骑兵的数量小于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("cavalry_in_province","有来自%s的骑兵", "没有来自%s的骑兵",ParadoxType::SCOPE);
	registerSimpleTrigger("province_has_center_of_trade_of_level","省份至少有%d级的贸易中心","省份没有至少%d级的贸易中心",ParadoxType::INTEGER);
	registerClausedTrigger("check_variable", new TriggerItem(registerShortString("check_variable"),
		{"变量%s的值至少为%s","变量%s的值小于%s"},
		{"src","tar"},
		{ParadoxType::STRING,ParadoxType::STRING},
		{0,1}
	),
	[](std::vector<std::pair<std::string,ParadoxBase*>>& map)-> bool {
		std::string src_string = "";
		std::string tar_string = "";
		if(map.size() != 1){

			if(map[0].first != "which") return false;
			auto fromPtr = map[0].second;

			auto [key2,value] = map[1];
			if(key2 == "value"){
				tar_string = std::to_string(value->getAsInteger()->getIntegerContent() / 1000);
			}
			else if(key2 == "which"){
				if(isCastable(value,ParadoxType::SCOPE)){
					std::string tar1 = "";
					if(value->getType() == ParadoxType::INTEGER) {
						tar1 = std::to_string(value->getAsInteger()->getIntegerContent() / 1000);
					}
					else tar1 = value->getAsString()->getStringContent();
					Scope* scope = createScopeFromString(tar1);
					if(scope == nullptr) return false;
					else tar_string = scope->toString();
				}
				else tar_string = map[1].second->getAsString()->getStringContent();
			}
			else return false;
			map.clear();
			map.push_back({"src", fromPtr});
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
	}
	);
	registerSingleArgTrigger("church_power","教会力量至少为%d","教会力量小于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("church_power","教会力量不低于%s","教会力量少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("coalition_target","%s是包围网的目标","%s不是包围网的目标",ParadoxType::SCOPE);
	registerSimpleTrigger("colonial_region","省份位于%s殖民地区","省份不位于%s殖民地区",ParadoxType::STRING);
	registerSimpleTrigger("colony","拥有至少%d个殖民领","拥有少于%d个殖民领",ParadoxType::INTEGER);
	registerSimpleTrigger("colony_claim","%s拥有殖民领宣称","%s没有殖民地宣称",ParadoxType::SCOPE);
	registerSimpleTrigger("colonysize","殖民地规模至少为%d","殖民地规模小于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("consort_adm","有一个行政能力至少为%d的配偶","没有一个行政能力至少为%d的配偶",ParadoxType::INTEGER);
	registerSimpleTrigger("consort_age","有一个至少%d岁的的配偶","没有一个至少%d岁的的配偶",ParadoxType::INTEGER);
	registerSimpleTrigger("consort_dip","有一个外交能力至少为%d的配偶","没有一个外交能力至少为%d的配偶",ParadoxType::INTEGER);
	registerSimpleTrigger("consort_mil","有一个军事能力至少为%d的配偶","没有一个军事能力至少为%d的配偶",ParadoxType::INTEGER);
	registerSimpleTrigger("consort_culture","有一个文化为%s的配偶","没有一个文化为%s的配偶",ParadoxType::STRING);
	registerSimpleTrigger("consort_has_personality","配偶拥有%s特质","配偶没有%s特质",ParadoxType::STRING);
	
	registerSingleArgTrigger("consort_religion","有一个信仰%s的配偶","没有一个信仰%s的配偶",ParadoxType::STRING);
	registerSingleArgTrigger("consort_religion", "有一个信仰%s正信的配偶","没有一个信仰%s正信的配偶",ParadoxType::SCOPE);

	registerSimpleTrigger("construction_progress","修建进度至少为%p%%","修建进度少于%p%%",ParadoxType::INTEGER);
	
	registerSingleArgTrigger("continent", "省份位于%s大陆","省份不位于%s大陆",ParadoxType::STRING);
	registerSingleArgTrigger("continent", "省份位于%s所在大陆","省份不位于%s所在大陆",ParadoxType::SCOPE);

	registerSimpleTrigger("controlled_by", "省份被%s所控制","省份未被%s所控制",ParadoxType::SCOPE);
	registerSimpleTrigger("controls","控制省份%s","没有控制省份%s",ParadoxType::SCOPE);
	
	registerSimpleTrigger("claim","拥有对%s的宣称","没有对%s的宣称",ParadoxType::SCOPE);
	registerSimpleTrigger("core_claim","拥有对%s的核心宣称","没有对%s的核心宣称",ParadoxType::SCOPE);

	registerSimpleTrigger("core_percentage","核心省份比例至少为%p%%","核心省份比例小于%p%%",ParadoxType::INTEGER);

	registerSimpleTrigger("corruption","腐败度至少为%d","腐败度小于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("council_position","在揭秘教辩论会中持有%s立场","没有在揭秘教辩论会中持有%s立场",ParadoxType::STRING);
	registerSimpleTrigger("country_or_non_sovereign_subject_holds","被%s或其非朝贡属国持有","没有被%s或其非朝贡属国持有",ParadoxType::SCOPE);
	registerSimpleTrigger("country_or_subject_holds","被%s或其属国持有","没有被%s或其属国持有",ParadoxType::SCOPE);

	registerSingleArgTrigger("crown_land_share","王室领地比例至少为%p%%","王室领地比例少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("crown_land_share","王室领地比例大于%s阶级持有比例","王室领地比例不高于%s阶级持有比例",ParadoxType::STRING);
	registerSingleArgTrigger("culture","省份文化为%s","省份文化不为%s",ParadoxType::STRING);
	registerSingleArgTrigger("culture","省份文化是%s的主流文化","省份文化不是%s的主流文化",ParadoxType::SCOPE);
	registerSimpleTrigger("culture_group","属于%s文化组","不属于%s文化组",ParadoxType::SCOPE);
	registerSimpleTrigger("culture_group_claim","%s拥有与我国主流文化相同文化组的省份","%s没有与我国主流文化相同文化组的省份",ParadoxType::SCOPE);
	
	registerSimpleTrigger("current_age","当前时代为%s","当前时代不是%s",ParadoxType::STRING);
	registerSimpleTrigger("current_bribe","该省份的议会席位想要%s类型的贿赂","该省份的议会席位不想要%s类型的贿赂",ParadoxType::STRING);
	registerSimpleTrigger("current_debate","当前议会正在辩论%s","当前议会没有辩论%s",ParadoxType::STRING);
	registerSimpleTrigger("current_icon","当前已激活%s","当前未激活%s",ParadoxType::STRING);
	registerSimpleTrigger("current_income_balance","上个月的净收入至少为%d","上个月的净收入少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("current_institution","当地最早尚未接纳的思潮支持度达到%d%%","当地最早尚未接纳的思潮支持度不足%d%%",ParadoxType::INTEGER);
	registerSimpleTrigger("current_institution_growth","当地最早尚未接纳的思潮增长达到%d%%","当地最早尚未接纳的思潮增长不足%d%%",ParadoxType::INTEGER);
	registerSimpleTrigger("current_size_of_parliament","当前议会至少拥有%d个席位","当前议会不足%d个席位",ParadoxType::INTEGER);
	registerSimpleTrigger("defensive_war_with","当前正在防御战争中对抗%s","当前没有在防御战争中对抗%s",ParadoxType::SCOPE);
	registerSimpleTrigger("devastation","荒废度达到%d","荒废度低于%d",ParadoxType::INTEGER);

	registerSimpleTrigger("innovativeness","创新度至少为%d","创新度小于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("treasury","拥有至少%d克朗","拥有少于%d克朗",ParadoxType::INTEGER);
	registerNumberRequiredTrigger("num_of_owned_provinces_with","value","至少%d个拥有的省份满足下列条件:","少于%d个拥有的省份满足下列条件:");
	registerSimpleTrigger("has_country_flag","国家标签'%s'已被设置","国家标签'%s'未被设置",ParadoxType::STRING);
	registerSimpleTrigger("monthly_dip","每月外交点数至少为%d","每月外交点数少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("monthly_adm","每月行政点数至少为%d","每月行政点数少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("monthly_mil","每月军事点数至少为%d","每月军事点数少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("was_tag","曾经是%s","以前不是%s",ParadoxType::SCOPE);

	// ===== a =====
	registerSimpleTrigger("active_major_mission","当前任务为%s","当前任务不是%s",ParadoxType::STRING);
	registerSimpleTrigger("all_regiments_morale_percent","所有军团士气至少为%p%%","所有军团士气低于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("alliance_with","与%s结盟","未与%s结盟",ParadoxType::SCOPE);
	registerSimpleTrigger("area","位于%s地区","不位于%s地区",ParadoxType::STRING);
	registerSimpleTrigger("average_effective_unrest","平均有效叛乱度至少为%d","平均有效叛乱度小于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("average_unrest","平均叛乱度至少为%d","平均叛乱度小于%d",ParadoxType::INTEGER);
	// ===== c =====
	registerSimpleTrigger("can_spawn_rebels","当地有效的叛军类型为%s","当地有效的叛军类型不是%s",ParadoxType::STRING);
	registerSimpleTrigger("can_use_crown_land_interaction","可以使用%s王室领地互动","不能使用%s王室领地互动",ParadoxType::STRING);
	// ===== d =====
	registerSingleArgTrigger("development","发展度至少为%d","发展度小于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("development","发展度至少与%s相同","发展度小于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("development_of_overlord_fraction","发展度至少为宗主国的%p%%","发展度低于宗主国的%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("devotion","奉献度至少为%d","奉献度小于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("devotion","拥有至少与%s相同的奉献度","奉献度少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("dip","统治者的外交能力至少为%d","统治者的外交能力低于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("dip","统治者的外交能力至少与%s相同","统治者的外交能力低于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("diplomatic_reputation","外交声誉至少为%d","外交声誉少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("diplomatic_reputation","拥有至少与%s相同的外交声誉","外交声誉少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("dip_power","外交点数至少为%d","外交点数少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("dip_power","拥有至少与%s相同的外交点数","外交点数少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("dip_tech","外交科技至少为%d","外交科技低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("dominant_culture","主要文化为%s","主要文化不是%s",ParadoxType::STRING);
	registerSimpleTrigger("dominant_religion","主流宗教为%s","主流宗教不是%s",ParadoxType::STRING);
	registerSingleArgTrigger("doom","末日值至少为%d","末日值少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("doom","拥有至少与%s相同的末日值","末日值少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("dynasty","统治王朝为%s","统治王朝不是%s",ParadoxType::STRING);
	registerSingleArgTrigger("dynasty","与%s为同一王朝","与%s不是同一王朝",ParadoxType::SCOPE);
	// ===== e =====
	registerSimpleTrigger("empire_of_china_reform_passed","已经通过%s中国帝国改革","尚未通过%s中国帝国改革",ParadoxType::STRING);
	registerSimpleTrigger("empire_of_china_num_reforms_passed","已通过至少%d项中国帝国改革","已通过的改革少于%d项",ParadoxType::INTEGER);
	registerNoArgTrigger("exiled_same_dynasty_as_current","被流放的统治者与现任统治者同属一个王朝","被流放的统治者与现任统治者不同王朝");
	registerBooleanTrigger("exists","国家存在","国家不存在");
	registerSingleArgTrigger("exists","%s存在","%s不存在",ParadoxType::SCOPE);
	registerSimpleTrigger("expelled_different_minorities","已驱逐%d种少数文化","驱逐的少数文化少于%d种",ParadoxType::INTEGER);
	// ===== f =====
	registerSimpleTrigger("faction_in_power","当前掌权派系为%s","当前掌权派系不是%s",ParadoxType::STRING);
	registerSimpleTrigger("federation_size","联邦至少有%d名成员","联邦成员少于%d名",ParadoxType::INTEGER);
	registerSimpleTrigger("fervor","宗教热情至少为%d","宗教热情少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("fort_level","要塞等级至少为%d","要塞等级低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("full_idea_group","已完整采纳%s理念组","尚未完整采纳%s理念组",ParadoxType::STRING);
	// ===== g =====
	registerSimpleTrigger("galley_fraction","桨帆船占海军比例至少为%p%%","桨帆船占海军比例少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("galleys_in_province","有至少%d艘桨帆船","桨帆船的数量少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("galleys_in_province","有来自%s的桨帆船","没有来自%s的桨帆船",ParadoxType::SCOPE);
	registerSimpleTrigger("garrison","驻军规模至少为%d","驻军规模少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("gives_military_access_to","向%s提供了军事通行权","没有向%s提供军事通行权",ParadoxType::SCOPE);
	registerSimpleTrigger("gives_fleet_basing_rights_to","向%s提供了舰队停泊权","没有向%s提供舰队停泊权",ParadoxType::SCOPE);
	registerSimpleTrigger("gold_income","黄金收入至少为%d","黄金收入少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("gold_income_percentage","黄金收入占总收入比例至少为%p%%","黄金收入占比少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("governing_capacity_percentage","治理容量使用率至少为%p%%","治理容量使用率少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("government","政体为%s","政体不是%s",ParadoxType::STRING);
	registerSimpleTrigger("government_rank","政体等级至少为%d","政体等级低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("grown_by_development","发展度已增长至少%d","发展度增长少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("grown_by_states","直属州数量已增长至少%d","直属州数量增长少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("great_power_rank","列强排名至少为%d","列强排名优于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("guaranteed_by","受到%s的保障","未受到%s的保障",ParadoxType::SCOPE);
	// ===== h (scalar) =====
	registerSimpleTrigger("had_recent_war","过去%d年内打过战争","过去%d年内没有打过战争",ParadoxType::INTEGER);
	registerSimpleTrigger("harmonization_progress","当前调和进度至少为%p%%","当前调和进度少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("harmony","调和度至少为%d","调和度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("has_active_policy","有生效中的%s政策","没有生效中的%s政策",ParadoxType::STRING);
	registerSimpleTrigger("has_active_triggered_province_modifier","拥有%s省份触发修正","没有%s省份触发修正",ParadoxType::STRING);
	registerSimpleTrigger("has_adopted_cult","已接纳%s崇拜","尚未接纳%s崇拜",ParadoxType::STRING);
	registerSimpleTrigger("has_age_ability","拥有%s时代能力","没有%s时代能力",ParadoxType::STRING);
	registerSingleArgTrigger("has_assimilated_culture_group","已同化%s文化组","尚未同化%s文化组",ParadoxType::STRING);
	registerSingleArgTrigger("has_assimilated_culture","已同化%s文化","尚未同化%s文化",ParadoxType::STRING);
	registerNoArgTrigger("has_border_with_religious_enemy","与宗教敌人接壤","没有与宗教敌人接壤");
	registerSimpleTrigger("has_building","拥有%s建筑","没有%s建筑",ParadoxType::STRING);
	registerSimpleTrigger("has_center_of_trade_of_level","拥有至少%d级的贸易中心","没有达到%d级的贸易中心",ParadoxType::INTEGER);
	registerSimpleTrigger("has_church_aspect","已启用%s教会信条","尚未启用%s教会信条",ParadoxType::STRING);
	registerSimpleTrigger("has_climate","气候为%s","气候不是%s",ParadoxType::STRING);
	registerSimpleTrigger("has_colonial_parent","%s是殖民母国","%s不是殖民母国",ParadoxType::SCOPE);
	registerSimpleTrigger("has_consort_flag","配偶标签'%s'已被设置","配偶标签'%s'未被设置",ParadoxType::STRING);
	registerSimpleTrigger("has_construction","正在修建%s","没有正在修建%s",ParadoxType::STRING);
	registerSimpleTrigger("has_country_modifier","拥有%s国家修正","没有%s国家修正",ParadoxType::STRING);
	registerSimpleTrigger("has_disaster","正经历%s灾难","没有经历%s灾难",ParadoxType::STRING);
	registerSimpleTrigger("has_discovered","已经发现%s","尚未发现%s",ParadoxType::SCOPE);
	registerSimpleTrigger("has_dlc","启用了%s扩展包","未启用%s扩展包",ParadoxType::STRING);
	registerSimpleTrigger("has_eclipsed","%s已无法再成为宿敌","%s仍可以成为宿敌",ParadoxType::SCOPE);
	registerSimpleTrigger("has_been_eclipsed_by","已被%s超越而无法被其敌视","未被%s超越",ParadoxType::SCOPE);
	registerNoArgTrigger("has_empty_adjacent_province","有相邻的空白省份","没有相邻的空白省份");
	registerSimpleTrigger("has_faction","拥有%s派系","没有%s派系",ParadoxType::STRING);
	registerSimpleTrigger("has_given_consort_to","已将配偶送给%s","未将配偶送给%s",ParadoxType::SCOPE);
	registerSimpleTrigger("has_guaranteed","已保障%s的独立","未保障%s的独立",ParadoxType::SCOPE);
	registerSimpleTrigger("has_global_flag","全局标签'%s'已被设置","全局标签'%s'未被设置",ParadoxType::STRING);
	registerSimpleTrigger("has_government_mechanic","使用%s政府机制","未使用%s政府机制",ParadoxType::STRING);
	registerSimpleTrigger("has_government_attribute","政府改革拥有%s属性","政府改革没有%s属性",ParadoxType::STRING);
	registerSingleArgTrigger("has_harmonized_with","已与%s调和","尚未与%s调和",ParadoxType::STRING);
	registerNoArgTrigger("has_harsh_treatment","已进行严厉镇压","未进行严厉镇压");
	registerSimpleTrigger("has_heir_flag","继承人标签'%s'已被设置","继承人标签'%s'未被设置",ParadoxType::STRING);
	registerSimpleTrigger("has_heir_leader_from","有来自%s的继承人统帅","没有来自%s的继承人统帅",ParadoxType::SCOPE);
	registerSimpleTrigger("has_idea","拥有%s理念","没有%s理念",ParadoxType::STRING);
	registerSimpleTrigger("has_idea_group","已选择%s理念组","尚未选择%s理念组",ParadoxType::STRING);
	registerSimpleTrigger("has_institution","已接纳%s思潮","尚未接纳%s思潮",ParadoxType::STRING);
	registerSimpleTrigger("has_latent_trade_goods","潜在贸易品为%s","潜在贸易品不是%s",ParadoxType::STRING);
	registerSimpleTrigger("has_leader","拥有名为%s的将领","没有名为%s的将领",ParadoxType::STRING);
	registerSingleArgTrigger("has_matching_religion","信仰%s或与之相融","不信仰%s且不相融",ParadoxType::STRING);
	registerSingleArgTrigger("has_matching_religion","与%s信仰相同或相融","与%s信仰不同且不相融",ParadoxType::SCOPE);
	registerSimpleTrigger("has_merchant","在贸易节点中拥有%s的商人","在贸易节点中没有%s的商人",ParadoxType::SCOPE);
	registerSimpleTrigger("has_mission","拥有%s任务","没有%s任务",ParadoxType::STRING);
	registerSimpleTrigger("has_monsoon","季风为%s","季风不是%s",ParadoxType::STRING);
	registerSimpleTrigger("has_most_province_trade_power","%s在该贸易节点拥有最多省份贸易力量","%s在该贸易节点不拥有最多省份贸易力量",ParadoxType::SCOPE);
	registerSimpleTrigger("has_naval_doctrine","拥有%s海军学说","没有%s海军学说",ParadoxType::STRING);
	registerNoArgTrigger("has_neighbor","拥有邻国","没有邻国");
	registerSimpleTrigger("has_num_flagships","拥有至少%d艘旗舰","旗舰少于%d艘",ParadoxType::INTEGER);
	registerSimpleTrigger("has_personal_deity","统治者已选择%s个人神祇","统治者没有选择%s个人神祇",ParadoxType::STRING);
	registerSimpleTrigger("has_pillaged_capital_against","曾对%s使用过洗劫首都","未对%s使用过洗劫首都",ParadoxType::SCOPE);
	registerSimpleTrigger("has_promote_investments","在%s贸易公司区域推动投资","未在%s贸易公司区域推动投资",ParadoxType::STRING);
	registerSimpleTrigger("has_province_flag","省份标签'%s'已被设置","省份标签'%s'未被设置",ParadoxType::STRING);
	registerSimpleTrigger("has_province_modifier","拥有%s省份修正","没有%s省份修正",ParadoxType::STRING);
	registerSimpleTrigger("has_rebel_faction","被%s叛军控制","没有被%s叛军控制",ParadoxType::STRING);
	registerSimpleTrigger("has_reform","拥有%s政府改革","没有%s政府改革",ParadoxType::STRING);
	registerSimpleTrigger("government_reform_progress","政府改革进度至少为%p%%","政府改革进度少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("has_ruler_flag","统治者标签'%s'已被设置","统治者标签'%s'未被设置",ParadoxType::STRING);
	registerSimpleTrigger("has_ruler_leader_from","有来自%s的统治者统帅","没有来自%s的统治者统帅",ParadoxType::SCOPE);
	registerSimpleTrigger("has_ruler_modifier","拥有%s统治者修正","没有%s统治者修正",ParadoxType::STRING);
	registerSimpleTrigger("has_saved_event_target","已保存事件目标%s","尚未保存事件目标%s",ParadoxType::STRING);
	registerSimpleTrigger("has_saved_global_event_target","已保存全局事件目标%s","尚未保存全局事件目标%s",ParadoxType::STRING);
	registerSimpleTrigger("has_spawned_rebels","有活跃的%s叛军","没有活跃的%s叛军",ParadoxType::STRING);
	registerSimpleTrigger("has_spawned_supported_rebels","有%s支持的活跃叛军","没有%s支持的活跃叛军",ParadoxType::SCOPE);
	registerSimpleTrigger("has_state_edict","直属州启用了%s法令","直属州没有启用%s法令",ParadoxType::STRING);
	registerSimpleTrigger("has_subject_of_type","拥有至少一个%s属国","没有%s属国",ParadoxType::STRING);
	registerNoArgTrigger("has_switched_nation","改变过游玩国家","从未改变过游玩国家");
	registerSimpleTrigger("has_terrain","地形为%s","地形不是%s",ParadoxType::STRING);
	registerSimpleTrigger("has_trader","%s在该贸易节点拥有商人","%s在该贸易节点没有商人",ParadoxType::SCOPE);
	registerSimpleTrigger("has_unembraced_institution","尚未接纳%s思潮","已经接纳%s思潮",ParadoxType::STRING);
	registerSimpleTrigger("has_unit_type","已将%s设为首选兵种","未将%s设为首选兵种",ParadoxType::STRING);
	registerSimpleTrigger("has_unlocked_cult","已解锁%s崇拜","尚未解锁%s崇拜",ParadoxType::STRING);
	registerSimpleTrigger("has_winter","冬季为%s","冬季不是%s",ParadoxType::STRING);
	registerSimpleTrigger("have_had_reform","曾经拥有%s政府改革","未曾拥有%s政府改革",ParadoxType::STRING);
	registerSimpleTrigger("heavy_ship_fraction","重型船占海军比例至少为%p%%","重型船占海军比例少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("heavy_ships_in_province","有至少%d艘重型船","重型船的数量少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("heavy_ships_in_province","有来自%s的重型船","没有来自%s的重型船",ParadoxType::SCOPE);
	registerSimpleTrigger("heir_adm","继承人的行政能力至少为%d","继承人的行政能力低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("heir_age","继承人至少%d岁","继承人不足%d岁",ParadoxType::INTEGER);
	registerSimpleTrigger("heir_dip","继承人的外交能力至少为%d","继承人的外交能力低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("heir_claim","继承人的宣称强度至少为%d","继承人的宣称强度低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("heir_culture","继承人的文化为%s","继承人的文化不是%s",ParadoxType::STRING);
	registerSimpleTrigger("heir_has_personality","继承人拥有%s特质","继承人没有%s特质",ParadoxType::STRING);
	registerSimpleTrigger("heir_mil","继承人的军事能力至少为%d","继承人的军事能力低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("heir_nationality","继承人的国籍为%s","继承人的国籍不是%s",ParadoxType::SCOPE);
	registerSimpleTrigger("heir_religion","继承人的宗教为%s","继承人的宗教不是%s",ParadoxType::STRING);
	registerSimpleTrigger("higher_development_than","发展度高于%s","发展度不高于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("historical_friend_with","与%s是历史友邦","与%s不是历史友邦",ParadoxType::SCOPE);
	registerSimpleTrigger("historical_rival_with","与%s是历史宿敌","与%s不是历史宿敌",ParadoxType::SCOPE);
	registerSimpleTrigger("holy_order","拥有%s教团","没有%s教团",ParadoxType::STRING);
	registerSingleArgTrigger("horde_unity","游牧团结至少为%d","游牧团结少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("horde_unity","拥有至少与%s相同的游牧团结","游牧团结少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("hre_heretic_religion","%s是神罗对立信仰","%s不是神罗对立信仰",ParadoxType::STRING);
	registerSimpleTrigger("hre_reform_passed","已经通过%s帝国改革","尚未通过%s帝国改革",ParadoxType::STRING);
	registerSimpleTrigger("hre_religion","%s是神罗官方信仰","%s不是神罗官方信仰",ParadoxType::STRING);
	registerSimpleTrigger("hre_size","神罗至少有%d名成员","神罗成员少于%d名",ParadoxType::INTEGER);
	registerSimpleTrigger("humiliated_by","曾受到%s的羞辱","未受到%s的羞辱",ParadoxType::SCOPE);
	// ===== i =====
	registerSimpleTrigger("imperial_influence","神罗皇帝权威至少为%d","神罗皇帝权威少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("imperial_mandate","中国皇帝的天命至少为%d","中国皇帝的天命少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("infantry_fraction","步兵占军队比例至少为%p%%","步兵占军队比例少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("infantry_in_province","有至少%d队步兵","步兵的数量小于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("infantry_in_province","有来自%s的步兵","没有来自%s的步兵",ParadoxType::SCOPE);
	registerSingleArgTrigger("inflation","通货膨胀至少为%p%%","通货膨胀少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("inflation","拥有至少与%s相同的通货膨胀","通货膨胀少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("invested_papal_influence","在下一任教皇选举中投入了至少%d教廷影响力","投入的教廷影响力少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("in_league","属于%s宗教同盟","不属于%s宗教同盟",ParadoxType::STRING);
	registerSimpleTrigger("is_advisor_employed","已雇佣id为%d的顾问","未雇佣id为%d的顾问",ParadoxType::INTEGER);
	registerNoArgTrigger("is_any_heresy_enabled","当前宗教启用了异端","当前宗教没有启用异端");
	registerSimpleTrigger("is_blockaded_by","被%s封锁","没有被%s封锁",ParadoxType::SCOPE);
	registerSimpleTrigger("is_capital_of","是%s的首都","不是%s的首都",ParadoxType::SCOPE);
	registerSimpleTrigger("is_claim","拥有对%s的宣称","没有对%s的宣称",ParadoxType::SCOPE);
	registerSimpleTrigger("is_permament_claim","拥有对%s的永久宣称","没有对%s的永久宣称",ParadoxType::SCOPE);
	registerSimpleTrigger("is_client_nation_of","是%s的仆从国","不是%s的仆从国",ParadoxType::SCOPE);
	registerSimpleTrigger("is_colonial_nation_of","是%s的殖民领","不是%s的殖民领",ParadoxType::SCOPE);
	registerSimpleTrigger("is_core","拥有对%s的核心","没有对%s的核心",ParadoxType::SCOPE);
	registerSimpleTrigger("is_defender_of_faith_of_tier","信仰守护者等级至少为%d","信仰守护者等级低于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("is_harmonizing_with","正在与%s调和","没有与%s调和",ParadoxType::STRING);
	registerSingleArgTrigger("is_harmonizing_with","正在与%s的宗教调和","没有与%s的宗教调和",ParadoxType::SCOPE);
	registerSimpleTrigger("is_hegemon_of_type","是%s霸权","不是%s霸权",ParadoxType::STRING);
	registerSimpleTrigger("is_hiring_condottiere_from","正在从%s雇佣雇佣兵","没有从%s雇佣雇佣兵",ParadoxType::SCOPE);
	registerSimpleTrigger("hegemon_strength","霸权进度至少为%d","霸权进度少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("is_incident_happened","事变%s已经发生","事变%s尚未发生",ParadoxType::STRING);
	registerSimpleTrigger("is_incident_possible","事变%s可以发生","事变%s不可以发生",ParadoxType::STRING);
	registerSimpleTrigger("is_incident_potential","事变%s在潜在状态","事变%s不在潜在状态",ParadoxType::STRING);
	registerSimpleTrigger("is_institution_enabled","已经发现%s思潮","尚未发现%s思潮",ParadoxType::STRING);
	registerSimpleTrigger("is_institution_origin","是%s思潮的发源地","不是%s思潮的发源地",ParadoxType::STRING);
	registerSimpleTrigger("is_in_trade_league_with","与%s同属一个贸易联盟","与%s不属于同一个贸易联盟",ParadoxType::SCOPE);
	registerSimpleTrigger("is_league_enemy","%s是同盟敌人","%s不是同盟敌人",ParadoxType::SCOPE);
	registerSimpleTrigger("is_league_friend","与%s同属一个宗教同盟","与%s不属于同一个宗教同盟",ParadoxType::SCOPE);
	registerSimpleTrigger("is_month","当前月份至少为%d","当前月份早于%d",ParadoxType::INTEGER);
	registerNoArgTrigger("is_migratory_tribe","是迁徙部落","不是迁徙部落");
	registerSimpleTrigger("is_most_powerful_estate","最强大的阶层是%s","最强大的阶层不是%s",ParadoxType::STRING);
	registerSimpleTrigger("is_neighbor_of","与%s接壤","与%s不接壤",ParadoxType::SCOPE);
	registerSimpleTrigger("is_neighbor_of_province","与%s的省份相邻","与%s的省份不相邻",ParadoxType::SCOPE);
	registerSimpleTrigger("is_origin_of_consort","%s是配偶的原籍国","%s不是配偶的原籍国",ParadoxType::SCOPE);
	registerSimpleTrigger("is_possible_march","%s可以成为卫戍国","%s不能成为卫戍国",ParadoxType::SCOPE);
	registerSimpleTrigger("is_possible_vassal","%s可以作为附庸释放","%s不能作为附庸释放",ParadoxType::SCOPE);
	registerSimpleTrigger("is_reform_available","可以使用%s政府改革","不能使用%s政府改革",ParadoxType::STRING);
	registerSimpleTrigger("is_religion_grant_colonial_claim","该省份被%s授予了殖民宣称","该省份未被%s授予殖民宣称",ParadoxType::SCOPE);
	registerSimpleTrigger("is_religion_enabled","%s宗教已启用","%s宗教未启用",ParadoxType::STRING);
	registerSimpleTrigger("is_renting_condottieri_to","正在向%s出租雇佣兵","没有向%s出租雇佣兵",ParadoxType::SCOPE);
	registerSimpleTrigger("is_rival","%s是宿敌","%s不是宿敌",ParadoxType::SCOPE);
	registerSimpleTrigger("is_state_core","拥有对%s的直属州核心","没有对%s的直属州核心",ParadoxType::SCOPE);
	registerSimpleTrigger("is_strongest_trade_power","%s在该贸易节点拥有最强贸易力量","%s在该贸易节点没有最强贸易力量",ParadoxType::SCOPE);
	registerSimpleTrigger("is_subject_of","是%s的属国","不是%s的属国",ParadoxType::SCOPE);
	registerSimpleTrigger("is_subject_of_type","是%s属国","不是%s属国",ParadoxType::STRING);
	registerSimpleTrigger("is_supporting_independence_of","正在支持%s的独立","没有支持%s的独立",ParadoxType::SCOPE);
	registerSimpleTrigger("is_territorial_core","拥有对%s的自治领核心","没有对%s的自治领核心",ParadoxType::SCOPE);
	registerSimpleTrigger("is_threat","受到%s的威胁","没有受到%s的威胁",ParadoxType::SCOPE);
	registerSimpleTrigger("is_year","当前年份至少为%d","当前年份早于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("isolationism","孤立主义等级至少为%d","孤立主义等级低于%d",ParadoxType::INTEGER);
	// ===== j/k =====
	registerSimpleTrigger("junior_union_with","是%s的联统被联统方","不是%s的联统被联统方",ParadoxType::SCOPE);
	registerSingleArgTrigger("karma","业力至少为%d","业力少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("karma","拥有至少与%s相同的业力","业力少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("knows_country","已经探明%s","尚未探明%s",ParadoxType::SCOPE);
	// ===== l =====
	registerSingleArgTrigger("land_forcelimit","陆军上限至少为%d","陆军上限少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("land_forcelimit","拥有至少与%s相同的陆军上限","陆军上限少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("land_maintenance","陆军维护费至少为%p%%","陆军维护费低于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("land_morale","陆军士气至少为%p%%","陆军士气少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("land_morale","拥有至少与%s相同的陆军士气","陆军士气少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("last_mission","上一个任务为%s","上一个任务不是%s",ParadoxType::STRING);
	registerSingleArgTrigger("legitimacy","正统性至少为%d","正统性少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("legitimacy","拥有至少与%s相同的正统性","正统性少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("legitimacy_equivalent","正统性等价值至少为%d","正统性等价值少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("legitimacy_equivalent","拥有至少与%s相同的正统性等价值","正统性等价值少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("legitimacy_or_horde_unity","正统性或游牧团结至少为%d","正统性或游牧团结少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("legitimacy_or_horde_unity","拥有至少与%s相同的正统性或游牧团结","正统性或游牧团结少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("liberty_desire","自由渴望至少为%d","自由渴望少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("light_ship_fraction","轻型船占海军比例至少为%p%%","轻型船占海军比例少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("light_ships_in_province","有至少%d艘轻型船","轻型船的数量少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("light_ships_in_province","有来自%s的轻型船","没有来自%s的轻型船",ParadoxType::SCOPE);
	registerSimpleTrigger("likely_rebels","最可能叛乱的叛军类型为%s","最可能叛乱的叛军类型不是%s",ParadoxType::STRING);
	registerSimpleTrigger("local_autonomy","地方自治度至少为%p%%","地方自治度低于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("local_autonomy_above_min","超出最低限度的地方自治度至少为%p%%","超出最低限度的地方自治度少于%p%%",ParadoxType::INTEGER);
	// ===== m =====
	registerSimpleTrigger("march_of","是%s的卫戍国","不是%s的卫戍国",ParadoxType::SCOPE);
	registerSimpleTrigger("manpower","可用人力至少为%d","可用人力少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("manpower_percentage","人力至少为上限的%p%%","人力少于上限的%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("marriage_with","与%s有王室联姻","与%s没有王室联姻",ParadoxType::SCOPE);
	registerSimpleTrigger("max_manpower","最大人力至少为%d","最大人力少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("mercantilism","重商主义至少为%p%%","重商主义少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("mercantilism","拥有至少与%s相同的重商主义","重商主义少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("meritocracy","贤能值至少为%d","贤能值少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("mil","统治者的军事能力至少为%d","统治者的军事能力低于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("mil","统治者的军事能力至少与%s相同","统治者的军事能力低于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("militarised_society","军事化程度至少为%d","军事化程度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("mil_power","军事点数至少为%d","军事点数少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("mil_power","拥有至少与%s相同的军事点数","军事点数少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("mil_tech","军事科技至少为%d","军事科技低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("mission_completed","已完成%s任务","尚未完成%s任务",ParadoxType::STRING);
	registerSingleArgTrigger("monthly_income","每月收入至少为%d","每月收入少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("monthly_income","拥有至少与%s相同的每月收入","每月收入少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("monthly_trade_income","每月贸易收入至少为%d","每月贸易收入少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("monthly_trade_income","拥有至少与%s相同的每月贸易收入","每月贸易收入少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("months_of_ruling","统治者已经执政至少%d个月","统治者执政不足%d个月",ParadoxType::INTEGER);
	registerSimpleTrigger("months_since_defection","省份在%d个月内叛变过","省份在%d个月内没有叛变过",ParadoxType::INTEGER);
	registerSimpleTrigger("nationalism","有至少%d年的分离主义","没有至少%d年的分离主义",ParadoxType::INTEGER);
	registerSimpleTrigger("nationalism_debug","民族主义为%d","民族主义不是%d",ParadoxType::INTEGER);
	registerSimpleTrigger("national_focus","国家焦点为%s","国家焦点不是%s",ParadoxType::STRING);
	registerSimpleTrigger("nation_designer_points","自定义国家点数至少为%d","自定义国家点数少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("native_ferocity","土著凶残度至少为%d","土著凶残度少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("native_hostileness","土著敌对度至少为%d","土著敌对度少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("native_policy","土著政策为%s","土著政策不是%s",ParadoxType::STRING);
	registerSimpleTrigger("native_size","土著数量至少为%d","土著数量少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("naval_forcelimit","海军上限至少为%d","海军上限少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("naval_forcelimit","拥有至少与%s相同的海军上限","海军上限少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("naval_maintenance","海军维护费至少为%p%%","海军维护费低于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("naval_morale","海军士气至少为%p%%","海军士气少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("naval_morale","拥有至少与%s相同的海军士气","海军士气少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("navy_size","海军规模至少为%d艘","海军规模少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("navy_size","拥有至少与%s相同的海军规模","海军规模少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("navy_size_percentage","海军规模至少为上限的%p%%","海军规模少于上限的%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("navy_tradition","海军传统至少为%d","海军传统少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("navy_tradition","拥有至少与%s相同的海军传统","海军传统少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("num_accepted_cultures","接纳的文化至少为%d个","接纳的文化少于%d个",ParadoxType::INTEGER);
	registerSimpleTrigger("num_free_building_slots","剩余建筑槽位至少为%d","剩余建筑槽位少于%d",ParadoxType::INTEGER);
	// ===== o =====
	registerSimpleTrigger("offensive_war_with","正处于对%s的进攻战争","没处于对%s的进攻战争",ParadoxType::SCOPE);
	registerSimpleTrigger("overextension_percentage","过度扩张至少为%p%%","过度扩张少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("overlord_of","是%s的宗主国","不是%s的宗主国",ParadoxType::SCOPE);
	registerSimpleTrigger("overseas_provinces_percentage","海外省份比例至少为%p%%","海外省份比例少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("owned_by","被%s拥有","未被%s拥有",ParadoxType::SCOPE);
	registerSimpleTrigger("owns","拥有省份%s","未拥有省份%s",ParadoxType::SCOPE);
	registerSimpleTrigger("owns_core_province","拥有并核心化了省份%s","未拥有并核心化省份%s",ParadoxType::SCOPE);
	registerSimpleTrigger("owns_or_non_sovereign_subject_of","自己或非朝贡属国拥有省份%s","自己或非朝贡属国未拥有省份%s",ParadoxType::SCOPE);
	registerSimpleTrigger("owns_or_subject_of","自己或属国拥有省份%s","自己或属国未拥有省份%s",ParadoxType::SCOPE);
	registerSimpleTrigger("owned_by_subject_of","被%s的属国所拥有","未被%s的属国所拥有",ParadoxType::SCOPE);
	// ===== p =====
	registerSingleArgTrigger("papal_influence","教廷影响力至少为%d","教廷影响力少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("papal_influence","拥有至少与%s相同的教廷影响力","教廷影响力少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("patriarch_authority","牧首权威至少为%p%%","牧首权威少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("percentage_backing_issue","支持当前议题的席位比例至少为%p%%","支持当前议题的席位比例少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("personality","AI个性为%s","AI个性不是%s",ParadoxType::STRING);
	registerSingleArgTrigger("piety","虔诚至少为%p%%","虔诚少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("piety","拥有至少与%s相同的虔诚","虔诚少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("preferred_emperor","最青睐的皇帝候选为%s","最青睐的皇帝候选不是%s",ParadoxType::SCOPE);
	registerSimpleTrigger("possible_buildings","建筑槽位(含已占用)至少为%d","建筑槽位少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("prestige","威望至少为%d","威望少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("prestige","拥有至少与%s相同的威望","威望少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("previous_owner","前任所有者是%s","前任所有者不是%s",ParadoxType::SCOPE);
	registerSimpleTrigger("power_projection","力量投射至少为%d","力量投射少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("primary_culture","主流文化为%s","主流文化不是%s",ParadoxType::STRING);
	registerSingleArgTrigger("primary_culture","%s的主流文化为%s","%s的主流文化不是%s",ParadoxType::SCOPE);
	registerSimpleTrigger("province_group","属于%s省份组","不属于%s省份组",ParadoxType::STRING);
	registerSimpleTrigger("province_id","省份id为%d","省份id不是%d",ParadoxType::INTEGER);
	registerSimpleTrigger("province_size","省份规模至少为%d像素","省份规模少于%d像素",ParadoxType::INTEGER);
	registerSimpleTrigger("province_trade_power","省份贸易力量至少为%d","省份贸易力量少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("provinces_on_continent","在%s大陆拥有省份","不在%s大陆拥有省份",ParadoxType::STRING);
	registerSimpleTrigger("provinces_on_capital_continent_of","在%s首都所在大陆拥有省份","未在%s首都所在大陆拥有省份",ParadoxType::SCOPE);
	registerSimpleTrigger("pure_unrest","基础叛乱度至少为%d","基础叛乱度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("production_efficiency","生产效率至少为%p%%","生产效率少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("production_efficiency","拥有至少与%s相同的生产效率","生产效率少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("production_income_percentage","生产收入占总收入比例至少为%p%%","生产收入占比少于%p%%",ParadoxType::INTEGER);
	// ===== r =====
	registerSimpleTrigger("range","在%s的殖民/贸易范围内","不在%s的殖民/贸易范围内",ParadoxType::SCOPE);
	registerSimpleTrigger("real_day_of_year","真实日期中的一年第%d天已过","真实日期未到一年第%d天",ParadoxType::INTEGER);
	registerSimpleTrigger("real_month_of_year","真实月份至少为%d","真实月份早于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("reform_desire","改革渴望至少为%p%%","改革渴望少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("receives_military_access_from","从%s获得军事通行权","未从%s获得军事通行权",ParadoxType::SCOPE);
	registerSimpleTrigger("receives_fleet_basing_rights_from","从%s获得舰队停泊权","未从%s获得舰队停泊权",ParadoxType::SCOPE);
	registerNoArgTrigger("recent_treasure_ship_passage","最近有运宝船队经过该贸易节点","最近没有运宝船队经过该贸易节点");
	registerSimpleTrigger("reform_level","政府改革等级至少为%d","政府改革等级低于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("region","位于%s地区","不位于%s地区",ParadoxType::STRING);
	registerSingleArgTrigger("religion","宗教为%s","宗教不是%s",ParadoxType::STRING);
	registerSingleArgTrigger("religion","与%s信仰相同","与%s信仰不同",ParadoxType::SCOPE);
	registerSimpleTrigger("religion_group","属于%s宗教组","不属于%s宗教组",ParadoxType::STRING);
	registerSimpleTrigger("religious_unity","宗教统一至少为%p%%","宗教统一少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("republican_tradition","共和传统至少为%d","共和传统少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("revanchism","复仇主义至少为%d","复仇主义少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("revolt_percentage","发生叛乱的省份比例至少为%p%%","发生叛乱的省份比例少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("revolution_spread","革命思潮传播度至少为%p%%","革命思潮传播度少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("ruler_age","统治者至少%d岁","统治者不足%d岁",ParadoxType::INTEGER);
	registerSimpleTrigger("ruler_consort_marriage_length","统治者与配偶的婚姻持续至少%d年","统治者与配偶的婚姻持续不足%d年",ParadoxType::INTEGER);
	registerSimpleTrigger("ruler_culture","统治者的文化为%s","统治者的文化不是%s",ParadoxType::STRING);
	registerSimpleTrigger("ruler_has_personality","统治者拥有%s特质","统治者没有%s特质",ParadoxType::STRING);
	registerSimpleTrigger("ruler_religion","统治者的宗教为%s","统治者的宗教不是%s",ParadoxType::STRING);
	// ===== s =====
	registerSimpleTrigger("sailors","水手至少为%d","水手少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("sailors_percentage","水手至少为上限的%p%%","水手少于上限的%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("max_sailors","最大水手至少为%d","最大水手少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("same_continent","与%s位于同一大陆","与%s不在同一大陆",ParadoxType::SCOPE);
	registerSimpleTrigger("same_trade_node_as","与%s位于同一贸易节点","与%s不在同一贸易节点",ParadoxType::SCOPE);
	registerSimpleTrigger("same_home_trade_node_as","与%s拥有相同的主要贸易节点","与%s的主要贸易节点不同",ParadoxType::SCOPE);
	registerSimpleTrigger("secondary_religion","相容宗教为%s","相容宗教不是%s",ParadoxType::STRING);
	registerSimpleTrigger("secondary_religion_group","相容宗教属于%s宗教组","相容宗教不属于%s宗教组",ParadoxType::STRING);
	registerSimpleTrigger("senior_union_with","是%s的联统主导方","不是%s的联统主导方",ParadoxType::SCOPE);
	registerSimpleTrigger("sieged_by","正被%s围攻","没有被%s围攻",ParadoxType::SCOPE);
	registerSimpleTrigger("splendor","辉煌度至少为%d","辉煌度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("stability","稳定度至少为%d","稳定度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("stability","拥有至少与%s相同的稳定度","稳定度少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("start_date","起始日期为%s","起始日期不是%s",ParadoxType::DATE);
	registerSimpleTrigger("started_in","起始日期为%s或更晚","起始日期早于%s",ParadoxType::DATE);
	registerSimpleTrigger("statists_vs_orangists","议会派与奥兰治派对比至少为%p%%","议会派与奥兰治派对比少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("subject_owns","属国拥有省份%s","属国未拥有省份%s",ParadoxType::SCOPE);
	registerSimpleTrigger("succession_claim","已宣称%s的王位","未宣称%s的王位",ParadoxType::SCOPE);
	registerSimpleTrigger("superregion","位于%s大区","不位于%s大区",ParadoxType::STRING);
	registerSingleArgTrigger("supply_limit","补给上限至少为%d","补给上限少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("supply_limit","拥有至少与%s相同的补给上限","补给上限少于%s",ParadoxType::SCOPE);
	// ===== t =====
	registerSimpleTrigger("tariff_value","殖民地关税至少为%p%%","殖民地关税少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("tax_income_percentage","税收收入占总收入比例至少为%p%%","税收收入占比少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("tech_difference","科技领先至少%d级","科技领先不足%d级",ParadoxType::INTEGER);
	registerSimpleTrigger("technology_group","科技组为%s","科技组不是%s",ParadoxType::STRING);
	registerSimpleTrigger("tolerance_to_this","对该宗教的容忍至少为%d","对该宗教的容忍少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("total_base_tax","总基础税收至少为%d","总基础税收少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("total_base_tax","拥有至少与%s相同的总基础税收","总基础税收少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("total_development","总发展度至少为%d","总发展度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("total_development","拥有比%s更高的总发展度","总发展度不高于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("total_number_of_cardinals","枢机主教总数至少为%d","枢机主教总数少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("total_own_and_non_tributary_subject_development","本国及非朝贡属国总发展度至少为%d","本国及非朝贡属国总发展度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("total_own_and_non_tributary_subject_development","本国及非朝贡属国总发展度高于%s","本国及非朝贡属国总发展度不高于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("transports_in_province","有至少%d艘运输船","运输船的数量少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("transports_in_province","有来自%s的运输船","没有来自%s的运输船",ParadoxType::SCOPE);
	registerSimpleTrigger("trade_company_region","属于%s贸易公司区域","不属于%s贸易公司区域",ParadoxType::STRING);
	registerSimpleTrigger("trade_company_size","贸易公司至少有%d个省份","贸易公司省份少于%d个",ParadoxType::INTEGER);
	registerSimpleTrigger("trade_efficiency","贸易效率至少为%p%%","贸易效率少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("trade_embargoing","正在禁运%s","没有禁运%s",ParadoxType::SCOPE);
	registerSimpleTrigger("trade_embargo_by","正被%s禁运","没有被%s禁运",ParadoxType::SCOPE);
	registerSimpleTrigger("trade_goods","贸易品为%s","贸易品不是%s",ParadoxType::STRING);
	registerSimpleTrigger("trade_income_percentage","贸易收入占总收入比例至少为%p%%","贸易收入占比少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("trade_node_value","贸易节点总价值至少为%d","贸易节点总价值少于%d",ParadoxType::INTEGER);
	registerSimpleTrigger("trade_range","在%s的贸易范围内","不在%s的贸易范围内",ParadoxType::SCOPE);
	registerSimpleTrigger("transfers_trade_power_to","正在向%s转移贸易力量","没有向%s转移贸易力量",ParadoxType::SCOPE);
	registerSimpleTrigger("transport_fraction","运输船占海军比例至少为%p%%","运输船占海军比例少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("tribal_allegiance","部落忠诚至少为%d","部落忠诚少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("tribal_development","部落发展度至少为%d","部落发展度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("tribal_development","拥有比%s更高的部落发展度","部落发展度不高于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("tribal_land_of","是%s的部落土地","不是%s的部落土地",ParadoxType::SCOPE);
	registerSimpleTrigger("truce_with","与%s有停战协议","与%s没有停战协议",ParadoxType::SCOPE);
	// ===== u =====
	registerNoArgTrigger("unit_has_leader","有单位拥有将领","没有单位拥有将领");
	registerSingleArgTrigger("units_in_province","有至少%d个单位","单位数量少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("units_in_province","有来自%s的单位","没有来自%s的单位",ParadoxType::SCOPE);
	registerSimpleTrigger("unit_type","兵种类型为%s","兵种类型不是%s",ParadoxType::STRING);
	registerSimpleTrigger("unrest","叛乱度至少为%d","叛乱度少于%d",ParadoxType::INTEGER);
	// ===== v/w/y =====
	registerSimpleTrigger("vassal_of","是%s的附庸","不是%s的附庸",ParadoxType::SCOPE);
	registerSingleArgTrigger("war_exhaustion","厌战度至少为%d","厌战度少于%d",ParadoxType::INTEGER);
	registerSingleArgTrigger("war_exhaustion","拥有至少与%s相同的厌战度","厌战度少于%s",ParadoxType::SCOPE);
	registerSimpleTrigger("war_score","战争分数至少为%p%%","战争分数少于%p%%",ParadoxType::INTEGER);
	registerSimpleTrigger("war_with","正在与%s交战","没有与%s交战",ParadoxType::SCOPE);
	registerSimpleTrigger("yearly_corruption_increase","年度腐败增长至少为%p%%","年度腐败增长少于%p%%",ParadoxType::INTEGER);
	registerSingleArgTrigger("years_of_income","国库存款至少相当于%d年收入","国库存款不足%d年收入",ParadoxType::INTEGER);
	registerSingleArgTrigger("years_of_income","国库存款至少相当于%s的年度收入","国库存款少于%s的年度收入",ParadoxType::SCOPE);
	registerSimpleTrigger("years_of_manpower","人力池至少相当于%d年人力","人力池不足%d年人力",ParadoxType::INTEGER);
	registerSimpleTrigger("years_of_sailors","水手池至少相当于%d年水手","水手池不足%d年水手",ParadoxType::INTEGER);
	registerSimpleTrigger("is_mod_active","启用了%s模组","未启用%s模组",ParadoxType::STRING);
	// ===== personal/unit extra =====
	registerSimpleTrigger("personal_union","拥有至少%d个联统属国","联统属国少于%d个",ParadoxType::INTEGER);
	// ===== num_of_* integer family =====
	registerSingleArgTrigger("num_of_janissaries","至少拥有%d队阿哈提","阿哈提少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_active_blessings","至少启用%d项牧首神赐","牧首神赐少于%d项",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_admirals","至少拥有%d名海军将领","海军将领少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_admirals_with_traits","至少拥有%d名带特质的海军将领","带特质的海军将领少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_allies","至少拥有%d个盟友","盟友少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_artillery","至少拥有%d队炮兵","炮兵少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_aspects","至少启用%d项教会信条","教会信条少于%d项",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_banners","至少拥有%d队变形者","变形者少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_buildings_in_province","至少拥有%d座建筑","建筑少于%d座",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_captured_ships_with_boarding_doctrine","登船作战学说下至少俘获%d艘船","俘获船只少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_centers_of_trade","至少拥有%d个贸易中心","贸易中心少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_cardinals","在教廷中至少拥有%d名枢机主教","枢机主教少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_caravel","至少拥有%d艘卡拉维尔帆船","卡拉维尔帆船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_cavalry","至少拥有%d队骑兵","骑兵少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_cawa","至少拥有%d队巡林客","巡林客少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_carolean","至少拥有%d队钨钢步兵","钨钢步兵少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_cities","至少拥有%d座城市","城市少于%d座",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_coalition_members","包围网至少包含%d个成员","包围网成员少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_colonies","至少拥有%d个未完工殖民地","未完工殖民地少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_colonists","至少拥有%d名殖民者","殖民者少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_conquistadors","至少拥有%d名征服者","征服者少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_consorts","统治者至少有过%d位配偶","配偶少于%d位",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_continents","至少在%d个大洲拥有省份","拥有省份的大洲少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_cossacks","至少拥有%d队奥拉恰夫","奥拉恰夫少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_custom_nations","游戏中至少有%d个自定义国家","自定义国家少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_different_religions_in_court","宫廷中至少有%d名不同宗教的顾问","宫廷中不同宗教的顾问少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_different_cultures_in_court","宫廷中至少有%d名不同文化的顾问","宫廷中不同文化的顾问少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_diplomatic_relations","至少占用%d个外交关系","外交关系少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_diplomats","至少拥有%d名外交官","外交官少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_electors","神罗至少有%d名选帝侯","选帝侯少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_explorers","至少拥有%d名探险家","探险家少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_federation_advancements","至少拥有%d项联邦进步","联邦进步少于%d项",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_foreign_hre_provinces","至少存在%d个非成员持有的神罗省份","非成员持有的神罗省份少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_free_diplomatic_relations","至少拥有%d个空闲外交关系槽","空闲外交关系槽少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_galley","至少拥有%d艘桨帆船","桨帆船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_galleon","至少拥有%d艘海兽","海兽少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_galleass","至少拥有%d艘加莱塞战船","加莱塞战船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_generals","至少拥有%d名陆军将领","陆军将领少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_generals_with_traits","至少拥有%d名带特质的陆军将领","带特质的陆军将领少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_geobukseon","至少拥有%d艘万桨宝舰","万桨宝舰少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_harmonized","至少已调和%d种宗教或宗教组","已调和的宗教少于%d种",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_heavy_ship","至少拥有%d艘重型船","重型船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_hussars","至少拥有%d队神话骑兵","神话骑兵少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_infantry","至少拥有%d队步兵","步兵少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_light_ship","至少拥有%d艘轻型船","轻型船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_loans","至少拥有%d笔贷款","贷款少于%d笔",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_mamluks","至少拥有%d队暗影战士","暗影战士少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_man_of_war","至少拥有%d艘巨兽舰","巨兽舰少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_marches","至少拥有%d个卫戍国","卫戍国少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_marines","至少拥有%d队海军陆战队","海军陆战队少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_mercenaries","至少拥有%d队雇佣兵","雇佣兵少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_merchants","至少拥有%d名商人","商人少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_missionaries","至少拥有%d名传教士","传教士少于%d名",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_musketeers","至少拥有%d队火枪手","火枪手少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_owned_and_controlled_institutions","拥有并控制至少%d个思潮发源地省份","思潮发源地省份少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_pashas","至少拥有%d个帕夏","帕夏少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_ports","至少拥有%d个本土港口","本土港口少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_ports_blockading","至少封锁%d个港口","封锁的港口少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_powerful_estates","至少拥有%d个影响力超过70的阶层","强势阶层少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_protectorates","至少拥有%d个受保护国","受保护国少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_provinces_in_states","直属州内至少有%d个省份","直属州内省份少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_provinces_in_territories","自治领内至少有%d个省份","自治领内省份少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_qizilbash","至少拥有%d队奇兹尔巴什","奇兹尔巴什少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_rajput","至少拥有%d队奇械术士兵团","奇械术士兵团少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_regiments_at_full_drill","至少拥有%d队满操练度的部队","满操练度的部队少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_rebel_armies","至少有%d支叛军","叛军少于%d支",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_rebel_controlled_provinces","至少有%d个省份被叛军控制","叛军控制省份少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_revolts","至少有%d次叛乱","叛乱少于%d次",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_revolutionary_guard","至少拥有%d队革命卫队","革命卫队少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_royal_marriages","至少拥有%d个王室联姻","王室联姻少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_samurai","至少拥有%d队武侠","武侠少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_streltsy","至少拥有%d队射击军","射击军少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_states","至少拥有%d个直属州","直属州少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_strong_trade_companies","至少拥有%d个强势贸易公司","强势贸易公司少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_subjects","至少拥有%d个属国","属国少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_tercio","至少拥有%d队西班牙方阵","西班牙方阵少于%d队",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_territories","至少拥有%d个自治领地区","自治领地区少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_times_improved","省份发展度至少被提升过%d次","发展度提升次数少于%d次",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_times_improved_by_owner","当前所有者至少提升过%d次发展度","当前所有者提升发展度少于%d次",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_times_used_pillage_capital","至少使用过%d次洗劫首都条款","使用洗劫首都少于%d次",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_times_used_transfer_development","至少使用过%d次集中发展度","使用集中发展度少于%d次",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_times_expanded_infrastructure","省份基础设施至少扩建过%d次","基础设施扩建少于%d次",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_total_ports","至少拥有%d个港口(全球)","港口少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_trade_companies","至少拥有%d个贸易公司","贸易公司少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_trade_embargos","至少实行%d项贸易禁运","贸易禁运少于%d项",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_trading_bonuses","至少拥有%d项贸易加成","贸易加成少于%d项",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_transport","至少拥有%d艘运输船","运输船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_trusted_allies","至少拥有%d个满信任盟友","满信任盟友少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_unlocked_cults","至少解锁%d个崇拜","解锁的崇拜少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_voc_indiamen","至少拥有%d艘逐日者商船","逐日者商船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_of_war_reparations","至少从%d个国家收取战争赔款","收取战争赔款的国家少于%d个",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_mothballed_forts","至少拥有%d座封存要塞","封存要塞少于%d座",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_ships_privateering","至少拥有%d艘私掠船","私掠船少于%d艘",ParadoxType::INTEGER);
	registerSingleArgTrigger("num_ships_protecting_trade","至少拥有%d艘护航商船","护航商船少于%d艘",ParadoxType::INTEGER);
	// scope overloads for common num_of_* comparisons
	registerSingleArgTrigger("num_of_admirals","拥有至少与%s相同数量的海军将领","海军将领少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_allies","拥有至少与%s相同数量的盟友","盟友少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_artillery","拥有至少与%s相同数量的炮兵","炮兵少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_cardinals","拥有至少与%s相同数量的枢机主教","枢机主教少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_cavalry","拥有至少与%s相同数量的骑兵","骑兵少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_cities","拥有至少与%s相同数量的城市","城市少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_colonies","拥有至少与%s相同数量的殖民地","殖民地少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_explorers","拥有至少与%s相同数量的探险家","探险家少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_galley","拥有至少与%s相同数量的桨帆船","桨帆船少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_generals","拥有至少与%s相同数量的陆军将领","陆军将领少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_heavy_ship","拥有至少与%s相同数量的重型船","重型船少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_infantry","拥有至少与%s相同数量的步兵","步兵少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_light_ship","拥有至少与%s相同数量的轻型船","轻型船少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_loans","拥有至少与%s相同数量的贷款","贷款少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_marches","拥有至少与%s相同数量的卫戍国","卫戍国少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_mercenaries","拥有至少与%s相同数量的雇佣兵","雇佣兵少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_ports","拥有至少与%s相同数量的本土港口","本土港口少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_rebel_armies","拥有至少与%s相同数量的叛军","叛军少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_rebel_controlled_provinces","拥有至少与%s相同数量的叛军控制省份","叛军控制省份少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_revolts","拥有至少与%s相同数量的叛乱","叛乱少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_royal_marriages","拥有至少与%s相同数量的王室联姻","王室联姻少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_total_ports","拥有至少与%s相同数量的港口","港口少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_transport","拥有至少与%s相同数量的运输船","运输船少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_of_war_reparations","收取战争赔款的国家至少与%s一样多","收取战争赔款的国家少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_ships_privateering","私掠船至少与%s一样多","私掠船少于%s",ParadoxType::SCOPE);
	registerSingleArgTrigger("num_ships_protecting_trade","护航商船至少与%s一样多","护航商船少于%s",ParadoxType::SCOPE);
	registerNumberRequiredTrigger("development_in_provinces","value","符合条件省份的总发展度至少为%d","符合条件省份的总发展度少于%d");
	registerSimpleClauseTrigger("employed_advisor",new TriggerItem(registerShortString("employed_advisor"),
		{},
		{"category","skill","religion","culture","type","is_male","is_discounted"},
		{ParadoxType::STRING,ParadoxType::INTEGER,ParadoxType::STRING,ParadoxType::STRING,ParadoxType::STRING,ParadoxType::BOOLEAN,ParadoxType::BOOLEAN},
		{},
		std::bitset<64>(),
		[](std::vector<ParadoxBase*> vec,bool reversed){
			std::string ret = reversed ? "没有雇佣一个" : "已经雇佣了一个";
			if(vec[6] != nullptr){
				const char* discount_str = vec[5]->getAsBoolean()->getValue() ? "有折扣的" : "没有折扣的";
				ret.append(discount_str);
			}
			if(vec[2] != nullptr){
				std::string religion = vec[2]->getAsString()->getStringContent();
				ret.append(applyPattern("信奉%s的",religion));
			}
			if(vec[3] != nullptr){
				std::string culture = vec[3]->getAsString()->getStringContent();
				ret.append(applyPattern("%s文化的",culture));
			}
			if(vec[1] != nullptr){
				ret.append(applyPattern("%d级",vec[1]->getAsInteger()->getIntegerContent()));
			}
			if(vec[5] != nullptr){
				ret.append(vec[5]->getAsBoolean()->getValue() ? "男性" : "女性");
			}
			if(vec[4] != nullptr){
				ret.append(getLocalization(vec[4]->getAsString()->getStringContent()));
			}
			else if(vec[0] != nullptr){
				std::string type = toUpperCase(vec[0]->getAsString()->getStringContent());
				if(type == "MIL") ret.append("军事顾问");
				else if(type == "DIP") ret.append("外交顾问");
				else if(type == "ADM") ret.append("行政顾问");
			}
			else ret.append("顾问");
			return ret;
		}
	));



	registerSimpleClauseTrigger("estate_influence",new TriggerItem(registerShortString("estate_influence"),
		{"%s阶层的影响力至少为%d","%s阶层的影响力少于%d"},
		{"estate","influence"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("estate_led_regency_influence",new TriggerItem(registerShortString("estate_led_regency_influence"),
		{"领导摄政的阶层影响力至少为%d","领导摄政的阶层影响力少于%d"},
		{"value"},
		{ParadoxType::INTEGER},
		{0}
	));
	registerSimpleClauseTrigger("estate_led_regency_loyalty",new TriggerItem(registerShortString("estate_led_regency_loyalty"),
		{"领导摄政的阶层忠诚度至少为%d","领导摄政的阶层忠诚度少于%d"},
		{"value"},
		{ParadoxType::INTEGER},
		{0}
	));
	registerSimpleClauseTrigger("estate_loyalty",new TriggerItem(registerShortString("estate_loyalty"),
		{"%s阶层的忠诚度至少为%d","%s阶层的忠诚度少于%d"},
		{"estate","loyalty"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("num_of_estate_agendas_completed",new TriggerItem(registerShortString("num_of_estate_agendas_completed"),
		{"%s阶层已完成至少%d项议程","%s阶层完成的议程少于%d项"},
		{"estate","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("estate_territory",new TriggerItem(registerShortString("estate_territory"),
		{"%s阶层控制至少%p%%的总发展度","%s阶层控制少于%p%%的总发展度"},
		{"estate","territory"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("faction_influence",new TriggerItem(registerShortString("faction_influence"),
		{"%s派系的影响力至少为%d","%s派系的影响力少于%d"},
		{"faction","influence"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("had_active_policy",new TriggerItem(registerShortString("had_active_policy"),
		{"%s政策已持续生效至少%d天","%s政策持续生效不足%d天"},
		{"policy","days"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("had_consort_flag",new TriggerItem(registerShortString("had_consort_flag"),
		{"配偶标签%s已在%d天前设置","配偶标签%s在%d天内设置过"},
		{"flag","days"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("had_country_flag",new TriggerItem(registerShortString("had_country_flag"),
		{"国家标签%s已在至少%d天前设置","国家标签%s在%d天内设置过"},
		{"flag","days"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("had_global_flag",new TriggerItem(registerShortString("had_global_flag"),
		{"全局标签'%s'已在至少%d天前设置","全局标签'%s'在%d天内设置过"},
		{"flag","days"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("had_heir_flag",new TriggerItem(registerShortString("had_heir_flag"),
		{"继承人标签'%s'已在至少%d天前设置","继承人标签'%s'在%d天内设置过"},
		{"flag","days"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("had_province_flag",new TriggerItem(registerShortString("had_province_flag"),
		{"省份标签'%s'已在至少%d天前设置","省份标签'%s'在%d天内设置过"},
		{"flag","days"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("had_ruler_flag",new TriggerItem(registerShortString("had_ruler_flag"),
		{"统治者标签'%s'已在至少%d天前设置","统治者标签'%s'在%d天内设置过"},
		{"flag","days"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_administration_efficiency_value",new TriggerItem(registerShortString("has_administration_efficiency_value"),
		{"行政效率至少为%p%%","行政效率少于%p%%"},
		{"value"},
		{ParadoxType::INTEGER},
		{0}
	));
	registerSimpleClauseTrigger("has_casus_belli",new TriggerItem(registerShortString("has_casus_belli"),
		{"有针对%s的%s宣战理由","没有针对%s的%s宣战理由"},
		{"target","type"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("has_disaster_progress",new TriggerItem(registerShortString("has_disaster_progress"),
		{"%s灾难进度至少为%d%%","%s灾难进度少于%d%%"},
		{"disaster","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_estate_influence_modifier",new TriggerItem(registerShortString("has_estate_influence_modifier"),
		{"%s阶层拥有影响力修正%s","%s阶层没有影响力修正%s"},
		{"estate","modifier"},
		{ParadoxType::STRING,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("has_estate_loyalty_modifier",new TriggerItem(registerShortString("has_estate_loyalty_modifier"),
		{"%s阶层拥有忠诚度修正%s","%s阶层没有忠诚度修正%s"},
		{"estate","modifier"},
		{ParadoxType::STRING,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("has_global_modifier_value",new TriggerItem(registerShortString("has_global_modifier_value"),
		{"全局修正%s的数值至少为%d","全局修正%s的数值少于%d"},
		{"which","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_government_power",new TriggerItem(registerShortString("has_government_power"),
		{"%s机制的%s数值至少为%d","%s机制的%s数值少于%d"},
		{"mechanic_type","power_type","value"},
		{ParadoxType::STRING,ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1,2}
	));
	registerSimpleClauseTrigger("has_great_project",new TriggerItem(registerShortString("has_great_project"),
		{"拥有%s等级%d或以上的奇观","没有%s等级%d或以上的奇观"},
		{"type","tier"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_leader_with",new TriggerItem(registerShortString("has_leader_with"),
		{},
		{"fire","shock","maneuver","siege","total_pips","admiral","general","is_monarch_leader"},
		{ParadoxType::INTEGER,ParadoxType::INTEGER,ParadoxType::INTEGER,ParadoxType::INTEGER,ParadoxType::BOOLEAN,ParadoxType::BOOLEAN,ParadoxType::BOOLEAN},
		{},
		std::bitset<64>(),
		[](std::vector<ParadoxBase*> vec,bool reversed){
			std::string ret = reversed ? "没有一个" : "有一个";
			std::string post("");
			if(vec[0] != nullptr) post.append(applyPattern("火力点数至少为%d",vec[0]->getAsInteger()->getIntegerContent()));
			if(vec[1] != nullptr) {
				if(!post.empty()) post.push_back(',');
				post.append(applyPattern("冲击点数至少为%d",vec[1]->getAsInteger()->getIntegerContent()));
			}
			if(vec[2] != nullptr){
				if(!post.empty()) post.push_back(',');
				post.append(applyPattern("机动点数至少为%d",vec[1]->getAsInteger()->getIntegerContent()));
			}
			if(vec[3] != nullptr){
				if(!post.empty()) post.push_back(',');
				post.append(applyPattern("围城点数至少为%d",vec[1]->getAsInteger()->getIntegerContent()));
			}
			if(vec[4] != nullptr){
				if(!post.empty()) post.push_back(',');
				post.append(applyPattern("总点数至少为%d",vec[1]->getAsInteger()->getIntegerContent()));
			}
			if(!post.empty()) post.append("的");
			if(vec[5] != nullptr) post.append("陆军将领");
			else if(vec[6] != nullptr) post.append("海军将领");
			else if(vec[7] != nullptr) post.append("君主将领");
			ret.append(post);
			return ret;
		}
	));
	registerSimpleClauseTrigger("has_leaders",new TriggerItem(registerShortString("has_leaders"),
		{},
		{"value","type","include_monarch","include_heir"},
		{ParadoxType::INTEGER,ParadoxType::STRING,ParadoxType::BOOLEAN,ParadoxType::BOOLEAN},
		{},
		std::bitset<64>(0x0000'0000'0000'0003),
		[](std::vector<ParadoxBase*> vec,bool reversed){
			std::string ret("");
			if(vec[1]->getAsString()->getStringContent() == "general") ret.append("陆军将领数量");
			else ret.append("海军将领数量");
			ret.append(reversed ? "少于" : "至少为");
			ret.append(std::to_string(vec[0]->getAsInteger()->getIntegerContent()));
			if(vec[2] != nullptr) {
				if(vec[3] != nullptr) ret.append("(不包含君主和继承人将领)");
				else ret.append("(不包含君主将领)");
			} 
			else ret.append("(不包含继承人将领)");
			return ret;
		}
	));
	registerSimpleClauseTrigger("has_local_modifier_value",new TriggerItem(registerShortString("has_local_modifier_value"),
		{"省份修正%s的数值至少为%d","省份修正%s的数值少于%d"},
		{"which","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_opinion",new TriggerItem(registerShortString("has_opinion"),
		{"对%s的看法至少为%d","对%s的看法低于%d"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_opinion_modifier",new TriggerItem(registerShortString("has_opinion_modifier"),
		{"对%s拥有%s看法修正","对%s没有%s看法修正"},
		{"who","modifier"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("has_privateer_share_in_trade_node",new TriggerItem(registerShortString("has_privateer_share_in_trade_node"),
		{"%s在该节点的私掠贸易份额至少为%p%%","%s在该节点的私掠贸易份额少于%p%%"},
		{"who","share"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_protecting_trade_share_in_trade_node",new TriggerItem(registerShortString("has_protecting_trade_share_in_trade_node"),
		{"%s在该节点护航的贸易份额至少为%p%%","%s在该节点护航的贸易份额少于%p%%"},
		{"who","share"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_spy_network_from",new TriggerItem(registerShortString("has_spy_network_from"),
		{"%s在此的间谍网至少为%d","%s在此的间谍网少于%d"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_spy_network_in",new TriggerItem(registerShortString("has_spy_network_in"),
		{"在%s的间谍网至少为%d","在%s的间谍网少于%d"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("has_trade_company_investment_in_area",new TriggerItem(registerShortString("has_trade_company_investment_in_area"),
		{"%s在该地区拥有%s贸易公司投资","%s在该地区没有%s贸易公司投资"},
		{"investor","investment"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("has_trade_modifier",new TriggerItem(registerShortString("has_trade_modifier"),
		{"%s在该贸易节点拥有%s贸易修正","%s在该贸易节点没有%s贸易修正"},
		{"who","key"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("has_won_war_against",new TriggerItem(registerShortString("has_won_war_against"),
		{"在最近%d年内赢得过对%s的战争","在最近%d年内没有赢得对%s的战争"},
		{"max_years_since","who"},
		{ParadoxType::INTEGER,ParadoxType::SCOPE},
		{0,1}
	));
	registerSimpleClauseTrigger("incident_variable_value",new TriggerItem(registerShortString("incident_variable_value"),
		{"%s事变的数值至少为%d","%s事变的数值少于%d"},
		{"incident","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("institution_difference",new TriggerItem(registerShortString("institution_difference"),
		{"比%s多接纳至少%d个思潮","比%s多接纳的思潮少于%d个"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("is_ahead_of_time_in_technology",new TriggerItem(registerShortString("is_ahead_of_time_in_technology"),
		{"在%s科技上超前于时代","在%s科技上未超前于时代"},
		{"tech"},
		{ParadoxType::STRING},
		{0}
	));
	registerSimpleClauseTrigger("is_or_was_tag",new TriggerItem(registerShortString("is_or_was_tag"),
		{"现在或曾经是%s","现在和以前都不是%s"},
		{"tag"},
		{ParadoxType::STRING},
		{0}
	));
	registerSimpleClauseTrigger("is_subject_of_type_with_overlord",new TriggerItem(registerShortString("is_subject_of_type_with_overlord"),
		{"是%s的%s属国","不是%s的%s属国"},
		{"who","type"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("military_strength",new TriggerItem(registerShortString("military_strength"),
		{"军事实力至少为%s的%d倍","军事实力少于%s的%d倍"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("naval_strength",new TriggerItem(registerShortString("naval_strength"),
		{"海军规模至少为%s的%d倍","海军规模少于%s的%d倍"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("num_of_estate_privileges",new TriggerItem(registerShortString("num_of_estate_privileges"),
		{"%s阶层已被授予至少%d项特权","%s阶层被授予的特权少于%d项"},
		{"estate","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("num_of_religion",new TriggerItem(registerShortString("num_of_religion"),
		{"%s宗教省份占比至少为%p%%","%s宗教省份占比少于%p%%"},
		{"religion","value","secondary"},
		{ParadoxType::STRING,ParadoxType::INTEGER,ParadoxType::BOOLEAN},
		{0,1}
	));
	registerSimpleClauseTrigger("num_of_units_in_province",new TriggerItem(registerShortString("num_of_units_in_province"),
		{"省份中有来自%s的至少%d个单位","省份中来自%s的单位少于%d个"},
		{"who","type","amount"},
		{ParadoxType::SCOPE,ParadoxType::STRING,ParadoxType::INTEGER},
		{0,2}
	));
	registerSimpleClauseTrigger("num_investments_in_trade_company_region",new TriggerItem(registerShortString("num_investments_in_trade_company_region"),
		{"贸易公司区域中至少有%d个%s投资","贸易公司区域中%s投资少于%d个"},
		{"investment","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{1,0}
	));
	registerSimpleClauseTrigger("num_of_special_units",new TriggerItem(registerShortString("num_of_special_units"),
		{"至少拥有%d队%s特殊兵种","特殊兵种%s少于%d队"},
		{"type","special_unit_category","value"},
		{ParadoxType::STRING,ParadoxType::STRING,ParadoxType::INTEGER},
		{1,2}
	));
	registerSimpleClauseTrigger("owes_favors",new TriggerItem(registerShortString("owes_favors"),
		{"欠%s%d点人情","欠%s的人情少于%d点"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("production_leader",new TriggerItem(registerShortString("production_leader"),
		{"是%s的主要生产国","不是%s的主要生产国"},
		{"trade_goods"},
		{ParadoxType::STRING},
		{0}
	));
	registerSimpleClauseTrigger("privateer_power",new TriggerItem(registerShortString("privateer_power"),
		{"%s在此节点的私掠贸易力量至少为%p%%","%s在此节点的私掠贸易力量少于%p%%"},
		{"country","share"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("rebel_progress_at_least",new TriggerItem(registerShortString("rebel_progress_at_least"),
		{"%s叛军的叛乱进度至少为%d","%s叛军的叛乱进度少于%d"},
		{"rebel_type","value"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("religious_school",new TriggerItem(registerShortString("religious_school"),
		{"信奉%s学派的%s教派","不信仰%s学派的%s教派"},
		{"group","school"},
		{ParadoxType::STRING,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("reverse_has_opinion",new TriggerItem(registerShortString("reverse_has_opinion"),
		{"%s对我的看法至少为%d","%s对我的看法低于%d"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("reverse_has_opinion_modifier",new TriggerItem(registerShortString("reverse_has_opinion_modifier"),
		{"%s对我拥有%s看法修正","%s对我没有%s看法修正"},
		{"who","modifier"},
		{ParadoxType::SCOPE,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("subsidised_percent_amount",new TriggerItem(registerShortString("subsidised_percent_amount"),
		{"收到相当于每月收入%p%%的补贴","收到的补贴不足每月收入的%p%%"},
		{"value"},
		{ParadoxType::INTEGER},
		{0}
	));
	registerSimpleClauseTrigger("total_losses_in_won_wars",new TriggerItem(registerShortString("total_losses_in_won_wars"),
		{"%s在输给%s的战争中损失至少%d军队","%s在输给%s的战争中损失不足%d军队"},
		{"winner","looser","casualties"},
		{ParadoxType::SCOPE,ParadoxType::SCOPE,ParadoxType::INTEGER},
		{1,0,2}
	));
	registerSimpleClauseTrigger("trade_goods_produced_amount",new TriggerItem(registerShortString("trade_goods_produced_amount"),
		{"至少生产%d单位的%s","生产的%s不足%d单位"},
		{"trade_goods","amount"},
		{ParadoxType::STRING,ParadoxType::INTEGER},
		{1,0}
	));
	registerSimpleClauseTrigger("trade_share",new TriggerItem(registerShortString("trade_share"),
		{"%s在该贸易节点掌控至少%p%%的贸易","%s在该贸易节点掌控少于%p%%的贸易"},
		{"country","share"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("trading_bonus",new TriggerItem(registerShortString("trading_bonus"),
		{"拥有%s的贸易加成","没有%s的贸易加成"},
		{"trade_goods"},
		{ParadoxType::STRING},
		{0}
	));
	registerSimpleClauseTrigger("trading_part",new TriggerItem(registerShortString("trading_part"),
		{"掌控至少%p%%的%s世界市场","掌控少于%p%%的%s世界市场"},
		{"value","trade_goods"},
		{ParadoxType::INTEGER,ParadoxType::STRING},
		{0,1}
	));
	registerSimpleClauseTrigger("trading_policy_in_node",new TriggerItem(registerShortString("trading_policy_in_node"),
		{"在该贸易节点实行%s贸易政策","在该贸易节点未实行%s贸易政策"},
		{"policy","node"},
		{ParadoxType::STRING,ParadoxType::SCOPE},
		{0}
	));
	registerSimpleClauseTrigger("trust",new TriggerItem(registerShortString("trust"),
		{"对%s的信任至少为%d","对%s的信任低于%d"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("war_score_against",new TriggerItem(registerShortString("war_score_against"),
		{"对%s的战争分数至少为%p%%","对%s的战争分数少于%p%%"},
		{"who","value"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("years_in_union_under",new TriggerItem(registerShortString("years_in_union_under"),
		{"已处于%s的联统下至少%d年","处于%s联统下不足%d年"},
		{"who","years"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	registerSimpleClauseTrigger("years_in_vassalage_under",new TriggerItem(registerShortString("years_in_vassalage_under"),
		{"已作为%s的附庸至少%d年","作为%s附庸不足%d年"},
		{"who","years"},
		{ParadoxType::SCOPE,ParadoxType::INTEGER},
		{0,1}
	));
	// ===== misc scalar leftovers =====
	registerBooleanTrigger("has_heir","有继承人","没有继承人");
	registerSingleArgTrigger("has_heir","有一个名为%s的继承人","没有名为%s的继承人",ParadoxType::STRING);
	registerSimpleTrigger("has_estate_privilege","已被授予%s阶层特权","未被授予%s阶层特权",ParadoxType::STRING);
	registerSimpleTrigger("has_supply_depot","拥有%s建造的补给站","没有%s建造的补给站",ParadoxType::SCOPE);

}


std::string TriggerItem::toString(std::vector<ParadoxBase*> base,bool reversed){
	if(this->overrideLocalization) return this->overrideLocalization(base,reversed);
	std::string usePattern = reversed ? this->reversePattern : this->pattern;
	if(this->usedParameter.size() == 0) return usePattern;
	Pattern p(usePattern);

	for(int i = 0;i < usedParameter.size();i++){
		int index = usedParameter[i];
		if(index == -1){
			return "<ERROR>";
		}
		else{
			if(index >= (int)base.size() || base[index] == nullptr) return "<ERROR>";
			ParadoxBase* base1 = base[index];
			if(isCastable(base1,parameterType[index])){
				ParadoxBase* base2 = castTo(base1,parameterType[index]);
				bool success = false;

				if(parameterType[index] == ParadoxType::INTEGER){
					success = p.setNextInteger(base2->getAsInteger()->getIntegerContent());
				}
				else if(parameterType[index] == ParadoxType::STRING){
					success = p.setNextString(base2->getAsString()->getStringContent());
				}
				else if(parameterType[index] == ParadoxType::DATE){
					success = p.setNextString(base2->getAsDate()->getDateContent().toString());			
				}
				else if(parameterType[index] == ParadoxType::SCOPE){
					Scope* scope = base2->getAsScope()->getValue();
					if(scope == nullptr) return "<ERROR>";
					success = p.setNextString(scope->toString());
						
				}
				if(!success) return "<ERROR>";
			} 
			else return "<ERROR>";
		}
	}
	return p.getOutput();
}

std::string TriggerItem::toHtml(std::vector<ParadoxBase*> base,bool reversed){
	if(this->overrideLocalization) return this->overrideLocalization(base,reversed);
	std::string usePattern = reversed ? this->reversePattern : this->pattern;
	if(this->usedParameter.size() == 0) return usePattern;
	Pattern p(usePattern);
	for(int i = 0;i < usedParameter.size();i++){
		int index = usedParameter[i];
		if(index == -1){
			return "<ERROR>";
		}
		else{
			if(index >= (int)base.size() || base[index] == nullptr) return "<ERROR>";
			ParadoxBase* base1 = base[index];
			if(isCastable(base1,parameterType[index])){
				ParadoxBase* base2 = castTo(base1,parameterType[index]);
				bool success = false;
				if(parameterType[index] == ParadoxType::INTEGER){
					success = p.setNextInteger(base2->getAsInteger()->getIntegerContent());
				}
				else if(parameterType[index] == ParadoxType::STRING){
					success = p.setNextString(base2->getAsString()->getStringContent());
				}
				else if(parameterType[index] == ParadoxType::DATE){
					success = p.setNextString(base2->getAsDate()->getDateContent().toString());			
				}
				else if(parameterType[index] == ParadoxType::SCOPE){
					Scope* scope = base2->getAsScope()->getValue();
					if(scope == nullptr) return "<ERROR>";
					success = p.setNextString(scope->toHtml());
						
				}
				if(!success) return "<ERROR>";
			} 
			else return "<ERROR>";
		}
	}
	return p.getOutput();
}


TriggerItem::TriggerItem(const std::string& _name,std::pair<std::string,std::string>&& patterns,std::vector<std::string>&& parameterName,std::vector<ParadoxType>&& parameterTypes,std::vector<int>&& usedParameter,std::bitset<64> _requiredParameter,std::function<std::string(std::vector<ParadoxBase*>,bool)> _overrideLocalization,ScopeType scope_type): name(_name){
	this->pattern = patterns.first;
	this->reversePattern = patterns.second;
	this->parameterType = parameterTypes;
	this->usedParameter = usedParameter;
	this->usable_scope = scope_type;
	this->overrideLocalization = _overrideLocalization;
	this->requiredParameter = _requiredParameter;
	for(int i = 0;i < parameterName.size();i++){
		this->parameterName[parameterName[i]] = i;
	}
}

void preInit(const int depth,std::string& str){
	for(int i = 0;i < depth;i++){
		str.append("*");
	}	
}
ComplexTrigger* Trigger::getAsComplexTrigger(){
	if(this->getType() == TriggerType::COMMON) return nullptr;
	return static_cast<ComplexTrigger*>(this);
}

LogicTrigger* Trigger::getAsLogicTrigger(){
	if(this->getType() != TriggerType::LOGIC) return nullptr;
	return static_cast<LogicTrigger*>(this);
}
CommonTrigger* Trigger::getAsCommonTrigger(){
	if(this->getType() != TriggerType::COMMON) return nullptr;
	return static_cast<CommonTrigger*>(this);
}

void ComplexTrigger::putTrigger(Trigger* trigger){
	this->subTriggers.push_back(trigger);
}
void ComplexTrigger::takeOverLifeCycle(){
	if(this->extra_data[0] != 0) return;
	this->extra_data[0] = 1;
	for(Trigger* trigger : this->subTriggers){
		trigger->takeOverLifeCycle();
	}
}
bool ComplexTrigger::hasAnyTrigger(bool(*predicate)(Trigger*)){
	if(predicate(this)) return true;
	for(Trigger* trigger : this->subTriggers){
		if(predicate(trigger)) return true;
	}
	return false;
}

bool ComplexTrigger::foreach(std::function<bool(Trigger*)> action){
	if(!action(this)) return false;
	for(Trigger* trigger : this->subTriggers){
		if(!trigger->foreach(action)) return false;
	}
	return true;
}
ChangeScopeTrigger::ChangeScopeTrigger(Scope* scope) : ComplexTrigger(){
	this->changedScope = scope;
}
CommonTrigger::CommonTrigger(TriggerItem* item) : Trigger(){
	this->item = item;
	this->reversed = false;
}
void CommonTrigger::pushObject(ParadoxBase* obj){
	this->base.push_back(obj);
}


LogicTrigger::LogicTrigger(LogicType logic) : ComplexTrigger(){
	this->type = logic;
}

std::string CommonTrigger::toHtml(bool reversed,int depth){
	std::string str("");
	preInit(depth,str);
	str.append(this->item->toString(this->base,Xor(reversed,this->reversed)));
	return str;
}
std::string CommonTrigger::toString(bool reversed,int depth) const{
	std::string str("");
	preInit(depth,str);
	str.append(this->item->toString(this->base,Xor(reversed,this->reversed)));
	return str;
}
void CommonTrigger::takeOverLifeCycle(){
	if(this->extra_data[0] != 0) return;
	this->extra_data[0] = 1;
	for(int i = 0;i < this->base.size();i++){
		if(this->base[i] != nullptr) this->base[i] = deep_copy(this->base[i]);
	}
}
bool CommonTrigger::hasAnyTrigger(bool(*predicate)(Trigger*)){
	return predicate(this);
}
bool CommonTrigger::foreach(std::function<bool(Trigger*)> action){
	return action(this);
}
std::string ChangeScopeTrigger::toString(bool reversed,int depth) const{
	std::string str("");
	if(this->subTriggers.empty()) return str;
	int cDepth = depth;
	if(this->changedScope != nullptr){
		preInit(depth,str);
		bool should_add_bracket = this->changedScope->getType() != ScopeType::ANY;
		if(should_add_bracket) str.append("(");
		if(this->hasType()){
			str.append(this->isAllType() ? "所有" : "任意");
		}
		str.append(this->changedScope->toString());
		if(should_add_bracket) str.append(")");
		str.append(":\n");
		cDepth++;
	}

	for(int i = 0;i < this->subTriggers.size();i++){
		str.append(this->subTriggers[i]->toString(reversed,cDepth));
		if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
	}
	return str;
}

std::string LogicTrigger::toString(bool reversed,int depth) const{
	std::string str("");
	int size = this->subTriggers.size();
	if(size == 0) return str;
	if(size != 1) preInit(depth,str);
	LogicType actual_type = this->type;
	if(reversed){
		if(actual_type == LogicType::AND) actual_type = LogicType::OR;
		else if(actual_type == LogicType::OR) actual_type = LogicType::AND;
	}
	switch(actual_type){
		case LogicType::AND:
			if(size == 1){
				return this->subTriggers[0]->toString(reversed,depth);
			}
			else{
				if(this->extra_data[1] == 0) str.append("下列条件需全部满足:\n");
				for(int i = 0;i < size;i++){
					LogicTrigger* subtrigger = this->subTriggers[i]->getAsLogicTrigger();
					bool shouldOmit = subtrigger != nullptr && subtrigger->type != LogicType::OR;
					int cDepth = depth + 1;
					if(shouldOmit){
						subtrigger->extra_data[1] = 1;
						//subtrigger->ignored = true; 
						//ignoreCurrentDepth(subtrigger);
						cDepth--;
					} 
					str.append(this->subTriggers[i]->toString(reversed,cDepth));
					if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
				}
			}
			break;
		case LogicType::OR:
			if(size == 1){
				return this->subTriggers[0]->toString(reversed,depth);
			}
			else{
				if(this->extra_data[1] == 0) str.append("下列条件至少满足一个:\n");
				for(int i = 0;i < size;i++){
					LogicTrigger* subtrigger = this->subTriggers[i]->getAsLogicTrigger();
					bool shouldOmit = subtrigger != nullptr && subtrigger->type == LogicType::OR;
					int cDepth = depth + 1;
					if(shouldOmit){
						subtrigger->extra_data[1] = 1;
						cDepth--;
					} 
					str.append(this->subTriggers[i]->toString(reversed,cDepth));
					if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
				}
			}
			break;
		case LogicType::NOT:
			if(size == 1){
				return this->subTriggers[0]->toString(!reversed,depth).append("\n");
			}
			else {
				if(this->extra_data[1] == 0) str.append("下列条件需全部满足:\n");
				for(int i = 0;i < size;i++){
					LogicTrigger* subtrigger = this->subTriggers[i]->getAsLogicTrigger();
					bool shouldOmit = subtrigger != nullptr && subtrigger->type == LogicType::OR;
					int cDepth = depth + 1;
					if(shouldOmit){
						subtrigger->extra_data[1] = 1;
						cDepth--;
					} 
					str.append(this->subTriggers[i]->toString(!reversed,cDepth));
					if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
				}
			}
			break;
	}
	return str;
}

std::string NumberRequiredTrigger::toString(bool reversed,int depth) const{
	std::string str("");
	if(this->subTriggers.empty()) return str;
	preInit(depth,str);
	ParadoxInteger* tmp = new ParadoxInteger(this->getAmount());
	std::vector<ParadoxBase*> base;
	base.push_back(tmp);
	str.append(this->item->toString(base,reversed));
	str.append("\n");
	for(int i = 0;i < this->subTriggers.size();i++){
		str.append(this->subTriggers[i]->toString(reversed,depth + 1));
		if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
	}
	delete tmp; 
	return str;
}

std::string ConditionalTrigger::toString(bool reversed,int depth) const{
	std::string str("");
	if(this->subTriggers.empty()) return str;
	if(this->condition->subTriggers.empty()) {
		if(!this->isElse()) return str;
		preInit(depth,str);
		str.append("否则需满足:\n");
		for(int i = 0;i < this->subTriggers.size();i++){
			str.append(this->subTriggers[i]->toString(reversed,depth + 1));
			if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
		}
		return str;
	}
	preInit(depth,str);
	if(this->isElse()) str.append("否则");
	str.append("当以下条件满足时:\n");
	for(int i = 0;i < this->condition->subTriggers.size();i++){
		str.append(this->condition->subTriggers[i]->toString(false,depth + 1));
		if(this->condition->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
	}
	preInit(depth,str);
	str.append("需满足下列要求:\n");
	for(int i = 0;i < this->subTriggers.size();i++){
		str.append(this->subTriggers[i]->toString(reversed,depth + 1));
		if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
	}
	//str.append("\n");
	
	return str;
}

std::string HiddenTrigger::toString(bool reversed,int depth) const{
	std::string str("");
	if(this->subTriggers.empty()) return str;
	if(this->isCurrentHidden()){
		return str;
	}
	preInit(depth,str);
	str.append("(隐藏条件):\n");
	for(int i = 0;i < this->subTriggers.size();i++){
		str.append(this->subTriggers[i]->toString(reversed,depth + 1));
		if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
	}
	return str;
}

std::string CustomTooltipTrigger::toString(bool reversed,int depth) const{
	std::string str("");
	if(this->subTriggers.empty()) return str;
	if(!this->isShowOrigin()){
		preInit(depth,str);
		str.append(this->tooltip);
		str.append("\n");
	}
	else{
		for(int i = 0;i < this->subTriggers.size();i++){
			str.append(this->subTriggers[i]->toString(reversed,depth));
			if(this->subTriggers[i]->getType() == TriggerType::COMMON) str.append("\n");
		}
	}
	return str;
}

std::string SpecialTrigger::toString(bool reversed,int depth) const{
	if(this->instance == nullptr && this->prototype->isFixed()) {
		std::vector<std::pair<std::string,ParadoxBase*>> data;
		if(this->args.size() != 0) data.push_back({"__REVERSED__", nullptr});
		this->instance = this->prototype->createInstance(data);
	}
	std::string ret("");
	preInit(depth,ret);
	std::string loc_pattern = this->prototype->getLocalizationPattern(reversed);
	if(!loc_pattern.empty()){
		NamedPattern np(loc_pattern);
		for(auto[key,value] : this->args){
			ParadoxType type = value->getType();
			if(type == ParadoxType::STRING) np.fillName(*key,value->toString()); 
			else if(type == ParadoxType::INTEGER) np.fillName(*key,value->getAsInteger()->getIntegerContent());
		}
		ret.append(np.getOutput());
		ret.push_back('\n');
		return ret;
	}
	return this->instance->toString(reversed,depth);
}

bool SpecialTrigger::hasAnyTrigger(bool(*predicate)(Trigger*)){
	if(predicate(this)) return true;
	if(predicate(this->instance)) return true;
	return false;
}

bool SpecialTrigger::foreach(std::function<bool(Trigger*)> action){
	if(!action(this)) return false;
	if(!action(this->instance)) return false;
	return true;
}

void SpecialTrigger::takeOverLifeCycle(){
	this->extra_data[0] = 1;
	for(auto[k,v] : this->args){
		if(v == nullptr) continue;
		v = deep_copy(v);
	}
}
void parseTrigger(ParadoxTag* tag,ComplexTrigger* trigger){
	for(int i = 0;i < tag->size();i++){
		auto[key, childNode] = (*tag)[i];
		std::string item = key;
		ParadoxBase* base = childNode;
		ParadoxTag* subTag = base->getAsTag();
		//if it is complicate....
		if(subTag != nullptr){
			//consider if first~
			if(item == "if"){
				ConditionalTrigger* ct = new ConditionalTrigger();
				//if a if-statement without condition,then pass it directly.
				if(subTag->get("limit",1) == nullptr || subTag->get("limit",1)->getAsTag() == nullptr) {
					delete ct;
					continue;
				}
				trigger->putTrigger(ct);
				bool success = parseConditionalTrigger(subTag,ct);
				if(!success){
					delete ct;
					trigger->subTriggers.pop_back();
				}
				continue;
			}
			//just forget else_if and else....
			if(item == "else_if"){
				ConditionalTrigger* ct = new ConditionalTrigger();
				ct->setElseState();
				if(subTag->get("limit",1) == nullptr || subTag->get("limit",1)->getAsTag() == nullptr) {
					delete ct;
					continue;
				}
				trigger->putTrigger(ct);
				bool success = parseConditionalTrigger(subTag,ct);
				if(!success){
					delete ct;
					trigger->subTriggers.pop_back();
				}
				continue;
			}
			if(item == "else"){
				ConditionalTrigger* ct = new ConditionalTrigger();
				ct->setElseState();
				if(subTag->get("limit",1) != nullptr) {
					delete ct;
					continue;
				}
				trigger->putTrigger(ct);
				parseTrigger(subTag,ct);
				continue;
			}
			//then logic
			if(item == "NOT"){
				LogicTrigger* lt = new LogicTrigger(LogicType::NOT);
				trigger->putTrigger(lt);
				parseTrigger(subTag,lt);
				continue;
			}
			if(item == "AND"){
				LogicTrigger* lt = new LogicTrigger(LogicType::AND);
				trigger->putTrigger(lt);
				parseTrigger(subTag,lt);
				continue;
			}
			if(item == "OR"){
				LogicTrigger* lt = new LogicTrigger(LogicType::OR);
				trigger->putTrigger(lt);
				parseTrigger(subTag,lt);
				continue;			
			}
			//then custom_tt
			if(item == "custom_trigger_tooltip"){
				CustomTooltipTrigger* ctt = new CustomTooltipTrigger();
				ParadoxString* tt = subTag->get("tooltip",1)->getAsString();
				
				if(tt == nullptr) {
					log_warning(current_location(),"No tooltip provided for a custom_tt,this content will be ignored.");
					continue;
				}
				else{
					ctt->tooltip = tt->getStringContent();
					subTag->remove("tooltip",1);
				} 
				trigger->putTrigger(ctt);
				parseTrigger(subTag,ctt);
				continue;
			}
			//then hidden_trigger
			if(item == "hidden_trigger"){
				HiddenTrigger* ht = new HiddenTrigger();
				trigger->putTrigger(ht);
				parseTrigger(subTag,ht);
				
				continue;
			}
			//then change_scope
			Scope* scope = createScopeFromString(item);
			if(scope != nullptr){
				ChangeScopeTrigger* cst = new ChangeScopeTrigger(scope);
				trigger->putTrigger(cst);
				parseTrigger(subTag,cst);
				continue;
			} 
			//at last trigger with clause.
			//first is NumberRequiredTrigger
			if(numberRequiredItems.find(item) != numberRequiredItems.end()){
				NumberRequiredTrigger* nrt = new NumberRequiredTrigger();	
				std::string cnt_tag = numberRequiredItems[item];
				TriggerItem* item2 = items[item];
				nrt->item = item2;
				ParadoxBase* base1 = subTag->get(cnt_tag,1);
				if(base1 == nullptr){
					delete nrt;
					continue;
				}
				ParadoxInteger* num = base1->getAsInteger();
				if(num == nullptr) {
					delete nrt;
					continue;
				}
				nrt->setAmount(num->getIntegerContent());
				subTag->remove(cnt_tag,1);
				trigger->putTrigger(nrt);
				parseTrigger(subTag,nrt);
				continue;
			}
			if(loadedSTs.find(item) != loadedSTs.end()){
				ScriptedTrigger* st = loadedSTs[item];
				if(st->isFixed()) continue;
				Trigger* ti = st->createInstance(subTag->contents);
				if(ti != nullptr){
					SpecialTrigger* spt = new SpecialTrigger(st,ti);
					for(int j = 0;j < subTag->size();j++){
						auto[k, v] = (*subTag)[j];
						spt->args[getStringPtr(k)] = v;
					}
					trigger->putTrigger(spt);
				}
				else {
					log_error(current_location(),"Cannot make instance of scripted_trigger \"",item,"\".");
					for(int j = 0;j < subTag->size();j++){
						auto[k, v] = (*subTag)[j];
						log_error(current_location(),"Parameters:");
						log_error(current_location(),k,":",v->toString());
					}
				}
				continue;
			}
			//then common clause triggers..
			//simple trigger(no overrides) first
			if(simpleTriggers.find(item) != simpleTriggers.end() && subTag->size() != 0) {
				TriggerItem* ti = items[item];
				bool error = false;
				CommonTrigger* ct = new CommonTrigger(ti);
				ct->base.resize(ti->parameterType.size());
				for(int j = 0;j < subTag->size();j++){
					auto[key1, childNode1] = (*subTag)[j];
					//to be honest I do not want to handle smth like typo...
					//however,if the programme crash just because type 'value' to 'valve'
					//that would be annoyed...
					std::string trigger_name = key1;
					if(ti->parameterName.find(trigger_name) == ti->parameterName.end()) {
						//ignore this tag..
						error = true;
						delete ct;
						break;
					}
					int index = ti->parameterName[trigger_name];
					//parameter type mismatch..
					if(!isCastable(childNode1,ti->parameterType[index])){
						error = true;
						delete ct;
						break; 
					}
					ct->base[index] = childNode1;
				}
				if(!error){
					if(missingRenderingParameter(ti,ct->base)) delete ct;
					else trigger->putTrigger(ct);
				}
				continue; 
			}
			//ignore those triggers which have not registered
			if(items.find(item) == items.end()) continue;
			//finally overrides claused trigger
			//f**king Paradox do not actually give a right localization text for some of those..
			//but for wikis,a appropriate localization is necessary.. 
			TriggerItem* ti = items[item];
			CommonTrigger* ct = new CommonTrigger(ti);
			OverrideHandler handler = overrideHandlers[item];
			
			ct->base.resize(ti->parameterType.size());
			bool success = handler(subTag->contents);
			if(!success){
				delete ct;
				continue; 
			} 
			for(auto& [k, v] : subTag->contents){
				int index = ti->parameterName[k];
				ct->base[index] = v;
			} 
			if(missingRenderingParameter(ti,ct->base)) delete ct;
			else trigger->putTrigger(ct);
			continue; 
		}
		else if(registeredTriggers.find(item) == registeredTriggers.end()) {

			//log_warning(current_location(),"unknown Trigger: ",item);
			continue;
		}
		if(loadedSTs.find(item) != loadedSTs.end()){
			if(!loadedSTs[item]->isFixed()) continue;
			else {
				
				ParadoxBoolean* pb = base->getAsBoolean(); 
				if(pb == nullptr) continue;
				std::vector<std::pair<std::string,ParadoxBase*>> args;
				if(!pb->getValue()){
					args.push_back({"__REVERSED__", nullptr});
				} 
				Trigger* ti = loadedSTs[item]->createInstance(args);
				SpecialTrigger* st = new SpecialTrigger(loadedSTs[item],ti);
				if(pb->getValue()) st->args[getStringPtr("__REVERSED__")] = nullptr;
				trigger->putTrigger(st);
			}
		}
		//for no overrides..
		if(simpleTriggers.find(item) != simpleTriggers.end()){
			
			TriggerItem* ti = items[item];
			ParadoxType type = ti->parameterType[0];
			ParadoxBase* base1 = castTo(base,type);
			if(base1 == nullptr) continue;
			CommonTrigger* ct = new CommonTrigger(ti);
			if(base1->getType() == ParadoxType::BOOLEAN) {
				ct->reversed = !base1->getAsBoolean()->getValue();
			}
			else {
				ct->pushObject(base1);
				
			}

			trigger->putTrigger(ct); 
			continue;
		}
		ParadoxType type = base->getType();
		if(type == ParadoxType::INTEGER){
			ParadoxInteger* pInteger = base->getAsInteger();
			bool flag = false;
			for(int i = 0;i < sizeof(INTEGER_MATCH_SEQUENCE) / sizeof(ParadoxType);i++) {
				std::string name("");
				name.append(item);
				name.append("@");
				name.append(std::to_string(static_cast<int>(INTEGER_MATCH_SEQUENCE[i])));
				if(!items.contains(name)) continue;
				TriggerItem* ti = items[name];
				ParadoxBase* arg1 = castTo(base,INTEGER_MATCH_SEQUENCE[i]);
				if(arg1 == nullptr) continue;
				CommonTrigger* ct = new CommonTrigger(ti);
				ct->pushObject(arg1);		
				trigger->putTrigger(ct);
				flag = true;
				break;
			}
			//when nothing matched
			if(!flag) log_error(current_location(),"ERROR: No Matching Trigger for \"",item," = ",pInteger->getIntegerContent() / 1000.0,"\"");
			
			
		}
		else if(type == ParadoxType::STRING){
			ParadoxString* pString = base->getAsString();
			bool flag = false;
			for(int i = 0;i < sizeof(STRING_MATCH_SEQUENCE) / sizeof(ParadoxType);i++){
				std::string name("");
				name.append(item);
				name.append("@");
				name.append(std::to_string(static_cast<int>(STRING_MATCH_SEQUENCE[i])));
				if(items.find(name) == items.end()) continue;		
				TriggerItem* ti = items[name];
				ParadoxBase* arg1 = castTo(pString,STRING_MATCH_SEQUENCE[i]);
				if(arg1 == nullptr) continue;
				CommonTrigger* ct = new CommonTrigger(ti);
				ct->pushObject(arg1);		
				trigger->putTrigger(ct);
				flag = true;
				break;
			}
			//when nothing matched
			if(!flag) log_error(current_location(),"#ERROR: No Matching Trigger for \"" , item , " = " , pString->getStringContent() , "\"");
		}
		else{
			std::string name("");
			name.append(item);
			name.append("@");
			name.append(std::to_string(static_cast<int>(type)));
			if(items.find(name) == items.end()) continue;
			TriggerItem* ti = items[name];
			CommonTrigger* ct = new CommonTrigger(ti);
			if(type == ParadoxType::BOOLEAN){
				ct->reversed = !base->getAsBoolean()->getValue();
			}
			else ct->pushObject(base);
			trigger->putTrigger(ct); 
		}
	}
}

ComplexTrigger* createBaseTrigger(){
	auto ct = new ChangeScopeTrigger(nullptr);
	return ct;
}

bool parseConditionalTrigger(ParadoxTag* tag,ConditionalTrigger* ct){
	ParadoxTag* lim = tag->get("limit",1)->getAsTag();
	ct->condition = createBaseTrigger();
	if(lim->size() == 0) return false;
	parseTrigger(lim,ct->condition);
	parseTrigger(tag,ct); 
	return true;
}

