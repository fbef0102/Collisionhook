#include "extension.h"
#include "collisionhooks.h"

DETOUR_DECL_STATIC2(PassServerEntityFilterFunc, bool, const IHandleEntity*, pTouch, const IHandleEntity*, pPass)
{
	cell_t iOriginalRet = DETOUR_STATIC_CALL(PassServerEntityFilterFunc)(pTouch, pPass);

#ifdef PLATFORM_WINDOWS
	// Can't call into SourcePawn off thread (left 4 dead2 and left 4 dead - windows crash fix).
	// This function is called not only from the main thread, 
	// which causes a server crash due to the fact that the sourcepawn does not support multithreading.
	// I don't know about other games.
	if (!ThreadInMainThread())
	{
		return iOriginalRet;
	}
#endif

	if (pTouch == pPass)
	{
		return iOriginalRet; // self checks aren't interesting
	}

	cell_t iEnt1 = CPassServerEntityFilterHook::EntityFromEntityHandle(pTouch);
	cell_t iEnt2 = CPassServerEntityFilterHook::EntityFromEntityHandle(pPass);

	// Checking the pointer for zero is already inside function 'EntityToBCompatRef'
	if (iEnt1 == INVALID_EHANDLE_INDEX || iEnt2 == INVALID_EHANDLE_INDEX)
	{
		return iOriginalRet; // we need both entities
	}

	// todo: do we want to fill result with with the game's result? perhaps the forward path is more performant...
	cell_t iPlResult = iOriginalRet;

	g_pPassFwd->PushCell(iEnt1);
	g_pPassFwd->PushCell(iEnt2);
	g_pPassFwd->PushCellByRef(&iPlResult);
	g_pPassFwd->Execute(NULL);

	// If the plugin has changed the result, then we do not allow values other than 1 or 0 to be returned to the game
	return (iPlResult != iOriginalRet) ? (iPlResult == 1) : iOriginalRet;
}

cell_t CPassServerEntityFilterHook::EntityFromEntityHandle(const IHandleEntity* pHandleEntity)
{
	//IHandleEntity* pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

	if (pHandleEntity == NULL)
	{
		return INVALID_EHANDLE_INDEX;
	}

	/*if (staticpropmgr->IsStaticProp(pHandleEntity))
	{
		return NULL;
	}*/

	CBaseHandle hndl = pHandleEntity->GetRefEHandle();

	if (hndl == INVALID_EHANDLE_INDEX)
	{
		return INVALID_EHANDLE_INDEX;
	}

	if (hndl.GetEntryIndex() >= MAX_EDICTS)
	{
		return (hndl.ToInt() | (1 << 31));
	}

	return hndl.GetEntryIndex();
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
