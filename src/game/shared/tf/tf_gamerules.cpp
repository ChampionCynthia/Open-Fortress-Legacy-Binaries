//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: The TF Game rules 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "KeyValues.h"
#include "tf_weaponbase.h"
#include "time.h"
#include "tf_shareddefs.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include "tier3/tier3.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "gameeventdefs.h"
#include "tf_halloween_boss.h"
#include "tf_zombie.h"
#include "entity_bossresource.h"
	
#ifdef CLIENT_DLL
	#include <game/client/iviewport.h>
	#include "c_tf_player.h"
	#include "c_tf_objective_resource.h"
#else
	#include "basemultiplayerplayer.h"
	#include "voice_gamemgr.h"
	#include "items.h"
	#include "team.h"
	#include "tf_bot_temp.h"
	#include "tf_player.h"
	#include "tf_team.h"
	#include "player_resource.h"
	#include "entity_tfstart.h"
	#include "filesystem.h"
	#include "tf_obj.h"
	#include "tf_objective_resource.h"
	#include "tf_player_resource.h"
	#include "team_control_point_master.h"
	#include "entity_roundwin.h"
	#include "playerclass_info_parse.h"
	#include "team_train_watcher.h"
	#include "entity_roundwin.h"
	#include "coordsize.h"
	#include "entity_healthkit.h"
	#include "entity_ammopack.h"
	#include "func_respawnroom.h"
	#include "func_regenerate.h"
	#include "tf_gamestats.h"
	#include "entity_capture_flag.h"
	#include "entity_weapon_spawner.h"
	#include "entity_condpowerup.h"
	#include "tf_player_resource.h"
	#include "tf_obj_sentrygun.h"
	#include "tier0/icommandline.h"
	#include "activitylist.h"
	#include "AI_ResponseSystem.h"
	#include "hl2orange.spa.h"
	#include "hltvdirector.h"
	#include "globalstate.h"
    #include "igameevents.h"
	#include "trains.h"
	#include "pathtrack.h"
	#include "entitylist.h"
	#include "trigger_area_capture.h"
	#include "ai_basenpc.h"
	#include "ai_dynamiclink.h"
	#include "vote_controller.h"
	#include "tf_weaponbase_grenadeproj.h"
	#include "tf_voteissues.h"
	#include "nav_mesh.h"
	#include "bot/tf_bot_manager.h"
	#include <../shared/gamemovement.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define ITEM_RESPAWN_TIME	10.0f

enum
{
	BIRTHDAY_RECALCULATE,
	BIRTHDAY_OFF,
	BIRTHDAY_ON,
};

static int g_TauntCamAchievements[] = 
{
	0,		// TF_CLASS_UNDEFINED

	0,		// TF_CLASS_SCOUT,	
	0,		// TF_CLASS_SNIPER,
	0,		// TF_CLASS_SOLDIER,
	0,		// TF_CLASS_DEMOMAN,
	0,		// TF_CLASS_MEDIC,
	0,		// TF_CLASS_HEAVYWEAPONS,
	0,		// TF_CLASS_PYRO,
	0,		// TF_CLASS_SPY,
	0,		// TF_CLASS_ENGINEER,

	0,		// TF_CLASS_MERCENARY,
	0,		// TF_CLASS_CIVILIAN,
	0,		// TF_CLASS_JUGGERNAUT,
	0,		// TF_CLASS_COUNT_ALL,
};

extern ConVar mp_capstyle;
extern ConVar sv_turbophysics;
extern ConVar of_bunnyhop;
extern ConVar of_crouchjump;
extern ConVar of_resistance;
extern ConVar mp_disable_respawn_times;
extern ConVar fraglimit;
extern ConVar of_bunnyhop_max_speed_factor;
extern ConVar tf_maxspeed;
extern ConVar sv_airaccelerate;
extern ConVar of_knockback_all;
extern ConVar of_knockback_bullets;
extern ConVar of_knockback_melee;
extern ConVar of_knockback_explosives;
extern ConVar teamplay;
extern ConVar of_gravitygun;

ConVar tf_caplinear						( "tf_caplinear", "1", FCVAR_REPLICATED, "If set to 1, teams must capture control points linearly." );
ConVar tf_stalematechangeclasstime		( "tf_stalematechangeclasstime", "20", FCVAR_REPLICATED, "Amount of time that players are allowed to change class in stalemates." );
ConVar tf_birthday						( "tf_birthday", "0", FCVAR_NOTIFY | FCVAR_REPLICATED );

