#include "extension.h"
#include "collisionhooks.h"

/* SDK 2013

//-----------------------------------------------------------------------------
// Converts an IHandleEntity to an CBaseEntity
//-----------------------------------------------------------------------------
inline const CBaseEntity *EntityFromEntityHandle( const IHandleEntity *pConstHandleEntity )
{
	IHandleEntity *pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

#ifdef CLIENT_DLL
	IClientUnknown *pUnk = (IClientUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#else
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return pUnk->GetBaseEntity();
#endif
}

//-----------------------------------------------------------------------------
//
// Shared client/server trace filter code
//
//-----------------------------------------------------------------------------
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass ) 
{
	if ( !pPass )
		return true;

	if ( pTouch == pPass )
		return false;

	const CBaseEntity *pEntTouch = EntityFromEntityHandle( pTouch );
	const CBaseEntity *pEntPass = EntityFromEntityHandle( pPass );
	if ( !pEntTouch || !pEntPass )
		return true;

	// don't clip against own missiles
	if ( pEntTouch->GetOwnerEntity() == pEntPass )
		return false;
	
	// don't clip against owner
	if ( pEntPass->GetOwnerEntity() == pEntTouch )
		return false;	


	return true;
}
*/

DETOUR_DECL_STATIC2(PassServerEntityFilterFunc, bool, const IHandleEntity*, pTouch, const IHandleEntity*, pPass)
{
#ifdef PLATFORM_WINDOWS
	// Can't call into SourcePawn off thread (left 4 dead2 and left 4 dead - windows crash fix).
	// This function is called not only from the main thread, 
	// which causes a server crash due to the fact that the sourcepawn does not support multithreading.
	// I don't know about other games.
	if (!ThreadInMainThread())
	{
		return DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass);
	}
#endif

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

bool CPassServerEntityFilterHook::CreateHook(char* error, size_t maxlen)
{
	m_pFilterDetour = DETOUR_CREATE_STATIC(PassServerEntityFilterFunc, "PassServerEntityFilter");
	if (!m_pFilterDetour)
	{
		snprintf(error, maxlen, "Unable to hook PassServerEntityFilter!");

		return false;
	}

	return true;
}

void CPassServerEntityFilterHook::DestroyHook()
{
	if (m_pFilterDetour)
	{
		m_pFilterDetour->Destroy();
		m_pFilterDetour = NULL;
	}
}

void CPassServerEntityFilterHook::EnableHook()
{
	if (IsHookEnabled())
	{
		return;
	}

	if (g_pPassFwd->GetFunctionCount() == 0)
	{
		return;
	}

	m_bFilterDetourEnabled = true;

	// re-enable check should be inside
	m_pFilterDetour->EnableDetour();
}

void CPassServerEntityFilterHook::DisableHook()
{
	if (!IsHookEnabled())
	{
		return;
	}

	if (g_pPassFwd->GetFunctionCount() > 0)
	{
		return;
	}

	// re-disable check should be inside
	m_pFilterDetour->DisableDetour();

	m_bFilterDetourEnabled = false;
}
