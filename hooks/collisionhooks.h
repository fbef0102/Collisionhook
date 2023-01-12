#ifndef _INCLUDE_CCOLLISIONHOOKS_H_
#define _INCLUDE_CCOLLISIONHOOKS_H_

#include "vphysics_interface.h"
#include "detours.h"

class CShouldCollideHook
{
public:
	bool EnableHook(char* error, size_t maxlen, ISmmAPI* ismm = NULL);

	void DisableHook();

private:
	IPhysicsEnvironment* CreateEnvironment();

	void SetCollisionSolver(IPhysicsCollisionSolver* pSolver);

#if SOURCE_ENGINE != SE_LEFT4DEAD2
	int VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2);
#else
	int VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2, \
									const PhysicsCollisionRulesCache_t& objCache1, const PhysicsCollisionRulesCache_t& objCache2);
#endif

	IPhysics* g_pPhysics = NULL;

	int g_iSetCollisionSolverHookId = 0;

	int g_iShouldCollideHookId = 0;
};

class CPassServerEntityFilterHook
{
public:
	bool EnableHook(char* error, size_t maxlen);

	void DisableHook();

private:
	CDetour* g_pFilterDetour = NULL;
};
 
#endif // _INCLUDE_CCOLLISIONHOOKS_H_
