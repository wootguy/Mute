void print(string text) { g_Game.AlertMessage( at_console, text); }
void println(string text) { print(text + "\n"); }

class PlayerState {
	array<string> muteList;
	array<string> unmuteList; // unmute these players next join
	
	bool mutedChatsounds = false;
	bool mutedVoiceCommands = false;
	bool mutedPlayerModels = false;
	bool mutedRadio = false;
}

// Menus need to be defined globally when the plugin is loaded or else paging doesn't work.
// Each player needs their own menu or else paging breaks when someone else opens the menu.
// These also need to be modified directly (not via a local var reference).
array<CTextMenu@> g_menus = {
	null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null,
	null
};

dictionary g_player_states;

CConCommand _extMute( "mute_ext", "Mute from other plugin", @extMute );

// Will create a new state if the requested one does not exit
PlayerState@ getPlayerState(CBasePlayer@ plr) {
	if (plr is null or !plr.IsConnected())
		return null;
		
	string steamId = g_EngineFuncs.GetPlayerAuthId( plr.edict() );
	if (steamId == 'STEAM_ID_LAN' or steamId == 'BOT') {
		steamId = plr.pev.netname;
	}
	
	if ( !g_player_states.exists(steamId) )
	{
		PlayerState state;
		g_player_states[steamId] = state;
	}
	return cast<PlayerState@>( g_player_states[steamId] );
}

void PluginInit() {
	g_Module.ScriptInfo.SetAuthor( "w00tguy" );
	g_Module.ScriptInfo.SetContactInfo( "github" );
	
	g_Hooks.RegisterHook( Hooks::Player::ClientSay, @ClientSay );
	g_Hooks.RegisterHook( Hooks::Player::ClientPutInServer, @ClientJoin );
}

HookReturnCode ClientJoin(CBasePlayer@ plr) {		
	PlayerState@ state = getPlayerState(plr);
	
	for (uint i = 0; i < state.unmuteList.size(); i++) {
		removeMute(plr, state.unmuteList[i]);
	}
	state.unmuteList.resize(0);
	
	return HOOK_CONTINUE;
}

CBasePlayer@ getMuteTarget(CBasePlayer@ caller, string name) {
	name = name.ToLowercase();
	int partialMatches = 0;
	CBasePlayer@ partialMatch;
	
	for (int i = 1; i <= g_Engine.maxClients; i++) {
		CBasePlayer@ plr = g_PlayerFuncs.FindPlayerByIndex(i);
		
		if (plr is null or !plr.IsConnected()) {
			continue;
		}
		
		const string steamId = g_EngineFuncs.GetPlayerAuthId(plr.edict()).ToLowercase();
		
		string plrName = string(plr.pev.netname).ToLowercase();
		if (plrName == name || steamId == name)
			return plr;
		else if (plrName.Find(name) != uint(-1))
		{
			@partialMatch = plr;
			partialMatches++;
		}
	}
	
	if (partialMatches == 1) {
		return partialMatch;
	} else if (partialMatches > 1) {
		if (caller !is null)
			g_PlayerFuncs.ClientPrint(caller, HUD_PRINTTALK, 'Mute failed. There are ' + partialMatches + ' players that have "' + name + '" in their name. Be more specific.\n');
	} else {
		if (caller !is null)
			g_PlayerFuncs.ClientPrint(caller, HUD_PRINTTALK, 'Mute failed. There is no player named "' + name + '".\n');
	}
	
	return null;
}

