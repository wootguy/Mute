#pragma once
#include "meta_init.h"
#include "misc_utils.h"
#include <vector>

enum message_arg_types {
	MARG_NONE,
	MARG_ANGLE,
	MARG_BYTE,
	MARG_CHAR,
	MARG_COORD,
	MARG_ENT,
	MARG_LONG,
	MARG_SHORT,
	MARG_STRING
};

struct MessageArg {
	int argType = MARG_NONE;
	int ival = 0;
	float fval = 0;
	const char* sval = "";

	MessageArg(int argType, int ival);
	MessageArg(int argType, float ival);
	MessageArg(int argType, const char* sval);
	void writeToCurrentMessage();
	const char* getString();
};

struct NetMessage {
	vector<MessageArg> args;
	int msg_type = -1;
	int msg_dest;
	float pOrigin[3];
	bool hasOrigin;
	edict_t* ed;

	NetMessage() {}
	void send(int msg_dest, edict_t* ed);
	void send();
	void clear();
	void print();
};
void MessageBegin(int msg_dest, int msg_type, const float* pOrigin, edict_t* ed);

void MessageEnd();
void MessageEnd_post();
void WriteAngle(float angle);
void WriteByte(int b);
void WriteChar(int c);
void WriteCoord(float coord);
void WriteEntity(int ent);
void WriteLong(int val);
void WriteShort(int val);
void WriteString(const char* s);