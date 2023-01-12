// Added commit:
// Fix hook leaks, affecting FPS over map restarts (Adrianilloo commit)
// https://github.com/Adrianilloo/Collisionhook/commit/567a221693b0f6ae223950847607e3a007beefa8
// 
// Functions 'CreateEnvironment' and 'SetCollisionSolver' are executed at map start each time, so this can happen.
// Pointer 'pSolver' passed to function 'SetCollisionSolver' is global and this pointer never changes, so we don't need to make a new hook 'ShouldCollide' every map.
//
// Fixed crash #1 in game left4dead2 on windows, lthough Linux should also have a crash, but the stack never broke there.
// After update 'The Last Stand' in 'CollisionEvent::ShouldCollide' added 2 new parameters.
//
// Fixed crash #2 in game left4dead2 on windows.
// The PassServerEntityFilter function is not only called from the main thread, which causes a crash in the sourcepawn VM.
// Of course this will not work so well, some collision cases may not be passed to the plugin.

#include "extension.h"
#include "hooks/collisionhooks.h"

CollisionHook g_CollisionHook;
SMEXT_LINK(&g_CollisionHook);

IGameConfig* g_pGameConf = NULL;

IForward* g_pCollisionFwd = NULL;
IForward* g_pPassFwd = NULL;

CPassServerEntityFilterHook g_pPassServerEntityFilterHook;
CShouldCollideHook g_pShouldCollideHook;

bool CollisionHook::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
	char szConfError[256] = "";
	if (!gameconfs->LoadGameConfigFile("collisionhook", &g_pGameConf, szConfError, sizeof(szConfError)))
	{
		snprintf(error, maxlength, "Could not read collisionhook gamedata: %s", szConfError);
		return false;
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);

	if (!g_pPassServerEntityFilterHook.EnableHook(error, maxlength))
	{
		return false;
	}

	sharesys->RegisterLibrary(myself, "collisionhook");

	g_pCollisionFwd = forwards->CreateForward("CH_ShouldCollide", ET_Hook, 3, NULL, Param_Cell, Param_Cell, Param_CellByRef);
	g_pPassFwd = forwards->CreateForward("CH_PassFilter", ET_Hook, 3, NULL, Param_Cell, Param_Cell, Param_CellByRef);

	return true;
}

void CollisionHook::SDK_OnUnload()
{
	g_pPassServerEntityFilterHook.DisableHook();

	forwards->ReleaseForward(g_pCollisionFwd);
	forwards->ReleaseForward(g_pPassFwd);

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool CollisionHook::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	if (!g_pShouldCollideHook.EnableHook(error, maxlen, ismm))
	{
		return false;
	}

	return true;
}

bool CollisionHook::SDK_OnMetamodUnload(char* error, size_t maxlength)
{
	g_pShouldCollideHook.DisableHook();

	return true;
}