// mute menu
void callbackMenuMute(CTextMenu@ menu, CBasePlayer@ plr, int itemNumber, const CTextMenuItem@ item) {
	if (item is null or plr is null or !plr.IsConnected()) {
		return;
	}

	PlayerState@ state = getPlayerState(plr);

	string option = "";
	item.m_pUserData.retrieve(option);
	
	if (option == "mute-player") {
		g_Scheduler.SetTimeout("openPlayerListMuteMenu", 0, EHandle(plr), 0);
	}
	else if (option == "mute-chatsounds") {
		g_Scheduler.SetTimeout("openMuteMenu", 0, EHandle(plr));
	}
	else if (option == "mute-voicecommands") {
		g_Scheduler.SetTimeout("openMuteMenu", 0, EHandle(plr));
	}
	else if (option == "mute-radio") {
		g_Scheduler.SetTimeout("openMuteMenu", 0, EHandle(plr));
	}
	else if (option == "mute-playermodels") {
		g_Scheduler.SetTimeout("openMuteMenu", 0, EHandle(plr));
	}
	else if (option == "mute-everything") {
		g_Scheduler.SetTimeout("openMuteMenu", 0, EHandle(plr));
	}
}

// choose player to mute
void callbackMenuPlayerListMute(CTextMenu@ menu, CBasePlayer@ plr, int itemNumber, const CTextMenuItem@ item) {
	if (item is null or plr is null or !plr.IsConnected()) {
		return;
	}

	string option = "";
	item.m_pUserData.retrieve(option);
	
	array<string> parts = option.Split("-");
	int page = atoi(parts[0]);
	string steamid = parts[1];
	
	toggleMute(plr, steamid);
	
	g_Scheduler.SetTimeout("openPlayerListMuteMenu", 0, EHandle(plr), page);
}

void addMute(CBasePlayer@ plr, string steamid) {
	PlayerState@ state = getPlayerState(plr);
	
	string upperSteamid = steamid;
	upperSteamid = upperSteamid.ToUppercase();
	
	int idPos = state.muteList.find(steamid);
	if (idPos == -1) {
		// mute player
		state.muteList.insertLast(steamid);
		g_EngineFuncs.ServerCommand("metamute \"" + plr.entindex() + '" "' + upperSteamid + '" ' + "0\n");
		g_EngineFuncs.ServerCommand("as_command .csmute_ext " + plr.entindex() + ' "' + steamid + '" ' + "1\n");
		g_EngineFuncs.ServerCommand("as_command .vcmute_ext " + plr.entindex() + ' "' + steamid + '" ' + " 1\n");
		//g_EngineFuncs.ServerCommand("modelswap_ext " + plr.entindex() + ' "' + steamid + '" ' + " player-10up\n");
		g_EngineFuncs.ServerCommand("as_command .modelswap_ext " + plr.entindex() + ' "' + steamid + '" ' + " player-10up\n");
		g_EngineFuncs.ServerExecute();
	}
}

void removeMute(CBasePlayer@ plr, string steamid) {
	PlayerState@ state = getPlayerState(plr);
	
	string upperSteamid = steamid;
	upperSteamid = upperSteamid.ToUppercase();
	
	int idPos = state.muteList.find(steamid);
	if (idPos != -1) {
		// unmute player
		g_EngineFuncs.ServerCommand("metamute \"" + plr.entindex() + '" "' + upperSteamid + '" ' + "1\n");
		g_EngineFuncs.ServerCommand("as_command .csmute_ext " + plr.entindex() + ' "' + steamid + '" ' + "0\n");
		g_EngineFuncs.ServerCommand("as_command .vcmute_ext " + plr.entindex() + ' "' + steamid + '" ' + " 0\n");
		//g_EngineFuncs.ServerCommand("modelswap_ext " + plr.entindex() + ' "' + steamid + '" ' + " ?unswap?\n");
		g_EngineFuncs.ServerCommand("as_command .modelswap_ext " + plr.entindex() + ' "' + steamid + '" ' + " ?unswap?\n");
		g_EngineFuncs.ServerExecute();
		state.muteList.removeAt(idPos);
	}
}

void removeMuteDisconnected(string muter, string target) {
	// can't unmute yet because this player isn't connected anymore. Wait until they join.
	// each plugin manages its own mute states so it's easiest to wait.
	
	if ( g_player_states.exists(muter) ) {
		PlayerState@ state = cast<PlayerState@>( g_player_states[muter] );
		int idPos = state.unmuteList.find(target);
		if (idPos == -1) {
			state.unmuteList.insertLast(target);
		}
	}
}

