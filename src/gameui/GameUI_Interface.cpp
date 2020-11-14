//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Implements all the functions exported by the GameUI dll
//
// $NoKeywords: $
//===========================================================================//

#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h>
#include <direct.h>
#include "sys_utils.h"
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <tier0/dbg.h>

#ifdef SendMessage
#undef SendMessage
#endif

#if defined(OSX)
#include <mach-o/dyld_images.h>
#include <mach-o/dyld.h>
#endif

#include "filesystem.h"
#include "GameUI_Interface.h"
#include "string.h"
#include "tier0/icommandline.h"

// interface to engine
#include "EngineInterface.h"

#include "VGuiSystemModuleLoader.h"
#include "bitmap/tgaloader.h"

#include "GameConsole.h"
#include "LoadingDialog.h"
#include "CDKeyEntryDialog.h"
#include "ModInfo.h"
#include "game/client/IGameClientExports.h"
#include "materialsystem/imaterialsystem.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "iachievementmgr.h"
#include "IGameUIFuncs.h"
#include "ienginevgui.h"
#include "video/ivideoservices.h"

// vgui2 interface
// note that GameUI project uses ..\vgui2\include, not ..\utils\vgui\include
#include "vgui/Cursor.h"
#include "tier1/KeyValues.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/IScheme.h"
#include "vgui/IVGui.h"
#include "vgui/ISystem.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/PHandle.h"
#include "tier3/tier3.h"
#include "matsys_controls/matsyscontrols.h"
#include "steam/steam_api.h"
//#include "protocol.h"
#include "game/server/iplayerinfo.h"
#include "avi/iavi.h"

#include <vgui/IInput.h>

#include "BasePanel.h"
IBasePanel* gBasePanel = NULL;
inline IBasePanel* GetBasePanel() { return gBasePanel; }

ConVar of_pausegame( "of_pausegame", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED, "If set, pauses whenever you open the in game menu." );;

