#include "extension.h"
#include "collisionhooks.h"

DETOUR_DECL_STATIC2(PassServerEntityFilterFunc, bool, const IHandleEntity*, pTouch, const IHandleEntity*, pPass)
{
#ifdef PLATFORM_WINDOWS
	// Can't call into SourcePawn off thread (left 4 dead 2 - windows crash fix).
	// This function is called not only from the main thread, 
	// which causes a server crash due to the fact that the sourcepawn does not support multithreading.
	if (!ThreadInMainThread())
	{
		return DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass);
	}
#endif

	if (g_pPassFwd->GetFunctionCount() == 0)
	{
		return DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass);
	}

	if (pTouch == pPass)
	{
		return DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass); // self checks aren't interesting
	}

	if (!pTouch || !pPass)
	{
		return DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass); // need two valid entities
	}

	CBaseEntity* pEnt1 = const_cast<CBaseEntity*>(UTIL_EntityFromEntityHandle(pTouch));
	CBaseEntity* pEnt2 = const_cast<CBaseEntity*>(UTIL_EntityFromEntityHandle(pPass));

	if (!pEnt1 || !pEnt2)
	{
		return DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass); // we need both entities
	}

	cell_t ent1 = gamehelpers->EntityToBCompatRef(pEnt1);
	cell_t ent2 = gamehelpers->EntityToBCompatRef(pEnt2);

	// todo: do we want to fill result with with the game's result? perhaps the forward path is more performant...
	cell_t result = 0;
	g_pPassFwd->PushCell(ent1);
	g_pPassFwd->PushCell(ent2);
	g_pPassFwd->PushCellByRef(&result);

	cell_t retValue = Pl_Continue;
	g_pPassFwd->Execute(&retValue);

	if (retValue > Pl_Continue)
	{
		// plugin wants to change the result
		return (result == 1);
	}

	// otherwise, game decides
	return DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass);
}

bool CPassServerEntityFilterHook::EnableHook(char* error, size_t maxlen)
{
	g_pFilterDetour = DETOUR_CREATE_STATIC(PassServerEntityFilterFunc, "PassServerEntityFilter");
	if (!g_pFilterDetour)
	{
		snprintf(error, maxlen, "Unable to hook PassServerEntityFilter!");

		return false;
	}

	g_pFilterDetour->EnableDetour();

	return true;
}

void CPassServerEntityFilterHook::DisableHook()
{
	if (g_pFilterDetour)
	{
		g_pFilterDetour->Destroy();
		g_pFilterDetour = NULL;
	}
}