void toggleMute(CBasePlayer@ plr, string steamid) {
	PlayerState@ state = getPlayerState(plr);

	CBasePlayer@ target = getMuteTarget(null, steamid);
	int idPos = state.muteList.find(steamid);
	if (idPos == -1) {
		//if (target !is null)
		//	g_PlayerFuncs.ClientPrint(target, HUD_PRINTNOTIFY, "[Mute] " + plr.pev.netname + " muted you.");
		addMute(plr, steamid);
	} else {
		//if (target !is null)
		//	g_PlayerFuncs.ClientPrint(target, HUD_PRINTNOTIFY, "[Mute] " + plr.pev.netname + " unmuted you.\n");
		removeMute(plr, steamid);
	}
}

void extMute(const CCommand@ args) {
	println("[MuteAs] extmute " + args[1] + " " + args[2] + " " + args[3]);
	CBasePlayer@ muter = getMuteTarget(null, args[1]);
	string targetid = args[2].ToLowercase();
	bool shouldMute = atoi(args[3]) != 0;
	
	if (shouldMute) {
		addMute(muter, targetid);
	} else {
		if (muter !is null) {
			removeMute(muter, targetid);
		} else {
			// track unmutes in this plugin instead of in the calling plugin
			removeMuteDisconnected(args[1].ToUppercase(), targetid);
		}
	}
}

void openPlayerListMuteMenu(EHandle h_plr, int page) {
	CBasePlayer@ plr = cast<CBasePlayer@>(h_plr.GetEntity());
	if (plr is null || !plr.IsConnected()) {
		return;
	}

	int eidx = plr.entindex();
	PlayerState@ state = getPlayerState(plr);
	
	@g_menus[eidx] = CTextMenu(@callbackMenuPlayerListMute);
	g_menus[eidx].SetTitle("\\yMute who?");
	
	int numPlayers = 0;
	int foundPlayers = 0;
	
	for (int i = 1; i <= g_Engine.maxClients; i++) {
		CBasePlayer@ target = g_PlayerFuncs.FindPlayerByIndex(i);
		
		if (target is null || !target.IsConnected() || i == eidx) {
			continue;
		}
		
		int itempage = (foundPlayers++) / 7;
		
		string steamId = g_EngineFuncs.GetPlayerAuthId( target.edict() ).ToLowercase();
		if (target !is null && target.IsConnected()) {
			numPlayers++;
			bool isMuted = state.muteList.find(steamId) != -1;
			g_menus[eidx].AddItem("\\w[" + (isMuted ? "\\rX\\w" : "  ") + "] " + target.pev.netname +"\\y", any("" + itempage + "-" + steamId));
		}
	}
	
	if (numPlayers == 0) {
		g_PlayerFuncs.ClientPrint(plr, HUD_PRINTTALK, "You're the only player in the server!");
		g_Scheduler.SetTimeout("openMuteMenu", 0, EHandle(plr));
		return;
	}

	g_menus[eidx].Register();
	g_menus[eidx].Open(0, page, plr);
}

void openMuteMenu(EHandle h_plr) {
	CBasePlayer@ plr = cast<CBasePlayer@>(h_plr.GetEntity());
	if (plr is null || !plr.IsConnected()) {
		return;
	}
	
	PlayerState@ state = getPlayerState(plr);
	
	int eidx = plr.entindex();
			
	@g_menus[eidx] = CTextMenu(@callbackMenuMute);
	g_menus[eidx].SetTitle("\\yMute what?");
	g_menus[eidx].AddItem("\\wPlayer(s)\\y", any('mute-player'));
	g_menus[eidx].AddItem("\\wChat sounds [" + (state.mutedChatsounds ? "X" : " ") + "]\\y", any('mute-chatsounds'));
	g_menus[eidx].AddItem("\\wVoice commands [" + (state.mutedVoiceCommands ? "X" : " ") + "]\\y", any('mute-voicecommands'));
	g_menus[eidx].AddItem("\\wRadio / text-to-speech [" + (state.mutedVoiceCommands ? "X" : " ") + "]\\y", any('mute-radio'));
	g_menus[eidx].AddItem("\\wCustom player models [" + (state.mutedPlayerModels ? "X" : " ") + "]\\y", any('mute-playermodels'));
	g_menus[eidx].AddItem("\\rEVERYTHING!!!\\y", any('mute-everything'));

	g_menus[eidx].Register();
	g_menus[eidx].Open(0, 0, plr);
}

