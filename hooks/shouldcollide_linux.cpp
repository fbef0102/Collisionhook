#include "extension.h"
#include "sourcehook.h"
#include "collisionhooks.h"

SH_DECL_HOOK0(IPhysics, CreateEnvironment, SH_NOATTRIB, 0, IPhysicsEnvironment*);
SH_DECL_HOOK1_void(IPhysicsEnvironment, SetCollisionSolver, SH_NOATTRIB, 0, IPhysicsCollisionSolver*);
SH_DECL_HOOK4(IPhysicsCollisionSolver, ShouldCollide, SH_NOATTRIB, 0, int, IPhysicsObject*, IPhysicsObject*, void*, void*);

IPhysicsEnvironment* CShouldCollideHook::CreateEnvironment()
{
	// in order to hook IPhysicsCollisionSolver::ShouldCollide, we need to know when a solver is installed
	// in order to hook any installed solvers, we need to hook any created physics environments
	IPhysicsEnvironment* pEnvironment = SH_CALL(g_pPhysics, &IPhysics::CreateEnvironment)();

	if (!pEnvironment)
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, pEnvironment); // just in case
	}

	// hook so we know when a solver is installed
	SH_ADD_HOOK(IPhysicsEnvironment, SetCollisionSolver, pEnvironment, SH_MEMBER(this, &CShouldCollideHook::SetCollisionSolver), false);

	RETURN_META_VALUE(MRES_SUPERCEDE, pEnvironment);
}

void CShouldCollideHook::SetCollisionSolver(IPhysicsCollisionSolver* pSolver)
{
	if (!pSolver)
	{
		RETURN_META(MRES_IGNORED); // this shouldn't happen, but knowing valve...
	}

	// the game is installing a solver, hook the func we want
	SH_ADD_HOOK(IPhysicsCollisionSolver, ShouldCollide, pSolver, SH_MEMBER(this, &CShouldCollideHook::VPhysics_ShouldCollide), false);

	RETURN_META(MRES_IGNORED);
}

int CShouldCollideHook::VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2)
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

	SH_ADD_HOOK(IPhysics, CreateEnvironment, g_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), false);

	return true;
}

void CShouldCollideHook::DisableHook()
{
	SH_REMOVE_HOOK(IPhysics, CreateEnvironment, g_pPhysics, SH_MEMBER(this, &CShouldCollideHook::CreateEnvironment), false);

	g_pPhysics = NULL;
}
