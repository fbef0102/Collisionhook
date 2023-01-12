#ifndef _INCLUDE_CCOLLISIONHOOKS_H_
#define _INCLUDE_CCOLLISIONHOOKS_H_

#include "vphysics_interface.h"
#include "detours.h"

class CShouldCollideHook
{
public:
#ifdef PLATFORM_LINUX
	bool EnableHook(char* error, size_t maxlen, ISmmAPI* ismm = NULL);
#else
	bool EnableHook(char* error, size_t maxlen);
#endif

	void DisableHook();

private:
#ifdef PLATFORM_LINUX
	IPhysicsEnvironment* CreateEnvironment();

	void SetCollisionSolver(IPhysicsCollisionSolver* pSolver);

	int VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2);

	IPhysics* g_pPhysics = NULL;

	int g_iSetCollisionSolverHookId = 0;

	int g_iShouldCollideHookId = 0;
#else
	CDetour* g_pShouldCollideDetour = NULL;
#endif
};

class CPassServerEntityFilterHook
{
public:
	bool EnableHook(char* error, size_t maxlen);

	void DisableHook();

private:
	CDetour* g_pFilterDetour = NULL;
};

#define DETOUR_DECL_STATIC6_STDCALL(name, ret, p1type, p1name, p2type, p2name, p3type, p3name, p4type, p4name, p5type, p5name, p6type, p6name) \
ret (__stdcall *name##_Actual)(p1type, p2type, p3type, p4type, p5type, p6type) = NULL; \
ret __stdcall name(p1type p1name, p2type p2name, p3type p3name, p4type p4name, p5type p5name, p6type p6name)

#define DETOUR_DECL_STATIC4_STDCALL(name, ret, p1type, p1name, p2type, p2name, p3type, p3name, p4type, p4name) \
ret (__stdcall *name##_Actual)(p1type, p2type, p3type, p4type) = NULL; \
ret __stdcall name(p1type p1name, p2type p2name, p3type p3name, p4type p4name)

#endif // _INCLUDE_CCOLLISIONHOOKS_H_