// Open Fortress Convars
ConVar of_arena						( "of_arena", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Arena mode." );
ConVar of_infection					( "of_infection", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Infection mode." );
ConVar of_threewave					( "of_threewave", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Threewave." );
ConVar of_juggernaught				( "of_juggernaught", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Juggernaught mode." );
ConVar of_coop						( "of_coop", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles Coop mode. (Pacifism)" );

ConVar of_allow_allclass_pickups 	( "of_allow_allclass_pickups", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Non-Mercenary Classes can pickup dropped weapons.");
ConVar of_allow_allclass_spawners 	( "of_allow_allclass_spawners", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Non-Mercenary Classes can pickup weapons from spawners.");
ConVar of_allow_special_classes		( "of_allow_special_classes", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allow Special classes outside of their respective modes.");
ConVar of_payload_override			( "of_payload_override", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Turn on Escort instead of Payload.");

ConVar of_disable_healthkits		("of_disable_healthkits", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Disable Healthkits." );
ConVar of_disable_ammopacks			("of_disable_ammopacks", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Disable Ammopacks." );
ConVar of_mutator			( "of_mutator", "0", FCVAR_NOTIFY | FCVAR_REPLICATED,
							"Defines the gamemode mutators to be used.\n List of mutators:\n 0 : Disabled\n 1 : Instagib(Railgun + Crowbar)\n 2 : Instagib(Railgun)\n 3 : Clan Arena\n 4 : Unholy Trinity\n 5 : Rocket Arena\n 6 : Gun Game\n 7 : Arsenal",
							true, 0, true, 7 );

/*	List of mutators:
	0: Disabled
	1: Instagib (Railgun + Crowbar)
	2: Instagib (Railgun)
	3: Clan Arena
	4: Unholy Trinity
	5: Rocket Arena
	6: Gun Game
*/

ConVar of_usehl2hull		( "of_usehl2hull", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Use HL2 collision hull." );
ConVar of_multiweapons		( "of_multiweapons", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the Quake-like multi weapon system." );
ConVar of_weaponspawners	( "of_weaponspawners", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles weapon spawners." );
ConVar of_powerups			( "of_powerups", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles powerups." );
ConVar of_forceclass		( "of_forceclass", "1", FCVAR_REPLICATED | FCVAR_NOTIFY , "Force players to be Mercenary in DM. Requires a map change to take effect." );
ConVar of_forcezombieclass	( "of_forcezombieclass", "0", FCVAR_REPLICATED | FCVAR_NOTIFY , "Force zombies to be Mercenaries in Infection. Requires a map change to take effect." );

ConVar of_disable_drop_weapons("of_disable_drop_weapons", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Disable dropping weapons on death."  );

#ifdef GAME_DLL
// TF overrides the default value of the convars below.
ConVar mp_waitingforplayers_time( "mp_waitingforplayers_time", "30", FCVAR_GAMEDLL, "Length in seconds to wait for players." );
ConVar tf_gravetalk( "tf_gravetalk", "1", FCVAR_NOTIFY, "Allows living players to hear dead players using text/voice chat." );
ConVar tf_spectalk( "tf_spectalk", "1", FCVAR_NOTIFY, "Allows living players to hear spectators using text chat." );

ConVar mp_humans_must_join_team( "mp_humans_must_join_team", "any", FCVAR_GAMEDLL | FCVAR_REPLICATED, "Restricts human players to a single team {any, blue, red, spectator}" );

// Infection stuff
ConVar of_infection_preparetime		 ( "of_infection_preparetime", "20", FCVAR_GAMEDLL, "How many seconds survivors have to prepare before the Infection." );
ConVar of_infection_roundtime		 ( "of_infection_roundtime", "300", FCVAR_GAMEDLL, "How many seconds survivors need to... survive for after the Infection." );
ConVar of_infection_zombie_threshold ( "of_infection_zombie_threshold", "6", FCVAR_GAMEDLL, "For every n humans, this many zombies are selected when the Infection starts." );

//Juggernaught
ConVar of_juggernaught_preparetime		 ( "of_juggernaught_preparetime", "15", FCVAR_GAMEDLL, "How many seconds before a player is randomly changed into the Juggernaught." );
ConVar of_juggernaught_wintime			 ( "of_juggernaught_wintime", "90", FCVAR_GAMEDLL, "How many seconds a player needs to be the Juggernaught until they win." );

ConVar of_dominations( "of_dominations", "1", FCVAR_GAMEDLL, "Enable or disable dominations for players." );

extern ConVar tf_halloween_bot_min_player_count;

ConVar tf_halloween_boss_spawn_interval( "tf_halloween_boss_spawn_interval", "480", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_boss_spawn_interval_variation( "tf_halloween_boss_spawn_interval_variation", "60", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_eyeball_boss_spawn_interval( "tf_halloween_eyeball_boss_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds" );
ConVar tf_halloween_eyeball_boss_spawn_interval_variation( "tf_halloween_eyeball_boss_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-" );

ConVar tf_halloween_zombie_mob_enabled( "tf_halloween_zombie_mob_enabled", "0", FCVAR_CHEAT, "If set to 1, spawn zombie mobs on non-Halloween Valve maps" );
ConVar tf_halloween_zombie_mob_spawn_interval( "tf_halloween_zombie_mob_spawn_interval", "180", FCVAR_CHEAT, "Average interval between zombie mob spawns, in seconds" );
ConVar tf_halloween_zombie_mob_spawn_count( "tf_halloween_zombie_mob_spawn_count", "20", FCVAR_CHEAT, "How many zombies to spawn" );

static bool isBossForceSpawning = false;
CON_COMMAND_F( tf_halloween_force_boss_spawn, "For testing.", FCVAR_CHEAT )
{
	isBossForceSpawning = true;
}

static bool isZombieMobForceSpawning = false;
CON_COMMAND_F( tf_halloween_force_mob_spawn, "For testing.", FCVAR_CHEAT )
{
	isZombieMobForceSpawning = true;
}	
#endif

ConVar of_retromode ( "of_retromode", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, \
					"Sets the Retro mode type, which turns on TFC classes and mechanics such as armor. Requires a map change to take effect. \n-1 = Default to map settings\n 0 = Force off\n 1 = Force on\n 2 = Force on for Blu only\n 3 = Force on for Red only" );

ConVar of_grenades	( "of_grenades", "-1", FCVAR_REPLICATED | FCVAR_NOTIFY, \
					"Enables grenades.\n-1 = Depends on Retro mode\n 0 = Forced off\n 1 = Forced on (frags only)\n 2 = Forced on (class-based grenades)" );

ConVar of_navmesh_spawns( "of_navmesh_spawns", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Select random spawns using the navigation mesh on Deathmatch mode" );

ConVar of_randomizer ( "of_randomizer", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, \
					"Turns on Randomizer, use of_randomizer_setting to set what kind of randomizer you use.");

ConVar of_randomizer_setting( "of_randomizer_setting", "TF2", FCVAR_REPLICATED | FCVAR_NOTIFY, \
					"Sets which Config randomizer pulls its weapons from.");

#ifdef GAME_DLL
void ValidateCapturesPerRound( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );

	if ( var.GetInt() <= 0 )
	{
		// reset the flag captures being played in the current round
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
			if ( !pTeam )
				continue;

			pTeam->SetFlagCaptures( 0 );
		}
	}
}
#endif	

ConVar tf_flag_caps_per_round( "tf_flag_caps_per_round", "3", FCVAR_REPLICATED, "Number of flag captures per round on CTF maps. Set to 0 to disable.", true, 0, true, 9
#ifdef GAME_DLL
							  , ValidateCapturesPerRound
#endif
							  );	

ConVar of_mctf_flag_caps_per_round( "of_mctf_flag_caps_per_round", "7", FCVAR_REPLICATED, "Number of flag captures per round on MCTF (not CTF!) maps. Set to 0 to disable.", true, 0, true, 9
#ifdef GAME_DLL
							  , ValidateCapturesPerRound
#endif
							  );	

		
/**
 * Player hull & eye position for standing, ducking, etc.  This version has a taller
 * player height, but goldsrc-compatible collision bounds.
 */
static CViewVectors g_TFViewVectors(
	Vector( 0, 0, 72 ),		//VEC_VIEW (m_vView) eye position
							
	Vector(-24, -24, 0 ),	//VEC_HULL_MIN (m_vHullMin) hull min
	Vector( 24,  24, 82 ),	//VEC_HULL_MAX (m_vHullMax) hull max
												
	Vector(-24, -24, 0 ),	//VEC_DUCK_HULL_MIN (m_vDuckHullMin) duck hull min
	Vector( 24,  24, 55 ),	//VEC_DUCK_HULL_MAX	(m_vDuckHullMax) duck hull max
	Vector( 0, 0, 45 ),		//VEC_DUCK_VIEW		(m_vDuckView) duck view
												
	Vector( -10, -10, -10 ),	//VEC_OBS_HULL_MIN	(m_vObsHullMin) observer hull min
	Vector(  10,  10,  10 ),	//VEC_OBS_HULL_MAX	(m_vObsHullMax) observer hull max
												
	Vector( 0, 0, 14 )		//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight) dead view height
);

static CViewVectors g_HLViewVectors(
	Vector( 0, 0, 64 ),			//VEC_VIEW (m_vView)
								
	Vector(-16, -16, 0 ),		//VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),		//VEC_HULL_MAX (m_vHullMax)
													
	Vector(-16, -16, 0 ),		//VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),		//VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),			//VEC_DUCK_VIEW		(m_vDuckView)
													
	Vector(-10, -10, -10 ),		//VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),		//VEC_OBS_HULL_MAX	(m_vObsHullMax)
													
	Vector( 0, 0, 14 )			//VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)
);					

const CViewVectors *CTFGameRules::GetViewVectors() const
{

	if ( ( m_bUsesHL2Hull && of_usehl2hull.GetInt() < 0) || of_usehl2hull.GetInt() > 0 )
		return &g_HLViewVectors;

	return &g_TFViewVectors;
}

REGISTER_GAMERULES_CLASS( CTFGameRules );

BEGIN_NETWORK_TABLE_NOBASE( CTFGameRules, DT_TFGameRules )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropInt( RECVINFO( m_nMutator ) ),
	RecvPropInt( RECVINFO( m_nRetroMode ) ),
	RecvPropInt( RECVINFO( m_iCosmeticCount ) ),
	RecvPropInt( RECVINFO( m_nCurrFrags ) ),
	RecvPropInt( RECVINFO( m_nHuntedCount_red ) ),
	RecvPropInt( RECVINFO( m_nMaxHunted_red ) ),
	RecvPropInt( RECVINFO( m_nHuntedCount_blu ) ),
	RecvPropInt( RECVINFO( m_nMaxHunted_blu ) ),	
	RecvPropInt( RECVINFO( m_nDomScore_limit ) ),	
	RecvPropInt( RECVINFO( m_nDomScore_blue ) ),	
	RecvPropInt( RECVINFO( m_nDomScore_red ) ),	
	RecvPropString( RECVINFO( m_pszTeamGoalStringRed ) ),
	RecvPropString( RECVINFO( m_pszTeamGoalStringBlue ) ),
	RecvPropString( RECVINFO(m_pszTeamGoalStringMercenary)),
	RecvPropBool( RECVINFO( m_bEscortOverride ) ),
	RecvPropBool( RECVINFO( m_bCapsInitialized ) ),
	RecvPropBool( RECVINFO( m_bIsTeamplay ) ),
	RecvPropBool( RECVINFO( m_bIsTDM ) ),
	RecvPropBool( RECVINFO( m_nbDontCountKills ) ),
	RecvPropBool( RECVINFO( m_bUsesHL2Hull ) ),
	RecvPropBool( RECVINFO( m_bForce3DSkybox ) ),
	RecvPropBool( RECVINFO( m_bUsesMoney ) ),
	RecvPropBool( RECVINFO( m_bKOTH ) ),
	RecvPropBool( RECVINFO( m_bAllClass ) ),
	RecvPropBool( RECVINFO( m_bAllClassZombie ) ),
	RecvPropEHandle( RECVINFO( m_hRedKothTimer ) ), 
	RecvPropEHandle( RECVINFO( m_hBlueKothTimer ) ),
	RecvPropEHandle( RECVINFO( m_itHandle ) ),
	RecvPropEHandle( RECVINFO( m_hInfectionTimer ) ),
	RecvPropInt( RECVINFO( m_halloweenScenario ) ),
	RecvPropInt( RECVINFO( m_iMaxLevel ) ),
#else

	SendPropInt( SENDINFO( m_nGameType ), TF_GAMETYPE_LAST, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nMutator ), 3, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nRetroMode ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iCosmeticCount ) ),
	SendPropInt( SENDINFO( m_nCurrFrags ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nHuntedCount_red ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMaxHunted_red ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nHuntedCount_blu ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nMaxHunted_blu ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDomScore_limit ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDomScore_blue ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDomScore_red ), 7, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszTeamGoalStringRed ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringBlue ) ),
	SendPropString( SENDINFO( m_pszTeamGoalStringMercenary ) ),
	SendPropBool( SENDINFO( m_bCapsInitialized ) ),
	SendPropBool( SENDINFO( m_bEscortOverride ) ),
	SendPropBool( SENDINFO( m_bIsTeamplay ) ),
	SendPropBool( SENDINFO( m_bIsTDM ) ),
	SendPropBool( SENDINFO( m_nbDontCountKills ) ),
	SendPropBool( SENDINFO( m_bUsesHL2Hull ) ),
	SendPropBool( SENDINFO( m_bForce3DSkybox ) ),
	SendPropBool( SENDINFO( m_bUsesMoney ) ),
	SendPropBool( SENDINFO( m_bKOTH ) ),
	SendPropBool( SENDINFO( m_bAllClass ) ),
	SendPropBool( SENDINFO( m_bAllClassZombie ) ),
	SendPropEHandle( SENDINFO( m_hRedKothTimer ) ), 
	SendPropEHandle( SENDINFO( m_hBlueKothTimer ) ),
	SendPropEHandle( SENDINFO( m_hInfectionTimer ) ),
	SendPropEHandle( SENDINFO( m_itHandle ) ),
	SendPropInt( SENDINFO( m_halloweenScenario ) ),
	SendPropInt( SENDINFO( m_iMaxLevel ) )
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_gamerules, CTFGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( TFGameRulesProxy, DT_TFGameRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_TFGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		RecvPropDataTable( "tf_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFGameRules ), RecvProxy_TFGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_TFGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CTFGameRules *pRules = TFGameRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CTFGameRulesProxy, DT_TFGameRulesProxy )
		SendPropDataTable( "tf_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_TFGameRules ), SendProxy_TFGameRules )
	END_SEND_TABLE()
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGameRulesProxy )
	//Keyfields
	DEFINE_KEYFIELD( m_bUsesHL2Hull , FIELD_BOOLEAN, "UsesHL2Hull"),
	DEFINE_KEYFIELD( m_bForce3DSkybox , FIELD_BOOLEAN, "Force3DSkybox"),
	DEFINE_KEYFIELD( m_bUsesMoney , FIELD_BOOLEAN, "UsesMoney"),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRedTeamRespawnWaveTime", InputSetRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBlueTeamRespawnWaveTime", InputSetBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMercenaryTeamRespawnWaveTime", InputSetMercenaryTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddRedTeamRespawnWaveTime", InputAddRedTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddBlueTeamRespawnWaveTime", InputAddBlueTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddMercenaryTeamRespawnWaveTime", InputAddMercenaryTeamRespawnWaveTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRedTeamGoalString", InputSetRedTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMercenaryTeamGoalString", InputSetMercenaryTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetBlueTeamGoalString", InputSetBlueTeamGoalString ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTeamRole", InputSetRedTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTeamRole", InputSetBlueTeamRole ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMercenaryTeamRole", InputSetMercenaryTeamRole ),
// - InputSetRedKothClockActive (Offset 0) (Input)(0 Bytes) - SetRedKothClockActive
// - InputSetBlueKothClockActive (Offset 0) (Input)(0 Bytes) - SetBlueKothClockActive
// - InputPlayVORed (Offset 0) (Input)(0 Bytes) - PlayVORed
// - InputPlayVOBlue (Offset 0) (Input)(0 Bytes) - PlayVOBlue
// - InputPlayVO (Offset 0) (Input)(0 Bytes) - PlayVO
	DEFINE_INPUTFUNC( FIELD_VOID, "SetRedKothClockActive", InputSetRedKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetBlueKothClockActive", InputSetBlueKothClockActive ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVORed", InputPlayVORed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOBlue", InputPlayVOBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVOMercenary", InputPlayVOMercenary ),
	DEFINE_INPUTFUNC( FIELD_STRING, "PlayVO", InputPlayVO ),
	
	DEFINE_OUTPUT( m_OutputIsCTF,	"IsCTF" ),
	DEFINE_OUTPUT( m_OutputIsCP,	"IsCP" ),
	DEFINE_OUTPUT( m_OutputIsDM,	"IsDM" ),
	DEFINE_OUTPUT( m_OutputIsTeamplay,	"IsTeamplay" ),
	DEFINE_OUTPUT( m_OutputIsGunGame,	"IsGunGame" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CTFGameRulesProxy::InputPlayVORed( inputdata_t &inputdata )
{
	if ( TFGameRules() )
		TFGameRules()->BroadcastSound( TF_TEAM_RED, inputdata.value.String() );
}

void CTFGameRulesProxy::InputPlayVOBlue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, inputdata.value.String() );
}

void CTFGameRulesProxy::InputPlayVOMercenary( inputdata_t &inputdata )
{
	if ( TFGameRules() )
		TFGameRules()->BroadcastSound( TF_TEAM_MERCENARY, inputdata.value.String() );
}

void CTFGameRulesProxy::InputPlayVO( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		TFGameRules()->BroadcastSound( TF_TEAM_RED, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TF_TEAM_BLUE, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TF_TEAM_MERCENARY, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TEAM_SPECTATOR, inputdata.value.String() );
		TFGameRules()->BroadcastSound( TEAM_UNASSIGNED, inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CTFGameRulesProxy::InputSetRedKothClockActive(inputdata_t &inputdata)
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
		{
			TFGameRules()->GetBlueKothRoundTimer()->InputDisable( inputdata );
		}
	}
}

void CTFGameRulesProxy::InputSetBlueKothClockActive(inputdata_t &inputdata)
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->InputEnable( inputdata );

		if ( TFGameRules()->GetRedKothRoundTimer() )
		{
			TFGameRules()->GetRedKothRoundTimer()->InputDisable( inputdata );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

void CTFGameRulesProxy::InputSetMercenaryTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_RED, inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_BLUE, inputdata.value.Float() );
}

void CTFGameRulesProxy::InputAddMercenaryTeamRespawnWaveTime( inputdata_t &inputdata )
{
	TFGameRules()->AddTeamRespawnWaveTime( TF_TEAM_MERCENARY, inputdata.value.Float() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_RED, inputdata.value.String() );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_BLUE, inputdata.value.String() );
}

void CTFGameRulesProxy::InputSetMercenaryTeamGoalString( inputdata_t &inputdata )
{
	TFGameRules()->SetTeamGoalString( TF_TEAM_MERCENARY, inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetRedTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_RED );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::InputSetBlueTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_BLUE );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

void CTFGameRulesProxy::InputSetMercenaryTeamRole( inputdata_t &inputdata )
{
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY );
	if ( pTeam )
	{
		pTeam->SetRole( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRulesProxy::Activate()
{
	TFGameRules()->Activate();
	TFGameRules()->m_bUsesHL2Hull = m_bUsesHL2Hull;
	TFGameRules()->m_bForce3DSkybox = m_bForce3DSkybox;
	TFGameRules()->m_bUsesMoney = m_bUsesMoney;

	BaseClass::Activate();
}

LINK_ENTITY_TO_CLASS(of_logic_loadout, CTFLogicLoadout);

BEGIN_DATADESC( CTFLogicLoadout )
	//Keyfields
	DEFINE_KEYFIELD( m_iClass, FIELD_INTEGER, "Class"),
END_DATADESC()

void CTFLogicLoadout::Spawn(void)
{
	BaseClass::Spawn();
}

bool CTFLogicLoadout::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( !Q_strncmp( szKeyName, "WeaponName", 10 ) )
	{
		hWeaponNames.AddToTail(AliasToWeaponID(szValue));
	}
	else
		BaseClass::KeyValue( szKeyName, szValue );
	
	return true;
}

//-----------------------------------------------------------------------------
// DM Logic 
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(of_logic_dm, CTFLogicDM);

BEGIN_DATADESC( CTFLogicDM )
	//Keyfields
	DEFINE_KEYFIELD( m_bIsTeamplay, FIELD_BOOLEAN, "IsTeamplay"),
	DEFINE_KEYFIELD( m_bDontCountKills, FIELD_BOOLEAN, "DontCountKills"),
END_DATADESC()

void CTFLogicDM::Spawn(void)
{
	TFGameRules()->m_bIsTeamplay = m_bIsTeamplay;
	TFGameRules()->m_nbDontCountKills = m_bDontCountKills;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// GG Logic 
//-----------------------------------------------------------------------------


LINK_ENTITY_TO_CLASS(of_logic_gg, CTFLogicGG);

BEGIN_DATADESC( CTFLogicGG )
	//Keyfields
	DEFINE_KEYFIELD( m_bListOnly, FIELD_BOOLEAN, "ListOnly"),
	DEFINE_KEYFIELD( m_iRequiredKills, FIELD_INTEGER, "RequiredKills"),
END_DATADESC()

CTFLogicGG::CTFLogicGG()
{
	pWeaponsData = new KeyValues( "WeaponsData" );
}

bool CTFLogicGG::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( !Q_strncmp( szKeyName, "WeaponName", 10 ) )
		pWeaponsData->SetString( szKeyName + 10, szValue );
	else
		BaseClass::KeyValue( szKeyName, szValue );
	
	return true;
}

//-----------------------------------------------------------------------------
// TDM Logic 
//-----------------------------------------------------------------------------
class CTFLogicTDM : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFLogicTDM, CBaseEntity );
	void	Spawn( void );
};

void CTFLogicTDM::Spawn( void )
{
	BaseClass::Spawn();
}

LINK_ENTITY_TO_CLASS( of_logic_tdm, CTFLogicTDM);

//-----------------------------------------------------------------------------
// JUG Logic 
//-----------------------------------------------------------------------------
class CTFLogicJug : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFLogicJug, CBaseEntity );
	void	Spawn( void );
};

void CTFLogicJug::Spawn( void )
{
	BaseClass::Spawn();
}

LINK_ENTITY_TO_CLASS( of_logic_jug, CTFLogicJug);

//-----------------------------------------------------------------------------
// Civ Escort Logic 
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( of_logic_esc, CTFLogicESC );

BEGIN_DATADESC( CTFLogicESC )
	//Keyfields
	DEFINE_KEYFIELD( m_nMaxHunted_red, FIELD_INTEGER, "MaxRedHunted" ),
	DEFINE_KEYFIELD( m_nMaxHunted_blu, FIELD_INTEGER, "MaxBluHunted" ),

	DEFINE_OUTPUT( m_OnHuntedDeath, "OnHuntedDeath" ),
END_DATADESC()

void CTFLogicESC::Spawn(void)
{
	if ( TFGameRules() )
	{
		TFGameRules()->m_nMaxHunted_red = m_nMaxHunted_red;
		TFGameRules()->m_nMaxHunted_blu = m_nMaxHunted_blu;
	}	

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Domination Logic (this can be combined with escort)
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( of_logic_dom, CTFLogicDOM );

BEGIN_DATADESC( CTFLogicDOM )
	//Keyfields
	DEFINE_KEYFIELD( m_nDomScore_limit, FIELD_INTEGER, "DomScoreLimit" ),
	DEFINE_KEYFIELD( m_nDomScore_time, FIELD_INTEGER, "DomScoreTime" ),
	DEFINE_KEYFIELD( m_bDomWinOnLimit, FIELD_BOOLEAN, "DomWinOnLimit" ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddDomScoreRed", InputAddDomScore_red ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddDomScoreBlue", InputAddDomScore_blue ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDomScoreRed", InputSetDomScore_red ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDomScoreBlue", InputSetDomScore_blue ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDomScoreLimit", InputSetDomScoreLimit ),

	DEFINE_OUTPUT( m_OnDomScoreHit_any, "OnScoreLimitHitAny" ),
	DEFINE_OUTPUT( m_OnDomScoreHit_red, "OnScoreLimitHitRed" ),
	DEFINE_OUTPUT( m_OnDomScoreHit_blue, "OnScoreLimitHitBlue" ),

	DEFINE_THINKFUNC( DOMThink ),
END_DATADESC()

void CTFLogicDOM::Spawn( void )
{
	if ( TFGameRules() )
	{
		TFGameRules()->m_nDomScore_limit = m_nDomScore_limit;
		TFGameRules()->m_nDomScore_time = m_nDomScore_time;
		TFGameRules()->m_bDomWinOnLimit = m_bDomWinOnLimit;
	}

	BaseClass::Spawn();

	SetThink( &CTFLogicDOM::DOMThink );
	SetNextThink( gpGlobals->curtime );
}

void CTFLogicDOM::DOMThink( void )
{
	if ( TFGameRules() && TFGameRules()->IsInWaitingForPlayers() )
	{
		SetNextThink( gpGlobals->curtime + m_nDomScore_time );
		return;
	}

	int nOwnedRed = 0;
	int nOwnedBlue = 0;

	// this sucks!
	CTeamControlPoint *pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( NULL, "team_control_point" );

	// find the score value of each of team-owned control points (usually just 1)
	while ( pTeamControlPoint )
	{
		if ( pTeamControlPoint->GetTeamNumber() == TF_TEAM_RED )
		{
			nOwnedRed = nOwnedRed + pTeamControlPoint->GetDomScoreAmount();
		}
		else if ( pTeamControlPoint->GetTeamNumber() == TF_TEAM_BLUE )
		{
			nOwnedBlue = nOwnedBlue + pTeamControlPoint->GetDomScoreAmount();
		}

		pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname( pTeamControlPoint, "team_control_point" );
	}

	// add the total score we got from all counted and owned control points
	AddDomScore_red( nOwnedRed );
	AddDomScore_blue( nOwnedBlue );

	SetNextThink( gpGlobals->curtime + m_nDomScore_time );
}

void CTFLogicDOM::AddDomScore_red( int amount )
{
	if ( amount == 0 )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( ( dom_score_red_old + amount ) > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_red + amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::AddDomScore_blue( int amount )
{
	if ( amount == 0 )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( ( dom_score_blue_old + amount ) > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_blue = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_blue = TFGameRules()->m_nDomScore_blue + amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::SetDomScore_red( int amount )
{
	if ( amount == TFGameRules()->m_nDomScore_red )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( amount > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_red = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_red = amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::SetDomScore_blue( int amount )
{
	if ( amount == TFGameRules()->m_nDomScore_blue )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// is the resulting score gonna be over the limit? clamp it
	if ( amount > TFGameRules()->m_nDomScore_limit )
		TFGameRules()->m_nDomScore_blue = TFGameRules()->m_nDomScore_limit;
	else
		TFGameRules()->m_nDomScore_blue = amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::SetDomScoreLimit( int amount )
{
	if ( amount == 0 )
		return;

	int dom_score_red_old = TFGameRules()->m_nDomScore_red;
	int dom_score_blue_old = TFGameRules()->m_nDomScore_blue;

	// clamp any scores if the limit becomes lower
	if ( TFGameRules()->m_nDomScore_red > amount )
		TFGameRules()->m_nDomScore_red = amount;
	else if ( TFGameRules()->m_nDomScore_blue > amount )
		TFGameRules()->m_nDomScore_blue = amount;

	TFGameRules()->m_nDomScore_limit = amount;

	TFGameRules()->CheckDOMScores( dom_score_red_old, dom_score_blue_old );
}

void CTFLogicDOM::InputAddDomScore_red( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		AddDomScore_red( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputAddDomScore_blue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		AddDomScore_blue( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputSetDomScore_red( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		SetDomScore_red( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputSetDomScore_blue( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		SetDomScore_blue( inputdata.value.Int() );
	}
}

void CTFLogicDOM::InputSetDomScoreLimit( inputdata_t &inputdata )
{
	if ( TFGameRules() )
	{
		SetDomScoreLimit( inputdata.value.Int() );
	}
}

//-----------------------------------------------------------------------------
// Infection Logic 
//-----------------------------------------------------------------------------

class CTFLogicInf : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFLogicInf, CBaseEntity);
	void	Spawn(void);
};

LINK_ENTITY_TO_CLASS(of_logic_inf, CTFLogicInf);

void CTFLogicInf::Spawn(void)
{
	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Arena Logic 
//-----------------------------------------------------------------------------

// inheriting from pointentity, not baseentity as the event inputs are required
class CTFLogicArena : public CPointEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTFLogicArena, CPointEntity );

	void	Spawn( void );

	CTFLogicArena();

	float		m_iCapEnableDelay;

	bool	m_bCapUnlocked;
	void	OnCapEnabled( void );

	//awful
	virtual void	InputRoundActivate( inputdata_t &inputdata );

	COutputEvent m_ArenaRoundStart;
	COutputEvent m_OnCapEnabled;
};

BEGIN_DATADESC( CTFLogicArena )
	DEFINE_KEYFIELD( m_iCapEnableDelay, FIELD_FLOAT, "CapEnableDelay" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

	DEFINE_OUTPUT( m_ArenaRoundStart, "OnArenaRoundStart" ),
	DEFINE_OUTPUT( m_OnCapEnabled, "OnCapEnabled" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS(tf_logic_arena, CTFLogicArena);

CTFLogicArena::CTFLogicArena()
{
	m_iCapEnableDelay = 60;

	m_bCapUnlocked = false;
}

void CTFLogicArena::Spawn(void)
{
	BaseClass::Spawn();

	if ( m_iCapEnableDelay == 0 )
		m_bCapUnlocked = true;
}

void CTFLogicArena::OnCapEnabled( void )
{
	if ( !m_bCapUnlocked )
	{
		m_bCapUnlocked = true; 
		// activator??
		m_OnCapEnabled.FireOutput( this, this );
	}
}

void CTFLogicArena::InputRoundActivate( inputdata_t &inputdata )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( !pMaster )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		// lock control point
		pPoint->SetLocked( true );
		pPoint->SetUnlockTime( m_iCapEnableDelay );
	}
}

//-----------------------------------------------------------------------------
// Barebone for now, just so maps don't complain
//-----------------------------------------------------------------------------
class CTFHolidayEntity : public CPointEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFHolidayEntity, CPointEntity );
	DECLARE_DATADESC();

	CTFHolidayEntity();
	virtual ~CTFHolidayEntity() { }

	virtual int UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_ALWAYS ); }
	virtual void FireGameEvent( IGameEvent *event );

	void InputHalloweenSetUsingSpells( inputdata_t& inputdata );
	void InputHalloweenTeleportToHell( inputdata_t& inputdata );

private:
	int m_nHolidayType;
	int m_nTauntInHell;
	int m_nAllowHaunting;
};

CTFHolidayEntity::CTFHolidayEntity()
{
	ListenForGameEvent( "player_turned_to_ghost" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_disconnect" );
}

void CTFHolidayEntity::FireGameEvent( IGameEvent *event )
{
}

void CTFHolidayEntity::InputHalloweenSetUsingSpells( inputdata_t& inputdata )
{
}

void CTFHolidayEntity::InputHalloweenTeleportToHell( inputdata_t& inputdata )
{
}

BEGIN_DATADESC( CTFHolidayEntity )

	DEFINE_KEYFIELD( m_nHolidayType, FIELD_INTEGER, "holiday_type" ),
	DEFINE_KEYFIELD( m_nTauntInHell, FIELD_INTEGER, "tauntInHell" ),
	DEFINE_KEYFIELD( m_nAllowHaunting, FIELD_INTEGER, "allowHaunting" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "HalloweenSetUsingSpells", InputHalloweenSetUsingSpells ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Halloween2013TeleportToHell", InputHalloweenTeleportToHell ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_holiday, CTFHolidayEntity )

//-----------------------------------------------------------------------------
// KOTH Logic 
//-----------------------------------------------------------------------------

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CBaseEntity - tf_logic_koth
- m_nTimerInitialLength (Offset 856) (Save|Key)(4 Bytes) - timer_length
- m_nTimeToUnlockPoint (Offset 860) (Save|Key)(4 Bytes) - unlock_point
- InputRoundSpawn (Offset 0) (Input)(0 Bytes) - RoundSpawn
- InputRoundActivate (Offset 0) (Input)(0 Bytes) - RoundActivate
- InputSetRedTimer (Offset 0) (Input)(0 Bytes) - SetRedTimer
- InputSetBlueTimer (Offset 0) (Input)(0 Bytes) - SetBlueTimer
- InputAddRedTimer (Offset 0) (Input)(0 Bytes) - AddRedTimer
- InputAddBlueTimer (Offset 0) (Input)(0 Bytes) - AddBlueTimer
*/

class CTFLogicKOTH : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFLogicKOTH, CBaseEntity );
	DECLARE_DATADESC();

	CTFLogicKOTH();

	// called at setup
	virtual void	InputRoundSpawn( inputdata_t &inputdata );

	virtual void	InputSetBlueTimer( inputdata_t &inputdata );
	virtual void	InputSetRedTimer( inputdata_t &inputdata );
	virtual void	InputAddBlueTimer( inputdata_t &inputdata );
	virtual void	InputAddRedTimer( inputdata_t &inputdata );

private:
	int m_nTimerInitialLength;
	int m_nTimeToUnlockPoint;

};

BEGIN_DATADESC( CTFLogicKOTH )

	DEFINE_KEYFIELD( m_nTimerInitialLength, FIELD_INTEGER, "timer_length" ),
	DEFINE_KEYFIELD( m_nTimeToUnlockPoint, FIELD_INTEGER, "unlock_point" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBlueTimer", InputSetBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetRedTimer", InputSetRedTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddBlueTimer", InputAddBlueTimer ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddRedTimer", InputAddRedTimer ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_koth, CTFLogicKOTH );

CTFLogicKOTH::CTFLogicKOTH()
{
	m_nTimerInitialLength = 180;
	m_nTimeToUnlockPoint = 30;
}

void CTFLogicKOTH::InputRoundSpawn(inputdata_t &inputdata)
{
	if ( TFGameRules() )
	{
		variant_t sVariant;

		sVariant.SetInt( TFGameRules()->GetTimeLeft() );

		TFGameRules()->SetBlueKothRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetBlueKothRoundTimer() )
		{
			TFGameRules()->GetBlueKothRoundTimer()->SetName( MAKE_STRING( "zz_blue_koth_timer" ) );

			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetBlueKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );

			TFGameRules()->GetBlueKothRoundTimer()->SetTimeRemaining( m_nTimerInitialLength ); 

			TFGameRules()->GetBlueKothRoundTimer()->SetShowInHud( false );

			TFGameRules()->GetBlueKothRoundTimer()->ChangeTeam( TF_TEAM_BLUE );
		}

		TFGameRules()->SetRedKothRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

		if ( TFGameRules()->GetRedKothRoundTimer() )
		{
			TFGameRules()->GetRedKothRoundTimer()->SetName( MAKE_STRING( "zz_red_koth_timer" ) );

			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "Pause", NULL, NULL, sVariant, 0 );
			TFGameRules()->GetRedKothRoundTimer()->AcceptInput( "SetTime", NULL, NULL, sVariant, 0 );

			TFGameRules()->GetRedKothRoundTimer()->SetTimeRemaining( m_nTimerInitialLength );

			TFGameRules()->GetRedKothRoundTimer()->SetShowInHud( false );

			TFGameRules()->GetRedKothRoundTimer()->ChangeTeam( TF_TEAM_RED );
		}
	}
}

void CTFLogicKOTH::InputAddBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CTFLogicKOTH::InputAddRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->AddTimerSeconds( inputdata.value.Int() );
	}
}

void CTFLogicKOTH::InputSetBlueTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetBlueKothRoundTimer() )
	{
		TFGameRules()->GetBlueKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

void CTFLogicKOTH::InputSetRedTimer( inputdata_t &inputdata )
{
	if ( TFGameRules() && TFGameRules()->GetRedKothRoundTimer() )
	{
		TFGameRules()->GetRedKothRoundTimer()->SetTimeRemaining( inputdata.value.Int() );
	}
}

#endif

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

// (We clamp ammo ourselves elsewhere).
ConVar ammo_max( "ammo_max", "5000", FCVAR_REPLICATED );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK ) ) != 0 );
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFGameRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Should always bleed currently.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_DROWNRECOVER );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_DROWN | DMG_BURN | DMG_NERVEGAS | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGameRules::Damage_GetShouldNotBleed( void )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGameRules::CTFGameRules()
{
#ifdef GAME_DLL
	// Create teams.
	TFTeamMgr()->Init();

	ResetMapTime();

	// Create the team managers
//	for ( int i = 0; i < ARRAYSIZE( teamnames ); i++ )
//	{
//		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "tf_team" ));
//		pTeam->Init( sTeamNames[i], i );
//
//		g_Teams.AddToTail( pTeam );
//	}

	// Reset Domination stuff
	m_nDomScore_red = 0;
	m_nDomScore_blue = 0;
	m_bDomRedThreshold = false;
	m_bDomBlueThreshold = false;
	m_bDomRedLeadThreshold = false;
	m_bDomBlueLeadThreshold = false;
	m_bStartedVote = false;
	m_bHasCivilianSpawns = false;
	m_bHasJuggernautSpawns = false;
	m_bEntityLimitPrevented = false;

	m_bFirstBlood = false;
	for(int i = 1; i <= 64; i++)
		m_InflictorsArray[i] = NULL;

	m_flIntermissionEndTime = 0.0f;
	m_flNextPeriodicThink = 0.0f;

	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );	
	ListenForGameEvent( "teamplay_point_unlocked" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
	
	Q_memset( m_vecPlayerPositions,0, sizeof(m_vecPlayerPositions) );

	m_iPrevRoundState = -1;
	m_iCurrentRoundState = -1;
	m_iCurrentMiniRoundMask = 0;
	m_flTimerMayExpireAt = -1.0f;

	// Lets execute a map specific cfg file
	// ** execute this after server.cfg!
	char szCommand[32];
	Q_snprintf( szCommand, sizeof( szCommand ), "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	engine->ServerCommand( szCommand );
	
	if ( filesystem->FileExists( "cfg/gungame.cfg" , "MOD" ) )
	{
		m_iRequiredKills = 2;
		KeyValues* pWeaponsData = new KeyValues( "GunGame" );
		pWeaponsData->LoadFromFile( filesystem, "cfg/gungame.cfg" );

		KeyValues *pWeapon = new KeyValues( "Weapon" );
		pWeapon = pWeaponsData->GetFirstValue();
		m_iMaxLevel = 0;
		for( pWeapon; pWeapon != NULL; pWeapon = pWeapon->GetNextValue() ) // Loop through all the keyvalues
		{
			m_iszWeaponName[m_iMaxLevel] = MAKE_STRING( pWeapon->GetString() );
			m_iMaxLevel++;
		}
	}

#else // GAME_DLL

	ListenForGameEvent( "game_newmap" );
	
#endif

	// Initialize the game type
//	m_nGameType.Set( TF_GAMETYPE_UNDEFINED );

	// Initialize the classes here.
	InitPlayerClasses();

	// Set turbo physics on.  Do it here for now.
	sv_turbophysics.SetValue( 1 );

	// Initialize the team manager here, etc...

	// If you hit these asserts its because you added or removed a weapon type 
	// and didn't also add or remove the weapon name or damage type from the
	// arrays defined in tf_shareddefs.cpp
	Assert( g_aWeaponDamageTypes[TF_WEAPON_COUNT] == TF_DMG_SENTINEL_VALUE );
	Assert( FStrEq( g_aWeaponNames[TF_WEAPON_COUNT], "TF_WEAPON_COUNT" ) );	

	m_iPreviousRoundWinners = TEAM_UNASSIGNED;
	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	// tells bots whether they should just move randomly around on the map, like if its free for all
	m_bIsFreeRoamMap = false;

	m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
	m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
	m_pszTeamGoalStringMercenary.GetForModify()[0] = '\0';

#ifdef GAME_DLL
	const char *szMapname = STRING( gpGlobals->mapname );
	if ( !Q_strncmp( szMapname, "cp_manor_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_MANOR;
	else if ( !Q_strncmp( szMapname, "koth_viaduct_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_VIADUCT;
	else if ( !Q_strncmp( szMapname, "koth_lakeside_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_LAKESIDE;
	else if ( !Q_strncmp( szMapname, "plr_hightower_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_HIGHTOWER;
	else if ( !Q_strncmp( szMapname, "sd_doomsday_event", MAX_MAP_NAME ) )
		m_halloweenScenario = HALLOWEEN_SCENARIO_DOOMSDAY;
#endif
}

bool CTFGameRules::InGametype( int nGametype )
{	
	Assert( nGametype >= 0 && nGametype < TF_GAMETYPE_LAST );
	return ( ( m_nGameType & (1<<nGametype) ) != 0 );
}

void CTFGameRules::AddGametype( int nGametype )
{
	Assert( nGametype >= 0 && nGametype < TF_GAMETYPE_LAST );
	m_nGameType |= (1<<nGametype);
}

void CTFGameRules::RemoveGametype( int nGametype )
{
	Assert( nGametype >= 0 && nGametype < TF_GAMETYPE_LAST );
	m_nGameType &= ~(1<<nGametype);
}

bool CTFGameRules::IsMutator( int nMutator )
{
	return ( m_nMutator == nMutator );
}

int CTFGameRules::GetMutator( void )
{
	return m_nMutator;
}

#ifdef GAME_DLL
void CTFGameRules::SetMutator( int nMutator )
{
	m_nMutator = nMutator;
}
#endif

bool CTFGameRules::IsRetroMode( int nRetroMode )
{
	return ( m_nRetroMode == nRetroMode );
}

int CTFGameRules::GetRetroMode( void )
{
	return m_nRetroMode;
}

#ifdef GAME_DLL
void CTFGameRules::SetRetroMode( int nRetroMode)
{
	m_nRetroMode = nRetroMode;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::FlagsMayBeCapped( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN )
		return true;

	return false;
}

bool CTFGameRules::WeaponSpawnersMayBeUsed( void )
{
	if ( State_Get() != GR_STATE_TEAM_WIN )
		return true;

	return false;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Determines whether we should allow mp_timelimit to trigger a map change
//-----------------------------------------------------------------------------
bool CTFGameRules::CanChangelevelBecauseOfTimeLimit( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	// we only want to deny a map change triggered by mp_timelimit if we're not forcing a map reset,
	// we're playing mini-rounds, and the master says we need to play all of them before changing (for maps like Dustbowl)
	if ( !m_bForceMapReset && pMaster && pMaster->PlayingMiniRounds() && pMaster->ShouldPlayAllControlPointRounds() )
	{
		if ( pMaster->NumPlayableControlPointRounds() )
		{
			return false;
		}
	}

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanGoToStalemate( void )
{
	// In CTF, don't go to stalemate if one of the flags isn't at home
	if ( m_nGameType == TF_GAMETYPE_CTF )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( NULL, "item_teamflag" ) );
		while( pFlag )
		{
			if ( pFlag->IsDropped() || pFlag->IsStolen() )
				return false;

			pFlag = dynamic_cast<CCaptureFlag*> ( gEntList.FindEntityByClassname( pFlag, "item_teamflag" ) );
		}

		// check that one team hasn't won by capping
		if ( CheckCapsPerRound() )
			return false;
	}

	return BaseClass::CanGoToStalemate();
}

// Classnames of entities that are preserved across round restarts
// Updated to modern TF2
static const char *s_PreserveEnts[] =
{
	"tf_gamerules",
	"tf_team_manager",
	"tf_player_manager",
	"tf_team",
	"tf_objective_resource",
	"keyframe_rope",
	"move_rope",
	"tf_",
	"tf_logic_training",
	"tf_logic_training_mode",
	"tf_powerup_bottle",
	"tf_mann_vs_machine_stats",
	"tf_viewmodel"
	"tf_wearable",
	"tf_wearable_demoshield",
	"tf_wearable_robot_arm",
	"tf_wearable_vm",
	"tf_logic_bonusround",
	"vote_controller",
	"monster_resource",
	"tf_logic_medieval",
	"tf_logic_cp_timer",
	"tf_logic_tower_defense",
	"func_upgradestation",
	"entity_rocket",
	"entity_carrier",
	"entity_sign",
	"entity_saucer",
	"tf_halloween_gift_pickup",
	"tf_logic_competitive",
	"tf_wearable_razorback",
	"info_ladder",
	"", // END Marker
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::Activate()
{
	SetupMutator();

	SetRetroMode( of_retromode.GetInt() );

	// this is required immediately so bots know what to do
	m_bIsFreeRoamMap = false;

#ifdef GAME_DLL
	if ( ( !Q_strncmp( STRING( gpGlobals->mapname), "dm_", 3 ) 
		|| !Q_strncmp( STRING( gpGlobals->mapname), "duel_", 5 ) 
		|| !Q_strncmp( STRING( gpGlobals->mapname), "inf_", 4 ) ) )
		m_bIsFreeRoamMap = true;	// dm and infection specific maps must always be freeroam due to the layout, used for mutators
	else if ( ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ) || ( TFGameRules()->IsInfGamemode() || TFGameRules()->IsCoopEnabled() ) )
		m_bIsFreeRoamMap = true;	// also account for enabling DM or infection on maps such as ctf_2fort
#else
	if ( ( !Q_strncmp( STRING( engine->GetLevelName() ), "dm_", 3 ) 
		|| !Q_strncmp( STRING( engine->GetLevelName() ), "duel_", 5 ) 
		|| !Q_strncmp( STRING( engine->GetLevelName() ), "inf_", 4 ) ) )
		m_bIsFreeRoamMap = true;	// dm and infection specific maps must always be freeroam due to the layout, used for mutators
	else if ( ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ) || ( TFGameRules()->IsInfGamemode() || TFGameRules()->IsCoopEnabled() ) )
		m_bIsFreeRoamMap = true;	// also account for enabling DM or infection on maps such as ctf_2fort
#endif

	ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server global gamemode config file\n");
	engine->ServerCommand("exec config_default_global.cfg\n");
	engine->ServerExecute();

	m_iBirthdayMode = BIRTHDAY_RECALCULATE;

	m_nCurrFrags.Set(0);
	m_bDisableRedSpawns = false;
	m_bDisableBluSpawns = false;

//	m_nGameType.Set(TF_GAMETYPE_UNDEFINED);
	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*> (gEntList.FindEntityByClassname(NULL, "item_teamflag"));
	if (pFlag)
	{
		AddGametype(TF_GAMETYPE_CTF);
	}
	CTeamTrainWatcher *pTrain = dynamic_cast<CTeamTrainWatcher*> (gEntList.FindEntityByClassname(NULL, "team_train_watcher"));

	if (pTrain)
	{
		AddGametype(TF_GAMETYPE_PAYLOAD);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Payload gamemode config file\n");
		engine->ServerCommand("exec config_default_pl.cfg \n");
		engine->ServerExecute();
	}
	
	if (g_hControlPointMasters.Count())
	{
		AddGametype(TF_GAMETYPE_CP);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server CP gamemode config file\n");
		engine->ServerCommand("exec config_default_cp.cfg \n");
		engine->ServerExecute();
	}

	if (gEntList.FindEntityByClassname(NULL, "tf_logic_arena") || !Q_strncmp(STRING(gpGlobals->mapname), "arena_", 6) || of_arena.GetBool() )
	{
		AddGametype(TF_GAMETYPE_ARENA);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Arena gamemode config file\n");
		engine->ServerCommand("exec config_default_arena.cfg \n");
		engine->ServerExecute();
	}

	if (gEntList.FindEntityByClassname(NULL, "tf_logic_koth") || !Q_strncmp(STRING(gpGlobals->mapname), "koth_", 5) )
	{
		AddGametype(TF_GAMETYPE_CP);
		m_bKOTH = true;
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server KOTH gamemode config file\n");
		engine->ServerCommand("exec config_default_koth.cfg \n");
		engine->ServerExecute();
	}

	if (gEntList.FindEntityByClassname(NULL, "of_logic_dm") || !Q_strncmp(STRING(gpGlobals->mapname), "dm_", 3) || !Q_strncmp(STRING(gpGlobals->mapname), "duel_", 5) )
	{
		AddGametype(TF_GAMETYPE_DM);
		if ( ((( teamplay.GetInt() < 0 || gEntList.FindEntityByClassname(NULL, "of_logic_tdm")) && m_bIsTeamplay ) || teamplay.GetInt() > 0 )  )
		{
			AddGametype(TF_GAMETYPE_TDM);
			ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server TDM gamemode config file\n");
			engine->ServerCommand("exec config_default_tdm.cfg \n");
			engine->ServerExecute();
		}
		else 
		{
			ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server DM gamemode config file\n");
			engine->ServerCommand("exec config_default_dm.cfg \n");
			engine->ServerExecute();
		}
	}
	
	if ( ( gEntList.FindEntityByClassname(NULL, "of_logic_gg") && !m_bListOnly ) || !Q_strncmp(STRING(gpGlobals->mapname), "gg_", 3) || TFGameRules()->IsMutator( GUN_GAME ) )
	{
		AddGametype( TF_GAMETYPE_GG );
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server GG gamemode config file\n");
		engine->ServerCommand("exec config_default_gg.cfg \n");
		engine->ServerExecute();
	}	
	
	if ( ( gEntList.FindEntityByClassname(NULL, "of_logic_3wave") && !m_bListOnly ) || of_threewave.GetInt() == 1 )
	{
		AddGametype(TF_GAMETYPE_3WAVE);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Threewave gamemode config file\n");
		engine->ServerCommand("exec config_default_3wave.cfg \n");
		engine->ServerExecute();
	}
	
	if ( gEntList.FindEntityByClassname(NULL, "of_logic_jug") || !Q_strncmp(STRING(gpGlobals->mapname), "jug_", 4) || of_juggernaught.GetBool() )
	{
		AddGametype(TF_GAMETYPE_JUG);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Juggernaught gamemode config file\n");
		engine->ServerCommand("exec config_default_jug.cfg \n");
		engine->ServerExecute();
	}
	
	if (gEntList.FindEntityByClassname(NULL, "of_logic_esc") || !Q_strncmp(STRING(gpGlobals->mapname), "esc_", 4) || ( of_payload_override.GetBool() && InGametype( TF_GAMETYPE_PAYLOAD ) ) )
	{
		AddGametype(TF_GAMETYPE_ESC);
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Escort gamemode config file\n");
		engine->ServerCommand("exec config_default_esc.cfg \n");
		engine->ServerExecute();
		
		if ( of_payload_override.GetBool() && !( gEntList.FindEntityByClassname(NULL, "of_logic_esc") || !Q_strncmp(STRING(gpGlobals->mapname), "esc_", 4) ) ) // We're replacing payload with Escort
		{
			m_bEscortOverride = true;
		}
	}

	if (gEntList.FindEntityByClassname(NULL, "of_logic_dom") || !Q_strncmp(STRING(gpGlobals->mapname), "dom_", 4) )
	{
		// no Domination gamemode is set in Escort
		if ( !IsESCGamemode() )
			AddGametype( TF_GAMETYPE_DOM );

		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Domination gamemode config file\n");
		engine->ServerCommand("exec config_default_dom.cfg \n");
		engine->ServerExecute();
	}

	// Infection
	if ( gEntList.FindEntityByClassname(NULL, "of_logic_inf") || !Q_strncmp(STRING(gpGlobals->mapname), "inf_", 4) || of_infection.GetBool() )
	{
		// incompatible gametypes with infection!
		for ( int i = 0; i < TF_GAMETYPE_LAST; i++ )
		{
			if ( TFGameRules()->InGametype( i ) )
				TFGameRules()->RemoveGametype( i );
				
		}

		AddGametype( TF_GAMETYPE_INF );
		ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server Infection gamemode config file\n");
		engine->ServerCommand("exec config_default_inf.cfg \n");
		engine->ServerExecute();
	}

	m_bAllClass = false;
	m_bAllClassZombie = false;

	if ( !of_forceclass.GetBool() )
		m_bAllClass = true;

	if ( !of_forcezombieclass.GetBool() )
		m_bAllClassZombie = true;

	CheckTDM();
	bMultiweapons = TFGameRules()->UsesDMBuckets();

	PrecacheGameMode();	

#ifdef GAME_DLL
	m_flMapStartTime = gpGlobals->curtime;
#endif
}

extern const char *s_aPlayerClassFiles[];

void CTFGameRules::PrecacheGameMode()
{
	// To-Do: Expand this to other cases and handle more stuff aside from weapons
	if ( !TFGameRules()->IsDMGamemode() || TFGameRules()->IsAllClassEnabled() || ( TFGameRules()->IsInfGamemode() && TFGameRules()->IsAllClassZombieEnabled() ) )
	{
		// Precache our base classes
		for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
		{
			const unsigned char *pKey = NULL;

			if ( g_pGameRules )
			{
				pKey = g_pGameRules->GetEncryptionKey();
			}

			KeyValues *kvFinal = ReadEncryptedKVFile( filesystem, s_aPlayerClassFiles[i], pKey );
			if( !kvFinal )
				continue;
		
			const char *pszModel = kvFinal->GetString("model");
			if ( pszModel && pszModel[0] )
			{
				int iModel = CBaseEntity::PrecacheModel( pszModel );
				PrecacheGibsForModel( iModel );
			}

			const char *pszModelArm = kvFinal->GetString("arm_model");
			if ( pszModelArm && pszModelArm[0] )
			{
				CBaseEntity::PrecacheModel( pszModelArm );
			}

			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_death") );
			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_crit_death") );
			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_melee_death") );
			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_explosion_death") );
			
			// Precache TFC classes if necessary
			if ( IsRetroModeEnabled() )
			{
				// TFC
				KeyValues* kvModifier = kvFinal->FindKey("TFC");
				if( kvModifier )
				{
					pszModel = kvModifier->GetString("model");
					if ( pszModel && pszModel[0] )
					{
						int iModel = CBaseEntity::PrecacheModel( pszModel );
						PrecacheGibsForModel( iModel );
					}

					pszModelArm = kvModifier->GetString("arm_model");
					if ( pszModelArm && pszModelArm[0] )
					{
						CBaseEntity::PrecacheModel( pszModelArm );
					}
			
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_crit_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_melee_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_explosion_death") );
				}
			}
			// Precache the zombie classes if necessary
			if ( TFGameRules()->IsInfGamemode() )
			{
				KeyValues* kvModifier = kvFinal->FindKey("Zombie");
				if( kvModifier )
				{
					pszModel = kvModifier->GetString("model");
					if ( pszModel && pszModel[0] )
					{
						int iModel = CBaseEntity::PrecacheModel( pszModel );
						PrecacheGibsForModel( iModel );
					}

					pszModelArm = kvModifier->GetString("arm_model");
					if ( pszModelArm && pszModelArm[0] )
					{
						CBaseEntity::PrecacheModel( pszModelArm );
					}	
			
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_crit_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_melee_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_explosion_death") );
				}
			}
		}
		// Precache all base weapons of every class
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
		{
			TFPlayerClassData_t* pData = GetPlayerClassData( iClass );

			for ( int iWeapon = 0; iWeapon < pData->m_iWeaponCount; iWeapon++ )
			{
				int iWeaponID = pData->m_aWeapons[iWeapon];
				if ( iWeaponID != TF_WEAPON_NONE )
				{
					const char* pszWeaponName = WeaponIdToClassname( iWeaponID );
					if ( !pszWeaponName )
						continue;

					UTIL_PrecacheOther( pszWeaponName );
				}
			}
		}
		
		// For spy's disguises
		CBaseEntity::PrecacheModel( "models/player/spy_mask.mdl" );
	}
	else // never add conditionals here, we gotta atleast precache something
	{
		// Precache only the mercenary
		const unsigned char *pKey = NULL;

		if ( g_pGameRules )
		{
			pKey = g_pGameRules->GetEncryptionKey();
		}

		KeyValues *kvFinal = ReadEncryptedKVFile( filesystem, s_aPlayerClassFiles[TF_CLASS_MERCENARY], pKey );
		if ( kvFinal )
		{
			const char *pszModel = kvFinal->GetString("model");
			if ( pszModel && pszModel[0] )
			{
				int iModel = CBaseEntity::PrecacheModel( pszModel );
				PrecacheGibsForModel( iModel );
			}

			const char *pszModelArm = kvFinal->GetString("arm_model");
			if ( pszModelArm && pszModelArm[0] )
			{
				CBaseEntity::PrecacheModel( pszModelArm );
			}

			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_death") );
			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_crit_death") );
			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_melee_death") );
			CBaseEntity::PrecacheScriptSound( kvFinal->GetString("sound_explosion_death") );
		
			// Precache TFC mercenary if necessary
			if ( IsRetroModeEnabled() )
			{
				// TFC
				KeyValues* kvModifier = kvFinal->FindKey("TFC");
				if( kvModifier )
				{
					pszModel = kvModifier->GetString("model");
					if ( pszModel && pszModel[0] )
					{
						int iModel = CBaseEntity::PrecacheModel( pszModel );
						PrecacheGibsForModel( iModel );
					}

					pszModelArm = kvModifier->GetString("arm_model");
					if ( pszModelArm && pszModelArm[0] )
					{
						CBaseEntity::PrecacheModel( pszModelArm );
					}
		
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_crit_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_melee_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_explosion_death") );
				}
			}
			// Precache the zombie mercenary if necessary
			if ( TFGameRules()->IsInfGamemode() )
			{
				KeyValues* kvModifier = kvFinal->FindKey("Zombie");
				if( kvModifier )
				{
					pszModel = kvModifier->GetString("model");
					if ( pszModel && pszModel[0] )
					{
						int iModel = CBaseEntity::PrecacheModel( pszModel );
						PrecacheGibsForModel( iModel );
					}

					pszModelArm = kvModifier->GetString("arm_model");
					if ( pszModelArm && pszModelArm[0] )
					{
						CBaseEntity::PrecacheModel( pszModelArm );
					}	
		
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_crit_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_melee_death") );
					CBaseEntity::PrecacheScriptSound( kvModifier->GetString("sound_explosion_death") );
				}
			}
		}

		// Precache all base weapons of the mercenary, all other weapons are precached by their spawners
		TFPlayerClassData_t* pData = GetPlayerClassData( TF_CLASS_MERCENARY );

		for ( int iWeapon = 0; iWeapon < pData->m_iWeaponCount; iWeapon++ )
		{
			int iWeaponID = pData->m_aWeapons[iWeapon];
			if ( iWeaponID != TF_WEAPON_NONE )
			{
				const char* pszWeaponName = WeaponIdToClassname( iWeaponID );
				if ( !pszWeaponName )
					continue;

				UTIL_PrecacheOther( pszWeaponName );

			}
		}
	}

	// Precache cosmetics for mercenary
	// undone: this precaching is now done when loadouts are updated at UpdateCosmetics in tf_player.cpp
	// undone the undone: that causes so much stuttering, so this has to be done
	KeyValues* pItemsGame = new KeyValues( "items_game" );
	pItemsGame->LoadFromFile( filesystem, "scripts/items/items_game.txt" );
	if ( pItemsGame )
	{
		KeyValues* pCosmetics = pItemsGame->FindKey( "Cosmetics" );
		if ( pCosmetics )
		{
			for ( KeyValues *pCosmetic = pCosmetics->GetFirstSubKey(); pCosmetic; pCosmetic = pCosmetic->GetNextKey() )
			{
				if ( pCosmetic )
				{
					if( Q_stricmp(pCosmetic->GetString( "Model" ), "BLANK") )
						CBaseEntity::PrecacheModel( pCosmetic->GetString( "Model" ) );
					
					if ( !Q_stricmp(pCosmetic->GetString("region"), "gloves") || !Q_stricmp(pCosmetic->GetString("region"), "suit") )
						if( Q_stricmp(pCosmetic->GetString( "viewmodel" ), "BLANK") )
							CBaseEntity::PrecacheModel( pCosmetic->GetString( "viewmodel" ) );					
				}
			}
		}
	}

	// Precache this for DM
	CBaseEntity::PrecacheModel( "models/player/attachments/mercenary_shield.mdl" );

	// birthday hat
	if ( TFGameRules() && TFGameRules()->IsBirthday() )
	{
		for ( int i = 1; i < ARRAYSIZE(g_pszBDayGibs); i++ )
		{
			CBaseEntity::PrecacheModel( g_pszBDayGibs[i] );
		}
		CBaseEntity::PrecacheModel( "models/effects/bday_hat.mdl" );
	}
}

#endif

#ifdef GAME_DLL
void CTFGameRules::OnNavMeshLoad( void )
{
	TheNavMesh->SetPlayerSpawnName( "info_player_teamspawn" );
}

void CTFGameRules::LevelShutdown( void )
{
	TheTFBots().OnLevelShutdown();
}
#endif

void CTFGameRules::CheckTDM( void )
{
	for( int i = TF_GAMETYPE_UNDEFINED + 1; i < TF_GAMETYPE_LAST; i++ )
	{
		if ( i == TF_GAMETYPE_DM || i == TF_GAMETYPE_TDM )
			continue;
		if ( InGametype ( i ) )
		{
			m_bIsTDM = false;
			return;
		}
	}
	m_bIsTDM = InGametype( TF_GAMETYPE_DM ) && InGametype( TF_GAMETYPE_TDM );
}

bool CTFGameRules::IsDMGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_DM ); 
}

bool CTFGameRules::IsTDMGamemode( void )
{ 
	return m_bIsTDM;
}

bool CTFGameRules::IsDOMGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_DOM ); 
}

bool CTFGameRules::IsTeamplay( void )
{ 
	return InGametype( TF_GAMETYPE_TDM );
}

bool CTFGameRules::DontCountKills( void )
{ 
	return m_nbDontCountKills || IsGGGamemode() || IsJugGamemode(); 
}

bool CTFGameRules::IsGGGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_GG );
}

bool CTFGameRules::Is3WaveGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_3WAVE );
}

bool CTFGameRules::IsArenaGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_ARENA );
}

bool CTFGameRules::IsESCGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_ESC );
}

bool CTFGameRules::IsInfGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_INF );
}

