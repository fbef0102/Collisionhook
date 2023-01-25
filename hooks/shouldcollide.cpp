#include "extension.h"
#include "sourcehook.h"
#include "collisionhooks.h"

SH_DECL_HOOK0(IPhysics, CreateEnvironment, SH_NOATTRIB, 0, IPhysicsEnvironment*);
SH_DECL_HOOK1_void(IPhysicsEnvironment, SetCollisionSolver, SH_NOATTRIB, 0, IPhysicsCollisionSolver*);

#if SOURCE_ENGINE != SE_LEFT4DEAD2
	SH_DECL_HOOK4(IPhysicsCollisionSolver, ShouldCollide, SH_NOATTRIB, 0, int, IPhysicsObject*, IPhysicsObject*, void*, void*);
#else
	SH_DECL_HOOK6(IPhysicsCollisionSolver, ShouldCollide, SH_NOATTRIB, 0, int, IPhysicsObject*, IPhysicsObject*, void*, void*, \
					const PhysicsCollisionRulesCache_t&, const PhysicsCollisionRulesCache_t&);
#endif

IPhysicsEnvironment* CShouldCollideHook::CreateEnvironment()
{
	// in order to hook IPhysicsCollisionSolver::ShouldCollide, we need to know when a solver is installed
	// in order to hook any installed solvers, we need to hook any created physics environments
	
	IPhysicsEnvironment* pEnvironment = META_RESULT_ORIG_RET(IPhysicsEnvironment*);

	if (!pEnvironment)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, pEnvironment); // just in case
	}

	// Hook globally so we know when any solver is installed
	m_iSetCollisionSolverHookId = SH_ADD_VPHOOK(IPhysicsEnvironment, SetCollisionSolver, pEnvironment, SH_MEMBER(this, &CShouldCollideHook::SetCollisionSolver), true);

	SH_REMOVE_HOOK(IPhysics, CreateEnvironment, m_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), true); // No longer needed

	RETURN_META_VALUE(MRES_SUPERCEDE, pEnvironment);
}

void CShouldCollideHook::SetCollisionSolver(IPhysicsCollisionSolver* pSolver)
{
	if (!pSolver)
	{
		RETURN_META(MRES_IGNORED); // this shouldn't happen, but knowing valve...
	}

	m_pSolver = pSolver;
	g_bExtLoading = false;

	// We got a pointer to interface 'IPhysicsCollisionSolver', we try to activate the hook, if some plugin(s) require it.
	EnableHook();

	SH_REMOVE_HOOK_ID(m_iSetCollisionSolverHookId); // No longer needed
	m_iSetCollisionSolverHookId = 0;

	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE != SE_LEFT4DEAD2
int CShouldCollideHook::VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2)
#else
int CShouldCollideHook::VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2, \
													const PhysicsCollisionRulesCache_t& objCache1, const PhysicsCollisionRulesCache_t& objCache2)
#endif
{
	if (pObj1 == pObj2)
	{
		RETURN_META_VALUE(MRES_IGNORED, 1); // self collisions aren't interesting
	}

	cell_t iEnt1 = gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity*>(pGameData1));
	cell_t iEnt2 = gamehelpers->EntityToBCompatRef(reinterpret_cast<CBaseEntity*>(pGameData2));

	// Checking the pointer for zero is already inside function 'EntityToBCompatRef'
	if (iEnt1 == INVALID_EHANDLE_INDEX || iEnt2 == INVALID_EHANDLE_INDEX)
	{
		RETURN_META_VALUE(MRES_IGNORED, 1); // we need two entities
	}

	// todo: do we want to fill result with with the game's result? perhaps the forward path is more performant...
	cell_t bResult = 0;
	g_pCollisionFwd->PushCell(iEnt1);
	g_pCollisionFwd->PushCell(iEnt2);
	g_pCollisionFwd->PushCellByRef(&bResult);

	cell_t iPlRetValue = Pl_Continue;
	g_pCollisionFwd->Execute(&iPlRetValue);

	if (iPlRetValue > Pl_Continue)
	{
		// plugin wants to change the result
		RETURN_META_VALUE(MRES_SUPERCEDE, (bResult == 1));
	}

	// otherwise, game decides
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

bool CShouldCollideHook::CreateHook(char* error, size_t maxlen, ISmmAPI* ismm)
{
	GET_V_IFACE_CURRENT(GetPhysicsFactory, m_pPhysics, IPhysics, VPHYSICS_INTERFACE_VERSION);

	SH_ADD_HOOK(IPhysics, CreateEnvironment, m_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), true);

	return true;
}

void CShouldCollideHook::DestroyHook()
{
	SH_REMOVE_HOOK(IPhysics, CreateEnvironment, m_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), true);

	SH_REMOVE_HOOK_ID(m_iSetCollisionSolverHookId);
	SH_REMOVE_HOOK_ID(m_iShouldCollideHookId);

	m_iSetCollisionSolverHookId = 0;
	m_iShouldCollideHookId = 0;

	m_pPhysics = NULL;
}

void CShouldCollideHook::EnableHook(bool bPluginLoad)
{
	if (IsHookEnabled())
	{
		return;
	}

	if (g_pCollisionFwd->GetFunctionCount() == 0)
	{
		return;
	}

	// This should not happen if the extension was loaded correctly before the map was loaded.
	// It's better to add a check to avoid crashes.
	if (m_pSolver == NULL)
	{
		ShowErrorMessage(bPluginLoad);

		return;
	}

	// The game installed a solver, globally hook ShouldCollide
	m_iShouldCollideHookId = SH_ADD_VPHOOK(IPhysicsCollisionSolver, ShouldCollide, m_pSolver, SH_MEMBER(this, &CShouldCollideHook::VPhysics_ShouldCollide), false);
}

void CShouldCollideHook::ShowErrorMessage(bool bPluginLoad)
{
	// Do not display an error message, the extension was not loaded too late, 
	// pointer 'm_pSolver' has not yet been set because the map has not loaded yet.
	if (g_bExtLoading && !g_bLateLoad)
	{
		return;
	}

	// Spam prevention.
	// We will spam messages every time a new plugin is loaded.
	if (bPluginLoad && m_bWarningMsgDisplayed)
	{
		return;
	}

	// This should not happen unless the function is done too early, e.g. from function 'SDK_OnMetamodLoad', while the extension is not fully loaded.
	// In any case, it is better to add a check so that there are no crashes.
	// We call this function only after the plugin is loaded.
	if (g_pSM)
	{
		g_pSM->LogError(myself, "Pointer g_pSolver is NULL! Was this extension uploaded late?");
		g_pSM->LogError(myself, "Forward 'CH_ShouldCollide' will not work on this map!");
	}

	Msg("Pointer g_pSolver is NULL! Was this extension uploaded late?""\n");
	Msg("Forward 'CH_ShouldCollide' will not work on this map!""\n");

	m_bWarningMsgDisplayed = true;
}

void CShouldCollideHook::DisableHook()
{
	if (!IsHookEnabled())
	{
		return;
	}

	if (g_pCollisionFwd->GetFunctionCount() > 0)
	{
		return;
	}

	SH_REMOVE_HOOK_ID(m_iShouldCollideHookId);
	m_iShouldCollideHookId = 0;
}
