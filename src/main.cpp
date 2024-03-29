#include "mmlib.h"
#include <string>
#include <map>
#include <set>
#include "message_overrides.h"
#include "main.h"

using namespace std;

// Description of plugin
plugin_info_t Plugin_info = {
	META_INTERFACE_VERSION,	// ifvers
	"Mute",	// name
	"1.0",	// version
	__DATE__,	// date
	"w00tguy",	// author
	"https://github.com/wootguy/",	// url
	"MUTE",	// logtag, all caps please
	PT_ANYTIME,	// (when) loadable
	PT_ANYPAUSE,	// (when) unloadable
};

map<string, PlayerState> g_player_states;

PlayerState& getPlayerState(string steamId) {
	steamId = toLowerCase(steamId);

	if (g_player_states.find(steamId) == g_player_states.end()) {
		g_player_states[steamId] = PlayerState();
	}

	return g_player_states[steamId];
}

PlayerState& getPlayerState(edict_t* plr) {
	return getPlayerState(getPlayerUniqueId(plr));
}

void metamute() {
	int ireceiver = atoi(CMD_ARGV(1));
	string senderId = CMD_ARGV(2);
	int blisten = atoi(CMD_ARGV(3));

	const char* action = blisten ? "Unmuted" : "Muted";
	println("[Mute] %d %s %d", ireceiver, senderId.c_str(), blisten);

	edict_t* receiver = INDEXENT(ireceiver);
	PlayerState& state = getPlayerState(receiver);

	if (CMD_ARGC() == 2) {
		if (state.muteList.size() <= 3) {
			string muteString = "";

			for (auto itr : state.muteList) {
				edict_t* plr = getPlayerByUniqueId(itr);
				string name = plr ? STRING(plr->v.netname) : itr;

				if (muteString.length()) {
					muteString += ", ";
				}
				
				muteString += name;
			}

			ClientPrint(receiver, HUD_PRINTTALK, UTIL_VarArgs("\nMuted players: %s\n\n", muteString.c_str()));
		}
		else {
			ClientPrint(receiver, HUD_PRINTTALK, UTIL_VarArgs("\nYou've muted %d players. Check console to see who is muted.\n\n", state.muteList.size()));
		}

		ClientPrint(receiver, HUD_PRINTCONSOLE, "    %-32s%-32s\n", "Steam ID", "Name");
		ClientPrint(receiver, HUD_PRINTCONSOLE, "-------------------------------------------------------------------\n");
		
		int i = 1;
		for (auto itr : state.muteList) {
			edict_t* plr = getPlayerByUniqueId(itr);
			string name = plr ? STRING(plr->v.netname) : "\\disconnected player\\";
			ClientPrint(receiver, HUD_PRINTCONSOLE, UTIL_VarArgs("%2d) %-32s%-32s\n", i++, itr.c_str(), name.c_str()));
		}

		ClientPrint(receiver, HUD_PRINTCONSOLE, "-------------------------------------------------------------------\n\n");
		ClientPrint(receiver, HUD_PRINTCONSOLE, "Type .mute or .unmute to edit your mute list.\n\n");

		return;
	}
	
	if (!isValidPlayer(receiver)) {
		println("[Mute] Invalid receiver");
		return;
	}

	if (senderId == "all") {
		if (blisten) {
			state.muteList.clear();

			for (int i = 1; i <= gpGlobals->maxClients; i++) {
				g_engfuncs.pfnVoice_SetClientListening(ireceiver, i, 1);
			}

			ClientPrint(receiver, HUD_PRINTTALK, "** Unmuted everyone\n");
		}
		else {
			for (int i = 1; i <= gpGlobals->maxClients; i++) {
				edict_t* ent = INDEXENT(i);

				if (!isValidPlayer(ent)) {
					continue;
				}

				state.muteList.insert(getPlayerUniqueId(ent));
				g_engfuncs.pfnVoice_SetClientListening(ireceiver, i, 0);
			}

			ClientPrint(receiver, HUD_PRINTTALK, "** Muted everyone\n");
		}

		return;
	}

	edict_t* sender = getPlayerByUniqueId(senderId);
	int isender = ENTINDEX(sender);

	if (sender && ireceiver == isender) {
		ClientPrint(receiver, HUD_PRINTTALK, "** You can't mute yourself\n");
		return;
	}

	if (blisten) {
		state.muteList.erase(senderId);
	}
	else {
		state.muteList.insert(senderId);
	}

	if (!isValidPlayer(sender)) {
		//ClientPrint(receiver, HUD_PRINTTALK, "** Failed to mute player\n");
		return;
	}

	ClientPrint(receiver, HUD_PRINTTALK, UTIL_VarArgs("** %s player \n%s\n", action, STRING(sender->v.netname)));

	g_engfuncs.pfnVoice_SetClientListening(ireceiver, isender, blisten);
}