bool CTFGameRules::IsJugGamemode( void )
{ 
	return InGametype( TF_GAMETYPE_JUG );
}

bool CTFGameRules::IsPayloadOverride( void )
{ 
	return m_bEscortOverride;
}

bool CTFGameRules::IsFreeRoam( void )
{ 
	return m_bIsFreeRoamMap;
}

bool CTFGameRules::IsCoopEnabled(void)
{
	return of_coop.GetBool();
}

bool CTFGameRules::UsesMoney( void )
{ 
	return m_bUsesMoney;
}
#ifdef GAME_DLL

void CTFGameRules::FireGamemodeOutputs()
{
#ifdef GAME_DLL
	CTFGameRulesProxy *pProxy = dynamic_cast<CTFGameRulesProxy*> (gEntList.FindEntityByClassname(NULL, "tf_gamerules"));
	if ( pProxy )
	{
		if ( InGametype( TF_GAMETYPE_CTF ) )
				pProxy->FireCTFOutput();
		if ( InGametype( TF_GAMETYPE_CP ) )
				pProxy->FireCPOutput();
		if ( InGametype( TF_GAMETYPE_DM ) )
			pProxy->FireDMOutput();
		if ( IsTeamplay() )
			pProxy->FireTeamplayOutput();
		if ( InGametype( TF_GAMETYPE_GG ) )
			pProxy->FireGunGameOutput();
		if ( InGametype( TF_GAMETYPE_JUG ) )
			pProxy->FireJugOutput();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanBotChooseClass( CBasePlayer *pBot, int iDesiredClassIndex )
{
	// TODO: Implement CTFBotRoster entity
	//return CanPlayerChooseClass( pBot, iDesiredClassIndex );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::CanBotChangeClass( CBasePlayer *pBot )
{
	if ( ( TFGameRules()->IsDMGamemode() && IsAllClassEnabled() ) )
		return false;

	if ( !pBot || !pBot->IsPlayer()/* || !DWORD(a2 + 2309) */ )
		return false;

	// TODO: Implement CTFBotRoster entity
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	bool bRetVal = true;

	if ( ( State_Get() == GR_STATE_TEAM_WIN ) && pVictim )
	{
		if ( pVictim->GetTeamNumber() == GetWinningTeam() )
		{
			CBaseTrigger *pTrigger = dynamic_cast< CBaseTrigger *>( info.GetInflictor() );

			// we don't want players on the winning team to be
			// hurt by team-specific trigger_hurt entities during the bonus time
			if ( pTrigger && pTrigger->UsesFilter() )
			{
				bRetVal = false;
			}
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetTeamGoalString( int iTeam, const char *pszGoal )
{
	if ( iTeam == TF_TEAM_RED )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringRed.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringRed.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringRed.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_BLUE )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringBlue.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringBlue.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringBlue.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
	else if ( iTeam == TF_TEAM_MERCENARY )
	{
		if ( !pszGoal || !pszGoal[0] )
		{
			m_pszTeamGoalStringMercenary.GetForModify()[0] = '\0';
		}
		else
		{
			if ( Q_stricmp( m_pszTeamGoalStringMercenary.Get(), pszGoal ) )
			{
				Q_strncpy( m_pszTeamGoalStringMercenary.GetForModify(), pszGoal, MAX_TEAMGOAL_STRING );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::RoundCleanupShouldIgnore( CBaseEntity *pEnt )
{
	if ( FindInList( s_PreserveEnts, pEnt->GetClassname() ) )
		return true;

	//There has got to be a better way of doing this.
	if ( Q_strstr( pEnt->GetClassname(), "tf_weapon_" ) )
		return true;

	return BaseClass::RoundCleanupShouldIgnore( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCreateEntity( const char *pszClassName )
{
	if ( FindInList( s_PreserveEnts, pszClassName ) )
		return false;

	return BaseClass::ShouldCreateEntity( pszClassName );
}

void CTFGameRules::CleanUpMap( void )
{
	BaseClass::CleanUpMap();

	if ( HLTVDirector() )
	{
		HLTVDirector()->BuildCameraList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------ob
void CTFGameRules::RecalculateControlPointState( void )
{
	Assert( ObjectiveResource() );

	if ( !g_hControlPointMasters.Count() )
		return;

	if ( g_pObjectiveResource && g_pObjectiveResource->PlayingMiniRounds() )
		return;

	for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, true );
		if ( iFarthestPoint == -1 )
			continue;

		// Now enable all spawn points for that spawn point
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);
			if ( pTFSpawn->GetControlPoint() )
			{
				if ( pTFSpawn->GetTeamNumber() == iTeam )
				{
					if ( pTFSpawn->GetControlPoint()->GetPointIndex() == iFarthestPoint )
					{
						pTFSpawn->SetDisabled( false );
					}
					else
					{
						pTFSpawn->SetDisabled( true );
					}
				}
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is being initialized
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundStart( void )
{
	SetupMutator();

	FireGamemodeOutputs();

	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		ObjectiveResource()->SetBaseCP( -1, i );
	}

	for ( int i = 0; i < TF_TEAM_COUNT; i++ )
	{
		m_iNumCaps[i] = 0;
	}

	m_bHasCivilianSpawns = false;
	m_bHasJuggernautSpawns = false;

	// Preload the GG settings here so changing mutators mid game doesnt crash
	CTFLogicGG *pLogicGG = dynamic_cast<CTFLogicGG *>(gEntList.FindEntityByClassname(NULL, "of_logic_gg") );
	if ( pLogicGG )
	{
		KeyValues *pWeapon = new KeyValues( "Weapon" );
		pWeapon = pLogicGG->pWeaponsData->GetFirstValue();
		m_iMaxLevel = 0;
		for( pWeapon; pWeapon != NULL; pWeapon = pWeapon->GetNextValue() ) // Loop through all the keyvalues
		{
			m_iszWeaponName[m_iMaxLevel] = MAKE_STRING( pWeapon->GetString() );
			m_iMaxLevel++;
		}
		m_iRequiredKills = pLogicGG->m_iRequiredKills;
	}

	// verify if info_player_teamspawns exist on this map
	// this also whether spawnpoints exist with the Civilian or Juggernaut spawnflag
	// see IsSpawnPointValid to see why this is necessary
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
	while ( pSpot )
	{
		CTFTeamSpawn *pTFSpawn = dynamic_cast< CTFTeamSpawn* >( pSpot );
		if ( pTFSpawn )
		{
			if ( pTFSpawn->AllowCivilian() )
				m_bHasCivilianSpawns = true;

			if ( pTFSpawn->AllowJuggernaut() )
				m_bHasJuggernautSpawns = true;
		}

		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
	}
	
	m_hRedAttackTrain = NULL;
	m_hBlueAttackTrain = NULL;
	m_hRedDefendTrain = NULL;
	m_hBlueDefendTrain = NULL;

	SetIT( NULL );

	// Let all entities know that a new round is starting
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundSpawn", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	// All entities have been spawned, now activate them
	pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		variant_t emptyVariant;
		pEnt->AcceptInput( "RoundActivate", NULL, NULL, emptyVariant, 0 );

		pEnt = gEntList.NextEnt( pEnt );
	}

	if ( g_pObjectiveResource && !g_pObjectiveResource->PlayingMiniRounds() )
	{
		// Find all the control points with associated spawnpoints
		Q_memset( m_bControlSpawnsPerTeam, 0, sizeof(bool) * MAX_TEAMS * MAX_CONTROL_POINTS );
		CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		while( pSpot )
		{
			CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);
			if ( pTFSpawn->GetControlPoint() )
			{
				m_bControlSpawnsPerTeam[ pTFSpawn->GetTeamNumber() ][ pTFSpawn->GetControlPoint()->GetPointIndex() ] = true;
				pTFSpawn->SetDisabled( true );
			}

			pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
		}

		RecalculateControlPointState();

		SetRoundOverlayDetails();
	}

	m_nDomScore_limit = 100;
	m_nDomScore_time = 3;
	m_nDomScore_red = 0;
	m_nDomScore_blue = 0;
	m_bDomWinOnLimit = true;
	m_bDomRedThreshold = false;
	m_bDomBlueThreshold = false;
	m_bDomRedLeadThreshold = false;
	m_bDomBlueLeadThreshold = false;
	m_bEntityLimitPrevented = false;
	
	if ( IsArenaGamemode() )
	{
		m_flStalemateStartTime = gpGlobals->curtime;
	}
#ifdef GAME_DLL
	m_szMostRecentCappers[0] = 0;
#endif

	if ( of_disable_healthkits.GetBool() )
	{
		for ( int i = 0; i < IHealthKitAutoList::AutoList().Count(); ++i )
		{
			CHealthKit *pHealthKit = static_cast< CHealthKit* >( IHealthKitAutoList::AutoList()[ i ] );
			pHealthKit->SetDisabled( true );
		}
	}
	else
	{
		for ( int i = 0; i < IHealthKitAutoList::AutoList().Count(); ++i )
		{
			CHealthKit *pHealthKit = static_cast< CHealthKit* >( IHealthKitAutoList::AutoList()[ i ] );
			pHealthKit->SetDisabled( false );
		}
	}

	if ( of_disable_ammopacks.GetBool() )
	{
		for ( int i = 0; i < IAmmoPackAutoList::AutoList().Count(); ++i )
		{
			CAmmoPack *pAmmoPack = static_cast< CAmmoPack* >( IAmmoPackAutoList::AutoList()[ i ] );
			pAmmoPack->SetDisabled( true );
		}
	}
	else
	{
		for ( int i = 0; i < IAmmoPackAutoList::AutoList().Count(); ++i )
		{
			CAmmoPack *pAmmoPack = static_cast< CAmmoPack* >( IAmmoPackAutoList::AutoList()[ i ] );
			pAmmoPack->SetDisabled( false );
		}
	}
	
	if ( !of_weaponspawners.GetBool() || of_randomizer.GetBool() || !TFGameRules()->IsMutator( NO_MUTATOR ) || TFGameRules()->IsGGGamemode() )
	{
		for ( int i = 0; i < IWeaponSpawnerAutoList::AutoList().Count(); ++i )
		{
			CWeaponSpawner *pWeaponSpawner = static_cast< CWeaponSpawner* >( IWeaponSpawnerAutoList::AutoList()[ i ] );
			pWeaponSpawner->SetDisabled( true );
		}
	}
	else
	{
		for ( int i = 0; i < IWeaponSpawnerAutoList::AutoList().Count(); ++i )
		{
			CWeaponSpawner *pWeaponSpawner = static_cast< CWeaponSpawner* >( IWeaponSpawnerAutoList::AutoList()[ i ] );
			pWeaponSpawner->SetDisabled( false );
		}
	}	

	if ( TFGameRules()->IsMutator( INSTAGIB ) || TFGameRules()->IsMutator( INSTAGIB_NO_MELEE ) || !of_powerups.GetBool() )
	{
		for ( int i = 0; i < ICondPowerupAutoList::AutoList().Count(); ++i )
		{
			CCondPowerup *pPowerup = static_cast< CCondPowerup* >( ICondPowerupAutoList::AutoList()[ i ] );
			pPowerup->SetDisabled( true );
		}
	}
	else
	{
		for ( int i = 0; i < ICondPowerupAutoList::AutoList().Count(); ++i )
		{
			CCondPowerup *pPowerup = static_cast< CCondPowerup* >( ICondPowerupAutoList::AutoList()[ i ] );
			pPowerup->SetDisabled( false );
		}
	}	

	m_hLogicLoadout.Purge();
	CTFLogicLoadout *pLogicLoadout = gEntList.NextEntByClass( (CTFLogicLoadout *)NULL );
	while ( pLogicLoadout )
	{
		m_hLogicLoadout.AddToTail( pLogicLoadout );
		pLogicLoadout = gEntList.NextEntByClass( pLogicLoadout );
	}	
	
	if ( TFGameRules()->IsInfGamemode() )
	{
		// put everyone into RED again
		CTFPlayer *pPlayer = NULL;
		int i;

		for ( i = 1; i <= gpGlobals->maxClients; i++ )
		{
			pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pPlayer && pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->m_Shared.SetZombie( false );
				pPlayer->ChangeTeam( TF_TEAM_RED, true );
				if ( TFGameRules()->IsAllClassEnabled() ) 
					pPlayer->SetDesiredPlayerClassIndex( TF_CLASS_MERCENARY );
			}
		}

		// remove the timer
		if ( GetInfectionRoundTimer() )
			UTIL_Remove( GetInfectionRoundTimer() );

		// HORRIBLE HORRIBLE HACK TO FIX SPAWN ROOMS ON 2FORT
		if ( !Q_strncmp( STRING( gpGlobals->mapname), "ctf_2fort", 9 ) )
		{
			CBaseEntity *pEntity = NULL;
			while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_door" ) ) != NULL )
			{
				UTIL_Remove( pEntity );
			}
		}		

		// no more visualizers, or respawn rooms, or regenerate lockers
		for ( int i = 0; i < IRegenerateZoneAutoList::AutoList().Count(); ++i )
		{
			UTIL_Remove( static_cast< CRegenerateZone* >( IRegenerateZoneAutoList::AutoList()[ i ] ) );
		}
		for ( int i = 0; i < IFuncRespawnRoomAutoList::AutoList().Count(); ++i )
		{
			CFuncRespawnRoom *pRespawnRoom = static_cast< CFuncRespawnRoom* >( IFuncRespawnRoomAutoList::AutoList()[ i ] );

			for ( int j = 0; j < pRespawnRoom->m_hVisualizers.Count(); j++ )
			{
				if ( pRespawnRoom->m_hVisualizers[j].IsValid() )
					UTIL_Remove( pRespawnRoom->m_hVisualizers[j].Get() );
			}

			UTIL_Remove( pRespawnRoom );
		}

		if ( !IsInWaitingForPlayers() )
		{
			// wow
			variant_t sVariant;
			sVariant.SetInt( 0 );

			// create the timer
			TFGameRules()->SetInfectionRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

			if ( TFGameRules()->GetInfectionRoundTimer() )
			{
				TFGameRules()->GetInfectionRoundTimer()->SetName( MAKE_STRING( "zz_infection_timer" ) );
				TFGameRules()->GetInfectionRoundTimer()->SetInfectionBeginning( true );
				TFGameRules()->GetInfectionRoundTimer()->SetTimeRemaining( of_infection_preparetime.GetInt() ); 
				TFGameRules()->GetInfectionRoundTimer()->SetShowInHud( true );
				TFGameRules()->GetInfectionRoundTimer()->ChangeTeam( TF_TEAM_RED );

				TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
				TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
			}

			// all teams
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "InfectionMusic.Warmup", false );
				BroadcastSound( i, "Benja.RoundStart" );
			}
		}
	}
}

// Ran at the start of every round, simply setups and performs anything necessary for mutators
void CTFGameRules::SetupMutator( void )
{
	SetMutator ( of_mutator.GetInt() );

	bool bIsGunGame = false;
	
	switch( GetMutator() )
	{
		case NO_MUTATOR:
			ConColorMsg(Color(86, 156, 143, 255), "[TFGameRules] Executing server DISABLED mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_disabled.cfg \n");
			engine->ServerExecute();
			break;
		case INSTAGIB:
			ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Instagib mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_instagib.cfg \n");
			engine->ServerExecute();
			break;
		case INSTAGIB_NO_MELEE:
			ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Instagib (no melee) mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_instagibnomelee.cfg \n");
			engine->ServerExecute();
			break;	
		case CLAN_ARENA:
			ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Clan Arena mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_clanarena.cfg \n");
			engine->ServerExecute();
			break;
		case UNHOLY_TRINITY:
			ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Unholy Trinity mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_unholytrinity.cfg \n");
			engine->ServerExecute();
			break;
		case ROCKET_ARENA:
			ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Rocket Arena mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_rocketarena.cfg \n");
			engine->ServerExecute();
			break;
		case GUN_GAME:
			bIsGunGame = true;
			// gungame shouldnt be a gametype...
			if ( !InGametype( TF_GAMETYPE_GG ) )
				AddGametype( TF_GAMETYPE_GG );
	
			ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Gun Game mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_gungame.cfg \n");
			engine->ServerExecute();
			break;
		case ARSENAL:
			ConColorMsg(Color(123, 176, 130, 255), "[TFGameRules] Executing server Arsenal mutator config file\n");
			engine->ServerCommand("exec config_default_mutator_arsenal.cfg \n");
			engine->ServerExecute();
			break;
	}
	
	if( !bIsGunGame && InGametype( TF_GAMETYPE_GG ) )
		RemoveGametype( TF_GAMETYPE_GG );
}

void CTFGameRules::PassAllTracks( void )
{
	// At here we trigger all but the last 2 Track Trains
	// We do this because some maps have doors that only open when the train passes
	// or completley block off areas
	for ( int i = 0; i < m_hTracksToPass.Count(); i++ )
	{
		if( m_hTracksToPass[i] ) 
			m_hTracksToPass[i]->Pass( NULL ); 
	}
	for ( int i = 0; i < m_hTracksToPass.Count(); i++ )
	{
		if( m_hTracksToPass[i] ) 
			m_hTracksToPass.Remove(i);
	}
	if ( m_bDisableRedSpawns )
		DisableSpawns( TF_TEAM_RED );
	if ( m_bDisableBluSpawns )
		DisableSpawns( TF_TEAM_BLUE );
}

void CTFGameRules::KeepTeamSpawns( int iTeamNumber )
{
	switch ( iTeamNumber )
	{
		case TF_TEAM_RED:
			m_bDisableRedSpawns = true;
		break;
		case TF_TEAM_BLUE:
			m_bDisableBluSpawns = true;
		break;
	}
}

void CTFGameRules::DisableSpawns( int iTeamNumber )
{
	const char *pEntClassName = "info_player_teamspawn";
	CBaseEntity *pSpot;
	// Get an initial spawn point.
	pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );
	if ( !pSpot )
	{
		// Sometimes the first spot can be NULL????
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
	// First we try to find a spawn point that is fully clear. If that fails,
	// we look for a spawnpoint that's clear except for another players. We
	// don't collide with our team members, so we should be fine.
	CBaseEntity *pFirstSpot = pSpot;
	do{
		if ( pSpot )
		{
			CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
			// don't run validity checks on a info_player_start, as spawns in singleplayer maps should always be valid
			bool bSpotCreated = false;
			// Check to see if this is a valid team spawn (player is on this team, etc.).
			if( pSpot->GetTeamNumber() == iTeamNumber )
			{
				// Check for a bad spawn entity.
				if ( pSpot->GetAbsOrigin() != Vector( 0, 0, 0 ) )
				{
					bool bKill = true;
					for ( int i = 0; i < m_hReEnableSpawns.Count(); i++ )
					{
						if ( m_hReEnableSpawns[i] == pCTFSpawn )
						{
							bKill = false;
						}
					}
					if ( bKill )
					{
						// Found a valid spawn point.
						pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
						bSpotCreated = true;
						inputdata_t Temp;
						UTIL_Remove( pCTFSpawn );//->InputDisable( Temp );
					}
				}
			}
			if ( !bSpotCreated )
				pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
			if ( pSpot != pFirstSpot )
			{
				continue;
			}
			else
				break;
		}
		else
			break;
	}
		// Continue until a valid spawn point is found or we hit the start.
	while ( pSpot != pFirstSpot );
} 

//-----------------------------------------------------------------------------
// Purpose: Called when a new round is off and running
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnRoundRunning( void )
{
	CTFLogicArena *pArena = dynamic_cast< CTFLogicArena* > ( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );

	if ( pArena )
		pArena->m_ArenaRoundStart.FireOutput( NULL, pArena );

	if ( IsArenaGamemode() )
		m_flStalemateStartTime = gpGlobals->curtime - tf_stalematechangeclasstime.GetFloat();

	// Let out control point masters know that the round has started
	for ( int i = 0; i < g_hControlPointMasters.Count(); i++ )
	{
		variant_t emptyVariant;
		if ( g_hControlPointMasters[i] )
		{
			g_hControlPointMasters[i]->AcceptInput( "RoundStart", NULL, NULL, emptyVariant, 0 );
		}
	}

	// Reset player speeds after preround lock
	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->TeamFortress_SetSpeed();
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_ROUND_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called before a new round is started (so the previous round can end)
//-----------------------------------------------------------------------------
void CTFGameRules::PreviousRoundEnd( void )
{
	// before we enter a new round, fire the "end output" for the previous round
	if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
	{
		g_hControlPointMasters[0]->FireRoundEndOutput();
	}

	m_iPreviousRoundWinners = GetWinningTeam();
}

//-----------------------------------------------------------------------------
// Purpose: Called when a round has entered stalemate mode (timer has run out)
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateStart( void )
{
	// Remove everyone's objects
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();
		}
	}

	// Respawn all the players
	RespawnPlayers( true );

	// exception for arena
	if ( InGametype( TF_GAMETYPE_ARENA ) )
	{
		CTFLogicArena *pArena = dynamic_cast<CTFLogicArena *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
		if ( pArena )
		{
			pArena->m_ArenaRoundStart.FireOutput( NULL, pArena );

			IGameEvent *event = gameeventmanager->CreateEvent( "arena_round_start" );

			if ( event )
				gameeventmanager->FireEvent( event );

			// all teams
			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				BroadcastSound( i, "Ambient.Siren", false );
				BroadcastSound( i, "AM_RoundStartRandom" );
			}

			m_flStalemateStartTime = gpGlobals->curtime;
		}
	}

	// Disable all the active health packs in the world
	for ( int i = 0; i < IHealthKitAutoList::AutoList().Count(); ++i )
	{
		CHealthKit *pHealthKit = static_cast< CHealthKit* >( IHealthKitAutoList::AutoList()[ i ] );
		pHealthKit->SetDisabled( true );
	}

	CTFPlayer *pPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SUDDENDEATH_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupOnStalemateEnd( void )
{
	for ( int i = 0; i < IHealthKitAutoList::AutoList().Count(); ++i )
	{
		CHealthKit *pHealthKit = static_cast< CHealthKit* >( IHealthKitAutoList::AutoList()[ i ] );
		pHealthKit->SetDisabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitTeams( void )
{
	BaseClass::InitTeams();

	// clear the player class data
	ResetFilePlayerClassInfoDatabase();
}


ConVar tf_fixedup_damage_radius ( "tf_fixedup_damage_radius", "1", FCVAR_CHEAT );
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//			&vecSrcIn - 
//			flRadius - 
//			iClassIgnore - 
//			*pEntityIgnore - 
//-----------------------------------------------------------------------------
void CTFGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT&(~CONTENTS_HITBOX);
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		falloff;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;

	if ( info.GetDamageType() & DMG_RADIUS_MAX )
		falloff = 0.0;
	else if ( info.GetDamageType() & DMG_HALF_FALLOFF )
		falloff = 0.5;
	else if ( flRadius )
		falloff = info.GetDamage() / flRadius;
	else
		falloff = 1.0;

	CBaseEntity *pInflictor = info.GetInflictor();
	
//	float flHalfRadiusSqr = Square( flRadius / 2.0f );

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// This value is used to scale damage when the explosion is blocked by some other object.
		float flBlockedDamagePercent = 0.0f;

		if ( pEntity == pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
		{// houndeyes don't hurt other houndeyes with their attack
			continue;
		}

		// Check that the explosion can 'see' this entity.
		vecSpot = pEntity->BodyTarget( vecSrc, false );
		CTraceFilterIgnorePlayers filter( info.GetInflictor(), COLLISION_GROUP_PROJECTILE );
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, &filter, &tr );

		if ( tr.fraction != 1.0 && tr.m_pEnt != pEntity )
			continue;

		// Adjust the damage - apply falloff.
		float flAdjustedDamage = 0.0f;
		//float flNonSelfDamage = 0.0f;

		float flDistanceToEntity;

		// Rockets store the ent they hit as the enemy and have already
		// dealt full damage to them by this time
		if ( pInflictor && ( pEntity == pInflictor->GetEnemy() ) )
		{
			// Full damage, we hit this entity directly
			flDistanceToEntity = 0.0f;
		}
		else if ( pEntity->IsPlayer() || pEntity->IsNPC() )
		{
			// Use whichever is closer, absorigin or worldspacecenter
			float flToWorldSpaceCenter = ( vecSrc - pEntity->WorldSpaceCenter() ).Length();
			float flToOrigin = ( vecSrc - pEntity->GetAbsOrigin() ).Length();

			flDistanceToEntity = min( flToWorldSpaceCenter, flToOrigin );
		}
		else
		{
			flDistanceToEntity = ( vecSrc - tr.endpos ).Length();
		}

		if ( tf_fixedup_damage_radius.GetBool() )
		{
			flAdjustedDamage = RemapValClamped(flDistanceToEntity, 0, flRadius, info.GetDamage(), info.GetDamage() * falloff);
		}
		else
		{
			flAdjustedDamage = flDistanceToEntity * falloff;
			flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
		}
		
		// NOTE: explosive damage is modified later in TakeDamage anyway to have 0 damage with some cvars, mutators etc
		if ( flAdjustedDamage <= 0 )
			continue;

		// insta kill on direct hit
		if ( flDistanceToEntity == 0.0f && TFGameRules()->IsMutator( ROCKET_ARENA ) )
		{
			flAdjustedDamage *= 10.0f;
		}

		// the explosion can 'see' this entity, so hurt them!
		if ( tr.startsolid )
		{
			// if we're stuck inside them, fixup the position and distance
			tr.endpos = vecSrc;
			tr.fraction = 0.0;
		}
		
		CTakeDamageInfo adjustedInfo = info;
		//Msg("%s: Blocked damage: %f percent (in:%f  out:%f)\n", pEntity->GetClassname(), flBlockedDamagePercent * 100, flAdjustedDamage, flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
		adjustedInfo.SetDamage( flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );

		adjustedInfo.SetDamageForForceCalc( adjustedInfo.GetDamage() );
		if( info.GetAttacker() == pEntity )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>( info.GetWeapon() );
			if ( pWeapon )
			{
				float flMultiplier = pWeapon->GetTFWpnData().m_nBlastJumpDamageForce / pWeapon->GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nDamage;
				if( flMultiplier != 1.0f )
				{
					adjustedInfo.SetDamageForceMult( flMultiplier );
				}
			}		
		}

		// Now make a consideration for skill level!
		if( info.GetAttacker() && info.GetAttacker()->IsPlayer() && pEntity->IsNPC() )
		{
			// An explosion set off by the player is harming an NPC. Adjust damage accordingly.
			adjustedInfo.AdjustPlayerDamageInflictedForSkillLevel();
		}

		Vector dir = vecSpot - vecSrc;
		VectorNormalize(dir);

		// If we don't have a damage force, manufacture one
		if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
		{
			CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc);
		}
		else
		{
			// Assume the force passed in is the maximum force. Decay it based on falloff.
			float flForce = adjustedInfo.GetDamageForce().Length() * falloff;

			adjustedInfo.SetDamageForce(dir * flForce);
			adjustedInfo.SetDamagePosition(vecSrc);
		}

		if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
		{
			ClearMultiDamage( );
			pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );

			ApplyMultiDamage();
		}
		else
		{
			pEntity->TakeDamage( adjustedInfo );
		}

		// Now hit all triggers along the way that respond to damage... 
		pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );
	}
}

	// --------------------------------------------------------------------------------------------------- //
	// Voice helper
	// --------------------------------------------------------------------------------------------------- //

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			// Dead players can only be heard by other dead team mates but only if a match is in progress
			if ( TFGameRules()->State_Get() != GR_STATE_TEAM_WIN && TFGameRules()->State_Get() != GR_STATE_GAME_OVER ) 
			{
				if ( pTalker->IsAlive() == false )
				{
					if ( pListener->IsAlive() == false || tf_gravetalk.GetBool() )
						return ( pListener->InSameTeam( pTalker ) );

					return false;
				}
			}

			return ( pListener->InSameTeam( pTalker ) );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

	// Load the objects.txt file.
	class CObjectsFileLoad : public CAutoGameSystem
	{
	public:
		virtual bool Init()
		{
			LoadObjectInfos( filesystem );
			return true;
		}
	} g_ObjectsFileLoad;

	// --------------------------------------------------------------------------------------------------- //
	// Globals.
	// --------------------------------------------------------------------------------------------------- //
/*
	// NOTE: the indices here must match TEAM_UNASSIGNED, TEAM_SPECTATOR, TF_TEAM_RED, TF_TEAM_BLUE, etc.
	char *sTeamNames[] =
	{
		"Unassigned",
		"Spectator",
		"Red",
		"Blue"
	};
*/
	// --------------------------------------------------------------------------------------------------- //
	// Global helper functions.
	// --------------------------------------------------------------------------------------------------- //
	
	// World.cpp calls this but we don't use it in TF.
	void InitBodyQue()
	{
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CTFGameRules::~CTFGameRules()
	{
		// Note, don't delete each team since they are in the gEntList and will 
		// automatically be deleted from there, instead.
		TFTeamMgr()->Shutdown();
		ShutdownCustomResponseRulesDicts();
	}

	//-----------------------------------------------------------------------------
	// Purpose: TF2 Specific Client Commands
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
	bool CTFGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pEdict );

		gameeventmanager->LoadEventsFromFile("resource/ModEvents.res");
		const char *pcmd = args[0];
		if ( FStrEq( pcmd, "objcmd" ) )
		{
			if ( args.ArgC() < 3 )
				return true;

			int entindex = atoi( args[1] );
			edict_t* pEdict = INDEXENT(entindex);
			if ( pEdict )
			{
				CBaseEntity* pBaseEntity = GetContainingEntity(pEdict);
				CBaseObject* pObject = dynamic_cast<CBaseObject*>(pBaseEntity);

				if ( pObject )
				{
					// We have to be relatively close to the object too...

					// BUG! Some commands need to get sent without the player being near the object, 
					// eg delayed dismantle commands. Come up with a better way to ensure players aren't
					// entering these commands in the console.

					//float flDistSq = pObject->GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );
					//if (flDistSq <= (MAX_OBJECT_SCREEN_INPUT_DISTANCE * MAX_OBJECT_SCREEN_INPUT_DISTANCE))
					{
						// Strip off the 1st two arguments and make a new argument string
						CCommand objectArgs( args.ArgC() - 2, &args.ArgV()[2]);
						pObject->ClientCommand( pPlayer, objectArgs );
					}
				}
			}

			return true;
		}

		// Handle some player commands here as they relate more directly to gamerules state
		if ( FStrEq( pcmd, "nextmap" ) )
		{
			if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
			{
				char szNextMap[32];

				if ( nextlevel.GetString() && *nextlevel.GetString() && engine->IsMapValid( nextlevel.GetString() ) )
				{
					Q_strncpy( szNextMap, nextlevel.GetString(), sizeof( szNextMap ) );
				}
				else
				{
					GetNextLevelName( szNextMap, sizeof( szNextMap ) );
				}

				ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_nextmap", szNextMap);

				pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
			}

			return true;
		}
		else if ( FStrEq( pcmd, "timeleft" ) )
		{	
			if ( pPlayer->m_flNextTimeCheck < gpGlobals->curtime )
			{
				if ( mp_timelimit.GetInt() > 0 )
				{
					int iTimeLeft = GetTimeLeft();

					char szMinutes[5];
					char szSeconds[3];

					if ( iTimeLeft <= 0 )
					{
						Q_snprintf( szMinutes, sizeof(szMinutes), "0" );
						Q_snprintf( szSeconds, sizeof(szSeconds), "00" );
					}
					else
					{
						Q_snprintf( szMinutes, sizeof(szMinutes), "%d", iTimeLeft / 60 );
						Q_snprintf( szSeconds, sizeof(szSeconds), "%02d", iTimeLeft % 60 );
					}				

					ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft", szMinutes, szSeconds );
				}
				else
				{
					ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_timeleft_nolimit" );
				}

				pPlayer->m_flNextTimeCheck = gpGlobals->curtime + 1;
			}
			return true;
		}
		else if( pPlayer->ClientCommand( args ) )
		{
            return true;
		}

		return BaseClass::ClientCommand( pEdict, args );
	}
	// Add the ability to ignore the world trace
	void CTFGameRules::Think()
	{
		if ( !g_fGameOver )
		{
			if ( gpGlobals->curtime > m_flNextPeriodicThink )
			{
				if ( State_Get() != GR_STATE_TEAM_WIN )
				{
					if ( CheckCapsPerRound() )
						return;
				}
			}
			// Only happens if we change the convar mid game
			// For the love of all Gaben please set your convars BEFORE you start the server
			// Literaly just look at this stank code
			if( TFGameRules()->UsesDMBuckets() != bMultiweapons )
			{
				bMultiweapons = TFGameRules()->UsesDMBuckets();
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CTFPlayer *pPlayer = ToTFPlayer(UTIL_PlayerByIndex( i ));
					if( pPlayer )
					{
						pPlayer->ClearSlots();
						CTFWeaponBase *pWeapon = (CTFWeaponBase *)pPlayer->GetWeapon( 0 );
						for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
						{
							pWeapon = (CTFWeaponBase *)pPlayer->GetWeapon( iWeapon );
							if ( pWeapon )
							{
								WeaponHandle hHandle;
								hHandle = pWeapon;
								if( hHandle )
								{
									if( pPlayer->m_hWeaponInSlot 
										&& pPlayer->m_hWeaponInSlot[pWeapon->GetSlot()][pWeapon->GetPosition()] 
										&& pPlayer->m_hWeaponInSlot[pWeapon->GetSlot()][pWeapon->GetPosition()] != hHandle)
									{
										pPlayer->DropWeapon( pPlayer->m_hWeaponInSlot[pWeapon->GetSlot()][pWeapon->GetPosition()].Get(),
										true, false, 
										pPlayer->m_hWeaponInSlot[pWeapon->GetSlot()][pWeapon->GetPosition()]->m_iClip1,
										pPlayer->m_hWeaponInSlot[pWeapon->GetSlot()][pWeapon->GetPosition()]->m_iReserveAmmo );
										UTIL_Remove( pPlayer->m_hWeaponInSlot[pWeapon->GetSlot()][pWeapon->GetPosition()] );
									}
									pPlayer->m_hWeaponInSlot[pWeapon->GetSlot()][pWeapon->GetPosition()] = hHandle;
								}
							}
						}
					}
				}
			}

			// there is less than 60 seconds left of time, start voting for next map
			if ( mp_timelimit.GetInt() > 0 && GetTimeLeft() <= 60 && !m_bStartedVote && !TFGameRules()->IsInWaitingForPlayers() )
			{
				DevMsg( "VoteController: Timeleft is less than 60 seconds, begin nextlevel voting... \n" );
				m_bStartedVote = true;
				//engine->ServerCommand( "callvote nextlevel" );
				char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
				szEmptyDetails[0] = '\0';
				g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
			}
			
			if ( IsArenaGamemode() )
			{
				if ( CountActivePlayers() > 1 && State_Get() == GR_STATE_RND_RUNNING )
				{
					int iDeadTeam = TEAM_UNASSIGNED;
					int iAliveTeam = TEAM_UNASSIGNED;

					// If a team is fully killed, the other team has won
					for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
					{
						CTeam *pTeam = GetGlobalTeam(i);
						Assert( pTeam );
						if ( !pTeam )
							continue;
						int iPlayers = pTeam->GetNumPlayers();
						if ( iPlayers )
						{
							int bFoundLiveOne = 0;
							for ( int player = 0; player < iPlayers; player++ )
							{
								if ( pTeam->GetPlayer(player) && pTeam->GetPlayer(player)->IsAlive())
								{
									bFoundLiveOne++;
									if ( pTeam->GetTeamNumber() != TF_TEAM_MERCENARY )
										break;
								}
							}
							if ( pTeam->GetTeamNumber() == TF_TEAM_MERCENARY  )
							{
								if ( bFoundLiveOne <= 1 )
								{
									iAliveTeam = i;
									iDeadTeam = i;
									break;
								}
								else
									iAliveTeam = i;
							}
							else
							{
								if ( bFoundLiveOne > 0 )
								{
									iAliveTeam = i;
								}
								else
								{
									iDeadTeam = i;
								}
							}							
						}
					}

					if ( iDeadTeam && iAliveTeam )
					{					
						SetWinningTeam( iAliveTeam, WINREASON_OPPONENTS_DEAD, m_bForceMapReset );
					}
				}
			}
			
			if ( IsESCGamemode() )
			{
				// If a team is fully killed, the other team has won
				for ( int i = LAST_SHARED_TEAM+1; i < GetNumberOfTeams(); i++ )
				{
					CTeam *pTeam = GetGlobalTeam(i);
					Assert( pTeam );
							int iPlayers = pTeam->GetNumPlayers();
					if ( iPlayers )
					{
						int bFoundCivs = 0;
						for ( int player = 0; player < iPlayers; player++ )
						{
							if ( pTeam->GetPlayer(player) && pTeam->GetPlayer(player)->IsConnected() )
							{
								CTFPlayer *pCiv = ToTFPlayer ( pTeam->GetPlayer(player) );
								if ( pCiv && pCiv->IsPlayerClass( TF_CLASS_CIVILIAN ) )
								{
									bFoundCivs++;
									if ( bFoundCivs > GetMaxHunted( i ) && !of_allow_special_classes.GetBool() )
									{
										pCiv->HandleCommand_JoinClass( "random", true );
										pCiv->AllowInstantSpawn();
										pCiv->ForceRespawn();											
										bFoundCivs--;
									}
								}
							}
						}
						if ( GetMaxHunted( i ) <= 0 )
							continue;
						int HuntedTracker = GetMaxHunted( i );
						if( iPlayers < GetMaxHunted( i ) )
							HuntedTracker = iPlayers;
						while( bFoundCivs < HuntedTracker )
						{
							int iPlayerIndex;
							iPlayerIndex = random->RandomInt( 0, iPlayers );
							if( pTeam->GetPlayer(iPlayerIndex) && pTeam->GetPlayer(iPlayerIndex)->IsConnected() )
							{
								CTFPlayer *pCiv = ToTFPlayer( pTeam->GetPlayer(iPlayerIndex) );
								if ( pCiv && !pCiv->IsPlayerClass( TF_CLASS_CIVILIAN ) )
								{
									pCiv->SetDesiredPlayerClassIndex( TF_CLASS_CIVILIAN );
									pCiv->AllowInstantSpawn();
									pCiv->ForceRespawn();								
									bFoundCivs++;
								}
							}
						}
					}
				}
			}
			
			if (m_hReEnableSpawns.Count())
			{
				for ( int i = 0; i < m_hReEnableSpawns.Count(); i++ )
				{
					if ( m_hReEnableSpawns[i] )
						{
							inputdata_t Temp; 
							m_hReEnableSpawns[i]->InputEnable( Temp );
						}
				}
			}
		} // Game playerdie
		// Play( MineOddity );
		
		SpawnHalloweenBoss();

		if ( !m_bEntityLimitPrevented )
			EntityLimitPrevention();
		
		BaseClass::Think();
	}
	// entity limit measures, if we are above 1950 then start clearing out entities 
	// this really only happens with >24 players on large maps such as tc_hydro	
	void CTFGameRules::EntityLimitPrevention()
	{
		if ( engine->GetEntityCount() > 2000 )
		{
			Warning("WARNING: Entity count exceeded 2000, removing unnecessary entities...");

			m_bEntityLimitPrevented = true;

			CBaseEntity *pEntity = NULL;
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "spotlight_end")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "beam")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "point_spotlight")) != NULL)
			{
				UTIL_Remove(pEntity);
			}

			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "env_lightglow")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "env_sprite")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "move_rope")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
			while ((pEntity = gEntList.FindEntityByClassname(pEntity, "keyframe_rope")) != NULL)
			{
				UTIL_Remove(pEntity);
			}
		}
	}	
	
	//Runs think for all player's conditions
	//Need to do this here instead of the player so players that crash still run their important thinks
	void CTFGameRules::RunPlayerConditionThink ( void )
	{
		for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pPlayer )
			{
				pPlayer->m_Shared.ConditionGameRulesThink();
			}
		}
	}

	void CTFGameRules::FrameUpdatePostEntityThink()
	{
		BaseClass::FrameUpdatePostEntityThink();

		RunPlayerConditionThink();
	}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SpawnHalloweenBoss( void )
{
	//if ( !IsHolidayActive( kHoliday_Halloween ) )
	//{
	//	SpawnZombieMob();
	//	return;
	//}

	float fSpawnInterval;
	float fSpawnVariation;
	CHalloweenBaseBoss::HalloweenBossType eBossType;

	if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_MANOR ) )
	{
		eBossType = CHalloweenBaseBoss::HEADLESS_HATMAN;
		fSpawnInterval = tf_halloween_boss_spawn_interval.GetFloat();
		fSpawnVariation = tf_halloween_boss_spawn_interval_variation.GetFloat();
	}
	else if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_VIADUCT ) )
	{
		eBossType = CHalloweenBaseBoss::EYEBALL_BOSS;
		fSpawnInterval = tf_halloween_eyeball_boss_spawn_interval.GetFloat();
		fSpawnVariation = tf_halloween_eyeball_boss_spawn_interval_variation.GetFloat();
	}
	else if ( /*IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE )*/false )
	{
		/*CWheelOfDoom *pWheelOfDoom = (CWheelOfDoom *)gEntList.FindEntityByClassname( NULL, "wheel_of_doom" );
		if ( pWheelOfDoom && !pWheelOfDoom->IsDoneBoardcastingEffectSound() )
			return;
		if (CMerasmus::m_level > 3)
		{
			fSpawnInterval = 60.0f;
			fSpawnVariation = 0.0f;
		}
		else
		{
			fSpawnInterval = tf_merasmus_spawn_interval.GetFloat();
			fSpawnVariation = tf_merasmus_spawn_interval_variation.GetFloat();
		}*/
		eBossType = CHalloweenBaseBoss::MERASMUS;
	}
	else
	{
		SpawnZombieMob();
		return;
	}

	if ( !m_hBosses.IsEmpty() )
	{
		if ( m_hBosses[0] )
		{
			isBossForceSpawning = false;
			StartBossTimer( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
			return;
		}
	}

	if ( !m_bossSpawnTimer.HasStarted() && !isBossForceSpawning )
	{
		if ( IsHalloweenScenario( HALLOWEEN_SCENARIO_LAKESIDE ) )
			StartBossTimer( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
		else
			StartBossTimer( RandomFloat( 0, fSpawnInterval + fSpawnVariation ) * 0.5 );

		return;
	}

	if ( m_bossSpawnTimer.IsElapsed() || isBossForceSpawning )
	{
		if ( !isBossForceSpawning )
		{
			if ( InSetup() || IsInWaitingForPlayers() )
				return;

			CUtlVector<CTFPlayer *> players;
			CollectPlayers( &players, TF_TEAM_RED, true );
			CollectPlayers( &players, TF_TEAM_BLUE, true, true );

			int nNumHumans = 0;
			for ( int i=0; i<players.Count(); ++i )
			{
				if ( !players[i]->IsBot() )
					nNumHumans++;
			}

			if ( tf_halloween_bot_min_player_count.GetInt() > nNumHumans )
				return;
		}

		Vector vecSpawnLoc = vec3_origin;
		if ( !g_hControlPointMasters.IsEmpty() && g_hControlPointMasters[0] )
		{
			CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
			for ( int i=0; i<pMaster->GetNumPoints(); ++i )
			{
				CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
				if ( !pMaster->IsInRound( pPoint ) )
					continue;

				if ( ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) == TF_TEAM_BLUE )
					continue;

				if ( !TFGameRules()->TeamMayCapturePoint( TF_TEAM_BLUE, pPoint->GetPointIndex() ) )
					continue;

				vecSpawnLoc = pPoint->GetAbsOrigin();
				if ( eBossType > CHalloweenBaseBoss::HEADLESS_HATMAN )
				{
					pPoint->ForceOwner( TEAM_UNASSIGNED );

					if ( TFGameRules()->IsInKothMode() )
					{
						CTeamRoundTimer *pRedTimer = TFGameRules()->GetRedKothRoundTimer();
						CTeamRoundTimer *pBluTimer = TFGameRules()->GetBlueKothRoundTimer();
						
						if ( pRedTimer )
						{
							variant_t emptyVar;
							pRedTimer->AcceptInput( "Pause", NULL, NULL, emptyVar, 0 );
						}
						if ( pBluTimer )
						{
							variant_t emptyVar;
							pBluTimer->AcceptInput( "Pause", NULL, NULL, emptyVar, 0 );				
						}
					}
				}
			}
			
			CBaseEntity *pSpawnPoint = gEntList.FindEntityByClassname( NULL, "spawn_boss" );
			if ( pSpawnPoint )
			{
				vecSpawnLoc = pSpawnPoint->GetAbsOrigin();
			}		
		}
		else
		{
			CBaseEntity *pSpawnPoint = gEntList.FindEntityByClassname( NULL, "spawn_boss" );
			if ( pSpawnPoint )
			{
				vecSpawnLoc = pSpawnPoint->GetAbsOrigin();
			}
			else
			{
				CUtlVector<CNavArea *> candidates;
				for ( int i=0; i<TheNavAreas.Count(); ++i )
				{
					CNavArea *area = TheNavAreas[i];
					if ( area->GetSizeX() >= 100.0f && area->GetSizeY() >= 100.0f )
						candidates.AddToTail( area );
				}

				if ( !candidates.IsEmpty() )
				{
					CNavArea *area = candidates.Random();
					vecSpawnLoc = area->GetCenter();
				}
			}
		}

		if( !vecSpawnLoc.IsZero() )
		{
			CHalloweenBaseBoss::SpawnBossAtPos( eBossType, vecSpawnLoc );
			isBossForceSpawning = false;
			StartBossTimer( RandomFloat( fSpawnInterval - fSpawnVariation, fSpawnInterval + fSpawnVariation ) );
		}
	}
}

