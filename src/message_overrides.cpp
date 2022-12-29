#include "message_overrides.h"
#include "main.h"

bool g_suppress_current_message = false;

// stores the last network message that was suppressed
NetMessage g_suppressed_message;

MessageArg::MessageArg(int argType, int ival) {
	this->argType = argType;
	this->ival = ival;
}

MessageArg::MessageArg(int argType, float fval) {
	this->argType = argType;
	this->fval = fval;
}

MessageArg::MessageArg(int argType, const char* sval) {
	this->argType = argType;
	this->sval = sval;
}

void MessageArg::writeToCurrentMessage() {
	switch (argType) {
	case MARG_NONE:
		break;
	case MARG_ANGLE:
		WRITE_ANGLE(fval);
		break;
	case MARG_BYTE:
		WRITE_BYTE(ival);
		break;
	case MARG_CHAR:
		WRITE_CHAR(ival);
		break;
	case MARG_COORD:
		WRITE_COORD(fval);
		break;
	case MARG_ENT:
		WRITE_ENTITY(ival);
		break;
	case MARG_LONG:
		WRITE_LONG(ival);
		break;
	case MARG_SHORT:
		WRITE_SHORT(ival);
		break;
	case MARG_STRING:
		WRITE_STRING(sval.c_str());
		break;
	default:
		break;
	}
}

const char* MessageArg::getString() {
	switch (argType) {
	case MARG_ANGLE:  return UTIL_VarArgs("ANGLE  : %f", fval);
	case MARG_BYTE:   return UTIL_VarArgs("BYTE   : %d", ival);
	case MARG_CHAR:   return UTIL_VarArgs("CHAR   : %d", ival);
	case MARG_COORD:  return UTIL_VarArgs("COORD  : %f", fval);
	case MARG_ENT:    return UTIL_VarArgs("ENTITY : %d", ival);
	case MARG_LONG:   return UTIL_VarArgs("LONG   : %d", ival);
	case MARG_SHORT:  return UTIL_VarArgs("SHORT  : %d", ival);
	case MARG_STRING: return UTIL_VarArgs("STRING: %s", sval);
	default:
		return "NONE";
	}
}

void NetMessage::send(int msg_dest, edict_t* ed) {
	if (msg_type == -1) {
		println("[Radio] Can't send unintialized net message");
		return;
	}

	const float* origin = hasOrigin ? pOrigin : NULL;

	MESSAGE_BEGIN(msg_dest, msg_type, origin, ed);

	for (int i = 0; i < args.size(); i++) {
		args[i].writeToCurrentMessage();
	}

	MESSAGE_END();
}

void NetMessage::send() {
	send(msg_dest, ed);
}

void NetMessage::clear() {
	args.clear();
	msg_type = -1;
}

const char* msgDestStr(int msg_dest) {
	const char* sdst = "";
	switch (msg_dest) {
	case MSG_BROADCAST:
		sdst = "MSG_BROADCAST";
		break;
	case MSG_ONE:
		sdst = "MSG_ONE";
		break;
	case MSG_ALL:
		sdst = "MSG_ALL";
		break;
	case MSG_INIT:
		sdst = "MSG_INIT";
		break;
	case MSG_PVS:
		sdst = "MSG_PVS";
		break;
	case MSG_PAS:
		sdst = "MSG_PAS";
		break;
	case MSG_PVS_R:
		sdst = "MSG_PVS_R";
		break;
	case MSG_PAS_R:
		sdst = "MSG_PAS_R";
		break;
	case MSG_ONE_UNRELIABLE:
		sdst = "MSG_ONE_UNRELIABLE";
		break;
	case MSG_SPEC:
		sdst = "MSG_SPEC";
		break;
	default:
		sdst = UTIL_VarArgs("%d (unkown)", msg_dest);
		break;
	}

	return sdst;
}

void NetMessage::print() {
	const char* origin = pOrigin ? UTIL_VarArgs("Vector(%f %f %f)", pOrigin[0], pOrigin[1], pOrigin[2]) : "NULL";
	const char* sed = ed ? STRING(ed->v.classname) : "NULL";

	println("BEGIN(%s, %d, %s, %s)", msgDestStr(msg_dest), msg_type, origin, sed);
	for (int i = 0; i < args.size(); i++) {
		println("    %s", args[i].getString());
	}
	println("END");
}

void hookTextMessage(NetMessage& msg) {
	int isender = msg.args[0].ival;
	edict_t* sender = INDEXENT(isender);

	if (!isValidPlayer(sender) || isender < 1 || isender > gpGlobals->maxClients) {
		msg.send(); // could be leave/join message
		return;
	}

	string senderid = getPlayerUniqueId(sender);

	if (msg.msg_dest == MSG_ALL || msg.msg_dest == MSG_BROADCAST) {
		for (int i = 1; i <= gpGlobals->maxClients; i++) {
			edict_t* receiver = INDEXENT(i);

			if (!isValidPlayer(receiver)) {
				continue;
			}

			PlayerState& state = getPlayerState(receiver);
			if (state.muteList.find(senderid) != state.muteList.end()) {
				ClientPrint(receiver, HUD_PRINTCONSOLE, UTIL_VarArgs("[Muted] %s", msg.args[1].sval.c_str()));
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
			ClientPrint(receiver, HUD_PRINTCONSOLE, UTIL_VarArgs("[Muted] %s", msg.args[1].sval.c_str()));
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
