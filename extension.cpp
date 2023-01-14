// Added commit:
// Fix hook leaks, affecting FPS over map restarts (Adrianilloo commit)
// https://github.com/Adrianilloo/Collisionhook/commit/567a221693b0f6ae223950847607e3a007beefa8
//
// Functions 'CreateEnvironment' and 'SetCollisionSolver' are executed at map start each time, so this can happen.
// Pointer 'pSolver' passed to function 'SetCollisionSolver' is global and this pointer never changes, so we don't need to make a new hook 'ShouldCollide' every map.
//
// Fixed crash #1 in game left4dead2 on windows, although linux should also have a crash, but the stack never broke there.
// After update 'The Last Stand' in 'CCollisionEvent::ShouldCollide' added 2 new parameters.
// The class inherits interface 'IPhysicsCollisionSolver', so it must also have these parameters.
// https://github.com/alliedmodders/hl2sdk/pull/123
//
// Fixed crash #2 in game left4dead2 on windows.
// The PassServerEntityFilter function is not only called from the main thread, which causes a crash in the sourcepawn VM.
// Of course this will not work so well, some collision cases may not be passed to the plugin.
//
// Added the ability to dynamically disable hooks if it is not used anywhere (if no plugin requires it).
// Added command 'sm_collisionhook', this command shows the current state of the hooks.
// Note: extension does not support late loading, and never did.

#include "extension.h"
#include "hooks/collisionhooks.h"

bool g_bLateLoad = true;
bool g_bExtLoading = false;

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

	if (!g_pPassServerEntityFilterHook.CreateHook(error, maxlength))
	{
		return false;
	}

	sharesys->RegisterLibrary(myself, "collisionhook");

	g_pCollisionFwd = forwards->CreateForward("CH_ShouldCollide", ET_Hook, 3, NULL, Param_Cell, Param_Cell, Param_CellByRef);
	g_pPassFwd = forwards->CreateForward("CH_PassFilter", ET_Hook, 3, NULL, Param_Cell, Param_Cell, Param_CellByRef);

	ConVar_Register(0, this);

	if (late)
	{
		Msg("Extension does not support late loading, most likely forward 'CH_ShouldCollide' will not work on this map!""\n");
		g_pSM->LogError(myself, "Extension does not support late loading, most likely forward 'CH_ShouldCollide' will not work on this map!");

		// Plugins can be loaded if the REQUIRE EXTENSIONS macro is not specified.
		// Need to check if plugins that work with this extension are loaded.
		g_pShouldCollideHook.EnableHook();
		g_pPassServerEntityFilterHook.EnableHook();
	}

	g_bLateLoad = late;
	g_bExtLoading = true;

	plsys->AddPluginsListener(&g_CollisionHook);

	return true;
}

void CollisionHook::SDK_OnUnload()
{
	ConVar_Unregister();

	plsys->RemovePluginsListener(&g_CollisionHook);

	g_pPassServerEntityFilterHook.DestroyHook();

	forwards->ReleaseForward(g_pCollisionFwd);
	forwards->ReleaseForward(g_pPassFwd);

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

bool CollisionHook::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	g_bExtLoading = true;

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

	if (!g_pShouldCollideHook.CreateHook(error, maxlen, ismm))
	{
		return false;
	}

	return true;
}

bool CollisionHook::SDK_OnMetamodUnload(char* error, size_t maxlength)
{
	g_pShouldCollideHook.DestroyHook();

	return true;
}

void CollisionHook::OnCoreMapEnd()
{
	g_pShouldCollideHook.OnCoreMapEnd();
}

void CollisionHook::OnPluginLoaded(IPlugin* plugin)
{
	g_pShouldCollideHook.EnableHook(true);
	g_pPassServerEntityFilterHook.EnableHook();
}

void CollisionHook::OnPluginUnloaded(IPlugin* plugin)
{
	g_pShouldCollideHook.DisableHook();
	g_pPassServerEntityFilterHook.DisableHook();
}

CON_COMMAND(sm_collisionhook, "Shows current state of hooks in extension 'CollisionHook'")
{
	Msg("State of hooks in extension 'CollisionHooks':""\n\n");

	Msg("Hook 'CCollisionEvent::ShouldCollide' is currently %s! Number of active forwards: %d!""\n", (g_pShouldCollideHook.IsHookEnabled()) ? "enabled" : "disabled", g_pCollisionFwd->GetFunctionCount());
	Msg("Hook 'PassServerEntityFilter' is currently %s! Number of active forwards: %d!""\n\n", (g_pPassServerEntityFilterHook.IsHookEnabled()) ? "enabled" : "disabled", g_pPassFwd->GetFunctionCount());
}

bool CollisionHook::RegisterConCommandBase(ConCommandBase* pVar)
{
	// Notify metamod of ownership
	return META_REGCVAR(pVar);
}