void CTFGameRules::SpawnZombieMob( void )
{
	if ( !tf_halloween_zombie_mob_enabled.GetBool() )
		return;

	if ( InSetup() || IsInWaitingForPlayers() )
	{
		m_mobSpawnTimer.Start( tf_halloween_zombie_mob_spawn_interval.GetFloat() );
		return;
	}
	
	if ( isZombieMobForceSpawning )
	{
		isZombieMobForceSpawning = false;
		m_mobSpawnTimer.Invalidate();
	}

	if ( m_nZombiesToSpawn > 0 && IsSpaceToSpawnHere( m_vecMobSpawnLocation ) )
	{
		if ( CZombie::SpawnAtPos( m_vecMobSpawnLocation, 0 ) )
			--m_nZombiesToSpawn;
	}

	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, TF_TEAM_RED, true );
	CollectPlayers( &players, TF_TEAM_BLUE, true, true );

	int nHumans = 0;
	for ( CTFPlayer *pPlayer : players )
	{
		if ( !pPlayer->IsBot() )
			++nHumans;
	}

	if ( nHumans <= 0 || !m_mobSpawnTimer.IsElapsed() )
		return;

	m_mobSpawnTimer.Start( tf_halloween_zombie_mob_spawn_interval.GetFloat() );

	CUtlVector<CTFNavArea *> validAreas;
	const float flSearchRange = 2000.0f;

	// populate a vector of valid spawn locations
	for ( CTFPlayer *pPlayer : players )
	{
		CUtlVector<CTFNavArea *> nearby;
		// ignore bots
		if ( pPlayer->IsBot() )
			continue;
		// are they on mesh?
		if ( pPlayer->GetLastKnownArea() == nullptr )
			continue;

		CollectSurroundingAreas( &nearby, pPlayer->GetLastKnownArea(), flSearchRange );
		for ( CTFNavArea *pArea : nearby )
		{
			if ( !pArea->IsValidForWanderingPopulation() )
				continue;

			if ( pArea->IsBlocked( TF_TEAM_RED ) || pArea->IsBlocked( TF_TEAM_BLUE ) )
				continue;

			validAreas.AddToTail( pArea );
		}
	}

	if ( validAreas.IsEmpty() )
		return;

	int iAttempts = 10;
	while( true )
	{
		CTFNavArea *pArea = validAreas.Random();
		m_vecMobSpawnLocation = pArea->GetCenter() + Vector( 0, 0, StepHeight );
		if ( IsSpaceToSpawnHere( m_vecMobSpawnLocation ) )
			break;

		if ( --iAttempts == 0 )
			return;
	}

	m_nZombiesToSpawn = tf_halloween_zombie_mob_spawn_count.GetInt();
}


	bool CTFGameRules::CheckCapsPerRound()
	{
		bool bCaps = false;
		if ( TFGameRules()->IsDMGamemode() && of_mctf_flag_caps_per_round.GetInt() > 0 )
			bCaps = true;
		else if ( !TFGameRules()->IsDMGamemode() && tf_flag_caps_per_round.GetInt() > 0 )
			bCaps = true;

		if ( bCaps )
		{
			int iMaxCaps = -1;
			CTFTeam *pMaxTeam = NULL;

			// check to see if any team has won a "round"
			int nTeamCount = TFTeamMgr()->GetTeamCount();
			for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
			{
				CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
				if ( !pTeam )
					continue;

				// we might have more than one team over the caps limit (if the server op lowered the limit)
				// so loop through to see who has the most among teams over the limit
				bool bOver = false;
				if ( TFGameRules()->IsDMGamemode() && pTeam->GetFlagCaptures() >= of_mctf_flag_caps_per_round.GetInt() )
					bOver = true;
				else if ( !TFGameRules()->IsDMGamemode() && pTeam->GetFlagCaptures() >= tf_flag_caps_per_round.GetInt() )
					bOver = true;

				if ( bOver )
				{
					if ( pTeam->GetFlagCaptures() > iMaxCaps )
					{
						iMaxCaps = pTeam->GetFlagCaptures();
						pMaxTeam = pTeam;
					}
				}
			}

			if ( iMaxCaps != -1 && pMaxTeam != NULL )
			{
				SetWinningTeam( pMaxTeam->GetTeamNumber(), WINREASON_FLAG_CAPTURE_LIMIT );
				return true;
			}
		}

		return false;
	}

	// ran everytime the score in DOM is changed
	void CTFGameRules::CheckDOMScores( int dom_score_red_old, int dom_score_blue_old )
	{
		// is red team's score higher than the score limit?
		if ( m_nDomScore_red >= m_nDomScore_limit )
		{
			// yes? send appropiate outputs to dom logic entity
			CTFLogicDOM *pDom = dynamic_cast< CTFLogicDOM* > ( gEntList.FindEntityByClassname( NULL, "of_logic_dom" ) );
			if ( pDom )
			{
				pDom->m_OnDomScoreHit_any.FireOutput( NULL, pDom );
				pDom->m_OnDomScoreHit_red.FireOutput( NULL, pDom );
			}

			// if win on score limit is enabled in the dom logic entity, set our winner
			if ( m_bDomWinOnLimit )
			{
				SetWinningTeam( TF_TEAM_RED, 0, true, true );
			}
			
			return;
		}
		// is blue team's score higher than the score limit?
		else if ( m_nDomScore_blue >= m_nDomScore_limit )
		{
			// yes? send appropiate outputs to dom logic entity
			CTFLogicDOM *pDom = dynamic_cast< CTFLogicDOM* > ( gEntList.FindEntityByClassname( NULL, "of_logic_dom" ) );
			if ( pDom )
			{
				pDom->m_OnDomScoreHit_any.FireOutput( NULL, pDom );
				pDom->m_OnDomScoreHit_blue.FireOutput( NULL, pDom );
			}

			// if win on score limit is enabled in the dom logic entity, set our winner
			if ( m_bDomWinOnLimit )
			{
				SetWinningTeam( TF_TEAM_BLUE, 0, true, true );
			}

			return;
		}

		//
		// announcer dialog
		//

		// don't create announcer dialog in Escort if domination is enabled there too
		if ( IsESCGamemode() )
			return;

		// if red's score is 90% or more of the score limit, then play appropiate dialog
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		// if we somehow end up with *less* score than last time and its less than 90%, then we want to allow the dialog to play again
		if ( ( m_nDomScore_red >= ( m_nDomScore_limit * 90 / 100 ) ) && !m_bDomRedThreshold )
		{
			m_bDomRedThreshold = true;

			BroadcastSound( TF_TEAM_RED, "DOM_FriendlyClose" );
			BroadcastSound( TF_TEAM_BLUE, "DOM_EnemyClose" );

			return;
		}
		else if ( ( ( m_nDomScore_red < dom_score_red_old ) && m_nDomScore_red < ( m_nDomScore_limit * 90 / 100 ) ) && m_bDomRedThreshold )
		{
			m_bDomRedThreshold = false;
		}

		// if blue's score is 90% or more of the score limit, then play appropiate dialog
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		// if we somehow end up with *less* score than last time and its less than 90%, then we want to allow the dialog to play again
		if ( ( m_nDomScore_blue >= ( m_nDomScore_limit * 90 / 100 ) ) && !m_bDomBlueThreshold )
		{
			m_bDomBlueThreshold = true;

			BroadcastSound( TF_TEAM_BLUE, "DOM_FriendlyClose" );
			BroadcastSound( TF_TEAM_RED, "DOM_EnemyClose" );

			return;
		}
		else if ( ( ( m_nDomScore_blue < dom_score_blue_old ) && m_nDomScore_blue < ( m_nDomScore_limit * 90 / 100 ) ) && m_bDomBlueThreshold )
		{
			m_bDomBlueThreshold = false;
		}

		// is red's score higher than blue's old score? we are in the lead, play announcer!
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		if ( ( m_nDomScore_red > dom_score_blue_old ) && !m_bDomRedLeadThreshold )
		{
			m_bDomBlueLeadThreshold = false;
			m_bDomRedLeadThreshold = true;

			BroadcastSound( TF_TEAM_RED, "DOM_FriendlyLead" );
			BroadcastSound( TF_TEAM_BLUE, "DOM_EnemyLead" );

			return;
		}

		// is blue's score higher than blue's old score? we are in the lead, play announcer!
		// however we don't want this dialog to play everytime! therefore the threshold bool is checked
		if ( ( m_nDomScore_blue > dom_score_red_old ) && !m_bDomBlueLeadThreshold )
		{
			m_bDomRedLeadThreshold = false;
			m_bDomBlueLeadThreshold = true;

			BroadcastSound( TF_TEAM_BLUE, "DOM_FriendlyLead" );
			BroadcastSound( TF_TEAM_RED, "DOM_EnemyLead" );

			return;
		}

		// play countdown dialog if we are within 10 points of reaching the limit
		if ( m_nDomScore_red > dom_score_red_old )
		{
			int counter;

			counter = m_nDomScore_limit - m_nDomScore_red;

			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				switch ( counter )
				{
				case 1:
					BroadcastSound( i, "RoundEnds1seconds" );
					break;
				case 2:
					BroadcastSound( i, "RoundEnds2seconds" );
					break;
				case 3:
					BroadcastSound( i, "RoundEnds3seconds" );
					break;
				case 4:
					BroadcastSound( i, "RoundEnds4seconds" );
					break;
				case 5:
					BroadcastSound( i, "RoundEnds5seconds" );
					break;
				case 6:
					BroadcastSound( i, "RoundEnds6seconds" );
					break;
				case 7:
					BroadcastSound( i, "RoundEnds7seconds" );
					break;
				case 8:
					BroadcastSound( i, "RoundEnds8seconds" );
					break;
				case 9:
					BroadcastSound( i, "RoundEnds9seconds" );
					break;
				default:
					break;
				}
			}
		}
		else if ( m_nDomScore_blue > dom_score_blue_old )
		{
			int counter;

			counter = m_nDomScore_limit - m_nDomScore_blue;

			for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
			{
				switch ( counter )
				{
				case 1:
					BroadcastSound( i, "RoundEnds1seconds" );
					break;
				case 2:
					BroadcastSound( i, "RoundEnds2seconds" );
					break;
				case 3:
					BroadcastSound( i, "RoundEnds3seconds" );
					break;
				case 4:
					BroadcastSound( i, "RoundEnds4seconds" );
					break;
				case 5:
					BroadcastSound( i, "RoundEnds5seconds" );
					break;
				case 6:
					BroadcastSound( i, "RoundEnds6seconds" );
					break;
				case 7:
					BroadcastSound( i, "RoundEnds7seconds" );
					break;
				case 8:
					BroadcastSound( i, "RoundEnds8seconds" );
					break;
				case 9:
					BroadcastSound( i, "RoundEnds9seconds" );
					break;
				default:
					break;
				}
			}
		}
	}

	bool CTFGameRules::CheckWinLimit()
	{
		if ( mp_winlimit.GetInt() != 0 )
		{
			bool bWinner = false;

			if ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"BLUE\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"RED\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY )->GetScore() >= mp_winlimit.GetInt() )
			{
				UTIL_LogPrintf( "Team \"Mercenary\" triggered \"Intermission_Win_Limit\"\n" );
				bWinner = true;
			}
			
			if ( bWinner )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "tf_game_over" );
				if ( event )
				{
					event->SetString( "reason", "Reached Win Limit" );
					gameeventmanager->FireEvent( event );
				}

				GoToIntermission();
				return true;
			}

			bool bVote = false;

			// 1 win left? allow voting
			if ( TFTeamMgr()->GetTeam( TF_TEAM_BLUE )->GetScore() == ( mp_winlimit.GetInt() - 1 ) )
			{
				bVote = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_RED )->GetScore() == ( mp_winlimit.GetInt() - 1 ) )
			{
				bVote = true;
			}
			else if ( TFTeamMgr()->GetTeam( TF_TEAM_MERCENARY )->GetScore() == ( mp_winlimit.GetInt() - 1 ) )
			{
				bVote = true;
			}

			if ( bVote && !m_bStartedVote )
			{
				DevMsg( "VoteController: One round remaining until winlimit, begin nextlevel voting... \n" );
				m_bStartedVote = true;
				//engine->ServerCommand( "callvote nextlevel" );
				char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
				szEmptyDetails[0] = '\0';
				g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
			}
		}

		return false;
	}

	bool CTFGameRules::CheckMaxRounds( bool bAllowEnd /*= true*/ )
	{
		if ( mp_maxrounds.GetInt() > 0 && IsInPreMatch() == false )
		{
			if ( m_nRoundsPlayed >= mp_maxrounds.GetInt() )
			{
				if ( bAllowEnd )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_game_over" );
					if ( event )
					{
						event->SetString( "reason", "Reached Round Limit" );
						gameeventmanager->FireEvent( event );
					}

					GoToIntermission();
				}
				return true;
			}

			if ( m_nRoundsPlayed == ( mp_maxrounds.GetInt() - 1  ) && !m_bStartedVote )
			{
				DevMsg( "VoteController: One round remaining until maxrounds, begin nextlevel voting... \n" );
				m_bStartedVote = true;
				//engine->ServerCommand( "callvote nextlevel" );
				char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
				szEmptyDetails[0] = '\0';
				g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
			}
		}

		return false;
	}

	bool CTFGameRules::IsInPreMatch() const
	{
		// TFTODO    return (cb_prematch_time > gpGlobals->time)
		return false;
	}

	float CTFGameRules::GetPreMatchEndTime() const
	{
		//TFTODO: implement this.
		return gpGlobals->curtime;
	}

	void CTFGameRules::GoToIntermission( void )
	{
		CTF_GameStats.Event_GameEnd();

		BaseClass::GoToIntermission();
	}

	bool CTFGameRules::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info)
	{
		// guard against NULL pointers if players disconnect
		if ( !pPlayer || !pAttacker )
			return false;

		// if pAttacker is an object, we can only do damage if pPlayer is our builder
		if ( pAttacker->IsBaseObject() )
		{
			CBaseObject *pObj = ( CBaseObject *)pAttacker;

			if ( pObj->GetBuilder() == pPlayer || pPlayer->GetTeamNumber() != pObj->GetTeamNumber() )
			{
				// Builder and enemies
				return true;
			}
			else
			{
				// Teammates of the builder
				return false;
			}
		}

		return BaseClass::FPlayerCanTakeDamage(pPlayer, pAttacker, info);
	}

