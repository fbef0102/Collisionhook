#ifndef _INCLUDE_COLLISIONHOOK_EXTENSION_H_
#define _INCLUDE_COLLISIONHOOK_EXTENSION_H_

#include "smsdk_ext.h"
#include "ihandleentity.h"

class CollisionHook :
	public SDKExtension,
	public IPluginsListener,
	public IConCommandBaseAccessor
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char* error, size_t maxlength, bool late);

	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	 //virtual void SDK_OnAllLoaded();

	 /**
	  * @brief Called when the pause state is changed.
	  */
	  //virtual void SDK_OnPauseChange(bool paused);

	  /**
	   * @brief this is called when Core wants to know if your extension is working.
	   *
	   * @param error		Error message buffer.
	   * @param maxlength	Size of error message buffer.
	   * @return			True if working, false otherwise.
	   */
	   //virtual bool QueryRunning(char *error, size_t maxlength);
	 
	  /**
	   * Called on server activation before plugins receive the OnServerLoad forward.
	   * 
	   * Parameters:
	   * @param pEdictList	Edicts list.
	   * @param edictCount	Number of edicts in the list.
	   * @param clientMax	Maximum number of clients allowed in the server.
	   */
	 //virtual void OnCoreMapStart(edict_t* pEdictList, int edictCount, int clientMax);

	 /**
	  * Called on level shutdown.
	  */
	 virtual void OnCoreMapEnd();

public: // IPluginsListener
	/**
	 * @brief Called when a plugin's required dependencies and natives have
	 * been bound. Plugins at this phase may be in any state Failed or
	 * lower. This is invoked immediately before OnPluginStart, and sometime
	 * after OnPluginCreated.
	 */
	void OnPluginLoaded(IPlugin* plugin) override;

	/**
	 * @brief Called when a plugin is about to be unloaded. This is called for
	 * any plugin for which OnPluginLoaded was called, and is invoked
	 * immediately after OnPluginEnd(). The plugin may be in any state Failed
	 * or lower.
	 *
	 * This function must not cause the plugin to re-enter script code. If
	 * you wish to be notified of when a plugin is unloading, and to forbid
	 * future calls on that plugin, use OnPluginWillUnload and use a
	 * plugin property to block future calls.
	 */
	void OnPluginUnloaded(IPlugin* plugin) override;

public: // IConCommandBaseAccessor
	/* Flags is a combination of FCVAR flags in cvar.h.
	 * hOut is filled in with a handle to the variable.
	*/
	bool RegisterConCommandBase(ConCommandBase* pVar) override;

public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlength, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodUnload(char* error, size_t maxlength);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlength		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	 //virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlength);
#endif
};

extern bool g_bLateLoad;
extern bool g_bExtLoading;

extern IForward* g_pCollisionFwd;
extern IForward* g_pPassFwd;

#endif // _INCLUDE_COLLISIONHOOK_EXTENSION_H_
