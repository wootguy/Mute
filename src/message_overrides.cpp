#include "message_overrides.h"
#include "main.h"
#include "NetMessage.h"

#define MSG_ChatMsg 74
#define MSG_TextMsg 75

bool g_suppress_current_message = false;

// stores the last network message that was suppressed
NetMessage g_suppressed_message;

void hookTextMessage(NetMessage& msg) {
	int isender = msg.args[0].ival;
	edict_t* sender = INDEXENT(isender);

	if (!isValidPlayer(sender) || isender < 1 || isender > gpGlobals->maxClients) {
		msg.send(); // could be leave/join message
		return;
	}

	string senderid = getPlayerUniqueId(sender);
	string chatMsg = msg.args[1].sval;

	if (msg.msg_dest == MSG_ALL || msg.msg_dest == MSG_BROADCAST) {
		for (int i = 1; i <= gpGlobals->maxClients; i++) {
			edict_t* receiver = INDEXENT(i);

			if (!isValidPlayer(receiver)) {
				continue;
			}

			PlayerState& state = getPlayerState(receiver);
			if (state.muteList.find(senderid) != state.muteList.end()) {
				//if (chatMsg.size())
				//	ClientPrint(receiver, HUD_PRINTCONSOLE, UTIL_VarArgs("[Muted] %s", chatMsg.c_str()));
			}
			else {
				int sendMode = msg.msg_dest == MSG_ALL ? MSG_ONE : MSG_ONE_UNRELIABLE;
				msg.send(MSG_ONE, receiver);
			}
		}
	}
	else if (msg.msg_dest == MSG_ONE || msg.msg_dest == MSG_ONE_UNRELIABLE) {
		edict_t* receiver = msg.ed;
		
		if (!isValidPlayer(receiver)) {
			return;
		}

		PlayerState& state = getPlayerState(receiver);
		if (state.muteList.find(senderid) == state.muteList.end()) {
			msg.send();
		}
		else {
			//if (chatMsg.size())
			//	ClientPrint(receiver, HUD_PRINTCONSOLE, UTIL_VarArgs("[Muted] %s", chatMsg.c_str()));
		}
	}
	else {
		msg.send();
	}
}

void MessageBegin(int msg_dest, int msg_type, const float* pOrigin, edict_t* ed) {	
	if (msg_type == MSG_ChatMsg) {
		// wait until the args are checked before sending this message.
		// music shouldn't be sent to radio listeners.
		g_suppress_current_message = true;
		g_suppressed_message.clear();
		g_suppressed_message.msg_type = msg_type;
		g_suppressed_message.hasOrigin = false;
		if (pOrigin) {
			g_suppressed_message.hasOrigin = true;
			g_suppressed_message.pOrigin[0] = pOrigin[0];
			g_suppressed_message.pOrigin[1] = pOrigin[1];
			g_suppressed_message.pOrigin[2] = pOrigin[2];
		}
		g_suppressed_message.msg_dest = msg_dest;
		g_suppressed_message.ed = ed;
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

void MessageEnd() {
	if (g_suppress_current_message) {
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void MessageEnd_post() {
	if (g_suppress_current_message) {
		g_suppress_current_message = false;
		hookTextMessage(g_suppressed_message);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteAngle(float angle) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_ANGLE, angle));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteByte(int b) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_BYTE, b));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteChar(int c) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_CHAR, c));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteCoord(float coord) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_COORD, coord));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteEntity(int ent) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_ENT, ent));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteLong(int val) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_LONG, val));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteShort(int val) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_SHORT, val));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}

void WriteString(const char* s) {
	if (g_suppress_current_message) {
		g_suppressed_message.args.push_back(MessageArg(MARG_STRING, s));
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_IGNORED);
}