bool doCommand(CBasePlayer@ plr, const CCommand@ args) {
	PlayerState@ state = getPlayerState(plr);
	
	if ( args.ArgC() >= 1 ) {
		if (args[0] == ".mute" || args[0] == "/mute" || (args[0] == "mute" && args.ArgC() == 1)) {
			if (args.ArgC() > 1) {
				CBasePlayer@ target = getMuteTarget(plr, args[1]);
				if (target is null) {
					return true;
				}
				string steamId = g_EngineFuncs.GetPlayerAuthId( target.edict() ).ToLowercase();
				
				if (args.ArgC() > 2) {
					if (atoi(args[2]) != 0) {
						addMute(plr, steamId);
					} else {
						removeMute(plr, steamId);
					}
				} else {
					toggleMute(plr, steamId);
				}				
			} else {
				openPlayerListMuteMenu(plr, 0);
			}
			
			return true;
		}
		else if (args[0] == ".listmutes") {
			array<array<string>> muted;
			muted.resize(33);
			
			for (int i = 1; i <= g_Engine.maxClients; i++) {
				CBasePlayer@ p = g_PlayerFuncs.FindPlayerByIndex(i);
				
				if (p is null or !p.IsConnected()) {
					continue;
				}
				
				string steamId = g_EngineFuncs.GetPlayerAuthId( p.edict() ).ToLowercase();
				
				for (int k = 1; k <= g_Engine.maxClients; k++) {
					CBasePlayer@ p2 = g_PlayerFuncs.FindPlayerByIndex(k);
					
					if (p2 is null or !p2.IsConnected()) {
						continue;
					}
					
					PlayerState@ p2state = getPlayerState(p2);
					if (int(p2state.muteList.find(steamId)) != -1) {
						muted[p.entindex()].insertLast(p2.pev.netname);
					}
				}
			}
			
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "\nPlayers who are muted\n----------------------------\n");
			
			bool anyMuted = false;
			
			for (uint i = 1; i < muted.size(); i++) {
				CBasePlayer@ p = g_PlayerFuncs.FindPlayerByIndex(i);
				
				if (p is null or !p.IsConnected()) {
					continue;
				}
				
				if (muted[i].size() == 0) {
					continue;
				}
				
				anyMuted = true;
				
				string muters = "";
				for (uint k = 0; k < muted[i].size(); k++) {
					muters += "    " + muted[i][k] + "\n";
				}
				
				g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, string(p.pev.netname) + ':\n' + muters + "\n");
			}
			
			if (!anyMuted) {
				g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "(nobody)\n");
			}
			
			g_PlayerFuncs.ClientPrint(plr, HUD_PRINTCONSOLE, "----------------------------\n\n");
			
			return true;
		}
	}
	
	return false;
}

HookReturnCode ClientSay( SayParameters@ pParams )
{	
	CBasePlayer@ plr = pParams.GetPlayer();
	const CCommand@ args = pParams.GetArguments();
	
	if (doCommand(plr, args))
	{
		pParams.ShouldHide = true;
		return HOOK_HANDLED;
	}
	
	return HOOK_CONTINUE;
}

CClientCommand _cmd1("mute", "mute player audio/model", @consoleCmd );
CClientCommand _cmd2("listmutes", "list muted players", @consoleCmd );

void consoleCmd( const CCommand@ args )
{
	CBasePlayer@ plr = g_ConCommandSystem.GetCurrentPlayer();
	doCommand(plr, args);
}
