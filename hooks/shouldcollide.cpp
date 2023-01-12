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
	g_iSetCollisionSolverHookId = SH_ADD_VPHOOK(IPhysicsEnvironment, SetCollisionSolver, pEnvironment, SH_MEMBER(this, &CShouldCollideHook::SetCollisionSolver), true);

	SH_REMOVE_HOOK(IPhysics, CreateEnvironment, g_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), true); // No longer needed

	RETURN_META_VALUE(MRES_SUPERCEDE, pEnvironment);
}

void CShouldCollideHook::SetCollisionSolver(IPhysicsCollisionSolver* pSolver)
{
	if (!pSolver)
	{
		RETURN_META(MRES_IGNORED); // this shouldn't happen, but knowing valve...
	}

	// The game installed a solver, globally hook ShouldCollide
	g_iShouldCollideHookId = SH_ADD_VPHOOK(IPhysicsCollisionSolver, ShouldCollide, pSolver, SH_MEMBER(this, &CShouldCollideHook::VPhysics_ShouldCollide), false);

	SH_REMOVE_HOOK_ID(g_iSetCollisionSolverHookId); // No longer needed

	g_iSetCollisionSolverHookId = 0;

	RETURN_META(MRES_IGNORED);
}

#if SOURCE_ENGINE != SE_LEFT4DEAD2
int CShouldCollideHook::VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2)
#else
int CShouldCollideHook::VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2, \
													const PhysicsCollisionRulesCache_t& objCache1, const PhysicsCollisionRulesCache_t& objCache2)
#endif
{
	if (g_pCollisionFwd->GetFunctionCount() == 0)
	{
		RETURN_META_VALUE(MRES_IGNORED, 1); // no plugins are interested, let the game decide
	}

	if (pObj1 == pObj2)
	{
		RETURN_META_VALUE(MRES_IGNORED, 1); // self collisions aren't interesting
	}

	CBaseEntity* pEnt1 = reinterpret_cast<CBaseEntity*>(pGameData1);
	CBaseEntity* pEnt2 = reinterpret_cast<CBaseEntity*>(pGameData2);

	if (!pEnt1 || !pEnt2)
	{
		RETURN_META_VALUE(MRES_IGNORED, 1); // we need two entities
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
		// plugin wants to change the result
		RETURN_META_VALUE(MRES_SUPERCEDE, result == 1);
	}

	// otherwise, game decides
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

bool CShouldCollideHook::EnableHook(char* error, size_t maxlen, ISmmAPI* ismm)
{
	GET_V_IFACE_CURRENT(GetPhysicsFactory, g_pPhysics, IPhysics, VPHYSICS_INTERFACE_VERSION);

	SH_ADD_HOOK(IPhysics, CreateEnvironment, g_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), true);

	return true;
}

void CShouldCollideHook::DisableHook()
{
	SH_REMOVE_HOOK(IPhysics, CreateEnvironment, g_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), true);

	SH_REMOVE_HOOK_ID(g_iSetCollisionSolverHookId);
	SH_REMOVE_HOOK_ID(g_iShouldCollideHookId);

	g_iSetCollisionSolverHookId = 0;
	g_iShouldCollideHookId = 0;

	g_pPhysics = NULL;
}