Vector DropToGround( 
	CBaseEntity *pMainEnt, 
	const Vector &vPos, 
	const Vector &vMins, 
	const Vector &vMaxs )
{
	trace_t trace;
	UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
	return trace.endpos;
}


void TestSpawnPointType( const char *pEntClassName )
{
	// Find the next spawn spot.
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, pEntClassName );

	while( pSpot )
	{
		// trace a box here
		Vector vTestMins = pSpot->GetAbsOrigin() + VEC_HULL_MIN;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + VEC_HULL_MAX;

		if ( UTIL_IsSpaceEmpty( pSpot, vTestMins, vTestMaxs ) )
		{
			// the successful spawn point's location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 100, 60 );

			// drop down to ground
			Vector GroundPos = DropToGround( NULL, pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

			// the location the player will spawn at
			NDebugOverlay::Box( GroundPos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 0, 255, 100, 60 );

			// draw the spawn angles
			QAngle spotAngles = pSpot->GetLocalAngles();
			Vector vecForward;
			AngleVectors( spotAngles, &vecForward );
			NDebugOverlay::HorzArrow( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin() + vecForward * 32, 10, 255, 0, 0, 255, true, 60 );
		}
		else
		{
			// failed spawn point location
			NDebugOverlay::Box( pSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 100, 60 );
		}

		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pEntClassName );
	}
}

// -------------------------------------------------------------------------------- //

void TestSpawns()
{
	TestSpawnPointType( "info_player_teamspawn" );
}
ConCommand cc_TestSpawns( "map_showspawnpoints", TestSpawns, "Dev - test the spawn points, draws for 60 seconds", FCVAR_CHEAT );

// -------------------------------------------------------------------------------- //

void cc_ShowRespawnTimes()
{
	CTFGameRules *pRules = TFGameRules();
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );

	if ( pRules && pPlayer )
	{
		float flRedMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_RED] : mp_respawnwavetime.GetFloat() );
		float flRedScalar = pRules->GetRespawnTimeScalar( TF_TEAM_RED );
		float flNextRedRespawn = pRules->GetNextRespawnWave( TF_TEAM_RED, NULL ) - gpGlobals->curtime;

		float flBlueMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_BLUE] : mp_respawnwavetime.GetFloat() );
		float flBlueScalar = pRules->GetRespawnTimeScalar( TF_TEAM_BLUE );
		float flNextBlueRespawn = pRules->GetNextRespawnWave( TF_TEAM_BLUE, NULL ) - gpGlobals->curtime;

		float flMercenaryMin = ( pRules->m_TeamRespawnWaveTimes[TF_TEAM_MERCENARY] >= 0 ? pRules->m_TeamRespawnWaveTimes[TF_TEAM_MERCENARY] : mp_respawnwavetime.GetFloat() );
		float flMercenaryScalar = pRules->GetRespawnTimeScalar( TF_TEAM_MERCENARY );
		float flNextMercenaryRespawn = pRules->GetNextRespawnWave( TF_TEAM_MERCENARY, NULL ) - gpGlobals->curtime;	
	
		char tempRed[128];
		Q_snprintf( tempRed, sizeof( tempRed ),   "Red:  Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flRedMin, flRedScalar, flNextRedRespawn );

		char tempBlue[128];
		Q_snprintf( tempBlue, sizeof( tempBlue ), "Blue: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flBlueMin, flBlueScalar, flNextBlueRespawn );
		
		char tempMercenary[128];
		Q_snprintf( tempMercenary, sizeof( tempMercenary ), "Mercenary: Min Spawn %2.2f, Scalar %2.2f, Next Spawn In: %.2f\n", flMercenaryMin, flMercenaryScalar, flNextMercenaryRespawn );

		ClientPrint( pPlayer, HUD_PRINTTALK, tempRed );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempBlue );
		ClientPrint( pPlayer, HUD_PRINTTALK, tempMercenary );
	}
}

ConCommand mp_showrespawntimes( "mp_showrespawntimes", cc_ShowRespawnTimes, "Show the min respawn times for the teams" );

// -------------------------------------------------------------------------------- //

CBaseEntity *CTFGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	if ( of_navmesh_spawns.GetBool() && IsDMGamemode() )
	{
		if ( TheNavMesh->IsLoaded() )
		{
			int iCount = TheNavAreas.Count();
			int iArea = RandomInt( 0, iCount - 1 );
			int iPatience;
			for (iPatience = iCount; iPatience; iPatience--)
			{
				if ( TheNavAreas[iArea]->IsFlat() )
				{
					for ( int iAreaPatience = 4; iAreaPatience; iAreaPatience-- )
					{
						Vector vSpawn = TheNavAreas[iArea]->GetRandomPoint() + Vector( 0, 0, 32 );
						trace_t tracehull;
						UTIL_TraceHull( vSpawn, vSpawn, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tracehull );
						if ( !tracehull.DidHit() )
						{
							//Take the angles pointing to the furthest wall, forward, backwards, right or left
							QAngle qEyeAngles(0, 0, 0);
							float curdistance = 0;
							for ( int iLine = 1; iLine <= 4; iLine++ )
							{
								trace_t traceline;
								Vector vDir;
								QAngle qDirAngles( 0, 90 * iLine, 0 );
								AngleVectors( qDirAngles, &vDir );
								UTIL_TraceLine( vSpawn, vSpawn + vDir * MAX_COORD_RANGE, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &traceline );
								if ( traceline.DidHit() )
								{
									Vector vRelative = vSpawn - traceline.endpos;
									float distance = VectorNormalize( vRelative );
									if ( distance > curdistance )
									{
										curdistance = distance;
										qEyeAngles = qDirAngles;
									}
								}
							}
							// SUCCESS
							pPlayer->SetLocalOrigin( vSpawn );
							pPlayer->SetAbsVelocity( vec3_origin );
							pPlayer->SetLocalAngles( qEyeAngles );
							pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
							pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
							pPlayer->SnapEyeAngles( qEyeAngles );

							return NULL;
						}
					}
				}
				if ( ++iArea >= iCount ) iArea -= iCount;
			}
			DevMsg( "No valid nav_area found in nav_mesh for of_navmesh_spawns, nav mesh has %d areas\n", iCount );
		}
		else
		{
			DevMsg( "No navigation mesh avalaible with of_navmesh_spawns enabled\n" );
		}
	}

	// get valid spawn point
	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();

	if ( pSpawnSpot )
	{
		// drop down to ground
		Vector GroundPos = DropToGround( pPlayer, pSpawnSpot->GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX );

		// Move the player to the place it said.
		pPlayer->SetLocalOrigin( GroundPos + Vector(0,0,1) );
		pPlayer->SetAbsVelocity( vec3_origin );
		pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
		pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
		pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
		pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );
	}

	return pSpawnSpot;
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if the player is on the correct team and whether or
//          not the spawn point is available.
//-----------------------------------------------------------------------------
bool CTFGameRules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers )
{
	// WTFWTF 

    // zombies can spawn everywhere
	if ( TFGameRules()->IsInfGamemode() && pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
	{
	}
	else if ( pSpot->GetTeamNumber() != pPlayer->GetTeamNumber() )
	{
		// some fools assigned teampoints to spectator team so i have to check this too
		if ( ( pSpot->GetTeamNumber() < FIRST_GAME_TEAM || pSpot->GetTeamNumber() == TF_TEAM_MERCENARY ) && ( TFGameRules()->IsFreeRoam() && TFGameRules()->IsDMGamemode() ) )
		{
		}
		else
		{
			// Hack: DM maps are supported in infection, but the spawnpoints have no teams assigned
			// Therefore just allow every spawnpoint if the map is prefixed with dm_ ...
			if ( ( TFGameRules()->IsInfGamemode() && TFGameRules()->IsFreeRoam() ) )
			{
			}
			else
			{
				return false;
			}
		}
	}

	if ( !pSpot->IsTriggered( pPlayer ) )
		return false;

	CTFTeamSpawn *pCTFSpawn = dynamic_cast<CTFTeamSpawn*>( pSpot );
	if ( pCTFSpawn )
	{
		if ( pCTFSpawn->IsDisabled() )
			return false;

		// live tf2 uses spawnpoints for the comp end screen, which are given the unassigned team (and unassigned spawnpoints are regarded as valid here)
		// therefore, avoid spawnpoints that are flagged as Loser or Winner for the comp end screen
		if ( pCTFSpawn->GetMatchSummary() >= 1 )
			return false;
		
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		int iClassIndex = pTFPlayer->GetPlayerClass()->GetClassIndex();

		switch (iClassIndex)
		{
			case TF_CLASS_UNDEFINED:
				Warning( "CTFGameRules::IsSpawnPointValid: Player has undefined class" );
					return false;
				break;
			case TF_CLASS_SCOUT:
				if ( !pCTFSpawn->AllowScout() )
					return false;
				break;
			case TF_CLASS_SNIPER:
				if ( !pCTFSpawn->AllowSniper() )
					return false;
				break;
			case TF_CLASS_SOLDIER:
				if ( !pCTFSpawn->AllowSoldier() )
					return false;
				break;
			case TF_CLASS_DEMOMAN:
				if ( !pCTFSpawn->AllowDemoman() )
					return false;
				break;
			case TF_CLASS_MEDIC:
				if ( !pCTFSpawn->AllowMedic() )
					return false;
				break;
			case TF_CLASS_HEAVYWEAPONS:
				if ( !pCTFSpawn->AllowHeavyweapons() )
					return false;
				break;
			case TF_CLASS_PYRO:
				if ( !pCTFSpawn->AllowPyro() )
					return false;
				break;
			case TF_CLASS_SPY:
				if ( !pCTFSpawn->AllowSpy() )
					return false;
				break;
			case TF_CLASS_ENGINEER:
				if ( !pCTFSpawn->AllowEngineer() )
					return false;
				break;
			case TF_CLASS_CIVILIAN:
				if ( !pCTFSpawn->AllowCivilian() )
				{
					// official tf2 maps will have spawnflags missing for Civilian, therefore I do a cheeky solution
					// get every single spawnpoint and check if any of them has a Civilian spawnflag before the round starts
					// if there is, then its likely a custom map for our mod and therefore we can just keep looping spawnpoints until we hit that one
					// otherwise, force ourselves to spawn here
					if ( TFGameRules()->HasCivilianSpawns() )
						return false;
				}
				break;
			case TF_CLASS_JUGGERNAUT:
				if ( !pCTFSpawn->AllowJuggernaut() )
				{
					if ( TFGameRules()->HasJuggernautSpawns() )
						return false;
				}
				break;
		}
	}

	Vector mins = GetViewVectors()->m_vHullMin;
	Vector maxs = GetViewVectors()->m_vHullMax;

	if ( !bIgnorePlayers )
	{
		Vector vTestMins = pSpot->GetAbsOrigin() + mins;
		Vector vTestMaxs = pSpot->GetAbsOrigin() + maxs;
		return UTIL_IsSpaceEmpty( pPlayer, vTestMins, vTestMaxs );
	}

	trace_t trace;
	UTIL_TraceHull( pSpot->GetAbsOrigin(), pSpot->GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	return ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
}

Vector CTFGameRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CTFGameRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

float CTFGameRules::FlItemRespawnTime( CItem *pItem )
{
	return ITEM_RESPAWN_TIME;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == true )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_Spec";
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "TF_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "TF_Chat_Team";
				}
			}
		}
	}
	// everyone
	else
	{	
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "TF_Chat_AllSpec";	
		}
		else
		{
			if ( pPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN )
			{
				pszFormat = "TF_Chat_AllDead";
			}
			else
			{
				pszFormat = "TF_Chat_All";	
			}
		}
	}

	return pszFormat;
}

