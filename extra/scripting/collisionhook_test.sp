#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <collisionhook>

#define ENTITY_NAME_MAX_LENGTH			32

#define DEBUG_FLAG_NONE					0
#define DEBUG_FLAG_SHOULDCOLLIDE		(1 << 0)
#define DEBUG_FLAG_PASSFILTER			(1 << 1)
#define DEBUG_FLAGS_ALL					DEBUG_FLAG_SHOULDCOLLIDE|DEBUG_FLAG_PASSFILTER

#define DEBUG_MESSAGE_CHAT				(1 << 0)
#define DEBUG_MESSAGE_SERVER_CONSOLE	(1 << 1)

ConVar
	g_hDebugFlags = null,
	g_hDebugMsgType = null;

public void OnPluginStart()
{
	char sValue[16], sDescription[128];

	IntToString(DEBUG_FLAGS_ALL, sValue, sizeof(sValue));
	Format(sDescription, sizeof(sDescription), "Show debug messages for: %d - disable messages, %d - ShouldCollide, %d - PassFilter, %d - for all.", \
											DEBUG_FLAG_NONE, DEBUG_FLAG_SHOULDCOLLIDE, DEBUG_FLAG_PASSFILTER, DEBUG_FLAGS_ALL);
	
	g_hDebugFlags = CreateConVar("ch_debug_flags", sValue, sDescription, _, true, float(DEBUG_FLAG_NONE), true, float(DEBUG_FLAGS_ALL));

	IntToString(DEBUG_MESSAGE_SERVER_CONSOLE, sValue, sizeof(sValue));
	Format(sDescription, sizeof(sDescription), "Where to show debug messages: %d - in the chat for players, %d - in the server console.", \
											DEBUG_MESSAGE_CHAT, DEBUG_MESSAGE_SERVER_CONSOLE);

	g_hDebugMsgType = CreateConVar("ch_debug_msg_type", sValue, sDescription, _, true, float(DEBUG_MESSAGE_CHAT), true, float(DEBUG_MESSAGE_SERVER_CONSOLE));
}

public Action CH_ShouldCollide(int iEntity1, int iEntity2, bool &bResult)
{
	DebugMessage(DEBUG_FLAG_SHOULDCOLLIDE, "CH_ShouldCollide", iEntity1, iEntity2, bResult);

	return Plugin_Continue;
}

public Action CH_PassFilter(int iEntity1, int iEntity2, bool &bResult)
{
	DebugMessage(DEBUG_FLAG_PASSFILTER, "CH_PassFilter", iEntity1, iEntity2, bResult);

	return Plugin_Continue;
}

void DebugMessage(int iDebugFlag, const char[] sFuncName, int iEntity1, int iEntity2, bool bResult)
{
	if (!(g_hDebugFlags.IntValue & iDebugFlag)) {
		return;
	}

	char sEntityName1[ENTITY_NAME_MAX_LENGTH], sEntityName2[ENTITY_NAME_MAX_LENGTH];
	GetEntityNameIsValid(iEntity1, sEntityName1, sizeof(sEntityName1));
	GetEntityNameIsValid(iEntity2, sEntityName2, sizeof(sEntityName2));

	if (g_hDebugMsgType.IntValue & DEBUG_MESSAGE_CHAT) {
		PrintToChatAll("[%s] iEntity1: %s (%d), iEntity2: %s (%d), result: %d!", sFuncName, sEntityName1, iEntity1, sEntityName2, iEntity2, bResult);

		return;
	}

	PrintToServer("[%s] iEntity1: %s (%d), iEntity2: %s (%d), result: %d!", sFuncName, sEntityName1, iEntity1, sEntityName2, iEntity2, bResult);
}

bool GetEntityNameIsValid(int iEntity, char[] sEntityName, const int iMaxLength)
{
	if (iEntity < 0 || !IsValidEntity(iEntity)) {
		FormatEx(sEntityName, iMaxLength, "Invalid entity");

		return false;
	}

	GetEntityClassname(iEntity, sEntityName, iMaxLength);

	return true;
}