ConVar ui_scaling("ui_scaling", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Scales VGUI elements with different screen resolutions.");

#include "tier0/dbg.h"
#include "engine/IEngineSound.h"
#include "gameui_util.h"

#include "vgui_controls/AnimationController.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef GAMEUI_EMBEDDED
IVEngineClient *engine = NULL;
IGameUIFuncs *gameuifuncs = NULL;
CGlobalVarsBase *gpGlobals = NULL;
IEngineSound *enginesound = NULL;
ISoundEmitterSystemBase *soundemitterbase = NULL;

static CSteamAPIContext g_SteamAPIContext;
CSteamAPIContext *steamapicontext = &g_SteamAPIContext;
#endif

IEngineVGui *enginevguifuncs = NULL;
vgui::ISurface *enginesurfacefuncs = NULL;
IAchievementMgr *achievementmgr = NULL;

CGameUI *g_pGameUI = NULL;

vgui::DHANDLE<CLoadingDialog> g_hLoadingDialog;
static CGameUI g_GameUI;
static WHANDLE g_hMutex = NULL;
static WHANDLE g_hWaitMutex = NULL;

static IGameClientExports *g_pGameClientExports = NULL;
IGameClientExports *GameClientExports()
{
	return g_pGameClientExports;
}

//-----------------------------------------------------------------------------
// Purpose: singleton accessor
//-----------------------------------------------------------------------------
CGameUI &GameUI()
{
	return g_GameUI;
}

//-----------------------------------------------------------------------------
// Purpose: hack function to give the module loader access to the main panel handle
//			only used in VguiSystemModuleLoader
//-----------------------------------------------------------------------------
vgui::VPANEL GetGameUIBasePanel()
{
	if (!enginevguifuncs)
	{
		Assert(0);
	}
	return enginevguifuncs->GetPanel(PANEL_GAMEUIDLL);
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameUI, IGameUI, GAMEUI_INTERFACE_VERSION, g_GameUI);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGameUI::CGameUI()
{
	g_pGameUI = this;
	m_bTryingToLoadFriends = false;
	m_iFriendsLoadPauseFrames = 0;
	m_iGameIP = 0;
	m_iGameConnectionPort = 0;
	m_iGameQueryPort = 0;
	m_bActivatedUI = false;
	m_szPreviousStatusText[0] = 0;
	m_bHasSavedThisMenuSession = false;
	m_bOpenProgressOnStart = false;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameUI::~CGameUI()
{
	g_pGameUI = NULL;
}

//#pragma message(FILE_LINE_STRING " !!FIXME!! replace all this with Sys_LoadGameModule")
void *GetGameInterface(const char *dll, const char *name)
{
	const char *pGameDir = CommandLine()->ParmValue("-game", "hl2");
	pGameDir = VarArgs("%s/bin/%s", pGameDir, dll);
	CSysModule *module = Sys_LoadModule(pGameDir);
	CreateInterfaceFn factory = Sys_GetFactory(module);
	return factory(name, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: Initialization
//-----------------------------------------------------------------------------
void CGameUI::Initialize( CreateInterfaceFn factory )
{
	MEM_ALLOC_CREDIT();
	ConnectTier1Libraries( &factory, 1 );
	ConnectTier2Libraries( &factory, 1 );
	ConVar_Register( FCVAR_CLIENTDLL );
	ConnectTier3Libraries( &factory, 1 );

	//#pragma message(FILE_LINE_STRING " !!FIXME!!")
	gpGlobals = ((IPlayerInfoManager *)GetGameInterface("server.dll", INTERFACEVERSION_PLAYERINFOMANAGER))->GetGlobalVars();

	gameuifuncs = (IGameUIFuncs *)factory(VENGINE_GAMEUIFUNCS_VERSION, NULL);
	soundemitterbase = (ISoundEmitterSystemBase *)factory(SOUNDEMITTERSYSTEM_INTERFACE_VERSION, NULL);

	enginesound = (IEngineSound *)factory(IENGINESOUND_CLIENT_INTERFACE_VERSION, NULL);
	engine = (IVEngineClient *)factory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );

	g_pVideo = (IVideoServices *)factory(VIDEO_SERVICES_INTERFACE_VERSION, NULL);

#if !defined _X360 && !defined NO_STEAM
	SteamAPI_InitSafe();
	steamapicontext->Init();
#endif

	vgui::VGui_InitInterfacesList( "GameUI", &factory, 1 );
	vgui::VGui_InitMatSysInterfacesList( "GameUI", &factory, 1 );

	// load localization file
	g_pVGuiLocalize->AddFile( "Resource/gameui_%language%.txt", "GAME", true );

	// load mod info
	ModInfo().LoadCurrentGameInfo();

	// load localization file for kb_act.lst
	g_pVGuiLocalize->AddFile( "Resource/valve_%language%.txt", "GAME", true );

	enginevguifuncs = (IEngineVGui *)factory( VENGINE_VGUI_VERSION, NULL );
	enginesurfacefuncs = (vgui::ISurface *)factory(VGUI_SURFACE_INTERFACE_VERSION, NULL);
	gameuifuncs = (IGameUIFuncs *)factory( VENGINE_GAMEUIFUNCS_VERSION, NULL );
	if (!enginesurfacefuncs || !gameuifuncs || !enginevguifuncs)
	{
		Warning( "CGameUI::Initialize() failed to get necessary interfaces\n" );
	}
}

void CGameUI::PostInit()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the specified panel as the background panel for the loading
//		dialog.  If NULL, default background is used.  If you set a panel,
//		it should be full-screen with an opaque background, and must be a VGUI popup.
//-----------------------------------------------------------------------------
void CGameUI::SetLoadingBackgroundDialog( vgui::VPANEL panel )
{
	// MrModez: No need to implement, all logic is in BaseModPanel
}

//-----------------------------------------------------------------------------
// Purpose: connects to client interfaces
//-----------------------------------------------------------------------------
void CGameUI::Connect( CreateInterfaceFn gameFactory )
{
	g_pGameClientExports = (IGameClientExports *)gameFactory(GAMECLIENTEXPORTS_INTERFACE_VERSION, NULL);

	achievementmgr = engine->GetAchievementMgr();

	if (!g_pGameClientExports)
	{
		Warning("CGameUI::Initialize() failed to get necessary interfaces\n");
	}

	m_GameFactory = gameFactory;
}

#ifdef _WIN32
//-----------------------------------------------------------------------------
// Purpose: Callback function; sends platform Shutdown message to specified window
//-----------------------------------------------------------------------------
int __stdcall SendShutdownMsgFunc(WHANDLE hwnd, int lparam)
{
	Sys_PostMessage(hwnd, Sys_RegisterWindowMessage("ShutdownValvePlatform"), 0, 1);
	return 1;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Searches for GameStartup*.mp3 files in the sound/ui folder and plays one
//-----------------------------------------------------------------------------
void CGameUI::PlayGameStartupSound()
{
	// MrModez: Moved to BaseModPanel
}

//-----------------------------------------------------------------------------
// Purpose: Called to setup the game UI
//-----------------------------------------------------------------------------
void CGameUI::Start()
{
	// determine Steam location for configuration
	if ( !FindPlatformDirectory( m_szPlatformDir, sizeof( m_szPlatformDir ) ) )
		return;

	if ( IsPC() )
	{
		// setup config file directory
		char szConfigDir[512];
		Q_strncpy( szConfigDir, m_szPlatformDir, sizeof( szConfigDir ) );
		Q_strncat( szConfigDir, "config", sizeof( szConfigDir ), COPY_ALL_CHARACTERS );

		DevMsg( "[GameUI] Steam config directory: %s\n", szConfigDir );

		g_pFullFileSystem->AddSearchPath(szConfigDir, "CONFIG");
		g_pFullFileSystem->CreateDirHierarchy("", "CONFIG");

		// user dialog configuration
		vgui::system()->SetUserConfigFile("InGameDialogConfig.vdf", "CONFIG");

		g_pFullFileSystem->AddSearchPath( "platform", "PLATFORM" );
	}

	// localization
	g_pVGuiLocalize->AddFile( "Resource/platform_%language%.txt");
	g_pVGuiLocalize->AddFile( "Resource/vgui_%language%.txt");
#ifdef _WIN32
	Sys_SetLastError( SYS_NO_ERROR );

	g_hMutex = Sys_CreateMutex( "ValvePlatformUIMutex" );
	g_hWaitMutex = Sys_CreateMutex( "ValvePlatformWaitMutex" );
	if ( g_hMutex == NULL || g_hWaitMutex == NULL || Sys_GetLastError() == SYS_ERROR_INVALID_HANDLE )
	{
		// error, can't get handle to mutex
		if (g_hMutex)
		{
			Sys_ReleaseMutex(g_hMutex);
		}
		if (g_hWaitMutex)
		{
			Sys_ReleaseMutex(g_hWaitMutex);
		}
		g_hMutex = NULL;
		g_hWaitMutex = NULL;
		Warning("Steam Error: Could not access Steam, bad mutex\n");
		return;
	}
	unsigned int waitResult = Sys_WaitForSingleObject(g_hMutex, 0);
	if (!(waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED))
	{
		// mutex locked, need to deactivate Steam (so we have the Friends/ServerBrowser data files)
		// get the wait mutex, so that Steam.exe knows that we're trying to acquire ValveTrackerMutex
		waitResult = Sys_WaitForSingleObject(g_hWaitMutex, 0);
		if (waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED)
		{
			Sys_EnumWindows(SendShutdownMsgFunc, 1);
		}
	}
#endif // _WIN32

	// now we are set up to check every frame to see if we can friends/server browser
	m_bTryingToLoadFriends = true;
	m_iFriendsLoadPauseFrames = 1;
}

#if defined(__linux__)
// based off game/shared/of/util/os_utils.cpp (momentum mod)
bool linux_platformdir_helper( char *buf, int size)
{
	FILE *f = fopen("/proc/self/maps", "r");
	if (!f) return false;

	while (!feof(f))
	{
		if (!fgets(buf, size, f)) break;

		char *tmp = strrchr(buf, '\n');
		if (tmp) *tmp = '\0';

		char *mapname = strchr(buf, '/');
		if (!mapname) continue;

		if (strcmp(basename(mapname), "hl2_linux") == 0)
		{
			fclose(f);
			memmove(buf, mapname, strlen(mapname)+1);
			return true;
		}
	}

	fclose(f);
	buf[0] = '\0';
	return false;
}
#endif

#if defined(OSX)
// Based off os_utils.cpp and the function above.
bool osx_platformdir_helper(char *buf, size_t length)
{
	task_t task;
	task_dyld_info dlyd_info;
	mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
	
	if (task_info(mach_task_self_, TASK_DYLD_INFO, (task_info_t)&dlyd_info, &count) == KERN_SUCCESS) //request info regarding dylibs in this task (hl2_osx)
	{
		dyld_all_image_infos *infos = (dyld_all_image_infos*)dlyd_info.all_image_info_addr;
		const uint32_t image_count = infos->infoArrayCount;
		const dyld_image_info *image_array = infos->infoArray;
		
		for(int i = image_count-1; i >= 0; i--) //iterate through all dylibs backwards (since engine.dylib is likely to have been loaded near the end)
		{
			if (strstr(image_array[i].imageFilePath, "hl2_osx")) //found it!
			{
				memmove(buf, image_array[i].imageFilePath, strlen(image_array[i].imageFilePath) + 1);
				return true; //success!
			}
		}
		return false; //failed to find the given process
	}
	buf[0] = '\0';
	return false; //failed to find task info
}
#endif // OSX

//-----------------------------------------------------------------------------
// Purpose: Finds which directory the platform resides in
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGameUI::FindPlatformDirectory(char *platformDir, int bufferSize)
{
	platformDir[0] = '\0';

	if ( platformDir[0] == '\0' )
	{
		// we're not under steam, so setup using path relative to game
		if ( IsPC() )
		{
#if defined(_WIN32)
			if (::GetModuleFileName((HINSTANCE)GetModuleHandle(NULL), platformDir, bufferSize))
			{
				char *lastslash = strrchr(platformDir, '\\'); // this should be just before the filename
#elif defined(__linux__)
			if ( linux_platformdir_helper(platformDir, bufferSize) )
			{
				char *lastslash = strrchr(platformDir, '/'); // this should be just before the filename
#elif defined(OSX)
			if ( osx_platformdir_helper(platformDir, bufferSize) )
			{
				char *lastslash = strrchr(platformDir, '/'); // this should be just before the filename
#else
#error "GameUI: Unknown platform!"
#endif
				if ( lastslash )
				{
					*lastslash = 0;
					Q_strncat(platformDir, "/platform/", bufferSize, COPY_ALL_CHARACTERS );
					return true;
				}
			}
		}
		else
		{
			// xbox fetches the platform path from exisiting platform search path
			// path to executeable is not correct for xbox remote configuration
			if ( g_pFullFileSystem->GetSearchPath( "PLATFORM", false, platformDir, bufferSize ) )
			{
				char *pSeperator = strchr( platformDir, ';' );
				if ( pSeperator )
					*pSeperator = '\0';
				return true;
			}
		}

		Warning( "Unable to determine platform directory\n" );
		return false;
	}

	return (platformDir[0] != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Called to Shutdown the game UI system
//-----------------------------------------------------------------------------
void CGameUI::Shutdown()
{
	// notify all the modules of Shutdown
	g_VModuleLoader.ShutdownPlatformModules();

	// unload the modules them from memory
	g_VModuleLoader.UnloadPlatformModules();

	ModInfo().FreeModInfo();
	
#ifdef _WIN32
	// release platform mutex
	// close the mutex
	if (g_hMutex)
	{
		Sys_ReleaseMutex(g_hMutex);
	}
	if (g_hWaitMutex)
	{
		Sys_ReleaseMutex(g_hWaitMutex);
	}
#endif

	steamapicontext->Clear();
#ifndef _X360
	// SteamAPI_Shutdown(); << Steam shutdown is controlled by engine
#endif
	
	ConVar_Unregister();
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
}

//-----------------------------------------------------------------------------
// Purpose: just wraps an engine call to activate the gameUI
//-----------------------------------------------------------------------------
void CGameUI::ActivateGameUI()
{
	engine->ExecuteClientCmd("gameui_activate");
}

//-----------------------------------------------------------------------------
// Purpose: just wraps an engine call to hide the gameUI
//-----------------------------------------------------------------------------
void CGameUI::HideGameUI()
{
	engine->ExecuteClientCmd("gameui_hide");
}

//-----------------------------------------------------------------------------
// Purpose: Toggle allowing the engine to hide the game UI with the escape key
//-----------------------------------------------------------------------------
void CGameUI::PreventEngineHideGameUI()
{
	engine->ExecuteClientCmd("gameui_preventescape");
}

//-----------------------------------------------------------------------------
// Purpose: Toggle allowing the engine to hide the game UI with the escape key
//-----------------------------------------------------------------------------
void CGameUI::AllowEngineHideGameUI()
{
	engine->ExecuteClientCmd("gameui_allowescape");
}

//-----------------------------------------------------------------------------
// Purpose: Activate the game UI
//-----------------------------------------------------------------------------
void CGameUI::OnGameUIActivated()
{
	//bool bWasActive = m_bActivatedUI;
	m_bActivatedUI = true;

	// pause the server in case it is pausable
	if ( of_pausegame.GetBool() )
		engine->ClientCmd_Unrestricted( "setpause nomsg" );

	SetSavedThisMenuSession( false );

	if (IBasePanel* panel = GetBasePanel())
	{
		bool bNeedActivation = true;
		if (panel->GetVguiPanel().IsVisible())
		{
			// Already visible, maybe don't need activation
			if (!IsInLevel() && IsInBackgroundLevel())
				bNeedActivation = false;
		}
		if (bNeedActivation)
		{
			panel->OnGameUIActivated();
		}
	}

	vgui::Panel *pGameUIPanel = vgui::ipanel()->GetPanel( GetGameUIBasePanel(), "GameUI" );
	if ( pGameUIPanel )
	{
		pGameUIPanel->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hides the game ui, in whatever state it's in
//-----------------------------------------------------------------------------
void CGameUI::OnGameUIHidden()
{
	//bool bWasActive = m_bActivatedUI;
	m_bActivatedUI = false;

	// unpause the game when leaving the UI
	engine->ClientCmd_Unrestricted( "unpause nomsg" );

	if (IBasePanel* panel = GetBasePanel())
	{
		panel->OnGameUIHidden();
	}

	// Close old panels
	if ( m_hCreateMultiplayerGameDialog )
	{
		m_hCreateMultiplayerGameDialog->Close();
	}
	if ( m_hPlayerListDialog )
	{
		m_hPlayerListDialog->Close();
	}
	if ( m_hOptionsDialog )
	{
		m_hOptionsDialog->Close();
	}

	g_VModuleLoader.DeactivateModule( "Servers" );
}

//-----------------------------------------------------------------------------
// Purpose: paints all the vgui elements
//-----------------------------------------------------------------------------
void CGameUI::RunFrame()
{
	if ( IsX360() && m_bOpenProgressOnStart )
	{
		m_bOpenProgressOnStart = false;
	}

	int wide, tall;
#if defined( TOOLFRAMEWORK_VGUI_REFACTOR )
	// resize the background panel to the screen size
	vgui::VPANEL clientDllPanel = enginevguifuncs->GetPanel( PANEL_ROOT );

	int x, y;
	vgui::ipanel()->GetPos( clientDllPanel, x, y );
	vgui::ipanel()->GetSize( clientDllPanel, wide, tall );
	staticPanel->SetBounds( x, y, wide,tall );
#else
	vgui::surface()->GetScreenSize(wide, tall);

	if (IBasePanel* panel = GetBasePanel())
	{
		panel->GetVguiPanel().SetSize(wide, tall);
	}
#endif

	// Run frames
	g_VModuleLoader.RunFrame();

	if (IBasePanel* panel = GetBasePanel())
	{
		panel->RunFrame();
	}
	
	vgui::GetAnimationController()->UpdateAnimations(Plat_FloatTime());

	// On POSIX and Mac OSX just load it without any care for race conditions
	// hackhack: posix steam gameui usage is rude and racey
	// -nopey

	if ( IsPC() && m_bTryingToLoadFriends && m_iFriendsLoadPauseFrames-- < 1
#ifdef _WIN32
		&& g_hMutex && g_hWaitMutex
#endif
	){
		// try and load Steam platform files
#ifdef _WIN32
		unsigned int waitResult = Sys_WaitForSingleObject(g_hMutex, 0);
		if (waitResult == SYS_WAIT_OBJECT_0 || waitResult == SYS_WAIT_ABANDONED)
#endif
		{
			// we got the mutex, so load Friends/Serverbrowser
			// clear the loading flag
			m_bTryingToLoadFriends = false;
			g_VModuleLoader.LoadPlatformModules(&m_GameFactory, 1, false);

#ifdef _WIN32
			// release the wait mutex
			Sys_ReleaseMutex(g_hWaitMutex);
#endif

			// notify the game of our game name
			const char *fullGamePath = engine->GetGameDirectory();
			const char *pathSep = strrchr( fullGamePath, '/' );
			if ( !pathSep )
			{
				pathSep = strrchr( fullGamePath, '\\' );
			}
			if ( pathSep )
			{
				KeyValues *pKV = new KeyValues("ActiveGameName" );
				pKV->SetString( "name", pathSep + 1 );
				pKV->SetInt( "appid", engine->GetAppID() );
				KeyValues *modinfo = new KeyValues("ModInfo");
				if ( modinfo->LoadFromFile( g_pFullFileSystem, "gameinfo.txt" ) )
				{
					pKV->SetString( "game", modinfo->GetString( "game", "" ) );
				}
				modinfo->deleteThis();
				
				g_VModuleLoader.PostMessageToAllModules( pKV );
			}

			// notify the ui of a game connect if we're already in a game
			if (m_iGameIP)
			{
				SendConnectedToGameMessage();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game connects to a server
//-----------------------------------------------------------------------------
void CGameUI::OLD_OnConnectToServer(const char *game, int IP, int port)
{
	// Nobody should use this anymore because the query port and the connection port can be different.
	// Use OnConnectToServer2 instead.
	Assert( false );
	OnConnectToServer2( game, IP, port, port );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game connects to a server
//-----------------------------------------------------------------------------
void CGameUI::OnConnectToServer2(const char *game, int IP, int connectionPort, int queryPort)
{
	m_iGameIP = IP;
	m_iGameConnectionPort = connectionPort;
	m_iGameQueryPort = queryPort;

	SendConnectedToGameMessage();
}

void CGameUI::SendConnectedToGameMessage()
{
	MEM_ALLOC_CREDIT();
	KeyValues *kv = new KeyValues( "ConnectedToGame" );
	kv->SetInt( "ip", m_iGameIP );
	kv->SetInt( "connectionport", m_iGameConnectionPort );
	kv->SetInt( "queryport", m_iGameQueryPort );

	g_VModuleLoader.PostMessageToAllModules( kv );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the game disconnects from a server
//-----------------------------------------------------------------------------
void CGameUI::OnDisconnectFromServer( uint8 eSteamLoginFailure )
{
	m_iGameIP = 0;
	m_iGameConnectionPort = 0;
	m_iGameQueryPort = 0;
	
	g_VModuleLoader.PostMessageToAllModules(new KeyValues("DisconnectedFromGame"));

#if 0
	if ( eSteamLoginFailure == STEAMLOGINFAILURE_NOSTEAMLOGIN )
	{
		if ( g_hLoadingDialog )
		{
			g_hLoadingDialog->DisplayNoSteamConnectionError();
		}
	}
	else if ( eSteamLoginFailure == STEAMLOGINFAILURE_VACBANNED )
	{
		if ( g_hLoadingDialog )
		{
			g_hLoadingDialog->DisplayVACBannedError();
		}
	}
	else if ( eSteamLoginFailure == STEAMLOGINFAILURE_LOGGED_IN_ELSEWHERE )
	{
		if ( g_hLoadingDialog )
		{
			g_hLoadingDialog->DisplayLoggedInElsewhereError();
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: activates the loading dialog on level load start
//-----------------------------------------------------------------------------
void CGameUI::OnLevelLoadingStarted( bool bShowProgressDialog )
{
	g_VModuleLoader.PostMessageToAllModules( new KeyValues( "LoadingStarted" ) );

	if (IBasePanel* panel = GetBasePanel())
	{
		panel->OnLevelLoadingStarted(NULL, bShowProgressDialog);
	}
}

//-----------------------------------------------------------------------------
// Purpose: closes any level load dialog
//-----------------------------------------------------------------------------
void CGameUI::OnLevelLoadingFinished(bool bError, const char *failureReason, const char *extendedReason)
{
	if (bError)
	{
		// kill the FUCKING modules so they stop eating precious input
		GameConsole().Hide();
		g_VModuleLoader.DeactivateModule("Servers");
	}

	// notify all the modules
	g_VModuleLoader.PostMessageToAllModules(new KeyValues("LoadingFinished"));

	bool bDatatable = false;
	if (failureReason)
	{
		if (Q_strstr(failureReason, "class tables"))
		{
			bDatatable = true;
		}
	}

	if (IBasePanel* panel = GetBasePanel())
	{
		KeyValues *pEvent = new KeyValues("LoadingFinished");
		if (bDatatable)
			pEvent->SetString("reason", "#GameUI_ErrorOutdatedBinaries");
		else
			pEvent->SetString("reason", failureReason);
		pEvent->SetInt("error", bError);
		panel->OnLevelLoadingFinished(pEvent);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates progress bar
// Output : Returns true if screen should be redrawn
//-----------------------------------------------------------------------------
bool CGameUI::UpdateProgressBar(float progress, const char *statusText)
{
	if (IBasePanel* panel = GetBasePanel())
	{
		return panel->UpdateProgressBar(progress, statusText);
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns prev settings
//-----------------------------------------------------------------------------
bool CGameUI::SetShowProgressText( bool show )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're currently playing the game
//-----------------------------------------------------------------------------
bool CGameUI::IsInLevel()
{
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0] && !engine->IsLevelMainMenuBackground())
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're at the main menu and a background level is loaded
//-----------------------------------------------------------------------------
bool CGameUI::IsInBackgroundLevel()
{
	const char *levelName = engine->GetLevelName();
	if (levelName && levelName[0] && engine->IsLevelMainMenuBackground())
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're in a multiplayer game
//-----------------------------------------------------------------------------
bool CGameUI::IsInMultiplayer()
{
	return (IsInLevel() && engine->GetMaxClients() > 1);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we're console ui
//-----------------------------------------------------------------------------
bool CGameUI::IsConsoleUI()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if we've saved without closing the menu
//-----------------------------------------------------------------------------
bool CGameUI::HasSavedThisMenuSession()
{
	return m_bHasSavedThisMenuSession;
}

void CGameUI::SetSavedThisMenuSession( bool bState )
{
	m_bHasSavedThisMenuSession = bState;
}

//-----------------------------------------------------------------------------

void CGameUI::NeedConnectionProblemWaitScreen()
{
#if 0
	BaseModUI::CUIGameData::Get()->NeedConnectionProblemWaitScreen();
#endif
}

void CGameUI::ShowPasswordUI( char const *pchCurrentPW )
{
#if 0
	BaseModUI::CUIGameData::Get()->ShowPasswordUI( pchCurrentPW );
#endif
}

//-----------------------------------------------------------------------------
void CGameUI::SetProgressOnStart()
{
	m_bOpenProgressOnStart = true;
}

#if defined( _X360 ) && defined( _DEMO )
void CGameUI::OnDemoTimeout()
{
	GetBasePanel().OnDemoTimeout();
}
#endif

#ifndef GAMEUI_EMBEDDED
//-----------------------------------------------------------------------------
// Purpose: Performs a var args printf into a static return buffer
// Input  : *format - 
//			... - 
// Output : char
//-----------------------------------------------------------------------------
char *VarArgs(const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	return string;
}

void GetHudSize(int& w, int &h)
{
	vgui::surface()->GetScreenSize(w, h);
}

//-----------------------------------------------------------------------------
// Purpose: ScreenHeight returns the height of the screen, in pixels
// Output : int
//-----------------------------------------------------------------------------
int ScreenHeight(void)
{
	int w, h;
	GetHudSize(w, h);
	return h;
}

//-----------------------------------------------------------------------------
// Purpose: ScreenWidth returns the width of the screen, in pixels
// Output : int
//-----------------------------------------------------------------------------
int ScreenWidth(void)
{
	int w, h;
	GetHudSize(w, h);
	return w;
}

void UTIL_StringToIntArray( int *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atoi( pfront );

		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

void UTIL_StringToColor32( color32 *color, const char *pString )
{
	int tmp[4];
	UTIL_StringToIntArray( tmp, 4, pString );
	color->r = tmp[0];
	color->g = tmp[1];
	color->b = tmp[2];
	color->a = tmp[3];
}
#endif

#define STUB_GAMEUI_FUNC(name, ret, val, ...) \
	ret CGameUI::name(__VA_ARGS__) \
	{ \
		DebuggerBreak(); \
		return val; \
	}

STUB_GAMEUI_FUNC(ShowNewGameDialog, void, , int chapter);
STUB_GAMEUI_FUNC(SessionNotification, void, , const int notification, const int param);
STUB_GAMEUI_FUNC(SystemNotification, void, , const int notification);
STUB_GAMEUI_FUNC(ShowMessageDialog, void, , const uint nType, vgui::Panel *pOwner);
STUB_GAMEUI_FUNC(UpdatePlayerInfo, void, , uint64 nPlayerId, const char *pName, int nTeam, byte cVoiceState, int nPlayersNeeded, bool bHost);
STUB_GAMEUI_FUNC(SessionSearchResult, void, , int searchIdx, void *pHostData, XSESSION_SEARCHRESULT *pResult, int ping);
STUB_GAMEUI_FUNC(OnCreditsFinished, void, , );
STUB_GAMEUI_FUNC(BonusMapUnlock, void, , const char *pchFileName, const char *pchMapName);
STUB_GAMEUI_FUNC(BonusMapComplete, void, , const char *pchFileName, const char *pchMapName);
STUB_GAMEUI_FUNC(BonusMapChallengeUpdate, void, , const char *pchFileName, const char *pchMapName, const char *pchChallengeName, int iBest);
STUB_GAMEUI_FUNC(BonusMapChallengeNames, void, , char *pchFileName, char *pchMapName, char *pchChallengeName);
STUB_GAMEUI_FUNC(BonusMapChallengeObjectives, void, , int &iBronze, int &iSilver, int &iGold);
STUB_GAMEUI_FUNC(BonusMapDatabaseSave, void, , );
STUB_GAMEUI_FUNC(BonusMapNumAdvancedCompleted, int, 0, );
STUB_GAMEUI_FUNC(BonusMapNumMedals, void, , int piNumMedals[3]);
STUB_GAMEUI_FUNC(ValidateStorageDevice, bool, false, int *pStorageDeviceValidated);
STUB_GAMEUI_FUNC(OnConfirmQuit, void ,);
STUB_GAMEUI_FUNC(IsMainMenuVisible, bool, false, );

void CGameUI::SetMainMenuOverride(vgui::VPANEL panel)
{
	vgui::Panel *basePanel = vgui::ipanel()->GetPanel(panel, "ClientDLL");
	// MRMODEZ: yes, we assume it's IBasePanel
	gBasePanel = dynamic_cast<IBasePanel*>(basePanel);
	if (!gBasePanel)
	{
		Assert(0);
	}
	gBasePanel->GetVguiPanel().SetParent(GetGameUIBasePanel());
}

void CGameUI::SendMainMenuCommand(const char *pszCommand)
{
	if (!Q_strcmp(pszCommand, "OpenOptionsDialog"))
	{
		GameUI().OpenOptionsDialog();
	}
	else if (!Q_strcmp(pszCommand, "OpenCreateMultiplayerGameDialog"))
	{
		GameUI().OpenCreateMultiplayerGameDialog();
	}
	else if (!Q_strcmp(pszCommand, "OpenPlayerListDialog"))
	{
		GameUI().OpenPlayerListDialog();
	}
}

//-----------------------------------------------------------------------------
// Purpose: moves the game menu button to the right place on the taskbar
//-----------------------------------------------------------------------------
static void BaseUI_PositionDialog(vgui::PHandle dlg)
{
	if (!dlg.Get())
		return;

	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds(x, y, ww, wt);
	dlg->GetSize(wide, tall);

	// Center it, keeping requested size
	dlg->SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

//=============================================================================
void CGameUI::OpenOptionsDialog()
{
	if ( !m_hOptionsDialog.Get() )
	{
		m_hOptionsDialog = new COptionsDialog( NULL );
		m_hOptionsDialog->SetParent( GetGameUIBasePanel() );
		BaseUI_PositionDialog( m_hOptionsDialog );
	}
	m_hOptionsDialog->Activate();
}

//=============================================================================
void CGameUI::OpenCreateMultiplayerGameDialog()
{
	if ( !m_hCreateMultiplayerGameDialog.Get() )
	{
		m_hCreateMultiplayerGameDialog = new CCreateMultiplayerGameDialog( NULL );
		m_hCreateMultiplayerGameDialog->SetParent( GetGameUIBasePanel() );
		BaseUI_PositionDialog( m_hCreateMultiplayerGameDialog );
	}
	m_hCreateMultiplayerGameDialog->Activate();
}

//=============================================================================
void CGameUI::OpenPlayerListDialog()
{
	if ( !m_hPlayerListDialog.Get() )
	{
		m_hPlayerListDialog = new CPlayerListDialog( NULL );
		m_hPlayerListDialog->SetParent( GetGameUIBasePanel() );
		BaseUI_PositionDialog( m_hPlayerListDialog );
	}
	m_hPlayerListDialog->Activate();
}

void CGameUI::OnOpenServerBrowser()
{
	g_VModuleLoader.ActivateModule( "Servers" );
}

static char *g_rgValidCommands[] =
{
	"OpenPlayerListDialog",
	"OpenOptionsDialog",
	"OpenCreateMultiplayerGameDialog",
	"Quit",
	"QuitNoConfirm",
	"ResumeGame",
	"Disconnect",
};

static void CC_GameMenuCommand(const CCommand &args)
{
	int c = args.ArgC();
	if (c < 2)
	{
		Msg("Usage:  gamemenucommand <commandname>\n");
		return;
	}

	g_pGameUI->SendMainMenuCommand(args[1]);
}

static int CC_GameMenuCompletionFunc(char const *partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	char const *cmdname = "gamemenucommand";

	char *substring = (char *)partial;
	if (Q_strstr(partial, cmdname))
	{
		substring = (char *)partial + strlen(cmdname) + 1;
	}

	int checklen = Q_strlen(substring);

	CUtlRBTree< CUtlString > symbols(0, 0, UtlStringLessFunc);

	int i;
	int c = ARRAYSIZE(g_rgValidCommands);
	for (i = 0; i < c; ++i)
	{
		if (Q_strnicmp(g_rgValidCommands[i], substring, checklen))
			continue;

		CUtlString str;
		str = g_rgValidCommands[i];

		symbols.Insert(str);

		// Too many
		if (symbols.Count() >= COMMAND_COMPLETION_MAXITEMS)
			break;
	}

	// Now fill in the results
	int slot = 0;
	for (i = symbols.FirstInorder(); i != symbols.InvalidIndex(); i = symbols.NextInorder(i))
	{
		char const *name = symbols[i].String();

		char buf[512];
		Q_strncpy(buf, name, sizeof(buf));
		Q_strlower(buf);

		Q_snprintf(commands[slot++], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s",
			cmdname, buf);
	}

	return slot;
}

static ConCommand gamemenucommand("gamemenucommand", CC_GameMenuCommand, "Issue game menu command.", 0, CC_GameMenuCompletionFunc);

CON_COMMAND_F(openserverbrowser, "Opens server browser", 0)
{
	bool isSteam = IsPC() && steamapicontext->SteamFriends() && steamapicontext->SteamUtils();
	if (isSteam)
	{
		// show the server browser
		g_VModuleLoader.ActivateModule("Servers");

		// if an argument was passed, that's the tab index to show, send a message to server browser to switch to that tab
		if (args.ArgC() > 1)
		{
			KeyValues *pKV = new KeyValues("ShowServerBrowserPage");
			pKV->SetInt("page", atoi(args[1]));
			g_VModuleLoader.PostMessageToAllModules(pKV);
		}


		KeyValues *pSchemeKV = new KeyValues("SetCustomScheme");
		pSchemeKV->SetString("SchemeName", "SwarmServerBrowserScheme");
		g_VModuleLoader.PostMessageToAllModules(pSchemeKV);

	}
}