VoiceCommandMenuItem_t *CTFGameRules::VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem )
{
	VoiceCommandMenuItem_t *pItem = BaseClass::VoiceCommand( pPlayer, iMenu, iItem );

	if ( pItem )
	{
		int iActivity = ActivityList_IndexForName( pItem->m_szGestureActivity );

		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		if ( iActivity != ACT_INVALID && pTFPlayer )
		{
			pTFPlayer->DoAnimationEvent( PLAYERANIMEVENT_VOICE_COMMAND_GESTURE, iActivity );

			if ( iMenu == 0 && iItem == 0 )
				pTFPlayer->m_lastCalledMedic.Start();
		}
	}

	return pItem;
}

//-----------------------------------------------------------------------------
// Purpose: Actually change a player's name.  
//-----------------------------------------------------------------------------
void CTFGameRules::ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName )
{
	const char *pszOldName = pPlayer->GetPlayerName();

	CReliableBroadcastRecipientFilter filter;
	UTIL_SayText2Filter( filter, pPlayer, false, "#TF_Name_Change", pszOldName, pszNewName );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
	if ( event )
	{
		event->SetInt( "userid", pPlayer->GetUserID() );
		event->SetString( "oldname", pszOldName );
		event->SetString( "newname", pszNewName );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SetPlayerName( pszNewName );

	pPlayer->m_flNextNameChangeTime = gpGlobals->curtime + 10.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	const char *pszOldName = pPlayer->GetPlayerName();

	CTFPlayer *pTFPlayer = (CTFPlayer*)pPlayer;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strncmp( pszOldName, pszName, MAX_PLAYER_NAME_LENGTH-1 ) )		
	{
		if ( pTFPlayer->m_flNextNameChangeTime < gpGlobals->curtime )
		{
			ChangePlayerName( pTFPlayer, pszName );
		}
		else
		{
			// no change allowed, force engine to use old name again
			engine->ClientCommand( pPlayer->edict(), "name \"%s\"", pszOldName );

			// tell client that he hit the name change time limit
			ClientPrint( pTFPlayer, HUD_PRINTTALK, "#Name_change_limit_exceeded" );
		}
	}

	// keep track of their hud_classautokill value
	int nClassAutoKill = Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "hud_classautokill" ) );
	pTFPlayer->SetHudClassAutoKill( nClassAutoKill > 0 ? true : false );

	// keep track of their tf_medigun_autoheal value
	pTFPlayer->SetMedigunAutoHeal( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_medigun_autoheal" ) ) > 0 );
	
	// keep track of their of_autoreload value
	pTFPlayer->SetAutoReload( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "of_autoreload" ) ) > 0 );
	
	// keep track of their cl_autorezoom value
	pTFPlayer->SetAutoRezoom( Q_atoi( engine->GetClientConVarValue( pPlayer->entindex(), "cl_autorezoom" ) ) > 0 );

	const char *pszFov = engine->GetClientConVarValue( pPlayer->entindex(), "fov_desired" );
	int iFov = atoi(pszFov);
	iFov = clamp( iFov, 50, 130 );
	pTFPlayer->SetDefaultFOV( iFov );

	pTFPlayer->m_vecViewmodelOffset.SetX( atof( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_offset_x" ) ) );
	pTFPlayer->m_vecViewmodelOffset.SetY( atof( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_offset_y" ) ) );
	pTFPlayer->m_vecViewmodelOffset.SetZ( atof( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_offset_z" ) ) );
	
	pTFPlayer->m_vecViewmodelAngle.SetX( atof( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_angle_x" ) ) );
	pTFPlayer->m_vecViewmodelAngle.SetY( atof( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_angle_y" ) ) );
	pTFPlayer->m_vecViewmodelAngle.SetZ( atof( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_angle_z" ) ) );

	pTFPlayer->m_bCentered  = atoi( engine->GetClientConVarValue( pPlayer->entindex(), "viewmodel_centered" ) ) == 1;
	pTFPlayer->m_bMinimized = atoi( engine->GetClientConVarValue( pPlayer->entindex(), "tf_use_min_viewmodels" ) ) == 1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameRules::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	BaseClass::ClientCommandKeyValues( pEntity, pKeyValues );

	CTFPlayer *pPlayer = ToTFPlayer( CBaseEntity::Instance( pEntity ) );
	if ( !pPlayer )
		return;

	if ( FStrEq( pKeyValues->GetName(), "FreezeCamTaunt" ) )
	{
		int iAchieverIndex = pKeyValues->GetInt( "achiever" );
		CTFPlayer *pAchiever = ToTFPlayer( UTIL_PlayerByIndex( iAchieverIndex ) );

		if ( pAchiever && pAchiever != pPlayer )
		{
			int iClass = pAchiever->GetPlayerClass()->GetClassIndex();

			if ( g_TauntCamAchievements[iClass] != 0 )
			{
				pAchiever->AwardAchievement( g_TauntCamAchievements[iClass] );
			}
		}
	}
}

void CTFGameRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	BaseClass::GetTaggedConVarList( pCvarTagList );
	
		// of_mutators
	KeyValues *pKeyValues = new KeyValues( "of_mutator" );
	pKeyValues->SetString( "convar", "of_mutator" );
	pKeyValues->SetString( "tag", "mutator" );

	pCvarTagList->AddSubKey( pKeyValues );

	/*
		// of_instagib
	KeyValues *pKeyValues = new KeyValues( "of_instagib" );
	pKeyValues->SetString( "convar", "of_instagib" );
	pKeyValues->SetString( "tag", "instagib" );

	pCvarTagList->AddSubKey( pKeyValues );

		// of_clanarena
	pKeyValues = new KeyValues( "of_clanarena" );
	pKeyValues->SetString( "convar", "of_clanarena" );
	pKeyValues->SetString( "tag", "clanarena" );

	pCvarTagList->AddSubKey( pKeyValues );

		// of_gungame
	pKeyValues = new KeyValues( "of_gungame" );
	pKeyValues->SetString( "convar", "of_gungame" );
	pKeyValues->SetString( "tag", "gungame" );

	pCvarTagList->AddSubKey( pKeyValues );
	*/
	
		// of_infiniteammo
	pKeyValues = new KeyValues( "of_infiniteammo" );
	pKeyValues->SetString( "convar", "of_infiniteammo" );
	pKeyValues->SetString( "tag", "infiniteammo" );

	pCvarTagList->AddSubKey( pKeyValues );
	
		// of_noreload
	pKeyValues = new KeyValues( "of_noreload" );
	pKeyValues->SetString( "convar", "of_noreload" );
	pKeyValues->SetString( "tag", "noreload" );

	pCvarTagList->AddSubKey( pKeyValues );
	
		// of_allow_special_teams 
	pKeyValues = new KeyValues( "of_allow_special_teams" );
	pKeyValues->SetString( "convar", "of_allow_special_teams" );
	pKeyValues->SetString( "tag", "specialteams" );

	pCvarTagList->AddSubKey( pKeyValues );
	
		// of_forceclass 
	pKeyValues = new KeyValues( "of_forceclass" );
	pKeyValues->SetString( "convar", "of_forceclass" );
	pKeyValues->SetString( "tag", "allclassesallowed" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
		// mp_disable_respawn_times  
	pKeyValues = new KeyValues( "mp_disable_respawn_times" );
	pKeyValues->SetString( "convar", "mp_disable_respawn_times" );
	pKeyValues->SetString( "tag", "norespawntimes" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
		// tf_weapon_criticals   
	pKeyValues = new KeyValues( "tf_weapon_criticals" );
	pKeyValues->SetString( "convar", "tf_weapon_criticals" );
	pKeyValues->SetString( "tag", "critsenabled" );

	pCvarTagList->AddSubKey( pKeyValues );		
	
		// tf_damage_disablespread   
	pKeyValues = new KeyValues( "tf_damage_disablespread" );
	pKeyValues->SetString( "convar", "tf_damage_disablespread" );
	pKeyValues->SetString( "tag", "nodamagespread" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
		// tf_use_fixed_weaponspreads   
	pKeyValues = new KeyValues( "tf_use_fixed_weaponspreads" );
	pKeyValues->SetString( "convar", "tf_use_fixed_weaponspreads" );
	pKeyValues->SetString( "tag", "norandomspread" );

	pCvarTagList->AddSubKey( pKeyValues );		

		// tf_birthday    
	pKeyValues = new KeyValues( "tf_birthday" );
	pKeyValues->SetString( "convar", "tf_birthday" );
	pKeyValues->SetString( "tag", "birthday" );

	pCvarTagList->AddSubKey( pKeyValues );	
	
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified player can carry any more of the ammo type
//-----------------------------------------------------------------------------
bool CTFGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
{
	if ( iAmmoIndex > -1 )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		if ( pTFPlayer )
		{
			// Get the player class data - contains ammo counts for this class.
			TFPlayerClassData_t *pData = pTFPlayer->GetPlayerClass()->GetData();
			if ( pData )
			{
				// Get the max carrying capacity for this ammo
				int iMaxCarry = pData->m_aAmmoMax[iAmmoIndex];

				// Does the player have room for more of this type of ammo?
				if ( pTFPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
				{
					return true;
				}
			}
		}
		// fixme experimental: this should fix npc ammo but im not sure
		else
		{
			return BaseClass::CanHaveAmmo( pPlayer, iAmmoIndex );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer *pScorer = ToBaseMultiplayerPlayer( GetDeathScorer( pKiller, pInflictor, pVictim ) );
	CBaseEntity *pAssister = NULL;
	CBaseObject *pObject = NULL;

	// if inflictor or killer is a base object, tell them that they got a kill
	// ( depends if a sentry rocket got the kill, sentry may be inflictor or killer )
	if ( pInflictor )
	{
		if ( pInflictor->IsBaseObject() )
		{
			pObject = dynamic_cast<CBaseObject *>( pInflictor );
		}
		else 
		{
			CBaseEntity *pInflictorOwner = pInflictor->GetOwnerEntity();
			if ( pInflictorOwner && pInflictorOwner->IsBaseObject() )
			{
				pObject = dynamic_cast<CBaseObject *>( pInflictorOwner );
			}
		}
		
	}
	else if( pKiller && pKiller->IsBaseObject() )
	{
		pObject = dynamic_cast<CBaseObject *>( pKiller );
	}

	if ( pObject )
	{
		pObject->IncrementKills();
		pInflictor = pObject;

		if ( pObject->ObjectType() == OBJ_SENTRYGUN )
		{
			CTFPlayer *pOwner = pObject->GetOwner();
			if ( pOwner )
			{
				int iKills = pObject->GetKills();

				// keep track of max kills per a single sentry gun in the player object
				if ( pOwner->GetMaxSentryKills() < iKills )
				{
					pOwner->SetMaxSentryKills( iKills );
					CTF_GameStats.Event_MaxSentryKills( pOwner, iKills );
				}

				// if we just got 10 kills with one sentry, tell the owner's client, which will award achievement if it doesn't have it already
				if ( iKills == 10 )
				{
					pOwner->AwardAchievement( ACHIEVEMENT_TF_GET_TURRETKILLS );
				}
			}
		}
	}

	// if not killed by  suicide or killed by world, see if the scorer had an assister, and if so give the assister credit
	if ( ( pVictim != pScorer ) && pKiller )
	{
		pAssister = GetAssister( pVictim, pScorer, pInflictor );
	}	

	//find the area the player is in and see if his death causes a block
	CTriggerAreaCapture *pArea = dynamic_cast<CTriggerAreaCapture *>(gEntList.FindEntityByClassname( NULL, "trigger_capture_area" ) );
	while( pArea )
	{
		if ( pArea->CheckIfDeathCausesBlock( ToBaseMultiplayerPlayer(pVictim), pScorer ) )
			break;

		pArea = dynamic_cast<CTriggerAreaCapture *>( gEntList.FindEntityByClassname( pArea, "trigger_capture_area" ) );
	}

	CTFPlayer *pTFPlayerAssister = ToTFPlayer( pAssister );

	// determine if this kill affected a nemesis relationship
	int iDeathFlags = 0;
	CTFPlayer *pTFPlayerVictim = ToTFPlayer( pVictim );
	CTFPlayer *pTFPlayerScorer = ToTFPlayer( pScorer );
	if ( pScorer )
	{	
		CalcDominationAndRevenge( pTFPlayerScorer, pTFPlayerVictim, false, &iDeathFlags );

		if ( pTFPlayerAssister )
		{
			CalcDominationAndRevenge( pTFPlayerAssister, pTFPlayerVictim, true, &iDeathFlags );
		}

		if ( IsTeamplay() && IsTDMGamemode() && !pTFPlayerScorer->InSameTeam(pTFPlayerVictim) && !DontCountKills() )
		{
			TFTeamMgr()->AddTeamScore( pTFPlayerScorer->GetTeamNumber(), 1 );
		}
	}
	pTFPlayerVictim->SetDeathFlags( iDeathFlags );	

	if ( pTFPlayerAssister )
	{
		CTF_GameStats.Event_AssistKill( pTFPlayerAssister, pVictim );
	}

	CBaseObject *pObjectAssister = dynamic_cast<CBaseObject *>( pInflictor );

	if ( pTFPlayerAssister && pObjectAssister )
	{
		pObjectAssister->IncrementAssists();
	}

	BaseClass::PlayerKilled( pVictim, info );
	
	if ( IsDMGamemode() && !DontCountKills() )
	{
		int iFragLimit = fraglimit.GetInt();
		
		if ( iFragLimit > 0 ) 
		{
			if ( IsTDMGamemode() )
			{
				if ( m_nCurrFrags < TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore() )
				{
					m_nCurrFrags = TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore();
					FireTargets( "game_fragincrease", TFTeamMgr()->GetTeam(TF_TEAM_RED), TFTeamMgr()->GetTeam(TF_TEAM_RED), USE_TOGGLE, 0 );
				}
				else if ( m_nCurrFrags < TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore() )
				{
					m_nCurrFrags = TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore();
					FireTargets( "game_fragincrease", TFTeamMgr()->GetTeam(TF_TEAM_BLUE), TFTeamMgr()->GetTeam(TF_TEAM_BLUE), USE_TOGGLE, 0 );
				}						
				if ( TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore() >= iFragLimit || TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore() >= iFragLimit )
				{
					SendTeamScoresEvent();
					GoToIntermission();
				}
				// one of our teams is at 80% of the fragcount, start voting for next map
				if ( !m_bStartedVote && ( TFTeamMgr()->GetTeam(TF_TEAM_RED)->GetScore() >= ( (float)iFragLimit * 0.8 ) ||
					( TFTeamMgr()->GetTeam(TF_TEAM_BLUE)->GetScore() >= ( (float)iFragLimit * 0.8 ) ) ) 
					&& !TFGameRules()->IsInWaitingForPlayers() )
				{
					DevMsg( "VoteController: Team fraglimit is 80%%, begin nextlevel voting... \n" );
					m_bStartedVote = true;
					//engine->ServerCommand( "callvote nextlevel" );
					char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
					szEmptyDetails[0] = '\0';
					g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
				}
			}
			else
			{
				if ( pTFPlayerScorer )
				{
					if ( m_nCurrFrags < pTFPlayerScorer->FragCount() )
					{
						m_nCurrFrags = pTFPlayerScorer->FragCount();
						FireTargets( "game_fragincrease", pTFPlayerScorer, pTFPlayerScorer, USE_TOGGLE, 0 );
						if( fraglimit.GetInt() - m_nCurrFrags == 10 || fraglimit.GetInt() - m_nCurrFrags <= 5 )
							BroadcastSound( TEAM_UNASSIGNED, UTIL_VarArgs( "FragsLeft%d", fraglimit.GetInt() - m_nCurrFrags ) );
					}

					if ( pTFPlayerScorer->FragCount() >= iFragLimit )
					{
						SetWinningTeam( TF_TEAM_MERCENARY, WINREASON_POINTLIMIT, true, true, false);
					}

					// one of our players is at 80% of the fragcount, start voting for next map
					if ( !m_bStartedVote && ( pTFPlayerScorer->FragCount() >= ( (float)iFragLimit * 0.8 ) ) && !TFGameRules()->IsInWaitingForPlayers() )
					{
						DevMsg( "VoteController: Player fraglimit is 80%%, begin nextlevel voting... \n" );
						m_bStartedVote = true;
						//engine->ServerCommand( "callvote nextlevel" );
						char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
						szEmptyDetails[0] = '\0';
						g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
					}
				}
			}
		}
	}

	if ( IsGGGamemode() )
	{			
		if ( pTFPlayerScorer && pTFPlayerScorer->GGLevel() == m_iMaxLevel )
		{
			SetWinningTeam( TF_TEAM_MERCENARY, WINREASON_POINTLIMIT, true, true, false);
		}

		if ( !m_bStartedVote && ( pTFPlayerScorer->GGLevel() >= ( (float)m_iMaxLevel * 0.8 ) ) && !TFGameRules()->IsInWaitingForPlayers() )
		{
			DevMsg( "VoteController: GGLevel is 80%%, begin nextlevel voting... \n" );
			m_bStartedVote = true;
			//engine->ServerCommand( "callvote nextlevel" );
			char szEmptyDetails[MAX_VOTE_DETAILS_LENGTH];
			szEmptyDetails[0] = '\0';
			g_voteController->CreateVote( DEDICATED_SERVER, "nextlevel", szEmptyDetails );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Determines if attacker and victim have gotten domination or revenge
//-----------------------------------------------------------------------------
void CTFGameRules::CalcDominationAndRevenge( CTFPlayer *pAttacker, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags )
{
	if ( !of_dominations.GetBool() )
		return;

	if ( TFGameRules()->IsInfGamemode() )
		return;

	PlayerStats_t *pStatsVictim = CTF_GameStats.FindPlayerStats( pVictim );

	// calculate # of unanswered kills between killer & victim - add 1 to include current kill
	int iKillsUnanswered = pStatsVictim->statsKills.iNumKilledByUnanswered[pAttacker->entindex()] + 1;		
	if ( TF_KILLS_DOMINATION == iKillsUnanswered )
	{			
		// this is the Nth unanswered kill between killer and victim, killer is now dominating victim
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_DOMINATION : TF_DEATH_DOMINATION );
		// set victim to be dominated by killer
		pAttacker->m_Shared.SetPlayerDominated( pVictim, true );
		// record stats
		CTF_GameStats.Event_PlayerDominatedOther( pAttacker );
	}
	else if ( pVictim->m_Shared.IsPlayerDominated( pAttacker->entindex() ) )
	{
		// the killer killed someone who was dominating him, gains revenge
		*piDeathFlags |= ( bIsAssist ? TF_DEATH_ASSISTER_REVENGE : TF_DEATH_REVENGE );
		// set victim to no longer be dominating the killer
		pVictim->m_Shared.SetPlayerDominated( pAttacker, false );
		// record stats
		CTF_GameStats.Event_PlayerRevenge( pAttacker );
	}

}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CTFGameRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create( "tf_player_manager", vec3_origin, vec3_angle );

	// Create the objective resource
	g_pObjectiveResource = (CTFObjectiveResource *)CBaseEntity::Create( "tf_objective_resource", vec3_origin, vec3_angle );

	Assert( g_pObjectiveResource );

	// Create the entity that will send our data to the client.
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_gamerules" );

	if ( !pEnt )
	{
		pEnt = CBaseEntity::Create( "tf_gamerules", vec3_origin, vec3_angle );
		pEnt->SetName( AllocPooledString( "tf_gamerules" ) );
	}

	
	CBaseEntity::Create( "vote_controller", vec3_origin, vec3_angle );

	new CKickIssue();
	CRestartGameIssue *pRestartVote = new CRestartGameIssue();
	if ( pRestartVote )
		pRestartVote->Init();
	new CChangeLevelIssue();
	new CChangeMutatorIssue();
	new CNextLevelIssue();
	CScrambleTeams *pScrambleVote = new CScrambleTeams();
	if ( pScrambleVote )
		pScrambleVote->Init();
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim )
{
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = TFGameRules()->GetDeathScorer( pKiller, pInflictor, pVictim );

	const char *killer_weapon_name = "world";

	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "tf_weapon_flamethrower";
	}
	if ( info.GetDamageType() & DMG_NERVEGAS )
	{
		// sawblade!
		killer_weapon_name = "saw_kill";
	}
	else if ( info.GetDamageCustom() == TF_DMG_CUSTOM_DECAPITATION_BOSS )
	{
		// special-case burning damage, since persistent burning damage may happen after attacker has switched weapons
		killer_weapon_name = "headtaker";
	}
	else if ( pScorer && pInflictor && ( pInflictor == pScorer ) )
	{
		// If the inflictor is the killer,  then it must be their current weapon doing the damage
		if ( pScorer->GetActiveWeapon() )
		{
			killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname(); 
		}
	}
	else if ( pInflictor )
	{
		killer_weapon_name = STRING( pInflictor->m_iClassname );
	}
	
	static char temp[128];
	V_strncpy( temp, killer_weapon_name, sizeof( temp ) );
	Q_strlower( temp );
	killer_weapon_name = temp;
	
	int proj = Q_strlen( "tf_projectile_" );
	if ( strncmp( killer_weapon_name, "tf_projectile_", proj ) == 0 )
	{
		CTFGrenadePipebombProjectile *pGrenade = dynamic_cast<CTFGrenadePipebombProjectile *>(pInflictor);
		if ( pGrenade ) 
		{
			if ( pGrenade->GetOriginalLauncher() )
				killer_weapon_name = pGrenade->GetOriginalLauncher()->GetClassname();
			else 
				killer_weapon_name = pGrenade->GetClassname();
		}
	}

	V_strncpy( temp, killer_weapon_name, sizeof( temp ) );
	Q_strlower( temp );
	killer_weapon_name = temp;
	// strip certain prefixes from inflictor's classname
	const char *prefix[] = { "tf_weapon_grenade_", "tf_weapon_", "NPC_", "func_" };
	for ( int i = 0; i< ARRAYSIZE( prefix ); i++ )
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = Q_strlen( prefix[i] );
		if ( strncmp( killer_weapon_name, prefix[i], len ) == 0 )
		{
			killer_weapon_name += len;
			break;
		}
	}

	// sentry kill icons based on level
	if ( 0 == Q_strcmp( killer_weapon_name, "obj_sentrygun" ) )
	{
		CBaseObject* pObject = dynamic_cast< CBaseObject * >( pInflictor );

		if ( pObject )
		{
			switch ( pObject->GetUpgradeLevel() )
			{
				case 1:
					killer_weapon_name = "obj_sentrygun";
					break;
				case 2:
					killer_weapon_name = "obj_sentrygun2";
					break;
				case 3:
				default: // up to level 90
					killer_weapon_name = "obj_sentrygun3";
					break;
			}
		}
	}

	// look out for sentry rocket as weapon and map it to sentry gun, so we get the sentry death icon
	if ( 0 == Q_strcmp( killer_weapon_name, "tf_projectile_sentryrocket" ) )
	{
		killer_weapon_name = "obj_sentrygun3";
	}

	return killer_weapon_name;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor )
{
	CTFPlayer *pTFScorer = ToTFPlayer( pScorer );
	CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
	if ( pTFScorer && pTFVictim )
	{
		// if victim killed himself, don't award an assist to anyone else, even if there was a recent damager
		if ( pTFScorer == pTFVictim )
			return NULL;

		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( static_cast<CBaseEntity *>( pTFScorer->m_Shared.GetFirstHealer() ) );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( TF_CLASS_MEDIC == pHealer->GetPlayerClass()->GetClassIndex() ) && ( NULL == dynamic_cast<CObjectSentrygun *>( pInflictor ) ) )
		{
			return pHealer;
		}

		// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
		// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
		CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 1, TF_TIME_ASSIST_KILL );
		if ( pRecentDamager && ( pRecentDamager != pScorer ) )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specifed recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CTFPlayer *CTFGameRules::GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed )
{
	Assert( iDamager < MAX_DAMAGER_HISTORY );

	DamagerHistory_t &damagerHistory = pVictim->GetDamagerHistory( iDamager );
	if ( ( NULL != damagerHistory.hDamager ) && ( gpGlobals->curtime - damagerHistory.flTimeDamage <= flMaxElapsed ) )
	{
		CTFPlayer *pRecentDamager = ToTFPlayer( damagerHistory.hDamager );
		if ( pRecentDamager )
			return pRecentDamager;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer *CTFGameRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim )
{
	if ( ( pKiller == pVictim ) && ( pKiller == pInflictor ) )
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window.  If so, award this as a kill to the damager.
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim )
		{
			CTFPlayer *pRecentDamager = GetRecentDamager( pTFVictim, 0, TF_TIME_SUICIDE_KILL_CREDIT );
			if ( pRecentDamager )
				return pRecentDamager;
		}
	}

	return BaseClass::GetDeathScorer( pKiller, pInflictor, pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------

extern CMoveData *g_pMoveData;

void CTFGameRules::DeathNotice(CBasePlayer *pVictim, const CTakeDamageInfo &info)
{
	// Find the killer & the scorer
	CTFPlayer *pTFPlayerVictim = ToTFPlayer(pVictim);
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer(pKiller, pInflictor, pVictim);
	CTFPlayer *pAssister = ToTFPlayer(GetAssister(pVictim, pScorer, pInflictor));

	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = GetKillingWeaponName(info, pTFPlayerVictim);

	IGameEvent *event = gameeventmanager->CreateEvent("player_death");

	if (event)
	{
		event->SetInt("userid", pVictim->GetUserID());
		event->SetInt("assister", pAssister ? pAssister->GetUserID() : -1);
		event->SetString("weapon", killer_weapon_name);
		event->SetInt("damagebits", info.GetDamageType());
		event->SetInt("customkill", info.GetDamageCustom());
		event->SetInt("priority", 7);	// HLTV event priority, not transmitted

		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_DOMINATION)
			event->SetInt("dominated", 1);
		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_DOMINATION)
			event->SetInt("assister_dominated", 1);
		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_REVENGE)
			event->SetInt("revenge", 1);
		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_REVENGE)
			event->SetInt("assister_revenge", 1);

		if(pScorer)
			event->SetInt("attacker", pScorer->GetUserID());

		//medals, only activate after warmup
		if(!IsInWaitingForPlayers())
		{
			int weaponType = 0;
			if(pScorer)
			{
				//streaks
				CTFPlayer *pTFPlayerScorer = ToTFPlayer(pScorer);
				event->SetInt("killer_pupkills", pTFPlayerScorer->m_iPowerupKills);
				event->SetInt("killer_kspree", pTFPlayerScorer->m_iSpreeKills);
				event->SetInt("ex_streak", pTFPlayerScorer->m_iEXKills);
				weaponType = GetKillingWeaponType(pInflictor, pScorer);
			}

			//more streaks
			event->SetInt("victim_pupkills", !pTFPlayerVictim->m_bHadPowerup ? -1 : pTFPlayerVictim->m_iPowerupKills);
			event->SetInt("victim_kspree", pTFPlayerVictim->m_iSpreeKills);

			//Humiliation
			event->SetBool("humiliation", weaponType == 1 ? true : false);
			
			if(weaponType == 2) //inflictor is an explosive projectile
			{
				//***************************
				//Midair
				bool MidAirTime = pTFPlayerVictim->m_fAirStartTime && gpGlobals->curtime >= pTFPlayerVictim->m_fAirStartTime + (g_pMoveData->m_vecVelocity[2] >= 0.f ? 0.4f : 0.8f);
				event->SetBool("midair", MidAirTime ? true : false);

				//***************************
				//Kamikaze
				m_InflictorsArray[pVictim->entindex()] = pInflictor;
				bool Kamikaze = false;

				if(pKiller != pVictim) //evaluating death of the victim
				{
					//scorer was killed by the same inflictor of the victim
					Kamikaze = m_InflictorsArray[pKiller->entindex()] && pInflictor == m_InflictorsArray[pKiller->entindex()];
					if(Kamikaze)
					{
						m_InflictorsArray[pKiller->entindex()] = NULL;
						m_InflictorsArray[pVictim->entindex()] = NULL;
					}
				}
				else //evaluating death of the suicidal killer
				{
					//kiler killed itself with something that might have caused the death of a player
					//who died before it, we need to scout through the array to see if we find a match
					for(int i = 1; i <= gpGlobals->maxClients; i++)
					{
						if(!m_InflictorsArray[i] || i == pKiller->entindex()) //ignore the index of the killer, it's a suicide of course it will match
							continue;

						if(pInflictor == m_InflictorsArray[i])
						{
							Kamikaze = true;
							m_InflictorsArray[pVictim->entindex()] = NULL;
							m_InflictorsArray[i] = NULL;
							break;
						}
					}
				}

				event->SetBool("kamikaze", Kamikaze);
			}

			//first blood
			event->SetBool("firstblood", !m_bFirstBlood ? true : false);
			m_bFirstBlood = true;
		}

		gameeventmanager->FireEvent(event);
	}
}

void CTFGameRules::ResetDeathInflictor(int index)
{
	m_InflictorsArray[index] = NULL;
}

int CTFGameRules::GetKillingWeaponType(CBaseEntity *pInflictor, CBasePlayer *pScorer)
{
	if (pScorer && pInflictor && (pInflictor == pScorer))
	{
		CTFWeaponBase *pWeapon = ToTFPlayer(pScorer)->GetActiveTFWeapon();
		int weaponID = 0;

		if (pWeapon)
			weaponID = pWeapon->GetWeaponID();
		else
			return weaponID;

		if(WeaponID_IsMeleeWeapon(weaponID))
			return 1; //meleee
	}
	else if (pInflictor)
	{
		if(IsExplosiveProjectile(STRING(pInflictor->m_iClassname)))
			return 2; //explosive projectile
	}

	return 0; //no special weapon
}

void CTFGameRules::ClientDisconnected( edict_t *pClient )
{
	// clean up anything they left behind
	CTFPlayer *pPlayer = ToTFPlayer( GetContainingEntity( pClient ) );
	if ( pPlayer )
	{
		pPlayer->TeamFortress_ClientDisconnected();
	}

	// are any of the spies disguising as this player?
	for ( int i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pTemp = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTemp && pTemp != pPlayer )
		{
			if ( pTemp->m_Shared.GetDisguiseTarget() == pPlayer )
			{
				// choose someone else...
				pTemp->m_Shared.FindDisguiseTarget();
			}
		}
	}

	BaseClass::ClientDisconnected( pClient );
}

// Falling damage stuff.
#define TF_PLAYER_MAX_SAFE_FALL_SPEED	650		

float CTFGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_Local.m_flFallVelocity > TF_PLAYER_MAX_SAFE_FALL_SPEED )
	{
		// Old TFC damage formula
		float flFallDamage = 5 * (pPlayer->m_Local.m_flFallVelocity / 300);

		// Fall damage needs to scale according to the player's max health, or
		// it's always going to be much more dangerous to weaker classes than larger.
		float flRatio = (float)pPlayer->GetMaxHealth() / 100.0;
		flFallDamage *= flRatio;

		flFallDamage *= random->RandomFloat( 0.8, 1.2 );

		return flFallDamage;
	}

	// Fall caused no damage
	return 0;
}

void CTFGameRules::SendWinPanelInfo( void )
{
	IGameEvent *winEvent = gameeventmanager->CreateEvent( "teamplay_win_panel" );

	if ( winEvent )
	{
		int iBlueScore = GetGlobalTeam( TF_TEAM_BLUE )->GetScore();
		int iRedScore = GetGlobalTeam( TF_TEAM_RED )->GetScore();
		int iMercenaryScore = GetGlobalTeam( TF_TEAM_MERCENARY )->GetScore();
		int iBlueScorePrev = iBlueScore;
		int iRedScorePrev = iRedScore;
		int iMercenaryScorePrev = iMercenaryScore;

		bool bRoundComplete = m_bForceMapReset || ( IsGameUnderTimeLimit() && ( GetTimeLeft() <= 0 ) );

		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		bool bScoringPerCapture = ( pMaster ) ? ( pMaster->ShouldScorePerCapture() ) : false;

		if ( bRoundComplete && !bScoringPerCapture )
		{
			// if this is a complete round, calc team scores prior to this win
			switch ( m_iWinningTeam )
			{
			case TF_TEAM_BLUE:
				iBlueScorePrev = ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iBlueScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TF_TEAM_RED:
				iRedScorePrev = ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iRedScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			case TEAM_UNASSIGNED:
				break;	// stalemate; nothing to do
			case TF_TEAM_MERCENARY:
				iMercenaryScorePrev = ( iMercenaryScore - TEAMPLAY_ROUND_WIN_SCORE >= 0 ) ? ( iMercenaryScore - TEAMPLAY_ROUND_WIN_SCORE ) : 0;
				break;
			}
		}
			
		winEvent->SetInt( "panel_style", WINPANEL_BASIC );
		winEvent->SetInt( "winning_team", m_iWinningTeam );
		winEvent->SetInt( "winreason", m_iWinReason );
		winEvent->SetString( "cappers",  ( m_iWinReason == WINREASON_ALL_POINTS_CAPTURED || m_iWinReason == WINREASON_FLAG_CAPTURE_LIMIT ) ?
			m_szMostRecentCappers : "" );
		if ( TFGameRules()->IsDMGamemode() )
			winEvent->SetInt( "flagcaplimit", of_mctf_flag_caps_per_round.GetInt() );
		else
			winEvent->SetInt( "flagcaplimit", tf_flag_caps_per_round.GetInt() );
		winEvent->SetInt( "blue_score", iBlueScore );
		winEvent->SetInt( "red_score", iRedScore );
		winEvent->SetInt( "blue_score_prev", iBlueScorePrev );
		winEvent->SetInt( "red_score_prev", iRedScorePrev );
		winEvent->SetInt( "round_complete", bRoundComplete );

		CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );
		if ( !pResource )
			return;
 
		// determine the 3 players on winning team who scored the most points this round

		// build a vector of players & round scores
		CUtlVector<PlayerRoundScore_t> vecPlayerScore;
		int iPlayerIndex;
		for( iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( !pTFPlayer || !pTFPlayer->IsConnected() )
				continue;
			// filter out spectators and, if not stalemate, all players not on winning team
			int iPlayerTeam = pTFPlayer->GetTeamNumber();
			if ( ( iPlayerTeam < FIRST_GAME_TEAM ) || ( m_iWinningTeam != TEAM_UNASSIGNED && ( m_iWinningTeam != iPlayerTeam ) ) )
				continue;

			int iRoundScore = 0, iTotalScore = 0;
			PlayerStats_t *pStats = CTF_GameStats.FindPlayerStats( pTFPlayer );
			if ( pStats )
			{
				if ( TFGameRules()->IsDMGamemode() && !TFGameRules()->DontCountKills() )
					iRoundScore = iTotalScore = pTFPlayer->FragCount();
				else
				{
					iRoundScore = CalcPlayerScore( &pStats->statsCurrentRound );
					iTotalScore = CalcPlayerScore( &pStats->statsAccumulated );
				}
			}
			PlayerRoundScore_t &playerRoundScore = vecPlayerScore[vecPlayerScore.AddToTail()];
			playerRoundScore.iPlayerIndex = iPlayerIndex;
			playerRoundScore.iRoundScore = iRoundScore;
			playerRoundScore.iTotalScore = iTotalScore;
		}
		// sort the players by round score
		vecPlayerScore.Sort( PlayerRoundScoreSortFunc );

		// set the top (up to) 3 players by round score in the event data
		int numPlayers = min( 3, vecPlayerScore.Count() );
		for ( int i = 0; i < numPlayers; i++ )
		{
			// only include players who have non-zero points this round; if we get to a player with 0 round points, stop
			if ( 0 == vecPlayerScore[i].iRoundScore )
				break;

			// set the player index and their round score in the event
			char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
			Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i+ 1 );
			Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i+ 1 );
			winEvent->SetInt( szPlayerIndexVal, vecPlayerScore[i].iPlayerIndex );
			winEvent->SetInt( szPlayerScoreVal, vecPlayerScore[i].iRoundScore );				
		}

		if ( !bRoundComplete && ( TEAM_UNASSIGNED != m_iWinningTeam ) )
		{
			// if this was not a full round ending, include how many mini-rounds remain for winning team to win
			if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
			{
				winEvent->SetInt( "rounds_remaining", g_hControlPointMasters[0]->CalcNumRoundsRemaining( m_iWinningTeam ) );
			}
		}

		// Send the event
		gameeventmanager->FireEvent( winEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sorts players by round score
//-----------------------------------------------------------------------------
int CTFGameRules::PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 )
{
	// sort first by round score	
	if ( pRoundScore1->iRoundScore != pRoundScore2->iRoundScore )
		return pRoundScore2->iRoundScore - pRoundScore1->iRoundScore;

	// if round scores are the same, sort next by total score
	if ( pRoundScore1->iTotalScore != pRoundScore2->iTotalScore )
		return pRoundScore2->iTotalScore - pRoundScore1->iTotalScore;

	// if scores are the same, sort next by player index so we get deterministic sorting
	return ( pRoundScore2->iPlayerIndex - pRoundScore1->iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Called when the teamplay_round_win event is about to be sent, gives
//			this method a chance to add more data to it
//-----------------------------------------------------------------------------
void CTFGameRules::FillOutTeamplayRoundWinEvent( IGameEvent *event )
{
	// determine the losing team
	int iLosingTeam;

	switch( event->GetInt( "team" ) )
	{
	case TF_TEAM_RED:
		iLosingTeam = TF_TEAM_BLUE + TF_TEAM_MERCENARY;
		break;
	case TF_TEAM_BLUE:
		iLosingTeam = TF_TEAM_RED + TF_TEAM_MERCENARY;
		break;
	case TF_TEAM_MERCENARY:
		iLosingTeam = TF_TEAM_RED + TF_TEAM_BLUE;
	break;
	case TEAM_UNASSIGNED:
	default:
		iLosingTeam = TEAM_UNASSIGNED;
		break;
	}

	// set the number of caps that team got any time during the round
	event->SetInt( "losing_team_num_caps", m_iNumCaps[iLosingTeam] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetupSpawnPointsForRound( void )
{
	if ( !g_hControlPointMasters.Count() || !g_hControlPointMasters[0] || !g_hControlPointMasters[0]->PlayingMiniRounds() )
		return;

	CTeamControlPointRound *pCurrentRound = g_hControlPointMasters[0]->GetCurrentRound();
	if ( !pCurrentRound )
	{
		return;
	}

	// loop through the spawn points in the map and find which ones are associated with this round or the control points in this round
	CBaseEntity *pSpot = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
	while( pSpot )
	{
		CTFTeamSpawn *pTFSpawn = assert_cast<CTFTeamSpawn*>(pSpot);

		if ( pTFSpawn )
		{
			CHandle<CTeamControlPoint> hControlPoint = pTFSpawn->GetControlPoint();
			CHandle<CTeamControlPointRound> hRoundBlue = pTFSpawn->GetRoundBlueSpawn();
			CHandle<CTeamControlPointRound> hRoundRed = pTFSpawn->GetRoundRedSpawn();
			CHandle<CTeamControlPointRound> hRoundMercenary = pTFSpawn->GetRoundMercenarySpawn();
			
			if ( hControlPoint && pCurrentRound->IsControlPointInRound( hControlPoint ) )
			{
				// this spawn is associated with one of our control points
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( hControlPoint->GetOwner() );
			}
			else if ( hRoundBlue && ( hRoundBlue == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_BLUE );
			}
			else if ( hRoundRed && ( hRoundRed == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_RED );
			}
			else if ( hRoundMercenary && ( hRoundMercenary == pCurrentRound ) )
			{
				pTFSpawn->SetDisabled( false );
				pTFSpawn->ChangeTeam( TF_TEAM_MERCENARY );
			}
			else
			{
				// this spawn isn't associated with this round or the control points in this round
				pTFSpawn->SetDisabled( true );
			}
		}

		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_teamspawn" );
	}
}


int CTFGameRules::SetCurrentRoundStateBitString( void )
{
	m_iPrevRoundState = m_iCurrentRoundState;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( !pMaster )
	{
		return 0;
	}

	int iState = 0;

	for ( int i=0; i<pMaster->GetNumPoints(); i++ )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );

		if ( pPoint->GetOwner() == TF_TEAM_BLUE )
		{
			// Set index to 1 for the point being owned by blue
			iState |= ( 1<<i );
		}
	}

	m_iCurrentRoundState = iState;

	return iState;
}


void CTFGameRules::SetMiniRoundBitMask( int iMask )
{
	m_iCurrentMiniRoundMask = iMask;
}

//-----------------------------------------------------------------------------
// Purpose: NULL pPlayer means show the panel to everyone
//-----------------------------------------------------------------------------
void CTFGameRules::ShowRoundInfoPanel( CTFPlayer *pPlayer /* = NULL */ )
{
	KeyValues *data = new KeyValues( "data" );

	if ( m_iCurrentRoundState < 0 )
	{
		// Haven't set up the round state yet
		return;
	}

	// if prev and cur are equal, we are starting from a fresh round
	if ( m_iPrevRoundState >= 0 && pPlayer == NULL )	// we have data about a previous state
	{
		data->SetInt( "prev", m_iPrevRoundState );
	}
	else
	{
		// don't send a delta if this is just to one player, they are joining mid-round
		data->SetInt( "prev", m_iCurrentRoundState );	
	}

	data->SetInt( "cur", m_iCurrentRoundState );

	// get bitmask representing the current miniround
	data->SetInt( "round", m_iCurrentMiniRoundMask );

	if ( pPlayer )
	{
		pPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
	}
	else
	{
		for ( int i = 1;  i <= MAX_PLAYERS; i++ )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

			if ( pTFPlayer && pTFPlayer->IsReadyToPlay() )
			{
				pTFPlayer->ShowViewPortPanel( PANEL_ROUNDINFO, true, data );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TimerMayExpire( void )
{
	// Prevent timers expiring while control points are contested
	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	for ( int iPoint = 0; iPoint < iNumControlPoints; iPoint++ )
	{
		if ( ObjectiveResource()->GetCappingTeam( iPoint ) )
		{
			// HACK: Fix for some maps adding time to the clock 0.05s after CP is capped.
			m_flTimerMayExpireAt = gpGlobals->curtime + 0.1f;
			return false;
		}
	}

	if ( m_flTimerMayExpireAt >= gpGlobals->curtime )
		return false;

	// return true;
	// ficool2 - this fixes the payload overtime logic breaking... but what about other gamemodes?
 	return BaseClass::TimerMayExpire();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::RoundRespawn( void )
{
	// remove any buildings, grenades, rockets, etc. the player put into the world
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();
		}
	}

	// reset the flag captures
	int nTeamCount = TFTeamMgr()->GetTeamCount();
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		pTeam->SetFlagCaptures( 0 );
	}

	CTF_GameStats.ResetRoundStats();

	BaseClass::RoundRespawn();

	// ** AFTER WE'VE BEEN THROUGH THE ROUND RESPAWN, SHOW THE ROUNDINFO PANEL
	if ( !IsInWaitingForPlayers() )
	{
		ShowRoundInfoPanel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InternalHandleTeamWin( int iWinningTeam )
{
	// remove any spies' disguises and make them visible (for the losing team only)
	// and set the speed for both teams (winners get a boost and losers have reduced speed)
	for ( int i = 1;  i <= MAX_PLAYERS; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer )
		{
			if ( pPlayer->GetTeamNumber() > LAST_SHARED_TEAM )
			{
				if ( pPlayer->GetTeamNumber() != iWinningTeam )
				{
					pPlayer->RemoveInvisibility();
//					pPlayer->RemoveDisguise();

					if ( pPlayer->HasTheFlag() )
					{
						pPlayer->DropFlag();
					}
				}

				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}

	// disable any sentry guns the losing team has built
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "obj_sentrygun" ) ) != NULL )
	{
		CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( pEnt );
		if ( pSentry )
		{
			if ( pSentry->GetTeamNumber() != iWinningTeam )
			{
				pSentry->SetDisabled( true );
			}
		}
	}

	if ( m_bForceMapReset )
	{
		m_iPrevRoundState = -1;
		m_iCurrentRoundState = -1;
		m_iCurrentMiniRoundMask = 0;
	}
}

// sort function for the list of players that we're going to use to scramble the teams
int ScramblePlayersSort( CTFPlayer* const *p1, CTFPlayer* const *p2 )
{
	CTFPlayerResource *pResource = dynamic_cast< CTFPlayerResource * >( g_pPlayerResource );

	if ( pResource )
	{
		// check the priority
		if ( pResource->GetTotalScore( (*p2)->entindex() ) > pResource->GetTotalScore( (*p1)->entindex() ) )
		{
			return 1;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleScrambleTeams( void )
{
	int i = 0;
	CTFPlayer *pTFPlayer = NULL;
	CUtlVector<CTFPlayer *> pListPlayers;

	// add all the players (that are on blue or red) to our temp list
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pTFPlayer && ( pTFPlayer->GetTeamNumber() >= TF_TEAM_RED ) )
		{
			pListPlayers.AddToHead( pTFPlayer );
		}
	}

	// sort the list
	pListPlayers.Sort( ScramblePlayersSort );

	// loop through and put everyone on Spectator to clear the teams (or the autoteam step won't work correctly)
	for ( i = 0 ; i < pListPlayers.Count() ; i++ )
	{
		pTFPlayer = pListPlayers[i];

		if ( pTFPlayer )
		{
			pTFPlayer->ForceChangeTeam( TEAM_SPECTATOR );
		}
	}

	// loop through and auto team everyone
	for ( i = 0 ; i < pListPlayers.Count() ; i++ )
	{
		pTFPlayer = pListPlayers[i];

		if ( pTFPlayer )
		{
			pTFPlayer->ForceChangeTeam( TF_TEAM_AUTOASSIGN );
		}
	}
	
	ResetTeamsRoundWinTracking();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::HandleSwitchTeams( void )
{
	int i = 0;

	// respawn the players
	for ( i = 1 ; i <= gpGlobals->maxClients ; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->TeamFortress_RemoveEverythingFromWorld();

			// Ignore players who aren't on an active team
			if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE && pPlayer->GetTeamNumber() != TF_TEAM_MERCENARY)
			{
				continue;
			}

			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_BLUE );
			}
			else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			{
				pPlayer->ForceChangeTeam( TF_TEAM_RED );
			}
		}
	}

	// switch the team scores
	CTFTeam *pRedTeam = GetGlobalTFTeam( TF_TEAM_RED );
	CTFTeam *pBlueTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
	if ( pRedTeam && pBlueTeam )
	{
		int nRed = pRedTeam->GetScore();
		int nBlue = pBlueTeam->GetScore();

		pRedTeam->SetScore( nBlue );
		pBlueTeam->SetScore( nRed );
	}
}

bool CTFGameRules::CanChangeClassInStalemate( void ) 
{ 
	return (gpGlobals->curtime < (m_flStalemateStartTime + tf_stalematechangeclasstime.GetFloat())); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SetRoundOverlayDetails( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( pMaster && pMaster->PlayingMiniRounds() )
	{
		CTeamControlPointRound *pRound = pMaster->GetCurrentRound();

		if ( pRound )
		{
			CHandle<CTeamControlPoint> pRedPoint = pRound->GetPointOwnedBy( TF_TEAM_RED );
			CHandle<CTeamControlPoint> pBluePoint = pRound->GetPointOwnedBy( TF_TEAM_BLUE );

			// do we have opposing points in this round?
			if ( pRedPoint && pBluePoint )
			{
				int iMiniRoundMask = ( 1<<pBluePoint->GetPointIndex() ) | ( 1<<pRedPoint->GetPointIndex() );
				SetMiniRoundBitMask( iMiniRoundMask );
			}
			else
			{
				SetMiniRoundBitMask( 0 );
			}

			SetCurrentRoundStateBitString();
		}
	}

	BaseClass::SetRoundOverlayDetails();
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether a team should score for each captured point
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldScorePerRound( void )
{ 
	bool bRetVal = true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster && pMaster->ShouldScorePerCapture() )
	{
		bRetVal = false;
	}

	return bRetVal;
}

#endif  // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints )
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam( iTeam );
	if ( iOwnedEnd == -1 )
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints-1;
	if ( iOwnedEnd != 0 )
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for ( int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk )
	{
		// If we've hit a point we don't own, we're done
		if ( ObjectiveResource()->GetOwningTeam( iPoint ) != iTeam )
			break;

		if ( bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint] )
			continue;

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::TeamMayCapturePoint( int iTeam, int iPointIndex ) 
{ 
	// point capturing allowed?
	if ( !PointsMayBeCaptured() )
		return false;

	// locked?
	if ( ObjectiveResource()->GetCPLocked( iPointIndex ) )
		return false;

	// don't run further logic in domination
	if ( !tf_caplinear.GetBool() || IsDOMGamemode() )
		return true; 

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, 0 );

	// Points set to require themselves are always cappable 
	if ( iPointNeeded == iPointIndex )
		return true;

	// No required points specified? Require all previous points.
	if ( iPointNeeded == -1 )
	{
		if ( !ObjectiveResource()->PlayingMiniRounds() )
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint( iTeam, false );
			return (abs(iFarthestPoint - iPointIndex) <= 1);
		}
		else
		{
			// No custom previous point, team must own all previous points in the current mini-round
			//tagES TFTODO: need to figure out a good algorithm for this
			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for ( int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++ )
	{
		int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint( iPointIndex, iTeam, iPrevPoint );
		if ( iPointNeeded != -1 )
		{
			if ( ObjectiveResource()->GetOwningTeam( iPointNeeded ) != iTeam )
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */ )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( !pTFPlayer )
	{
		return false;
	}

	// Disguised and invisible spies cannot capture points
	if ( pTFPlayer->m_Shared.InCondInvis() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_stealthed" );
		}
		return false;
	}

	if ( pTFPlayer->m_Shared.InCondUber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return false;
	}

 	if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_disguised" );
		}
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( !pTFPlayer )
		return false;

	// Invuln players can block points
	if ( pTFPlayer->m_Shared.InCondUber() )
	{
		if ( pszReason )
		{
			Q_snprintf( pszReason, iMaxReasonLength, "#Cant_cap_invuln" );
		}
		return true;
	}

	return false;
}

#if defined( GAME_DLL )
//-----------------------------------------------------------------------------
// Purpose: Fills a vector with valid points that the player can capture right now
// Input:	pPlayer - The player that wants to capture
//			controlPointVector - A vector to fill with results
//-----------------------------------------------------------------------------
void CTFGameRules::CollectCapturePoints( CBasePlayer *pPlayer, CUtlVector<CTeamControlPoint *> *controlPointVector )
{
	Assert( ObjectiveResource() );
	if ( !controlPointVector || !pPlayer )
		return;

	controlPointVector->RemoveAll();

	if ( g_hControlPointMasters.IsEmpty() )
		return;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	if ( !pMaster || !pMaster->IsActive() )
		return;

	if ( IsInKothMode() && pMaster->GetNumPoints() == 1 )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( 0 );
		if ( pPoint && pPoint->GetPointIndex() == 0 )
			controlPointVector->AddToTail( pPoint );

		return;
	}

	for ( int i = 0; i < pMaster->GetNumPoints(); ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( pMaster->IsInRound( pPoint ) &&
			ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) != pPlayer->GetTeamNumber() &&
			ObjectiveResource()->TeamCanCapPoint( pPoint->GetPointIndex(), pPlayer->GetTeamNumber() ) &&
			TeamMayCapturePoint( pPlayer->GetTeamNumber(), pPoint->GetPointIndex() ) )
		{
			controlPointVector->AddToTail( pPoint );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills a vector with valid points that the player needs to defend from capture
// Input:	pPlayer - The player that wants to defend
//			controlPointVector - A vector to fill with results
//-----------------------------------------------------------------------------
void CTFGameRules::CollectDefendPoints( CBasePlayer *pPlayer, CUtlVector<CTeamControlPoint *> *controlPointVector )
{
	Assert( ObjectiveResource() );
	if ( !controlPointVector || !pPlayer )
		return;

	controlPointVector->RemoveAll();

	if ( g_hControlPointMasters.IsEmpty() )
		return;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	if ( !pMaster || !pMaster->IsActive() )
		return;

	for ( int i = 0; i < pMaster->GetNumPoints(); ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( pMaster->IsInRound( pPoint ) &&
			ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) == pPlayer->GetTeamNumber() &&
			ObjectiveResource()->TeamCanCapPoint( pPoint->GetPointIndex(), GetEnemyTeam( pPlayer ) ) &&
			TeamMayCapturePoint( GetEnemyTeam( pPlayer ), pPoint->GetPointIndex() ) )
		{
			controlPointVector->AddToTail( pPoint );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToPush( int iTeam )
{
	if ( GetGameType() != TF_GAMETYPE_PAYLOAD )
		return nullptr;

	if ( iTeam == TF_TEAM_BLUE )
	{
		if ( m_hBlueAttackTrain )
			return m_hBlueAttackTrain;

		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			if ( !pWatcher->IsDisabled() && pWatcher->GetTeamNumber() == iTeam )
			{
				m_hBlueAttackTrain = pWatcher;
				return m_hBlueAttackTrain;
			}

			pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
	}

	if ( iTeam == TF_TEAM_RED )
	{
		if ( m_hRedAttackTrain )
			return m_hRedAttackTrain;

		CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
		while ( pWatcher )
		{
			if ( !pWatcher->IsDisabled() && pWatcher->GetTeamNumber() == iTeam )
			{
				m_hRedAttackTrain = pWatcher;
				return m_hRedAttackTrain;
			}

			pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamTrainWatcher *CTFGameRules::GetPayloadToBlock( int iTeam )
{
	if ( m_nGameType != TF_GAMETYPE_PAYLOAD )
		return nullptr;

	if ( iTeam == TF_TEAM_BLUE )
	{
		if ( m_hBlueDefendTrain || HasMultipleTrains() )
			return m_hBlueDefendTrain;
	}

	if ( iTeam != TF_TEAM_RED )
		return nullptr;

	if ( m_hRedDefendTrain || HasMultipleTrains() )
		return m_hRedAttackTrain;

	CTeamTrainWatcher *pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( NULL, "team_train_watcher" ) );
	while ( pWatcher )
	{
		if ( !pWatcher->IsDisabled() )
		{
			m_hRedDefendTrain = pWatcher;
			return m_hRedDefendTrain;
		}

		pWatcher = dynamic_cast<CTeamTrainWatcher *>( gEntList.FindEntityByClassname( pWatcher, "team_train_watcher" ) );
	}

	return m_hRedDefendTrain;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Calculates score for player
//-----------------------------------------------------------------------------
int CTFGameRules::CalcPlayerScore( RoundStats_t *pRoundStats )
{
	int iScore =	( pRoundStats->m_iStat[TFSTAT_KILLS] * TF_SCORE_KILL ) + 
					( pRoundStats->m_iStat[TFSTAT_CAPTURES] * TF_SCORE_CAPTURE ) + 
					( pRoundStats->m_iStat[TFSTAT_DEFENSES] * TF_SCORE_DEFEND ) + 
					( pRoundStats->m_iStat[TFSTAT_BUILDINGSDESTROYED] * TF_SCORE_DESTROY_BUILDING ) + 
					( pRoundStats->m_iStat[TFSTAT_HEADSHOTS] * TF_SCORE_HEADSHOT ) + 
					( pRoundStats->m_iStat[TFSTAT_BACKSTABS] * TF_SCORE_BACKSTAB ) + 
					( pRoundStats->m_iStat[TFSTAT_HEALING] / TF_SCORE_HEAL_HEALTHUNITS_PER_POINT ) +  
					( pRoundStats->m_iStat[TFSTAT_KILLASSISTS] / TF_SCORE_KILL_ASSISTS_PER_POINT ) + 
					( pRoundStats->m_iStat[TFSTAT_TELEPORTS] / TF_SCORE_TELEPORTS_PER_POINT ) +
					( pRoundStats->m_iStat[TFSTAT_INVULNS] / TF_SCORE_INVULN ) +
					( pRoundStats->m_iStat[TFSTAT_REVENGE] / TF_SCORE_REVENGE );
	
	return Max( iScore, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::IsBirthday( void )
{
	if ( m_iBirthdayMode == BIRTHDAY_RECALCULATE )
	{
		m_iBirthdayMode = BIRTHDAY_OFF;
		if ( tf_birthday.GetBool() )
		{
			m_iBirthdayMode = BIRTHDAY_ON;
		}
		else
		{
			time_t ltime = time(0);
			const time_t *ptime = &ltime;
			struct tm *today = localtime( ptime );
			if ( today )
			{
//								TEAM FORTRESS 2									OPEN FORTRESS
				if ( (today->tm_mon == 7 && today->tm_mday == 24) || (today->tm_mon == 1 && today->tm_mday == 13) ) // Months start from 0, days from 1
				{
					m_iBirthdayMode = BIRTHDAY_ON;
				}
			}
		}
	}

	return ( m_iBirthdayMode == BIRTHDAY_ON );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameRules::SetIT( CBaseEntity *pEnt )
{
	CBasePlayer *pPlayer = ToBasePlayer( pEnt );
	if (pPlayer)
	{
		if (!m_itHandle.Get() || pPlayer != m_itHandle.Get())
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", pPlayer->GetPlayerName() );
			ClientPrint( pPlayer, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_WARN_VICTIM", pPlayer->GetPlayerName() );

			CSingleUserRecipientFilter filter( pPlayer );
			CBaseEntity::EmitSound( filter, pEnt->entindex(), "Player.YouAreIT" );
			pEnt->EmitSound( "Halloween.PlayerScream" );
		}
	}

	CBasePlayer *pIT = ToBasePlayer( m_itHandle.Get() );
	if (pIT && pEnt != pIT)
	{
		if (pIT->IsAlive())
		{
			CSingleUserRecipientFilter filter( pIT );
			CBaseEntity::EmitSound( filter, pIT->entindex(), "Player.TaggedOtherIT" );
			ClientPrint( pIT, HUD_PRINTTALK, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
			ClientPrint( pIT, HUD_PRINTCENTER, "#TF_HALLOWEEN_BOSS_LOST_AGGRO" );
		}
	}

	m_itHandle = pEnt;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	// The smaller number is always first
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		int tmp = collisionGroup0;
		collisionGroup0 = collisionGroup1;
		collisionGroup1 = tmp;
	}
	
	//Don't stand on COLLISION_GROUP_WEAPONs
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	// Don't stand on projectiles
	if( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		return false;
	}

	// Rockets need to collide with players when they hit, but
	// be ignored by player movement checks
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return true;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;
		
	// rockets do not collide with each other (tf_point_weapon_mimic)
	if ( ( collisionGroup0 == TFCOLLISION_GROUP_ROCKETS ) && 
		( collisionGroup1 == TFCOLLISION_GROUP_ROCKETS ) )
		return false;

	// Grenades don't collide with players. They handle collision while flying around manually.
	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ) && 
		( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
		return false;

	// Respawn rooms only collide with players
	if ( collisionGroup1 == TFCOLLISION_GROUP_RESPAWNROOMS )
		return ( collisionGroup0 == COLLISION_GROUP_PLAYER ) || ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT );
	
/*	if ( collisionGroup0 == COLLISION_GROUP_PLAYER )
	{
		// Players don't collide with objects or other players
		if ( collisionGroup1 == COLLISION_GROUP_PLAYER  )
			 return false;
 	}

	if ( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		// This is only for probing, so it better not be on both sides!!!
		Assert( collisionGroup0 != COLLISION_GROUP_PLAYER_MOVEMENT );

		// No collide with players any more
		// Nor with objects or grenades
		switch ( collisionGroup0 )
		{
		default:
			break;
		case COLLISION_GROUP_PLAYER:
			return false;
		}
	}
*/
	// don't want caltrops and other grenades colliding with each other
	// caltops getting stuck on other caltrops, etc.)
	if ( ( collisionGroup0 == TF_COLLISIONGROUP_GRENADES ) && 
		 ( collisionGroup1 == TF_COLLISIONGROUP_GRENADES ) )
	{
		return false;
	}


	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_PLAYER &&
		collisionGroup1 == TFCOLLISION_GROUP_COMBATOBJECT )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of this player towards capturing a point
//-----------------------------------------------------------------------------
int	CTFGameRules::GetCaptureValueForPlayer( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer->GetPlayerClass()->GetCapNumber() > 1)
	{
		if ( mp_capstyle.GetInt() == 1 )
		{
			// Scouts count for 2 people in timebased capping.
			return pTFPlayer->GetPlayerClass()->GetCapNumber();
		}
		else
		{
			// Scouts can cap all points on their own.
			return pTFPlayer->GetPlayerClass()->GetCapNumber() * 5;
		}
	}

	return BaseClass::GetCaptureValueForPlayer( pPlayer );
}

bool CTFGameRules::UsesDMBuckets()
{
	return ( of_multiweapons.GetBool() && ( IsDMGamemode() || IsInfGamemode() || IsJugGamemode() ) && !of_randomizer.GetBool() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetTimeLeft( void )
{
	float flTimeLimit = mp_timelimit.GetInt() * 60;

	Assert( flTimeLimit > 0 && "Should not call this function when !IsGameUnderTimeLimit" );

	float flMapChangeTime = m_flMapResetTime + flTimeLimit;

	return ( (int)(flMapChangeTime - gpGlobals->curtime) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetMaxHunted( int iTeamNumber )
{
	switch ( iTeamNumber )
	{
		case TF_TEAM_RED:
			return m_nMaxHunted_red;
			break;
		case TF_TEAM_BLUE:
			return m_nMaxHunted_blu;
			break;
		default:
			return -1;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGameRules::GetHuntedCount( int iTeamNumber )
{
	switch ( iTeamNumber )
	{
		case TF_TEAM_RED:
			return m_nHuntedCount_red;
			break;
		case TF_TEAM_BLUE:
			return m_nHuntedCount_blu;
			break;
		default:
			return -2;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "teamplay_point_captured" ) )
	{
#ifdef GAME_DLL
		RecalculateControlPointState();

		// keep track of how many times each team caps
		int iTeam = event->GetInt( "team" );
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		m_iNumCaps[iTeam]++;

		// award a capture to all capping players
		const char *cappers = event->GetString( "cappers" );

		Q_strncpy( m_szMostRecentCappers, cappers, ARRAYSIZE( m_szMostRecentCappers ) );	
		for ( int i =0; i < Q_strlen( cappers ); i++ )
		{
			int iPlayerIndex = (int) cappers[i];
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
			if ( pPlayer )
			{
				CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );				
			}
		}
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_capture_blocked" ) )
	{
#ifdef GAME_DLL
		int iPlayerIndex = event->GetInt( "blocker" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );
#endif
	}	
	else if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
#ifdef GAME_DLL
		int iWinningTeam = event->GetInt( "team" );
		bool bFullRound = event->GetBool( "full_round" );
		float flRoundTime = event->GetFloat( "round_time" );
		bool bWasSuddenDeath = event->GetBool( "was_sudden_death" );
		CTF_GameStats.Event_RoundEnd( iWinningTeam, bFullRound, flRoundTime, bWasSuddenDeath );
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_point_unlocked" ) )
	{
#ifdef GAME_DLL
		// if this is an unlock event and we're in arena, fire OnCapEnabled		
		CTFLogicArena *pArena = dynamic_cast<CTFLogicArena *>( gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );

		if ( pArena )
			pArena->m_OnCapEnabled.FireOutput( NULL, pArena );
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_flag_event" ) )
	{
#ifdef GAME_DLL
		// if this is a capture event, remember the player who made the capture		
		int iEventType = event->GetInt( "eventtype" );
		if ( TF_FLAGEVENT_CAPTURE == iEventType )
		{
			int iPlayerIndex = event->GetInt( "player" );
			m_szMostRecentCappers[0] = iPlayerIndex;
			m_szMostRecentCappers[1] = 0;
		}
#endif
	}
	else if ( !Q_strcmp( eventName, "teamplay_point_unlocked" ) )
	{
#ifdef GAME_DLL
		// unlock my point	
		CTFLogicArena *pArena = dynamic_cast< CTFLogicArena * >(gEntList.FindEntityByClassname( NULL, "tf_logic_arena" ) );
		if ( pArena )
			pArena->OnCapEnabled();
#endif
	}
#ifdef CLIENT_DLL
	else if ( !Q_strcmp( eventName, "game_newmap" ) )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Init ammo definitions
//-----------------------------------------------------------------------------

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			1	

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

		// Start at 1 here and skip the dummy ammo type to make CAmmoDef use the same indices
		// as our #defines.
		for (int i = 1; i < TF_AMMO_COUNT; i++)
		{
			def.AddAmmoType(g_aAmmoNames[i], DMG_BULLET, TRACER_LINE, 0, 0, "ammo_max", 2400, 10, 14);
			Assert(def.Index(g_aAmmoNames[i]) == i);
		}
	}

	return &def;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGameRules::GetTeamGoalString( int iTeam )
{
	if ( iTeam == TF_TEAM_RED )
		return m_pszTeamGoalStringRed.Get();
	if ( iTeam == TF_TEAM_BLUE )
		return m_pszTeamGoalStringBlue.Get();
	if ( iTeam == TF_TEAM_MERCENARY )
		return m_pszTeamGoalStringMercenary.Get();
	return NULL;
}

int CTFGameRules::GetAssignedHumanTeam( void ) const
{
#ifdef GAME_DLL
	if ( FStrEq( mp_humans_must_join_team.GetString(), "blue" ) )
		return TF_TEAM_BLUE;
	else if ( FStrEq( mp_humans_must_join_team.GetString(), "red" ) )
		return TF_TEAM_RED;
	else if ( FStrEq( mp_humans_must_join_team.GetString(), "any" ) )
		return TEAM_ANY;
#endif
	return TEAM_ANY;
}

const wchar_t *CTFGameRules::GetLocalizedGameTypeName( void )
{
	wchar_t *GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_UNDEFINED]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_GG ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_GG]);
	else if ( TFGameRules()->InGametype( TF_GAMETYPE_DM ) )
	{
		if ( TFGameRules()->InGametype( TF_GAMETYPE_TDM ) )
			GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_TDM]);
		else
			GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_DM]);
	}
	if ( TFGameRules()->InGametype( TF_GAMETYPE_CP ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_CP]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_CTF ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_CTF]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_ARENA ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_ARENA]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_ESC ) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_ESC]);
	if ( TFGameRules()->InGametype(TF_GAMETYPE_PAYLOAD) && !TFGameRules()->m_bEscortOverride )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_PAYLOAD]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_DOM) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_DOM]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_INF) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_INF]);
	if ( TFGameRules()->InGametype( TF_GAMETYPE_JUG) )
		GameType = g_pVGuiLocalize->Find(g_aGameTypeNames[TF_GAMETYPE_JUG]);
	return GameType;
}

#ifdef GAME_DLL

	Vector MaybeDropToGround( 
							CBaseEntity *pMainEnt, 
							bool bDropToGround, 
							const Vector &vPos, 
							const Vector &vMins, 
							const Vector &vMaxs )
	{
		if ( bDropToGround )
		{
			trace_t trace;
			UTIL_TraceHull( vPos, vPos + Vector( 0, 0, -500 ), vMins, vMaxs, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );
			return trace.endpos;
		}
		else
		{
			return vPos;
		}
	}

	//-----------------------------------------------------------------------------
	// Purpose: This function can be used to find a valid placement location for an entity.
	//			Given an origin to start looking from and a minimum radius to place the entity at,
	//			it will sweep out a circle around vOrigin and try to find a valid spot (on the ground)
	//			where mins and maxs will fit.
	// Input  : *pMainEnt - Entity to place
	//			&vOrigin - Point to search around
	//			fRadius - Radius to search within
	//			nTries - Number of tries to attempt
	//			&mins - mins of the Entity
	//			&maxs - maxs of the Entity
	//			&outPos - Return point
	// Output : Returns true and fills in outPos if it found a spot.
	//-----------------------------------------------------------------------------
	bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround )
	{
		// This function moves the box out in each dimension in each step trying to find empty space like this:
		//
		//											  X  
		//							   X			  X  
		// Step 1:   X     Step 2:    XXX   Step 3: XXXXX
		//							   X 			  X  
		//											  X  
		//

		Vector mins, maxs;
		pMainEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		mins -= pMainEnt->GetAbsOrigin();
		maxs -= pMainEnt->GetAbsOrigin();

		// Put some padding on their bbox.
		float flPadSize = 5;
		Vector vTestMins = mins - Vector( flPadSize, flPadSize, flPadSize );
		Vector vTestMaxs = maxs + Vector( flPadSize, flPadSize, flPadSize );

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( pMainEnt, vOrigin + vTestMins, vOrigin + vTestMaxs ) )
		{
			outPos = MaybeDropToGround( pMainEnt, bDropToGround, vOrigin, vTestMins, vTestMaxs );
			return true;
		}

		Vector vDims = vTestMaxs - vTestMins;


		// Keep branching out until we get too far.
		int iCurIteration = 0;
		int nMaxIterations = 15;

		int offset = 0;
		do
		{
			for ( int iDim=0; iDim < 3; iDim++ )
			{
				float flCurOffset = offset * vDims[iDim];

				for ( int iSign=0; iSign < 2; iSign++ )
				{
					Vector vBase = vOrigin;
					vBase[iDim] += (iSign*2-1) * flCurOffset;

					if ( UTIL_IsSpaceEmpty( pMainEnt, vBase + vTestMins, vBase + vTestMaxs ) )
					{
						// Ensure that there is a clear line of sight from the spawnpoint entity to the actual spawn point.
						// (Useful for keeping things from spawning behind walls near a spawn point)
						trace_t tr;
						UTIL_TraceLine( vOrigin, vBase, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &tr );

						if ( tr.fraction != 1.0 )
						{
							continue;
						}

						outPos = MaybeDropToGround( pMainEnt, bDropToGround, vBase, vTestMins, vTestMaxs );
						return true;
					}
				}
			}

			++offset;
		} while ( iCurIteration++ < nMaxIterations );

		//	Warning( "EntityPlacementTest for ent %d:%s failed!\n", pMainEnt->entindex(), pMainEnt->GetClassname() );
		return false;
	}

#else // GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( State_Get() == GR_STATE_STARTGAME )
	{
		m_iBirthdayMode = BIRTHDAY_RECALCULATE;
	}
}

void CTFGameRules::HandleOvertimeBegin()
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pTFPlayer )
	{
		pTFPlayer->EmitSound( "Game.Overtime" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFGameRules::ShouldShowTeamGoal( void )
{
	if ( State_Get() == GR_STATE_PREROUND || State_Get() == GR_STATE_RND_RUNNING || InSetup() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFGameRules::GetTeamGlowColor( int nTeam )
{
	float r, g, b;
	switch (nTeam)
	{
	case TF_TEAM_BLUE:
		r = 0.49f; g = 0.66f; b = 0.77f;
		break;

	case TF_TEAM_RED:
		r = 0.74f; g = 0.23f; b = 0.23f;
		break;

	case TF_TEAM_MERCENARY: //The team used in dm should use the color of the player for glow color
		r = of_color_r.GetFloat();
		g = of_color_g.GetFloat();
		b = of_color_b.GetFloat();
		if ( r < TF_GLOW_COLOR_CLAMP && g < TF_GLOW_COLOR_CLAMP && b < TF_GLOW_COLOR_CLAMP )
		{
			float maxi = max(max(r, g), b);
			maxi = TF_GLOW_COLOR_CLAMP - maxi;
			r += maxi;
			g += maxi;
			b += maxi;
		}
		r = r / 255.0f;
		g = g / 255.0f;
		b = b / 255.0f;	
		break;

	default:
		r = 0.76f; g = 0.76f; b = 0.76f;
		break;
	}

	return Vector(r, g, b);
}

#endif

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::ShutdownCustomResponseRulesDicts()
{
	DestroyCustomResponseSystems();

	if ( m_ResponseRules.Count() != 0 )
	{
		int nRuleCount = m_ResponseRules.Count();
		for ( int iRule = 0; iRule < nRuleCount; ++iRule )
		{
			m_ResponseRules[iRule].m_ResponseSystems.Purge();
		}
		m_ResponseRules.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::InitCustomResponseRulesDicts()
{
	MEM_ALLOC_CREDIT();

	// Clear if necessary.
	ShutdownCustomResponseRulesDicts();

	// Initialize the response rules for TF.
	m_ResponseRules.AddMultipleToTail( TF_CLASS_COUNT_ALL );

	char szName[512];
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		m_ResponseRules[iClass].m_ResponseSystems.AddMultipleToTail( MP_TF_CONCEPT_COUNT );

		for ( int iConcept = 0; iConcept < MP_TF_CONCEPT_COUNT; ++iConcept )
		{
			AI_CriteriaSet criteriaSet;
			criteriaSet.AppendCriteria( "playerclass", g_aPlayerClassNames_NonLocalized[iClass] );
			criteriaSet.AppendCriteria( "Concept", g_pszMPConcepts[iConcept] );
			// 1 point for player class and 1 point for concept.
			float flCriteriaScore = 2.0f;

			// Name.
			V_snprintf( szName, sizeof( szName ), "%s_%s\n", g_aPlayerClassNames_NonLocalized[iClass], g_pszMPConcepts[iConcept] );
			m_ResponseRules[iClass].m_ResponseSystems[iConcept] =
			BuildCustomResponseSystemGivenCriteria( 
			"scripts/talker/response_rules.txt", 
			szName,
			criteriaSet, 
			flCriteriaScore );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, HudNotification_t iType )
{
	UserMessageBegin( filter, "HudNotify" );
		WRITE_BYTE( iType );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam /*= TEAM_UNASSIGNED*/ )
{
	UserMessageBegin( filter, "HudNotifyCustom" );
		WRITE_STRING( pszText );
		WRITE_STRING( pszIcon );
		WRITE_BYTE( iTeam );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Is the player past the required delays for spawning
//-----------------------------------------------------------------------------
bool CTFGameRules::HasPassedMinRespawnTime( CBasePlayer *pPlayer )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer && pTFPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		return true;

	float flMinSpawnTime = GetMinTimeWhenPlayerMaySpawn( pPlayer ); 

	return ( gpGlobals->curtime > flMinSpawnTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameRules::PushAllPlayersAway( Vector const &vecPos, float flRange, float flForce, int iTeamNum, CUtlVector<CTFPlayer *> *outVector )
{
	CUtlVector<CTFPlayer *> players;
	CollectPlayers( &players, iTeamNum, true );

	for ( CTFPlayer *pPlayer : players )
	{
		Vector vecTo = pPlayer->EyePosition() - vecPos;
		if ( vecTo.LengthSqr() > Square( flRange ) )
			continue;

		vecTo.NormalizeInPlace();

		pPlayer->ApplyAbsVelocityImpulse( vecTo * flForce );

		if ( outVector )
			outVector->AddToTail( pPlayer );
	}
}

#endif

bool CTFGameRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
	// IsConnectedUserInfoChangeAllowed allows the clients to change
	// cvars with the FCVAR_NOT_CONNECTED rule if it returns true
#ifndef GAME_DLL
	pPlayer = C_BasePlayer::GetLocalPlayer();
#endif

	if ( pPlayer && !pPlayer->IsAlive() && pPlayer->GetTeamNumber() < TF_TEAM_RED )
		return true;

	return false;
}

#ifdef CLIENT_DLL
const char *CTFGameRules::GetVideoFileForMap( bool bWithExtension /*= true*/ )
{
	char mapname[MAX_MAP_NAME];

	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname ) );
	Q_strlower( mapname );

#ifdef _X360
	// need to remove the .360 extension on the end of the map name
	char *pExt = Q_stristr( mapname, ".360" );
	if ( pExt )
	{
		*pExt = '\0';
	}
#endif

	static char strFullpath[MAX_PATH];
	Q_strncpy( strFullpath, "media/", MAX_PATH );	// Assume we must play out of the media directory
	Q_strncat( strFullpath, mapname, MAX_PATH );

	if ( bWithExtension )
	{
		Q_strncat( strFullpath, ".bik", MAX_PATH );		// Assume we're a .bik extension type
	}

	return strFullpath;
}
#endif

#ifdef GAME_DLL

void CTFGameRules::BeginInfection( void )
{
	TFGameRules()->SelectInfector();

	// all teams
	for ( int i = FIRST_GAME_TEAM; i < GetNumberOfTeams(); i++ )
	{
		BroadcastSound( i, "RoundStart" );
		BroadcastSound( i, "InfectionMusic.Begin", false );
	}

	// remove the timer
	if ( GetInfectionRoundTimer() )
		UTIL_Remove( GetInfectionRoundTimer() );

	// wow
	variant_t sVariant;
	sVariant.SetInt( 0 );

	// create the timer
	TFGameRules()->SetInfectionRoundTimer( ( CTeamRoundTimer* )CBaseEntity::Create( "team_round_timer", vec3_origin, vec3_angle ) );

	if ( TFGameRules()->GetInfectionRoundTimer() )
	{
		TFGameRules()->GetInfectionRoundTimer()->SetName( MAKE_STRING( "zz_infection_timer" ) );
		TFGameRules()->GetInfectionRoundTimer()->SetInfectionBeginning( false );
		TFGameRules()->GetInfectionRoundTimer()->SetTimeRemaining( of_infection_roundtime.GetInt() ); 
		TFGameRules()->GetInfectionRoundTimer()->SetShowInHud( true );
		TFGameRules()->GetInfectionRoundTimer()->ChangeTeam( TF_TEAM_RED );

		TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Enable", NULL, NULL, sVariant, 0 );
		TFGameRules()->GetInfectionRoundTimer()->AcceptInput( "Resume", NULL, NULL, sVariant, 0 );
	}
}

void CTFGameRules::SelectInfector( void )
{
	Msg("TFGameRulesInfection: Infection is selecting zombies...\n");
	
	// Find random player(s) to infect
	// This is based on the amount of players (default ratio: 1 zombie for every 6 humans, rounded up)
	CBasePlayer *pPlayer = NULL;
	int i;

	int playercount = 0;
	int targets = 0;
	int candidates = 0;
	
	// Get a count of all players, and then a count of those players who will be suitable targets
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			playercount++;

			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
				targets++;
		}
	}

	if ( targets <= 0 )
	{
		Msg( "TFGameRulesInfection: Infection found no players to zombify!\n" );
		if ( GetInfectionRoundTimer() )
			UTIL_Remove( GetInfectionRoundTimer() );
		TFGameRules()->SetInWaitingForPlayers( true );
		return;
	}

	if ( targets > 1 )
	{
		int threshold = of_infection_zombie_threshold.GetInt();

		if ( threshold <= 0 )
			threshold = 6;
		
		// Get a count of players who will be selected as zombies, this division is rounded up
		candidates = ceil( (float)targets / threshold );

		if ( candidates <= 0 )
		{
			Msg( "TFGameRulesInfection: Candidates for Infection is 0!\n" );
			if ( GetInfectionRoundTimer() )
				UTIL_Remove( GetInfectionRoundTimer() );
			TFGameRules()->SetInWaitingForPlayers( true );
			return;
		}
	}
	else
	{
		Msg( "TFGameRulesInfection: Needs more than 1 player on RED to begin.\n" );
		if ( GetInfectionRoundTimer() )
			UTIL_Remove( GetInfectionRoundTimer() );
		TFGameRules()->SetInWaitingForPlayers( true );
		return;
	}
	
	int failsafe = 0;

	// Pick a random player index between 0 and the player count, and zombify them, until all candidates are exhausted
	do
	{
		Msg( "TFGameRulesInfection: Infection is finding a potential candidate...\n" );
		int index = RandomInt( 0, playercount - 1 );
		
		// Potential fix for an infinite loop here
		failsafe++;	

		pPlayer = UTIL_PlayerByIndex( index );

		if ( pPlayer && pPlayer->GetTeamNumber() == TF_TEAM_RED )
		{
			candidates--;

			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

			if ( pTFPlayer )
			{
				Msg( "TFGameRulesInfection: Infection found a suitable candidate! Zombifying...\n" );
				pTFPlayer->ChangeTeam( TF_TEAM_BLUE, false );
				pTFPlayer->CommitSuicide( true, true );
			}
		}
	} 
	while ( candidates > 0 && failsafe < 65 );
}

void CTFGameRules::FinishInfection( void )
{
	SetWinningTeam( TF_TEAM_RED, WINREASON_DEFEND_UNTIL_TIME_LIMIT, false );
}



void CTFGameRules::PickJuggernaught( void )
{
	Msg("TFGameRulesJuggernaught: Selecting possible Juggernaught...\n");
	
	// Find random player(s) to infect
	// This is based on the amount of players (default ratio: 1 zombie for every 6 humans, rounded up)
	CBasePlayer *pPlayer = NULL;
	int i;

	int playercount = 0;
	int targets = 0;
	int candidates = 0;
	
	// Get a count of all players, and then a count of those players who will be suitable targets
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer )
		{
			playercount++;
			targets++;
		}
	}

	if ( targets <= 0 )
	{
		Msg( "TFGameRulesJuggernaught: Found no players to become Juggernaught!\n" );
		TFGameRules()->SetInWaitingForPlayers( true );
		return;
	}

	if ( targets > 1 )
	{
		int threshold = 1;
		
		// Get a count of players who will be selected as zombies, this division is rounded up
		candidates = ceil( (float)targets / threshold );

		if ( candidates <= 0 )
		{
			Msg( "TFGameRulesJuggernaught: Candidates for Juggernaught is 0!\n" );
			TFGameRules()->SetInWaitingForPlayers( true );
			return;
		}
	}
	else
	{
		Msg( "TFGameRulesJuggernaught: Needs more than 1 player to begin.\n" );
		TFGameRules()->SetInWaitingForPlayers( true );
		return;
	}
	
	int failsafe = 0;

	// Pick a random player index between 0 and the player count, and zombify them, until all candidates are exhausted
	do
	{
		Msg( "TFGameRulesJuggernaught: Finding a potential candidate...\n" );
		int index = RandomInt( 0, playercount - 1 );
		
		// Potential fix for an infinite loop here
		failsafe++;	

		pPlayer = UTIL_PlayerByIndex( index );

		if ( pPlayer )
		{
			candidates--;

			CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

			if ( pTFPlayer )
			{
				Msg( "TFGameRulesJuggernaught: Found a suitable candidate!\n" );
				pTFPlayer->BecomeJuggernaught();
			}
		}
	} 
	while ( candidates > 0 && failsafe < 65 );
}

#endif