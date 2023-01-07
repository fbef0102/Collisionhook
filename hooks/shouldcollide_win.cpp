#include "extension.h"
#include "collisionhooks.h"

DETOUR_DECL_STATIC6_STDCALL(ShouldCollideFunc, int, IPhysicsObject*, pObj1, IPhysicsObject*, pObj2, void*, pGameData1, void*, pGameData2, \
								void*, espFix1, void*, espFix2)
{
	// Can't call into SourcePawn off thread (left 4 dead 2 - windows crash fix).
	// It seems this function is called only from the main thread unlike function 'PassServerEntityFilterFunc',
	// so there is no need to check.
	/*if (!ThreadInMainThread())
	{
		return DETOUR_STATIC_CALL(ShouldCollideFunc)(pObj1, pObj2, pGameData1, pGameData2, espFix1, espFix2);
	}*/

	if (g_pCollisionFwd->GetFunctionCount() == 0)
	{
		// no plugins are interested, let the game decide
		return DETOUR_STATIC_CALL(ShouldCollideFunc)(pObj1, pObj2, pGameData1, pGameData2, espFix1, espFix2);
	}

	if (pObj1 == pObj2)
	{
		// self collisions aren't interesting
		return DETOUR_STATIC_CALL(ShouldCollideFunc)(pObj1, pObj2, pGameData1, pGameData2, espFix1, espFix2);
	}

	CBaseEntity *pEnt1 = static_cast<CBaseEntity *>(pGameData1);
	CBaseEntity *pEnt2 = static_cast<CBaseEntity *>(pGameData2);

	if (!pEnt1 || !pEnt2)
	{
		// we need two entities
		return DETOUR_STATIC_CALL(ShouldCollideFunc)(pObj1, pObj2, pGameData1, pGameData2, espFix1, espFix2);
	}

	cell_t ent1 = gamehelpers->EntityToBCompatRef(pEnt1);
	cell_t ent2 = gamehelpers->EntityToBCompatRef(pEnt2);

	// todo: do we want to fill result with with the game's result? perhaps the forward path is more performant...
	cell_t result = 0;
	g_pCollisionFwd->PushCell(ent1);
	g_pCollisionFwd->PushCell(ent2);
	g_pCollisionFwd->PushCellByRef(&result);

	cell_t retValue = Pl_Continue;
	g_pCollisionFwd->Execute(&retValue);

	if (retValue > Pl_Continue)
	{
		return (result == 1); // plugin wants to change the result
	}

	// otherwise, game decides
	return DETOUR_STATIC_CALL(ShouldCollideFunc)(pObj1, pObj2, pGameData1, pGameData2, espFix1, espFix2);
}

bool CShouldCollideHook::EnableHook(char* error, size_t maxlen)
{
	g_pShouldCollideDetour = DETOUR_CREATE_STATIC(ShouldCollideFunc, "CCollisionEvent::ShouldCollide");
	if (!g_pShouldCollideDetour)
	{
		snprintf(error, maxlen, "Unable to hook CCollisionEvent::ShouldCollide!");

		return false;
	}

	g_pShouldCollideDetour->EnableDetour();

	return true;
}

void CShouldCollideHook::DisableHook()
{
	if (g_pShouldCollideDetour)
	{
		g_pShouldCollideDetour->Destroy();
		g_pShouldCollideDetour = NULL;
	}
}