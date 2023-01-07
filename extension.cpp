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

#ifdef PLATFORM_WINDOWS
	if (!g_pShouldCollideHook.EnableHook(error, maxlength))
	{
		return false;
	}
#endif

	sharesys->RegisterLibrary(myself, "collisionhook");

	g_pCollisionFwd = forwards->CreateForward("CH_ShouldCollide", ET_Hook, 3, NULL, Param_Cell, Param_Cell, Param_CellByRef);
	g_pPassFwd = forwards->CreateForward("CH_PassFilter", ET_Hook, 3, NULL, Param_Cell, Param_Cell, Param_CellByRef);

	return true;
}

void CollisionHook::SDK_OnUnload()
{
	g_pPassServerEntityFilterHook.DisableHook();

#ifdef PLATFORM_WINDOWS
	g_pShouldCollideHook.DisableHook();
#endif

	forwards->ReleaseForward(g_pCollisionFwd);
	forwards->ReleaseForward(g_pPassFwd);

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

#ifdef PLATFORM_LINUX
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
#endif