void metajail() {
	string prisonerId = CMD_ARGV(1);
	int isJailed = atoi(CMD_ARGV(2));

	println("[Mute] Jail %s %d", prisonerId.c_str(), isJailed);

	PlayerState& state = getPlayerState(prisonerId);
	state.isJailed = isJailed != 0;
}

void ClientJoin(edict_t* plr) {
	int joinerIdx = ENTINDEX(plr);
	PlayerState& joinerState = getPlayerState(plr);
	string joinerId = getPlayerUniqueId(plr);

	// set mutes for this joiner
	for (int i = 1; i <= gpGlobals->maxClients; i++) {
		edict_t* ent = INDEXENT(i);

		if (!isValidPlayer(ent)) {
			continue;
		}

		string steamid = getPlayerUniqueId(ent);
		PlayerState& state = getPlayerState(ent);

		// mute joiner for existing player
		bool shouldListen = state.muteList.find(joinerId) == state.muteList.end();
		g_engfuncs.pfnVoice_SetClientListening(i, joinerIdx, shouldListen);

		// mute existing player for new joiner
		bool joinerShouldListen = joinerState.muteList.find(steamid) == joinerState.muteList.end();
		g_engfuncs.pfnVoice_SetClientListening(joinerIdx, i, joinerShouldListen);
	}

	RETURN_META(MRES_IGNORED);
}

void ClientCommand(edict_t* plr) {
	const char* cmd = CMD_ARGC() > 0 ? CMD_ARGV(0) : "";

	PlayerState& state = getPlayerState(plr);

	if (state.isJailed) {
		// TODO: load from file
		static set<string> g_cmd_filter = {
			".vote",
			".view",
			".viewlast",
			"rtv",
			"nom",
			"nominate",
			"rnom",
			".observer",
			".observe",
			".spectate",
			"afk?",
			"ending?",
			"endless?",
			"survival?",
			"series?"
		};

		string cmd = toLowerCase(CMD_ARGV(0));
		string arg = CMD_ARGC() > 1 ? toLowerCase(CMD_ARGV(1)) : "";
		bool isChatCmd = (cmd == "say" || cmd == "say_team") && arg.length();

		if (g_cmd_filter.find(cmd) != g_cmd_filter.end() || (isChatCmd && g_cmd_filter.find(arg) != g_cmd_filter.end())) {
			ClientPrint(plr, HUD_PRINTCONSOLE, "[JAIL] Command forbidden while in jail.\n");
			RETURN_META(MRES_SUPERCEDE);
		}
	}

	if (strcmp(cmd, "vban") == 0) {
		PlayerState& state = getPlayerState(plr);
		const char* vban = CMD_ARGC() > 1 ? CMD_ARGV(1) : "0";

		if (state.lastVban != vban && strcmp(vban, "0")) {
			ClientPrint(plr, HUD_PRINTTALK, "[Info] Scoreboard muting is broken. Use the \".mute\" command instead.\n");
		}

		state.lastVban = vban;
	}

	RETURN_META(MRES_IGNORED);
}

int Voice_SetClientListening(int iReceiver, int iSender, int blisten) {
	// don't allow muting via vban commands.
	// scoreboard muting is broken in two ways:
	// 1) muting one player often mutes multiple people or the entire server
	// 2) muting players in server A will mute different players in server B,
	//    even if you never muted someone in server B.
	
	RETURN_META_VALUE(MRES_SUPERCEDE, 1); // never mute
}

void PluginInit() {
	g_dll_hooks.pfnClientCommand = ClientCommand;
	g_dll_hooks.pfnClientPutInServer = ClientJoin;
	g_engine_hooks.pfnVoice_SetClientListening = Voice_SetClientListening;

	g_engine_hooks.pfnMessageBegin = MessageBegin;
	g_engine_hooks.pfnWriteAngle = WriteAngle;
	g_engine_hooks.pfnWriteByte = WriteByte;
	g_engine_hooks.pfnWriteChar = WriteChar;
	g_engine_hooks.pfnWriteCoord = WriteCoord;
	g_engine_hooks.pfnWriteEntity = WriteEntity;
	g_engine_hooks.pfnWriteLong = WriteLong;
	g_engine_hooks.pfnWriteShort = WriteShort;
	g_engine_hooks.pfnWriteString = WriteString;
	g_engine_hooks.pfnMessageEnd = MessageEnd;
	g_engine_hooks_post.pfnMessageEnd = MessageEnd_post;

	REG_SVR_COMMAND("metamute", metamute);
	REG_SVR_COMMAND("metajail", metajail);
}

void PluginExit() {}