#pragma once
#include <set>
#include <string>
#include <map>

struct PlayerState {
	std::set<std::string> muteList;
	std::string lastVban;
};

extern std::map<std::string, PlayerState*> g_player_states;