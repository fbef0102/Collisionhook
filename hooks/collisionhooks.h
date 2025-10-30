#ifndef _INCLUDE_CCOLLISIONHOOKS_H_
#define _INCLUDE_CCOLLISIONHOOKS_H_

#include "vphysics_interface.h"
#include "detours.h"

class CShouldCollideHook
{
public:
	// ISmmAPI* ismm - for GetPhysicsFactory
	bool CreateHook(char* error, size_t maxlen, ISmmAPI* ismm);
	void DestroyHook();

	void EnableHook(bool bPluginLoad = false);
	void DisableHook();

	void ShowErrorMessage(bool bPluginLoad = false);

	inline void OnCoreMapEnd()
	{
		m_bWarningMsgDisplayed = false;
	}

	inline bool IsHookEnabled()
	{
		return (m_iShouldCollideHookId != 0);
	}
	
private:
	IPhysicsEnvironment* CreateEnvironment();

	void SetCollisionSolver(IPhysicsCollisionSolver* pSolver);

#if SOURCE_ENGINE != SE_LEFT4DEAD2
	int VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2);
#else
	int VPhysics_ShouldCollide(IPhysicsObject* pObj1, IPhysicsObject* pObj2, void* pGameData1, void* pGameData2, \
									const PhysicsCollisionRulesCache_t& objCache1, const PhysicsCollisionRulesCache_t& objCache2);
#endif
	/**
	 * Pointer to class 'CCollisionEvent' in game.
	 * CCollisionEvent g_Collisions;
	 * Can never point to NULL, if the extension was not loaded after the map was loaded.
	 */
	IPhysicsCollisionSolver* m_pSolver = NULL;

	IPhysics* m_pPhysics = NULL;

	int m_iSetCollisionSolverHookId = 0;

	int m_iShouldCollideHookId = 0;

	bool m_bWarningMsgDisplayed = false;
};

class CPassServerEntityFilterHook
{
public:
	bool CreateHook(char* error, size_t maxlen);
	void DestroyHook();

	void EnableHook();
	void DisableHook();

	static cell_t EntityFromEntityHandle(const IHandleEntity* pHandleEntity);

	inline bool IsHookEnabled()
	{
		return (m_bFilterDetourEnabled);
	}

private:
	CDetour* m_pFilterDetour = NULL;

	bool m_bFilterDetourEnabled = false;
};

#if SOURCE_ENGINE == SE_TF2 && defined(PLATFORM_LINUX) && defined(__i386__)
	#define DETOUR_DECL_STATIC2_REGPARM(name, ret, p1type, p1name, p2type, p2name) \
		ret (*name##_Actual)(p1type, p2type) __attribute__((regparm(2))) = NULL; \
		ret name(p1type p1name, p2type p2name)

	#define DETOUR_CUSTOM_STATIC2 DETOUR_DECL_STATIC2_REGPARM
#elif SOURCE_ENGINE == SE_CSGO && defined(PLATFORM_WINDOWS)
	#define DETOUR_DECL_STATIC2_FASTCALL(name, ret, p1type, p1name, p2type, p2name) \
		ret (__fastcall *name##_Actual)(p1type, p2type) = NULL; \
		ret __fastcall name(p1type p1name, p2type p2name)

	#define DETOUR_CUSTOM_STATIC2 DETOUR_DECL_STATIC2_FASTCALL
#else
	#define DETOUR_CUSTOM_STATIC2 DETOUR_DECL_STATIC2
#endif

#endif // _INCLUDE_CCOLLISIONHOOKS_H_
