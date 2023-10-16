#pragma once
#include "mmlib.h"
#include <vector>

using namespace std;

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