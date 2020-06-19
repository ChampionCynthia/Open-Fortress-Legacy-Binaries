//====== Copyright � 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_player.h"
#include "c_user_message_register.h"
#include "view.h"
#include "iclientvehicle.h"
#include "ivieweffects.h"
#include "input.h"
#include "IEffects.h"
#include "fx.h"
#include "c_basetempentity.h"
#include "hud_macros.h"
#include "engine/ivdebugoverlay.h"
#include "smoke_fog_overlay.h"
#include "playerandobjectenumerator.h"
#include "bone_setup.h"
#include "in_buttons.h"
#include "r_efx.h"
#include "dlight.h"
#include "shake.h"
#include "cl_animevent.h"
#include "tf_weaponbase.h"
#include "c_tf_playerresource.h"
#include "toolframework/itoolframework.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "prediction.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "tf_fx_muzzleflash.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include "view_scene.h"
#include "ai_debug_shared.h"
#include "c_baseobject.h"
#include "toolframework_client.h"
#include "soundenvelope.h"
#include "voice_status.h"
#include "clienteffectprecachesystem.h"
#include "functionproxy.h"
#include "toolframework_client.h"
#include "choreoevent.h"
#include "vguicenterprint.h"
#include "eventlist.h"
#include "tf_hud_statpanel.h"
#include "input.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_hud_mediccallers.h"
#include "in_main.h"
#include "basemodelpanel.h"
#include "c_team.h"
#include "collisionutils.h"
#include "tf_viewmodel.h"
#include "cdll_int.h"
#include "filesystem.h"

#include "dt_utlvector_recv.h"

// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "c_tf_team.h"
#include "c_entitydissolve.h"

#if defined( CTFPlayer )
#undef CTFPlayer
#endif

#include "materialsystem/imesh.h"		//for materials->FindMaterial
#include "iviewrender.h"				//for view->

#include "cam_thirdperson.h"
#include "tf_hud_chat.h"
#include "iclientmode.h"
#include "tf_viewmodel.h"

#include "gameui/of/dm_loadout.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_playergib_forceup( "tf_playersgib_forceup", "1.0", FCVAR_CHEAT, "Upward added velocity for gibs." );
ConVar tf_playergib_force( "tf_playersgib_force", "500.0", FCVAR_CHEAT, "Gibs force." );
ConVar tf_playergib_maxspeed( "tf_playergib_maxspeed", "400", FCVAR_CHEAT, "Max gib speed." );

ConVar cl_autorezoom( "cl_autorezoom", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "When set to 1, sniper rifle will re-zoom after firing a zoomed shot." );

// ficool2: disabled this by default as it causes framerate issues
ConVar of_muzzlelight("of_muzzlelight", "0", FCVAR_CHEAT, "Enable dynamic lights for muzzleflashes, projectiles and the flamethrower.");

ConVar of_beta_muzzleflash("of_beta_muzzleflash", "0", FCVAR_ARCHIVE, "Enable the TF2 beta muzzleflash model when firing.");

ConVar of_idleview("of_idleview", "0", FCVAR_ARCHIVE, "Enables/Disables idle shake." );

ConVar of_gore( "of_gore", "1", FCVAR_ARCHIVE, "Enables or disables dismemberment and gore effects on players. Does not affect gibbing" );

ConVar of_color_r( "of_color_r", "128", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets merc color's red channel value", true, -1, true, 255 );
ConVar of_color_g( "of_color_g", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets merc color's green channel value", true, -1, true, 255 );
ConVar of_color_b( "of_color_b", "128", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets merc color's blue channel value", true, -1, true, 255 );

ConVar of_tennisball( "of_tennisball", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Big Tiddie Tennis GF" );
ConVar of_mercenary_hat( "of_mercenary_hat", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Because you can't have TF2 without hats" );
ConVar of_disable_cosmetics( "of_disable_cosmetics", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Because you CAN have TF2 without hats" );

ConVar tf_hud_no_crosshair_on_scope_zoom( "tf_hud_no_crosshair_on_scope_zoom", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Disable the crosshair when scoped in with a Sniper Rifle or Railgun." );

static ConVar cl_fp_ragdoll( "cl_fp_ragdoll", "0", FCVAR_ARCHIVE, "Enable first person ragdolls." );

ConVar of_respawn_particle( "of_respawn_particle", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Particle that plays when you spawn in Deathmatch", true, 1, true, 36 );

ConVar of_critglow_saturation( "of_critglow_saturation", "0.1", FCVAR_ARCHIVE | FCVAR_USERINFO, "How Saturated the critglow in deathmatch is." );

ConVar tf_taunt_first_person( "tf_taunt_first_person", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Should taunts be performed in firstperson? (cl_first_person_uses_world_model 1 recommended)." );

ConVar cl_quickzoom( "cl_quickzoom", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Softzoom on right click whenever avalible" );
ConVar tf_always_deathanim( "tf_always_deathanim", "0", FCVAR_CHEAT, "Forces death animation." );

ConVar of_first_person_respawn_particles( "of_first_person_respawn_particles", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Show respawn particles in first person." );

ConVar of_respawn_particles( "of_respawn_particles", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Draw respawn particles of players?" );

extern ConVar cl_first_person_uses_world_model;
extern ConVar of_jumpsound;

extern const char *g_aLoadoutConvarNames[];
extern const char *g_aArsenalConvarNames[];

void RefreshDesiredCosmetics( int iClass )
{
	if( GetLoadout() )
	{
		KeyValues *pCosmetics = GetLoadout()->FindKey("Cosmetics");
		if( pCosmetics )
		{
			KeyValues *pClass = pCosmetics->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
			if( pClass )
			{
				KeyValues *pHat = pClass->GetFirstValue();
				char szCommand[128];
				szCommand[0] = '\0';
				for( pHat; pHat != NULL; pHat = pHat->GetNextValue() ) // Loop through all the keyvalues
				{
					if( szCommand[0] != '\0' )
						Q_snprintf( szCommand, sizeof( szCommand ), "%s %s", szCommand, pHat->GetString() );
					else
						Q_snprintf( szCommand, sizeof( szCommand ), "%s", pHat->GetString() );
				}
				ConVarRef var( g_aLoadoutConvarNames[iClass] );
				if ( var.IsValid() )
				{
					var.SetValue(szCommand);
				}
			}
		}
	}
}

void RefreshDesiredWeapons( int iClass )
{
	if( GetLoadout() )
	{
		KeyValues *pWeapons = GetLoadout()->FindKey("Weapons");
		if( pWeapons )
		{
			KeyValues *pClass = pWeapons->FindKey( g_aPlayerClassNames_NonLocalized[iClass] );
			if( pClass )
			{
				KeyValues *pWeapon = pClass->GetFirstValue();
				char szCommand[128];
				szCommand[0] = '\0';
				for( pWeapon; pWeapon != NULL; pWeapon = pWeapon->GetNextValue() ) // Loop through all the keyvalues
				{
					if( szCommand[0] != '\0' )
						Q_snprintf( szCommand, sizeof( szCommand ), "%s %s %d", szCommand, pWeapon->GetName(), GetItemSchema()->GetWeaponID(pWeapon->GetString()) );
					else
						Q_snprintf( szCommand, sizeof( szCommand ), "%s %d", pWeapon->GetName(), GetItemSchema()->GetWeaponID(pWeapon->GetString()) );
				}
				ConVarRef var( g_aArsenalConvarNames[iClass] );
				if ( var.IsValid() )
				{
					var.SetValue(szCommand);
				}
			}
		}
	}
}


void OnCosmeticEquip( const CCommand& args, KeyValues *pClass )
{
	KeyValues *pValue = GetCosmetic( abs( atoi(args[3]) ) );

	if( !pValue )
		return;

	const char *szRegion = pValue->GetString( "region", "none" );
	
	pClass->SetString( szRegion, args[3] );

	GetLoadout()->SaveToFile( filesystem, "cfg/loadout.cfg" );
	
	RefreshDesiredCosmetics( GetClassIndexFromString( args[2], TF_CLASS_COUNT_ALL ) );
}

void OnWeaponEquip( const CCommand& args, KeyValues *pClass )
{
	KeyValues *pValue = GetWeaponFromSchema( args[3] );

	if( !pValue )
		return;

	BaseModUI::DMLoadout *pDMLoadout = static_cast<BaseModUI::DMLoadout*>(BaseModUI::CBaseModPanel::GetSingleton().GetWindow(BaseModUI::WT_DM_LOADOUT));
	
	KeyValues *pWeapons = GetLoadout()->FindKey("Weapons");
	if( pWeapons )
	{
		KeyValues *pLoadoutClass = pWeapons->FindKey( args[2] );
		if( pLoadoutClass )
		{
			int iOtherSlot = (atoi( args[4] ) - 1) ? 1 : 2;
			const char *szOtherWeapon = pLoadoutClass->GetString( VarArgs("%d", iOtherSlot) );
			if( !Q_strcmp( args[3], szOtherWeapon ) )
			{
				pLoadoutClass->SetString( VarArgs( "%d", iOtherSlot), pLoadoutClass->GetString( args[4] ) );
				if(pDMLoadout)
				{
					pDMLoadout->SelectWeapon( iOtherSlot, pLoadoutClass->GetString(args[4]), true );
				}
			}
		}
	}

	pClass->SetString( args[4], args[3] );

	GetLoadout()->SaveToFile( filesystem, "cfg/loadout.cfg" );

	if (pDMLoadout)
	{
		pDMLoadout->SelectWeapon(atoi(args[4]), args[3]);
	}

	RefreshDesiredWeapons( GetClassIndexFromString( args[2], TF_CLASS_COUNT_ALL ) );		
}

// --------------------------------------------------------------------
// Purpose: Cycle through the aim & move modes.
// --------------------------------------------------------------------
void LoadoutEquip( const CCommand& args )
{
	// args[1] if ever needed, this will check the class
	if( !GetLoadout() )
		return;
	
	int iCategory = 0;
	
	char szCategory[64];
	Q_strncpy(szCategory, args[1], sizeof(szCategory) );
	strlwr(szCategory);	
	
	for( int i = 0; i < 2 ; i++ )
	{
		if( !Q_strcmp( szCategory, g_aLoadoutCategories[i] ) )
		{
			iCategory = i;
			break;
		}
	}
	
	KeyValues *pCategory = GetLoadout()->FindKey( args[1] );
	
	if( !pCategory )
	{
		for ( int i = 0; i < 2; i++ )
		{
			size_t length = strlen( g_aLoadoutCategories[i] );

			if ( length <= strlen( args[1] ) && !Q_strnicmp( g_aLoadoutCategories[i], szCategory, length ) )
				ResetLoadout( szCategory );
		}
		return;
	}
	KeyValues *pClass = pCategory->FindKey( args[2] );
	if( !pClass )
	{
		for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
		{
			size_t length = strlen( g_aPlayerClassNames_NonLocalized[i] );

			if ( length <= strlen( args[2] ) && !Q_strnicmp( g_aPlayerClassNames_NonLocalized[i], args[2], length ) )
				ResetLoadout( args[1] );
		}
		return;
	}
	
	switch( iCategory )
	{
		default:
		case 0:
			OnCosmeticEquip( args, pClass );
		break;
		case 1:
			OnWeaponEquip( args, pClass );
		break;
	}
}
static ConCommand loadout_equip("loadout_equip", LoadoutEquip );

void LoadoutUnEquip( const CCommand& args )
{
	// args[1] if ever needed, this will check the class
	if( !GetLoadout() )
		return;

	int iCategory = 0;
	
	char szCategory[64];
	Q_strncpy(szCategory, args[1], sizeof(szCategory) );
	strlwr(szCategory);	
	
	for( int i = 0; i < 2 ; i++ )
	{
		if( !Q_strcmp( szCategory, g_aLoadoutCategories[i] ) )
		{
			iCategory = i;
			break;
		}
	}
	
	KeyValues *pCategory = GetLoadout()->FindKey( args[1] );
	
	if( !pCategory )
	{
		for ( int i = 0; i < 2; i++ )
		{
			size_t length = strlen( g_aLoadoutCategories[i] );

			if ( length <= strlen( args[1] ) && !Q_strnicmp( g_aLoadoutCategories[i], szCategory, length ) )
				ResetLoadout( szCategory );
		}
		return;
	}
	KeyValues *pClass = pCategory->FindKey( args[2] );
	if( !pClass )
	{
		for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
		{
			size_t length = strlen( g_aPlayerClassNames_NonLocalized[i] );

			if ( length <= strlen( args[2] ) && !Q_strnicmp( g_aPlayerClassNames_NonLocalized[i], args[2], length ) )
				ResetLoadout( args[1] );
		}
		return;
	}
	KeyValues *pValue;
	switch( iCategory )
	{
		default:
		case 0:
			pValue = GetCosmetic( abs( atoi(args[3]) ) );
		break;
		case 1:
			pValue = GetWeaponFromSchema( args[3] );
		break;
	}
	
	if( !pValue )
		return;
	
	if( iCategory == 1 )
	{
		const char *szRegion = pValue->GetString( "region", "none" );
	
		pClass->RemoveSubKey( pClass->FindKey( szRegion ) );
	}
	else
	{
		pClass->RemoveSubKey( pClass->FindKey( args[4] ) );
	}

	GetLoadout()->SaveToFile( filesystem, "cfg/loadout.cfg" );
	
	switch( iCategory )
	{
		default:
		case 0:
			RefreshDesiredCosmetics( GetClassIndexFromString( args[2], TF_CLASS_COUNT_ALL ) );
		break;
		case 1:
			RefreshDesiredWeapons( GetClassIndexFromString( args[2], TF_CLASS_COUNT_ALL ) );
		break;
	}
}
static ConCommand loadout_unequip("loadout_unequip", LoadoutUnEquip );

#define BDAY_HAT_MODEL		"models/effects/bday_hat.mdl"
#define DM_SHIELD_MODEL 	"models/player/attachments/mercenary_shield.mdl"

IMaterial	*g_pHeadLabelMaterial[3] = { NULL, NULL, NULL }; 
void	SetupHeadLabelMaterials( void );

extern CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
										   const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params );

const char *pszHeadLabelNames[] =
{
	"effects/speech_voice_red",
	"effects/speech_voice_blue",
	"effects/speech_voice_mercenary"
};

Color TennisBall (0,255,0);

#define TF_PLAYER_HEAD_LABEL_RED 0
#define TF_PLAYER_HEAD_LABEL_BLUE 1
#define TF_PLAYER_HEAD_LABEL_MERCENARY 2

CLIENTEFFECT_REGISTER_BEGIN( PrecacheInvuln )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_blue.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_red.vmt" )
CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_mercenary.vmt" )
CLIENTEFFECT_REGISTER_END()

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		VPROF( "C_TEPlayerAnimEvent::PostDataUpdate" );

		// Create the effect.
		if ( m_iPlayerIndex == TF_PLAYER_INDEX_NONE )
			return;

		EHANDLE hPlayer = cl_entitylist->GetNetworkableHandle( m_iPlayerIndex );
		if ( !hPlayer )
			return;

		C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer* >( hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
		}	
	}

public:
	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

//-----------------------------------------------------------------------------
// Data tables and prediction tables.
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: Gore!
//-----------------------------------------------------------------------------

// Scale head to nothing
static void ScaleGoreHead( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_head", "prp_helmet", "prp_hat", "prp_cig" };

		for ( int i = 0; i < 4; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
	
}

// Scale left arm to nothing
static void ScaleGoreLeftArm( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_lowerArm_L", "bip_hand_L", "hlp_forearm_L", 
									"bip_thumb_0_L", "bip_index_0_L", "bip_middle_0_L", "bip_ring_0_L", "bip_pinky_0_L",
									"bip_thumb_1_L", "bip_index_1_L", "bip_middle_1_L", "bip_ring_1_L", "bip_pinky_1_L",
									"bip_thumb_2_L", "bip_index_2_L", "bip_middle_2_L", "bip_ring_2_L", "bip_pinky_2_L",
								  };

		for ( int i = 0; i < 18; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

// Scale left hand to nothing
static void ScaleGoreLeftHand( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_hand_L",
									"bip_thumb_0_L", "bip_index_0_L", "bip_middle_0_L", "bip_ring_0_L", "bip_pinky_0_L",
									"bip_thumb_1_L", "bip_index_1_L", "bip_middle_1_L", "bip_ring_1_L", "bip_pinky_1_L",
									"bip_thumb_2_L", "bip_index_2_L", "bip_middle_2_L", "bip_ring_2_L", "bip_pinky_2_L",
								  };

		for ( int i = 0; i < 16; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

// Scale right arm to nothing
static void ScaleGoreRightArm( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_lowerArm_R", "bip_hand_R", "hlp_forearm_R",
									"bip_thumb_0_R", "bip_index_0_R", "bip_middle_0_R", "bip_ring_0_R", "bip_pinky_0_R",
									"bip_thumb_1_R", "bip_index_1_R", "bip_middle_1_R", "bip_ring_1_R", "bip_pinky_1_R",
									"bip_thumb_2_R", "bip_index_2_R", "bip_middle_2_R", "bip_ring_2_R", "bip_pinky_2_R",
								  };

		for ( int i = 0; i < 18; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

// Scale right hand to nothing
static void ScaleGoreRightHand( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_hand_R",
									"bip_thumb_0_R", "bip_index_0_R", "bip_middle_0_R", "bip_ring_0_R", "bip_pinky_0_R",
									"bip_thumb_1_R", "bip_index_1_R", "bip_middle_1_R", "bip_ring_1_R", "bip_pinky_1_R",
									"bip_thumb_2_R", "bip_index_2_R", "bip_middle_2_R", "bip_ring_2_R", "bip_pinky_2_R",
								  };

		for ( int i = 0; i < 16; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

// Scale left knee to nothing
static void ScaleGoreLeftKnee( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_knee_L", "bip_foot_L", "bip_toe_L" };

		for ( int i = 0; i < 3; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

// Scale left foot to nothing
static void ScaleGoreLeftFoot( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_foot_L", "bip_toe_L" };

		for ( int i = 0; i < 2; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

// Scale right knee to nothing
static void ScaleGoreRightKnee( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_knee_r", "bip_foot_r", "bip_toe_R" };

		for ( int i = 0; i < 3; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

// Scale right foot to nothing
static void ScaleGoreRightFoot( C_BaseAnimating *pAnimating )
{
	if ( pAnimating )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_foot_R", "bip_toe_R" };

		for ( int i = 0; i < 2; i++ )
		{
			iBone = pAnimating->LookupBone( boneNames[i] );
			if ( iBone != -1 )
			  MatrixScaleBy( 0.001f, pAnimating->GetBoneForWrite( iBone ) );
		}
	}
}

//=============================================================================
//
// Ragdoll
//
// ----------------------------------------------------------------------------- //
// Client ragdoll entity.
// ----------------------------------------------------------------------------- //
ConVar cl_ragdoll_physics_enable( "cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics." );
ConVar cl_ragdoll_fade_time( "cl_ragdoll_fade_time", "15", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_forcefade( "cl_ragdoll_forcefade", "0", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_pronecheck_distance( "cl_ragdoll_pronecheck_distance", "64", FCVAR_GAMEDLL );

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_TFRagdoll, DT_TFRagdoll, CTFRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO(m_vecRagdollVelocity) ),
	RecvPropInt( RECVINFO( m_nForceBone ) ),
	RecvPropBool( RECVINFO( m_bGib ) ),
	RecvPropBool( RECVINFO( m_bBurning ) ),
	RecvPropInt( RECVINFO( m_iTeam ) ),
	RecvPropInt( RECVINFO( m_iClass ) ),
	RecvPropInt( RECVINFO( m_iDamageCustom ) ),
	RecvPropInt( RECVINFO( m_iGoreHead ) ),
	RecvPropInt( RECVINFO( m_iGoreLeftArm ) ),
	RecvPropInt( RECVINFO( m_iGoreRightArm ) ),
	RecvPropInt( RECVINFO( m_iGoreLeftLeg ) ),
	RecvPropInt( RECVINFO( m_iGoreRightLeg ) ),
	RecvPropBool( RECVINFO( m_bFlagOnGround ) ),
	RecvPropBool( RECVINFO( m_bDissolve ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::C_TFRagdoll()
{
	m_iPlayerIndex = TF_PLAYER_INDEX_NONE;
	m_fDeathTime = -1;
	m_bFadingOut = false;
	m_bGib = false;
	m_bBurning = false;
	m_flBurnEffectStartTime = 0.0f;
	m_iTeam = -1;
	m_iClass = -1;
	m_nForceBone = -1;
	m_flDeathAnimationTime = 0.0f;

	// takedamageinfo.h
	//m_bitsDamageType = 0;
	m_iDamageCustom = 0;

	m_bGoreEnabled = false;

	m_iGoreDecalAmount = 0;
	m_iGoreDecalBone = 0;
	m_fGoreDecalTime = -1;

	m_iGoreHead = 0;
	m_iGoreLeftArm = 0;
	m_iGoreRightArm = 0;
	m_iGoreLeftLeg = 0;
	m_iGoreRightLeg = 0;

	m_bFlagOnGround = false;
	m_bDissolve = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::~C_TFRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSourceEntity - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    	
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(),
				pDestEntry->watcher->GetDebugName() ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup vertex weights for drawing
//-----------------------------------------------------------------------------
void C_TFRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	// While we're dying, we want to mimic the facial animation of the player. Once they're dead, we just stay as we are.
	EHANDLE hPlayer = GetPlayerHandle();
	if ( ( hPlayer && hPlayer->IsAlive()) || !hPlayer )
	{
		BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
	else if ( hPlayer )
	{
		hPlayer->SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
}

	// Clientside animation
float C_TFRagdoll::FrameAdvance( float flInterval )
{
	// move frame forward
	//FrameAdvance(0.0f); // 0 means to use the time we last advanced instead of a constant
	// float flRet = 0;
	float flRet = BaseClass::FrameAdvance( flInterval );

	if ( m_flDeathAnimationTime != 0.0f && gpGlobals->curtime >= m_flDeathAnimationTime )
	{
		// Turn it into a ragdoll.
		if ( cl_ragdoll_physics_enable.GetBool() )
		{
			m_flDeathAnimationTime = 0.0f;
			// Make us a ragdoll..
			m_nRenderFX = kRenderFxRagdoll;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.05f;

			//if ( pPlayer && !pPlayer->IsDormant() && !bChangedModel )
			//{
			//	pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			//}
			//else
			//{
				GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			//}

			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );

			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			//SetAbsOrigin( /* m_vecRagdollOrigin : */ pPlayer->GetRenderOrigin() );			
			//SetAbsAngles( pPlayer->GetRenderAngles() );
			//SetAbsVelocity( m_vecRagdollVelocity );
			SetAbsVelocity( vec3_origin );
		}
		else
		{
				ClientLeafSystem()->SetRenderGroup( GetRenderHandle(), RENDER_GROUP_TRANSLUCENT_ENTITY );
		}
	}

	return flRet;
}

//-----------------------------------------------------------------------------
// Purpose: Scale the bones that need to be scaled for gore
//-----------------------------------------------------------------------------
void C_TFRagdoll::BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed );

	if ( m_bGoreEnabled )
		ScaleGoreBones();
}

void C_TFRagdoll::ScaleGoreBones()
{
	if ( m_iGoreHead > 1 )
		ScaleGoreHead( this );

	if ( m_iGoreLeftArm == 2 )
		ScaleGoreLeftHand( this );
	else if ( m_iGoreLeftArm == 3 )
		ScaleGoreLeftArm( this );

	if ( m_iGoreRightArm == 2 )
		ScaleGoreRightHand( this );
	else if ( m_iGoreRightArm == 3 )
		ScaleGoreRightArm( this );

	if ( m_iGoreLeftLeg == 2 )
		ScaleGoreLeftFoot( this );
	else if ( m_iGoreLeftLeg == 3 )
		ScaleGoreLeftKnee( this );

	if ( m_iGoreRightLeg == 2 )
		ScaleGoreRightFoot( this );
	else if ( m_iGoreRightLeg == 3 )
		ScaleGoreRightKnee( this );
}

void C_TFRagdoll::DismemberHead()
{
	m_iGoreHead = 3;

	int m_HeadBodygroup = FindBodygroupByName( "head" );

	if ( m_HeadBodygroup >= 0 )
		SetBodygroup( m_HeadBodygroup, 2 );

	int iAttach = LookupBone( "bip_neck" );

	if ( iAttach != -1 )
	{
		ParticleProp()->Create( "blood_decap", PATTACH_BONE_FOLLOW, "bip_neck" );
		ParticleProp()->Create( "env_sawblood_mist", PATTACH_BONE_FOLLOW, "bip_neck" );

		EmitSound( "TFPlayer.Decapitated" );

		m_iGoreDecalAmount += 15;
		m_iGoreDecalBone = iAttach;

	}
}

void C_TFRagdoll::DismemberBase( char const *szBodyPart, bool bLevel, bool bBloodEffects, char const *szParticleBone )
{
	int m_Bodygroup = FindBodygroupByName( szBodyPart );

	// Bodygroup 1 is reserved for blank models
	// So if we're already blank, don't change it
	if ( bLevel )
	{
		if ( m_Bodygroup >= 0 && GetBodygroup(m_Bodygroup) != 1 )
			SetBodygroup( m_Bodygroup, 3 );
	}
	else
	{
		if ( m_Bodygroup >= 0 && GetBodygroup(m_Bodygroup) != 1 )
			SetBodygroup( m_Bodygroup, 2 );
	}
/*
	for( int i = 0; i < m_hCosmetic.Count(); i++ )
	{
		m_Bodygroup = m_hCosmetic[i].Get()->FindBodygroupByName( szBodyPart );
		
		// Forward the bodygroup to the cosmetics regardless if the bodygroup is already blank
		if( m_Bodygroup >= 0 )
		{
			m_hCosmetic[i].Get()->SetBodygroup( m_Bodygroup, bLevel + 2 );
		}
	}
*/
	int iAttach = LookupBone( szParticleBone );

	if ( iAttach != -1 )
	{
		// I'm too lazy to make a new particle which is less bloody than blood_decap, but whatever, this works
		ParticleProp()->Create( "blood_decap_arterial_spray", PATTACH_BONE_FOLLOW, szParticleBone );
		ParticleProp()->Create( "env_sawblood_mist", PATTACH_BONE_FOLLOW, szParticleBone );
		ParticleProp()->Create( "env_sawblood_goop", PATTACH_BONE_FOLLOW, szParticleBone );
		ParticleProp()->Create( "env_sawblood_chunk", PATTACH_BONE_FOLLOW, szParticleBone );
		ParticleProp()->Create( "blood_impact_red_01_chunk", PATTACH_BONE_FOLLOW, szParticleBone );
	}

	if ( iAttach != -1 )
	{
		m_iGoreDecalAmount += 4;
		m_iGoreDecalBone = iAttach;

		EmitSound( "Flesh_Bloody.ImpactHard" );
	}
}

void C_TFRagdoll::DismemberLeftArm( bool bLevel )
{
	DismemberBase( "leftarm", bLevel, true, bLevel ? "bip_upperArm_L" : "bip_lowerArm_L" );
	int m_HandBodygroup = FindBodygroupByName( "lefthand" );
	
	if ( m_HandBodygroup >= 0 )
		SetBodygroup( m_HandBodygroup, 1 );

	if ( bLevel )
		m_iGoreLeftArm = 3;
	else
		m_iGoreLeftArm = 2;
}

void C_TFRagdoll::DismemberRightArm( bool bLevel )
{
	DismemberBase( "rightarm", bLevel, true, bLevel ? "bip_upperArm_R" : "bip_lowerArm_R" );
	int m_HandBodygroup = FindBodygroupByName( "righthand" );
	
	if ( m_HandBodygroup >= 0 )
		SetBodygroup( m_HandBodygroup, 1 );

	if ( bLevel )
		m_iGoreRightArm = 3;
	else
		m_iGoreRightArm = 2;
}

void C_TFRagdoll::DismemberLeftLeg( bool bLevel )
{
	DismemberBase( "leftleg", bLevel, true, bLevel ? "bip_knee_L" : "bip_foot_L" );

	if ( bLevel )
		m_iGoreLeftLeg = 3;
	else
		m_iGoreLeftLeg = 2;
}

void C_TFRagdoll::DismemberRightLeg( bool bLevel )
{
	DismemberBase( "rightleg", bLevel, true, bLevel ? "bip_knee_R" : "bip_foot_R" );

	if ( bLevel )
		m_iGoreRightLeg = 3;
	else
		m_iGoreRightLeg = 2;
}

void C_TFRagdoll::InitDismember()
{
	// HHH
	if ( m_iDamageCustom == TF_DMG_CUSTOM_DECAPITATION_BOSS )
		m_iGoreHead = 2;

	// head does not have two levels of dismemberment, only one
	if ( m_iGoreHead > 1 )
		DismemberHead();

	if ( m_iGoreLeftArm == 3 )
		DismemberLeftArm( true );
	else if ( m_iGoreLeftArm == 2 )
		DismemberLeftArm( false );

	if ( m_iGoreRightArm == 3 )
		DismemberRightArm( true );
	else if ( m_iGoreRightArm == 2 )
		DismemberRightArm( false );

	if ( m_iGoreLeftLeg == 3 )
		DismemberLeftLeg( true );
	else if ( m_iGoreLeftLeg == 2 )
		DismemberLeftLeg( false );

	if ( m_iGoreRightLeg == 3 )
		DismemberRightLeg( true );
	else if ( m_iGoreRightLeg == 2 )
		DismemberRightLeg( false );
}

void C_TFRagdoll::DismemberRandomLimbs( void )
{
	int iGore = 0;

	// NOTE: head is not dismembered here intentionally

	if ( m_iGoreLeftArm < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 2 )
			DismemberLeftArm( false );
		else if ( iGore == 3 )
			DismemberLeftArm( true );
	}

	if ( m_iGoreRightArm < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 2 )
			DismemberRightArm( false );
		else if ( iGore == 3 )
			DismemberRightArm( true );
	}

	if ( m_iGoreLeftLeg < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 2 )
			DismemberLeftLeg( false );
		else if ( iGore == 3 )
			DismemberLeftLeg( true );
	}

	if ( m_iGoreRightLeg < 3 )
	{
		iGore = random->RandomInt( 0,3 );

		if ( iGore == 2 )
			DismemberRightLeg( false );
		else if ( iGore == 3 )
			DismemberRightLeg( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTrace - 
//			iDamageType - 
//			*pCustomImpactName - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::ImpactTrace(trace_t *pTrace, int iDamageType, const char *pCustomImpactName)
{
	VPROF( "C_TFRagdoll::ImpactTrace" );

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
	{
		return;
	}

	Vector vecDir;

	VectorSubtract( pTrace->endpos, pTrace->startpos, vecDir );

	// m_iGore<limb> has a level, from 0 to 3
	// 1 is unused (reserved for normal TF bodygroups like pyro's head)
	// 2 means the lower limb is marked for removal, 3 means the upper limb is marked for removal, the head is an exception as it only has level 2
	// if our current level is at level 3, that means we can't dismember this limb anymore
	// if our current level is at level 2, that means we can dismember this limb once more up to level 3
	// if our current level is at level 0/1, that means we can dismember this limb up to level 2
	// Dismember<limb> function accepts true or false, true means this limb will be dismembered to level 3, false means dismembered to level 2

	if ( m_bGoreEnabled )
	{
		switch ( pTrace->hitgroup )
		{
			case HITGROUP_HEAD:
				if ( m_iGoreHead == 3 )
				{
					break;
				}
				else if ( m_iGoreHead == 2 )
				{
					break;
				}
				else
				{
					DismemberHead();
					break;
				}
			case HITGROUP_LEFTARM:
				if ( m_iGoreLeftArm == 3 )
				{
					break;
				}
				else if ( m_iGoreLeftArm == 2 )
				{
					DismemberLeftArm( true );
					break;
				}
				else
				{
					DismemberLeftArm( false );
					break;
				}
			case HITGROUP_RIGHTARM:
				if ( m_iGoreRightArm == 3 )
				{
					break;
				}
				else if ( m_iGoreRightArm == 2 )
				{
					DismemberRightArm( true );
					break;
				}
				else
				{
					DismemberRightArm( false );
					break;
				}
			case HITGROUP_LEFTLEG:
				if ( m_iGoreLeftLeg == 3 )
				{
					break;
				}
				else if ( m_iGoreLeftLeg == 2 )
				{
					DismemberLeftLeg( true );
					break;
				}
				else
				{
					DismemberLeftLeg( false );
					break;
				}
			case HITGROUP_RIGHTLEG:
				if ( m_iGoreRightLeg == 3 )
				{
					break;
				}
				else if ( m_iGoreRightLeg == 2 )
				{
					DismemberRightLeg( true );
					break;
				}
				else
				{
					DismemberRightLeg( false );
					break;
				}
			default:
				break;
			}
	}

	if ( iDamageType == DMG_BLAST )
	{
		// don't affect gibs
		if ( m_pRagdoll )
		{
			// Adjust the impact strength and apply the force at the center of mass.
			if ( iDamageType & DMG_CRITICAL )
				vecDir *= 4999999;
			else
				vecDir *= 2999999;
			pPhysicsObject->ApplyForceCenter( vecDir );
		}
		if ( m_bGoreEnabled )
			DismemberRandomLimbs();
	}
	else
	{
		// Find the apporx. impact point.
		Vector vecHitPos;  
		VectorMA( pTrace->startpos, pTrace->fraction, vecDir, vecHitPos );
		VectorNormalize( vecDir );

		// Adjust the impact strength and apply the force at the impact point..
		if (  m_pRagdoll )
		{
			if ( iDamageType & DMG_CRITICAL )
				vecDir *= 20000;
			else
				vecDir *= 5000;
		}
		else
		{
			vecDir *= 1000;
		}

		pPhysicsObject->ApplyForceOffset( vecDir, vecHitPos );	

		// make ragdolls emit blood decals
		// not a great way to do it, but it only works if all of it is defined here for some reason

		// make blood decal on the wall!
		trace_t Bloodtr;
		Vector vecTraceDir;
		float flNoise;
		int i;

		int cCount = 3;

		flNoise = 0.3;

		float flTraceDist = 172;

		for ( i = 0 ; i < cCount ; i++ )
		{
			vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

			vecTraceDir.x += random->RandomFloat( -flNoise, flNoise );
			vecTraceDir.y += random->RandomFloat( -flNoise, flNoise );
			vecTraceDir.z += random->RandomFloat( -flNoise, flNoise );

			// Don't bleed on grates.
			AI_TraceLine( pTrace->endpos, pTrace->endpos + vecTraceDir * -flTraceDist, MASK_SOLID_BRUSHONLY & ~CONTENTS_GRATE, this, COLLISION_GROUP_NONE, &Bloodtr);

			if ( Bloodtr.fraction != 1.0 )
			{
				UTIL_BloodDecalTrace( &Bloodtr, BLOOD_COLOR_RED );
			}
		}
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFRagdoll( void )
{
	// Get the player.
	C_TFPlayer *pPlayer = NULL;
	EHANDLE hPlayer = GetPlayerHandle();
	if ( hPlayer )
	{
		pPlayer = dynamic_cast<C_TFPlayer*>( hPlayer.Get() );
	}

	TFPlayerClassData_t *pData = nullptr;

	if ( pPlayer && pPlayer->GetPlayerClass() )
		pData = pPlayer->GetPlayerClass()->GetData();

	if ( pData )
	{
		int nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );

		if (  pPlayer && pPlayer->GetPlayerClass()->UsesCustomModel() )
			nModelIndex = modelinfo->GetModelIndex( pPlayer->GetPlayerClass()->GetSetCustomModel() );

		SetModelIndex( nModelIndex );

		int iVisibleTeam = m_iTeam < TF_TEAM_RED ? TF_TEAM_MERCENARY : m_iTeam;
		
		if ( iVisibleTeam == TF_TEAM_RED )
		{
			m_nSkin = 0;
		}
		else if ( iVisibleTeam == TF_TEAM_BLUE )
		{
			m_nSkin = 1;
		}
		else if ( iVisibleTeam == TF_TEAM_MERCENARY ) //mercenary
		{
			if ( of_tennisball.GetBool() && m_iClass == TF_CLASS_MERCENARY )
				m_nSkin = 6;
			else
				m_nSkin = 4;
		}
		else
		{
			m_nSkin = 0;
		}
	}

#ifdef _DEBUG
	DevMsg( 2, "CreateTFRagdoll %d %d\n", gpGlobals->framecount, pPlayer ? pPlayer->entindex() : 0 );
#endif
	
	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// Move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		RemoveEffects( EF_NOSHADOW );

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.		
		if ( !pPlayer->IsLocalPlayer() && pPlayer->IsInterpolationEnabled() )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( /* m_vecRagdollOrigin : */ pPlayer->GetRenderOrigin() );			
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( m_vecRagdollVelocity );

			// Hack! Find a neutral standing pose or use the idle.
			int iSeq = LookupSequence( "RagdollSpawn" );
			if ( iSeq == -1 )
			{
				Assert( false );
				iSeq = 0;
			}			
			SetSequence( iSeq );
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}

		m_nBody = pPlayer->GetBody();
		
		if( !of_disable_cosmetics.GetBool() )
		{
			for ( int i = 0; i < pPlayer->m_iCosmetics.Count(); i++ )
			{
				if ( pPlayer->m_iCosmetics[i] )
				{
					KeyValues *pCosmetic = GetCosmetic( pPlayer->m_iCosmetics[i] );
					if( !pCosmetic )
						continue;

					// can't draw headwear regions in firstperson ragdolls
					if ( cl_fp_ragdoll.GetBool() && pPlayer == C_TFPlayer::GetLocalTFPlayer() )
					{
						const char *pRegion = pCosmetic->GetString( "region" );
						if ( pRegion && ( !Q_strcmp( pRegion, "hat" ) || 
							 !Q_strcmp( pRegion, "face" ) || 
							 !Q_strcmp( pRegion, "glasses" ) ) )
						{
							continue;
						}
					}

					if ( Q_strcmp( pCosmetic->GetString( "Model" ), "BLANK" ) )
					{

						CosmeticHandle handle = C_PlayerAttachedModel::Create( pCosmetic->GetString( "Model" , "models/empty.mdl" ), this, LookupAttachment("partyhat"), vec3_origin, PAM_PERMANENT, 0, EF_BONEMERGE, false );	
						
						if( handle )
						{
							int iVisibleTeam = m_iTeam < TF_TEAM_RED ? TF_TEAM_MERCENARY : m_iTeam;
							handle->m_nSkin = iVisibleTeam - 2;
							
							m_hCosmetic.AddToTail(handle);
						}
					}
				}
			}
		}
	}
	else
	{
		// Overwrite network origin so later interpolation will use this position.
		SetNetworkOrigin( m_vecRagdollOrigin );
		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
	}	
	
	bool bPlayDeathAnim = false;
	int iRandom = random->RandomInt( 0 , 3 ); // 25% chance to play

	if ( pPlayer && m_bFlagOnGround && ( iRandom == 1 || tf_always_deathanim.GetBool() ) )
	{
		int iSeq = pPlayer->m_Shared.PlayDeathAnimation( this, m_iDamageCustom, m_bDissolve );

		if ( iSeq != -1 )
		{
			bPlayDeathAnim = true;

			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( pPlayer->GetNetworkOrigin() );
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( vec3_origin );
			m_vecForce = vec3_origin;

			// must be here otherwise the animation doesn't play!
			ForceClientSideAnimationOn();

			// Hack! Find a neutral standing pose or use the idle.
			//int iSeq = LookupSequence( "RagdollSpawn" );
			//if ( iSeq == -1 )
			//{
			//	Assert( false );
			//	iSeq = 0;
			//}	
			SetSequence( iSeq );
			m_flDeathAnimationTime = gpGlobals->curtime + SequenceDuration();
			SetCycle( 0.0f );

			// Stubs for weapon prediction
			ResetSequenceInfo();
		}
	}

	// Turn it into a ragdoll.
	if ( !bPlayDeathAnim )
	{
		if ( cl_ragdoll_physics_enable.GetBool() )
		{
			// Make us a ragdoll..
			m_nRenderFX = kRenderFxRagdoll;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.05f;

			// We have to make sure that we're initting this client ragdoll off of the same model.
			// GetRagdollInitBoneArrays uses the *player* Hdr, which may be a different model than
			// the ragdoll Hdr, if we try to create a ragdoll in the same frame that the player
			// changes their player model.
			CStudioHdr *pRagdollHdr = GetModelPtr();
			CStudioHdr *pPlayerHdr = NULL;
			if ( pPlayer )
					pPlayerHdr = pPlayer->GetModelPtr();

			bool bChangedModel = false;

			if ( pRagdollHdr && pPlayerHdr )
			{
				bChangedModel = pRagdollHdr->GetVirtualModel() != pPlayerHdr->GetVirtualModel();

				Assert( !bChangedModel && "C_TFRagdoll::CreateTFRagdoll: Trying to create ragdoll with a different model than the player it's based on" );
			}

			RemoveEffects( EF_NOSHADOW );

			if ( pPlayer && !pPlayer->IsDormant() && !bChangedModel )
			{
				pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			}
			else
			{
				GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			}

			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		else
		{
			ClientLeafSystem()->SetRenderGroup( GetRenderHandle(), RENDER_GROUP_TRANSLUCENT_ENTITY );
		}
	}

	int iBone = LookupBone( "bip_knee_L" );

	// enable dismemberment if we find this knee, as this is likely a TF2 compatible model then
	// blame the TFC civilian placeholder for needing to do this
	if ( iBone != -1 )
	{
		if ( of_gore.GetBool() )
			m_bGoreEnabled = true;
	}
	else
		m_bGoreEnabled = false;

	if ( m_bGoreEnabled )
		m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );

	if ( m_bBurning )
	{
		m_flBurnEffectStartTime = gpGlobals->curtime;
		ParticleProp()->Create( "burningplayer_corpse", PATTACH_ABSORIGIN_FOLLOW );
	}

	// Fade out the ragdoll in a while
	StartFadeOut( cl_ragdoll_fade_time.GetFloat() );

	//SetNextClientThink( gpGlobals->curtime + cl_ragdoll_fade_time.GetFloat() * 0.33f );

	// must think immediately for dismemberment
	SetNextClientThink( gpGlobals->curtime + 0.1f );

	// Birthday mode.
	if ( pPlayer && TFGameRules() && TFGameRules()->IsBirthday() )
	{
		AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
		breakablepropparams_t breakParams( m_vecRagdollOrigin, GetRenderAngles(), m_vecRagdollVelocity, angularImpulse );
		breakParams.impactEnergyScale = 1.0f;
		pPlayer->DropPartyHat( breakParams, m_vecRagdollVelocity.GetForModify() );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFGibs( void )
{
	C_TFPlayer *pPlayer = NULL;
	EHANDLE hPlayer = GetPlayerHandle();
	if ( hPlayer )
	{
		pPlayer = dynamic_cast<C_TFPlayer*>( hPlayer.Get() );
	}
	if ( pPlayer && ( pPlayer->m_hFirstGib == NULL ) )
	{
		Vector vecVelocity = m_vecForce + m_vecRagdollVelocity;
		VectorNormalize( vecVelocity );
		pPlayer->CreatePlayerGibs( m_vecRagdollOrigin, vecVelocity, m_vecForce.Length() );
	}

	if ( pPlayer )
	{
		if ( TFGameRules() && TFGameRules()->IsBirthday() )
		{
			DispatchParticleEffect( "bday_confetti", pPlayer->GetAbsOrigin() + Vector(0,0,32), vec3_angle );

			C_BaseEntity::EmitSound( "Game.HappyBirthday" );
		}
		else
			pPlayer->EmitSound( "Player.Gib" );
	}

	EndFadeOut();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		bool bCreateRagdoll = true;

		// Get the player.
		EHANDLE hPlayer = GetPlayerHandle();
		if ( hPlayer )
		{
			// If we're getting the initial update for this player (e.g., after resetting entities after
			//  lots of packet loss, then don't create gibs, ragdolls if the player and it's gib/ragdoll
			//  both show up on same frame.
			if ( abs( hPlayer->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}
		else if ( C_BasePlayer::GetLocalPlayer() )
		{
			// Ditto for recreation of the local player
			if ( abs( C_BasePlayer::GetLocalPlayer()->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}	
		
		if ( bCreateRagdoll )
		{
			if ( m_bGib )
			{
				CreateTFGibs();
			}
			else
			{
				CreateTFRagdoll();

				if ( of_gore.GetBool() && m_bGoreEnabled )
				{
					InitDismember();
				}
			}
		}
		C_TFPlayer *pPlayer = NULL;
		if ( hPlayer )
		{
			pPlayer = dynamic_cast<C_TFPlayer*>( hPlayer.Get() );
		}
		if( pPlayer )
		{
			for ( int i = 0; i < pPlayer->m_hCosmetic.Count(); i++ )
			{
				if ( pPlayer->m_hCosmetic[i] )
				{
					pPlayer->m_hCosmetic[i]->Release();
				}
			}
		}
	}
	else 
	{
		if ( !cl_ragdoll_physics_enable.GetBool() )
		{
			// Don't let it set us back to a ragdoll with data from the server.
			m_nRenderFX = kRenderFxNone;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll* C_TFRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFRagdoll::IsRagdollVisible()
{
	Vector vMins = Vector(-1,-1,-1);	//WorldAlignMins();
	Vector vMaxs = Vector(1,1,1);	//WorldAlignMaxs();
		
	Vector origin = GetAbsOrigin();
	
	if( !engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin) )
	{
		return false;
	}
	else if( engine->CullBox( vMins + origin, vMaxs + origin ) )
	{
		return false;
	}

	return true;
}

void C_TFRagdoll::ClientThink( void )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	if ( m_bFadingOut == true )
	{
		int iAlpha = GetRenderColor().a;
		int iFadeSpeed = 600.0f;

		iAlpha = max( iAlpha - ( iFadeSpeed * gpGlobals->frametime ), 0 );

		SetRenderMode( kRenderTransAlpha );
		SetRenderColorA( iAlpha );

		if ( iAlpha == 0 )
		{
			EndFadeOut(); // remove clientside ragdoll
		}

		return;
	}

	// if the player is looking at us, delay the fade
	if ( IsRagdollVisible() )
	{
		if ( cl_ragdoll_forcefade.GetBool() )
		{
			m_bFadingOut = true;
			float flDelay = cl_ragdoll_fade_time.GetFloat() * 0.33f;
			m_fDeathTime = gpGlobals->curtime + flDelay;

			// If we were just fully healed, remove all decals
			RemoveAllDecals();
		}

		// emit some blood decals if necessary
		if ( m_iGoreDecalAmount > 0 && m_fGoreDecalTime < gpGlobals->curtime )
		{
			// emit another decal again after 0.1 seconds
			m_fGoreDecalTime = gpGlobals->curtime + 0.1f;
			m_iGoreDecalAmount--;

			if ( m_iGoreDecalBone != -1 )
			{
				Vector direction;
				Vector start;
				QAngle dummy;
				trace_t	tr;

				GetBonePosition( m_iGoreDecalBone, start, dummy );

				// any random direction
				direction.x = random->RandomFloat ( -32, 32 );
				direction.y = random->RandomFloat ( -32, 32 );
				direction.z = random->RandomFloat ( -32, 32 );

				UTIL_TraceLine ( start, start + direction, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
				UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );

				//debugoverlay->AddLineOverlay( start, start + direction, 0, 255, 0, true, 1 ); 
			}
		}

		StartFadeOut( cl_ragdoll_fade_time.GetFloat() * 0.33f );
		return;
	}

	if ( m_fDeathTime > gpGlobals->curtime )
		return;

	EndFadeOut(); // remove clientside ragdoll
}

void C_TFRagdoll::StartFadeOut( float fDelay )
{
	if ( !cl_ragdoll_forcefade.GetBool() )
	{
		m_fDeathTime = gpGlobals->curtime + fDelay;
	}
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_TFRagdoll::EndFadeOut()
{
	for ( int i = 0; i < m_hCosmetic.Count(); i++ )
	{
		if ( m_hCosmetic[i] )
		{
			m_hCosmetic[i]->SetNextClientThink( CLIENT_THINK_NEVER );
			m_hCosmetic[i]->Release();
		}
	}
	m_hCosmetic.Purge();
	SetNextClientThink( CLIENT_THINK_NEVER );
	ClearRagdoll();
	SetRenderMode( kRenderNone );
	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CSpyInvisProxy : public CEntityMaterialProxy
{
public:
						CSpyInvisProxy( void );
	virtual				~CSpyInvisProxy( void );
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial *	GetMaterial();

private:

	IMaterialVar		*m_pPercentInvisible;
	IMaterialVar		*m_pCloakColorTint;
};

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CInvisProxy : public CSpyInvisProxy
{
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSpyInvisProxy::CSpyInvisProxy( void )
{
	m_pPercentInvisible = NULL;
	m_pCloakColorTint = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSpyInvisProxy::~CSpyInvisProxy( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CSpyInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bInvis;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bInvis );

	bool bTint;
	m_pCloakColorTint = pMaterial->FindVar( "$cloakColorTint", &bTint );

	return ( bInvis && bTint );
}

ConVar tf_teammate_max_invis( "tf_teammate_max_invis", "0.95", FCVAR_CHEAT );
ConVar tf_vm_min_invis( "tf_vm_min_invis", "0.22", FCVAR_CHEAT, "Minimum invisibility value for view model", true, 0.0, true, 1.0 );
ConVar tf_vm_max_invis( "tf_vm_max_invis", "0.5", FCVAR_CHEAT, "Maximum invisibility value for view model", true, 0.0, true, 1.0 );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CSpyInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if( !m_pPercentInvisible || !m_pCloakColorTint )
		return;

	if ( !pEnt )
		return;

//################################


	CTFViewModel *pVM = dynamic_cast<CTFViewModel *>( pEnt );
	if ( pVM )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( pVM->GetOwner() );

		if ( !pPlayer )
		{
			m_pPercentInvisible->SetFloatValue( 0.0f );
			return;
		}

		float flPercentInvisible = pPlayer->GetPercentInvisible();

		// remap from 0.22 to 0.5
		// but drop to 0.0 if we're not invis at all
		float flWeaponInvis = ( flPercentInvisible < 0.01 ) ?
			0.0 :
			RemapVal( flPercentInvisible, 0.0, 1.0, tf_vm_min_invis.GetFloat(), tf_vm_max_invis.GetFloat() );

		m_pPercentInvisible->SetFloatValue( flWeaponInvis );
		float r, g, b;

		switch( pPlayer->GetTeamNumber() )
		{
			case TF_TEAM_RED:
				r = 1.0; g = 0.5; b = 0.4;
				break;

			case TF_TEAM_BLUE:
				r = 0.4; g = 0.5; b = 1.0;
				break;
			case TF_TEAM_MERCENARY:
				{
					Vector Color = pPlayer->m_vecPlayerColor;
					r = Color.x; 
					g = Color.y; 
					b = Color.z; 
				}
				break;
			default:
				r = 0.4; g = 0.5; b = 1.0;
				break;
		}

		m_pCloakColorTint->SetVecValue( r, g, b );
		return;
	}
//################################	
	C_TFPlayer *pPlayer = ToTFPlayer( pEnt );

	if ( !pPlayer )
	{
		pPlayer = ToTFPlayer( pEnt->GetMoveParent() ); // For when ficool inevidably parents shit to the player again
	}

	if ( pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
	}
	else
	{
		m_pPercentInvisible->SetFloatValue( 0.0 );
		return;
	}
	
	float r, g, b;	
	
	switch( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			r = 1.0; g = 0.5; b = 0.4;
			break;

		case TF_TEAM_BLUE:
			r = 0.4; g = 0.5; b = 1.0;
			break;
		case TF_TEAM_MERCENARY:
				{
					Vector Color = pPlayer->m_vecPlayerColor;				
					r = Color.x; 
					g = Color.y; 
					b = Color.z; 
				}
			break;
		default:
			r = 0.4; g = 0.5; b = 1.0;
			break;
	}

		m_pCloakColorTint->SetVecValue( r, g, b );
		return;

}

IMaterial *CSpyInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}

EXPOSE_INTERFACE( CSpyInvisProxy, IMaterialProxy, "spy_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

// Dummy to support live tf2 invis stuff, since drew keeps putting "invis" in his items instead of "spy_invis"
EXPOSE_INTERFACE( CInvisProxy, IMaterialProxy, "invis" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for invulnerability material
//			Returns 1 if the player is invulnerable, and 0 if the player is losing / doesn't have invuln.
//-----------------------------------------------------------------------------
class CProxyInvulnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_TFPlayer *pPlayer = NULL;
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;
		
		if ( pEntity->IsPlayer() )
		{
			pPlayer = dynamic_cast< C_TFPlayer* >( pEntity );
		}
		else
		{
			// See if it's a weapon
			C_TFWeaponBase *pWeapon = dynamic_cast< C_TFWeaponBase* >( pEntity );
			C_PlayerAttachedModel *pCosmetic = dynamic_cast< C_PlayerAttachedModel* >( pEntity );
			if ( pWeapon )
			{
				pPlayer = (C_TFPlayer*)pWeapon->GetOwner();
			}
			else if ( pCosmetic )
			{
				pPlayer = dynamic_cast<C_TFPlayer*>(pCosmetic->GetMoveParent());
			}
			else
			{
				C_BaseViewModel *pVM = dynamic_cast< C_BaseViewModel* >( pEntity );
				if ( pVM )
				{
					pPlayer = dynamic_cast<C_TFPlayer*>(pVM->GetOwner());
				}
				else
				{
					pPlayer = ToTFPlayer( pEntity->GetMoveParent() );
				}
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->m_Shared.InCondUber() && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF ) )
			{
				m_pResult->SetFloatValue( 1.0 );
			}
			else
			{
				m_pResult->SetFloatValue( 0.0 );
			}
		}

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyInvulnLevel, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for burning material on player models
//			Returns 0.0->1.0 for level of burn to show on player skin
//-----------------------------------------------------------------------------
class CProxyBurnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( !pC_BaseEntity )
			return;

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		// default to zero
		float flBurnStartTime = 0;
			
		C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer* >( pEntity );
		if ( pPlayer )		
		{
			// is the player burning?
			if (  pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
			{
				flBurnStartTime = pPlayer->m_flBurnEffectStartTime;
			}
		}
		else
		{
			// is the ragdoll burning?
			C_TFRagdoll *pRagDoll = dynamic_cast< C_TFRagdoll* >( pEntity );
			if ( pRagDoll )
			{
				flBurnStartTime = pRagDoll->GetBurnStartTime();
			}
		}

		float flResult = 0.0;
		
		// if player/ragdoll is burning, set the burn level on the skin
		if ( flBurnStartTime > 0 )
		{
			float flBurnPeakTime = flBurnStartTime + 0.3;
			float flTempResult;
			if ( gpGlobals->curtime < flBurnPeakTime )
			{
				// fade in from 0->1 in 0.3 seconds
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnStartTime, flBurnPeakTime, 0.0, 1.0 );
			}
			else
			{
				// fade out from 1->0 in the remaining time until flame extinguished
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnPeakTime, flBurnStartTime + TF_BURNING_FLAME_LIFE, 1.0, 0.0 );
			}	

			// We have to do some more calc here instead of in materialvars.
			flResult = 1.0 - abs( flTempResult - 1.0 );
		}

		m_pResult->SetFloatValue( flResult );

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyBurnLevel, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: dummy to stop live tf2 materials from going black
//-----------------------------------------------------------------------------
class CProxyYellowLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		// stop stuff from going black
		m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
	}
};

EXPOSE_INTERFACE(CProxyYellowLevel, IMaterialProxy, "YellowLevel" IMATERIAL_PROXY_INTERFACE_VERSION);

class CProxyModelGlowColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( !pC_BaseEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		Vector vecColor = Vector( 1, 1, 1 );

		C_TFPlayer *pPlayer = ToTFPlayer( pEntity );;

		if ( !pPlayer )
		{
			C_BaseCombatWeapon *pWeapon = pEntity->MyCombatWeaponPointer();
			if ( pWeapon )
			{
				pPlayer = ToTFPlayer( pWeapon->GetOwner() );
			}
			else
			{
				C_BaseViewModel *pVM = dynamic_cast<C_BaseViewModel *>( pEntity );
				if ( pVM )
				{
					pPlayer = ToTFPlayer( pVM->GetOwner() );
				}
			}
		}	
		
		if ( pPlayer  && pPlayer->m_Shared.InCondCrit()  )
		{
			if ( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || pPlayer->GetTeamNumber() == pPlayer->m_Shared.GetDisguiseTeam() || !pPlayer->IsEnemyPlayer() )
			{
				switch ( pPlayer->GetTeamNumber() )
				{
				case TF_TEAM_RED:
					vecColor = Vector( 94, 8, 5 );
					break;
				case TF_TEAM_BLUE:
					vecColor = Vector( 6, 21, 80 );
					break;
				case TF_TEAM_MERCENARY:
				{
					Vector critColor = pPlayer->m_vecPlayerColor;
					critColor *= 255;
					critColor *= of_critglow_saturation.GetFloat();
					vecColor = critColor;
				}
					break;

				}
			}
		}
		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};

EXPOSE_INTERFACE( CProxyModelGlowColor, IMaterialProxy, "ModelGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

class CProxyItemTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( of_tennisball.GetBool() )
		{	
			float r = floorf( TennisBall[0] ) / 255.0f;
			float g = floorf( TennisBall[1] ) / 255.0f;
			float b = floorf( TennisBall[2] ) / 255.0f;	
			m_pResult->SetVecValue( r, g, b );
			return;
		}		
		
		if ( !pC_BaseEntity )
		{

			float r = floorf( of_color_r.GetFloat() ) / 255.0f;
			float g = floorf( of_color_g.GetFloat() ) / 255.0f;
			float b = floorf( of_color_b.GetFloat() ) / 255.0f;
			m_pResult->SetVecValue( r, g, b );
			return;
		
		}
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;

		Vector vecColor = pEntity->GetItemTintColor();

		if ( vecColor == vec3_origin )
		{
			C_BaseEntity *pOwner = pEntity->GetItemTintColorOwner(); ;
			while( pOwner && pOwner->GetItemTintColorOwner() != NULL )
			{
				pOwner = pOwner->GetItemTintColorOwner();
				if( pOwner == pOwner->GetItemTintColorOwner() )
					break;
			}
			if ( pOwner )
			{
				vecColor = pOwner->GetItemTintColor();
			}
		}
		m_pResult->SetVecValue( vecColor.x, vecColor.y, vecColor.z );
		return;


		m_pResult->SetVecValue( 1, 1, 1 );
	}
};

EXPOSE_INTERFACE( CProxyItemTintColor, IMaterialProxy, "ItemTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );

class CProxyLocalPlayerColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		if ( of_tennisball.GetBool() )
		{	
			float r = floorf( TennisBall[0] ) / 255.0f;
			float g = floorf( TennisBall[1] ) / 255.0f;
			float b = floorf( TennisBall[2] ) / 255.0f;	
			m_pResult->SetVecValue( r, g, b );
			return;
		}		
		
		float r = floorf( of_color_r.GetFloat() ) / 255.0f;
		float g = floorf( of_color_g.GetFloat() ) / 255.0f;
		float b = floorf( of_color_b.GetFloat() ) / 255.0f;
		m_pResult->SetVecValue( r, g, b );
	}
};

EXPOSE_INTERFACE( CProxyLocalPlayerColor, IMaterialProxy, "LocalPlayerColor" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for invulnerability material
//			Returns 1 if the player is invulnerable, and 0 if the player is losing / doesn't have invuln.
//-----------------------------------------------------------------------------
class CProxyReserveAmmo : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_TFPlayer *pPlayer = NULL;
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;
		
		if ( pEntity->IsPlayer() )
		{
			pPlayer = dynamic_cast< C_TFPlayer* >( pEntity );
		}
		else
		{
			// See if it's a weapon
			C_TFWeaponBase *pWeapon = dynamic_cast< C_TFWeaponBase* >( pEntity );
			C_PlayerAttachedModel *pCosmetic = dynamic_cast< C_PlayerAttachedModel* >( pEntity );
			if ( pWeapon )
			{
				pPlayer = dynamic_cast< C_TFPlayer* >(pWeapon->GetOwner());
			}
			else if ( pCosmetic )
			{
				pPlayer = dynamic_cast< C_TFPlayer* >(pCosmetic->GetMoveParent());
			}
			else
			{
				C_BaseViewModel *pVM = dynamic_cast< C_BaseViewModel* >( pEntity );
				if ( pVM )
				{
					pPlayer = dynamic_cast< C_TFPlayer* >(pVM->GetOwner());
				}
				else
				{
					pPlayer = ToTFPlayer( pEntity->GetMoveParent() );
				}
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->GetActiveTFWeapon() )
				m_pResult->SetFloatValue( (float)pPlayer->GetActiveTFWeapon()->m_iReserveAmmo / (float)pPlayer->GetActiveTFWeapon()->GetMaxReserveAmmo() );
		}

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyReserveAmmo, IMaterialProxy, "ReserveAmmo" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for invulnerability material
//			Returns 1 if the player is invulnerable, and 0 if the player is losing / doesn't have invuln.
//-----------------------------------------------------------------------------
class CProxyClipCount : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_TFPlayer *pPlayer = NULL;
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;
		
		if ( pEntity->IsPlayer() )
		{
			pPlayer = dynamic_cast< C_TFPlayer* >( pEntity );
		}
		else
		{
			// See if it's a weapon
			C_TFWeaponBase *pWeapon = dynamic_cast< C_TFWeaponBase* >( pEntity );
			C_PlayerAttachedModel *pCosmetic = dynamic_cast< C_PlayerAttachedModel* >( pEntity );
			if ( pWeapon )
			{
				pPlayer = (C_TFPlayer*)pWeapon->GetOwner();
			}
			else if ( pCosmetic )
			{
				pPlayer = dynamic_cast< C_TFPlayer* >(pCosmetic->GetMoveParent());
			}
			else
			{
				C_BaseViewModel *pVM = dynamic_cast< C_BaseViewModel* >( pEntity );
				if ( pVM )
				{
					pPlayer = (C_TFPlayer*)pVM->GetOwner();
				}
				else
				{
					pPlayer = ToTFPlayer( pEntity->GetMoveParent() );
				}
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->GetActiveTFWeapon() )
				m_pResult->SetFloatValue( (float)pPlayer->GetActiveTFWeapon()->Clip1() / (float)pPlayer->GetActiveTFWeapon()->GetMaxClip1() );
		}

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyClipCount, IMaterialProxy, "ClipCount" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for rage material
//			Returns 0 if the player is in Berserk, and 1 if the player is not.
//			I know.. Its confusing
//-----------------------------------------------------------------------------
class CProxyRage : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		m_pResult->SetFloatValue( 1.0 );
		Assert( m_pResult );

		C_TFPlayer *pPlayer = NULL;
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
			return;
		
		if ( pEntity->IsPlayer() )
		{
			pPlayer = dynamic_cast< C_TFPlayer* >( pEntity );
		}
		else
		{
			// See if it's a weapon
			C_TFWeaponBase *pWeapon = dynamic_cast< C_TFWeaponBase* >( pEntity );
			C_PlayerAttachedModel *pCosmetic = dynamic_cast< C_PlayerAttachedModel* >( pEntity );
			if ( pWeapon )
			{
				pPlayer = (C_TFPlayer*)pWeapon->GetOwner();
			}
			else if ( pCosmetic )
			{
				pPlayer = dynamic_cast< C_TFPlayer* >(pCosmetic->GetMoveParent());
			}
			else
			{
				C_BaseViewModel *pVM = dynamic_cast< C_BaseViewModel* >( pEntity );
				if ( pVM )
				{
					pPlayer = (C_TFPlayer*)pVM->GetOwner();
				}
				else
				{
					pPlayer = ToTFPlayer( pEntity->GetMoveParent() );
				}
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_BERSERK ))
			{
				m_pResult->SetFloatValue( 0.0 );
			}
			else
			{
				m_pResult->SetFloatValue( 1.0 );
			}
		}

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};

EXPOSE_INTERFACE( CProxyRage, IMaterialProxy, "RipAndTear" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Player's object UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_PlayerObjectList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFPlayer *pPlayer = (C_TFPlayer*)pStruct;
	CBaseHandle *pHandle = (CBaseHandle*)(&(pPlayer->m_aObjects[pData->m_iElement])); 
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );
}

void RecvProxyArrayLength_PlayerObjects( void *pStruct, int objectID, int currentArrayLength )
{
	C_TFPlayer *pPlayer = (C_TFPlayer*)pStruct;

	if ( pPlayer->m_aObjects.Count() != currentArrayLength )
	{
		pPlayer->m_aObjects.SetSize( currentArrayLength );
	}

	pPlayer->ForceUpdateObjectHudState();
}

// specific to the local player
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropArray2( 
		RecvProxyArrayLength_PlayerObjects,
		RecvPropInt( "player_object_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerObjectList ), 
		MAX_OBJECTS_PER_PLAYER, 
		0, 
		"player_object_array"	),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
//	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

END_RECV_TABLE()

// all players except the local player
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFNonLocalPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_TFPlayer, DT_TFPlayer, CTFPlayer )

    // medic call
	RecvPropBool(RECVINFO(m_bSaveMeParity)),
	// stuff when writing a chat message
	RecvPropBool(RECVINFO(m_bChatting)),
	RecvPropBool(RECVINFO(m_bRetroMode)),
	RecvPropBool(RECVINFO(m_bHauling)),

	// This will create a race condition will the local player, but the data will be the same so.....
	RecvPropInt( RECVINFO( m_nWaterLevel ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropDataTable( RECVINFO_DT( m_PlayerClass ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerClassShared ) ),
	RecvPropDataTable( RECVINFO_DT( m_Shared ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerShared ) ),

	RecvPropEHandle( RECVINFO(m_hItem ) ),
	
	RecvPropVector( RECVINFO( m_vecPlayerColor ) ),
	
	RecvPropVector( RECVINFO( m_vecViewmodelOffset ) ),
	RecvPropVector( RECVINFO( m_vecViewmodelAngle ) ),
	
	RecvPropBool( RECVINFO( m_bCentered ) ),
	RecvPropBool( RECVINFO( m_bMinimized ) ),

	RecvPropDataTable( "tflocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFLocalPlayerExclusive) ),
	RecvPropDataTable( "tfnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFNonLocalPlayerExclusive) ),

	RecvPropInt( RECVINFO( m_iSpawnCounter ) ),
	
	RecvPropInt( RECVINFO( m_bResupplied ) ),

	RecvPropInt( RECVINFO( m_iAccount ) ),

	RecvPropUtlVector( RECVINFO_UTLVECTOR( m_iCosmetics ), 32, RecvPropInt(NULL, 0, sizeof(int)) ),

END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_TFPlayer )
	DEFINE_PRED_TYPEDESCRIPTION( m_Shared, CTFPlayerShared ),
	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nMuzzleFlashParity, FIELD_CHARACTER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE  ),
	DEFINE_PRED_FIELD( m_hOffHandWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// ------------------------------------------------------------------------------------------ //
// C_TFPlayer implementation.
// ------------------------------------------------------------------------------------------ //

C_TFPlayer::C_TFPlayer() : 
	m_iv_angEyeAngles( "C_TFPlayer::m_iv_angEyeAngles" )
{
	m_PlayerAnimState = CreateTFPlayerAnimState( this );
	m_Shared.Init( this );

	m_iIDEntIndex = 0;

	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_pTeleporterEffect = NULL;
	m_pBurningSound = NULL;
	m_pBurningEffect = NULL;
	m_flBurnEffectStartTime = 0;
	m_flBurnEffectEndTime = 0;
	m_pDisguisingEffect = NULL;
	m_pSaveMeEffect = NULL;

	m_pChattingEffect = NULL;
	m_bChatting = false;

	m_aGibs.Purge();

	m_bCigaretteSmokeActive = false;

	m_hRagdoll.Set( NULL );

	m_iPreviousMetal = 0;
	m_bIsDisplayingNemesisIcon = false;

	m_bWasTaunting = false;
	bInitialSpawn = false;

	//m_bPlayingMusic = false;
	//m_fMusicDuration = 0.0f;

	m_angTauntPredViewAngles.Init();
	m_angTauntEngViewAngles.Init();

	m_flWaterImpactTime = 0.0f;

	m_flWaterEntryTime = 0;
	m_nOldWaterLevel = WL_NotInWater;
	m_bWaterExitEffectActive = false;

	m_bUpdateObjectHudState = false;
	
	iCosmeticCount = 0;

	ListenForGameEvent( "player_jump" );

	//LoadMapMusic(::filesystem);

	m_blinkTimer.Invalidate();
}

C_TFPlayer::~C_TFPlayer()
{
	ShowNemesisIcon( false );
	m_PlayerAnimState->Release();
}


C_TFPlayer* C_TFPlayer::GetLocalTFPlayer()
{
	return ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
}

const QAngle& C_TFPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateOnRemove( void )
{
	// Stop the taunt.
	if ( m_bWasTaunting )
	{
		TurnOffTauntCam();
	}

	// HACK!!! ChrisG needs to fix this in the particle system.
	ParticleProp()->OwnerSetDormantTo( true );
	ParticleProp()->StopParticlesInvolving( this );

	m_Shared.RemoveAllCond( this );

	if ( IsLocalPlayer() )
	{
		CTFStatPanel *pStatPanel = GetStatPanel();
		pStatPanel->OnLocalPlayerRemove( this );
	}

	/*
	int surplus = g_Mags.Count() - 2;

	if (surplus <= 0)
		return;

	// clear out any magazines
	int i;

	C_FadingPhysPropClientside *pCandidate;
	for ( i = 0; i < g_Mags.Count(); i++ )
	{
		pCandidate = g_Mags[i];
		Assert( !pCandidate->IsEffectActive( EF_NORECEIVESHADOW ) );

		g_Mags.FastRemove( i );

		pCandidate->AddEffects( EF_NORECEIVESHADOW );

		pCandidate->StartFadeOut( 0.1 );
	}
	*/

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose: returns max health for this player
//-----------------------------------------------------------------------------
int C_TFPlayer::GetMaxHealth( void ) const
{	
	if ( g_PR )
	{
		C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
		if ( tf_PR )
		{
			int index = ( (C_BasePlayer *) this )->entindex();
			return tf_PR->GetMaxHealth( index );
		}
	}
	return 1;
}

//-----------------------------------------------------------------------------
// Deal with recording
//-----------------------------------------------------------------------------
void C_TFPlayer::GetToolRecordingState( KeyValues *msg )
{
#ifndef _XBOX
	BaseClass::GetToolRecordingState( msg );
	BaseEntityRecordingState_t *pBaseEntityState = (BaseEntityRecordingState_t*)msg->GetPtr( "baseentity" );

	bool bDormant = IsDormant();
	bool bDead = !IsAlive();
	bool bSpectator = ( GetTeamNumber() == TEAM_SPECTATOR );
	bool bNoRender = ( GetRenderMode() == kRenderNone );
	bool bDeathCam = (GetObserverMode() == OBS_MODE_DEATHCAM);
	bool bNoDraw = IsEffectActive(EF_NODRAW);

	bool bVisible = 
		!bDormant && 
		!bDead && 
		!bSpectator &&
		!bNoRender &&
		!bDeathCam &&
		!bNoDraw;

	bool changed = m_bToolRecordingVisibility != bVisible;
	// Remember state
	m_bToolRecordingVisibility = bVisible;

	pBaseEntityState->m_bVisible = bVisible;
	if ( changed && !bVisible )
	{
		// If the entity becomes invisible this frame, we still want to record a final animation sample so that we have data to interpolate
		//  toward just before the logs return "false" for visiblity.  Otherwise the animation will freeze on the last frame while the model
		//  is still able to render for just a bit.
		pBaseEntityState->m_bRecordFinalVisibleSample = true;
	}
#endif
}


void C_TFPlayer::UpdateClientSideAnimation()
{
	// Update the animation data. It does the local check here so this works when using
	// a third-person camera (and we don't have valid player angles).
	if ( this == C_TFPlayer::GetLocalTFPlayer() )
		m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );
	else
		m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetDormant( bool bDormant )
{
	// If I'm burning, stop the burning sounds
	if ( !IsDormant() && bDormant )
	{
		if ( m_pBurningSound ) 
		{
			StopBurningSound();
		}
		if ( m_bIsDisplayingNemesisIcon )
		{
			ShowNemesisIcon( false );
		}
	}

	if ( IsDormant() && !bDormant )
	{
		m_bUpdatePlayerAttachments = true;
		m_bUpdateCosmetics = true;
	}

	// Deliberately skip base combat weapon
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldHealth = m_iHealth;
	m_iOldPlayerClass = m_PlayerClass.GetClassIndex();
	m_iOldState = m_Shared.GetCond();
	m_iOldSpawnCounter = m_iSpawnCounter;
	m_bOldResupplied = m_bResupplied;
	
	m_bOldSaveMeParity = m_bSaveMeParity;
	m_nOldWaterLevel = GetWaterLevel();

	m_iOldTeam = GetTeamNumber();
	C_TFPlayerClass *pClass = GetPlayerClass();
	m_iOldClass = pClass ? pClass->GetClassIndex() : TF_CLASS_UNDEFINED;
	m_bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
	m_iOldDisguiseTeam = m_Shared.GetDisguiseTeam();
	m_iOldDisguiseClass = m_Shared.GetDisguiseClass();

	m_Shared.OnPreDataChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );
	
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		InitInvulnerableMaterial();
	}
	else
	{
		if ( m_iOldTeam != GetTeamNumber() || m_iOldDisguiseTeam != m_Shared.GetDisguiseTeam() )
		{
			InitInvulnerableMaterial();
			m_bUpdatePlayerAttachments = true;
			m_bUpdateCosmetics = true;
		}
	}

	UpdateVisibility();
	
	// Check for full health and remove decals.
	if ( ( m_iHealth > m_iOldHealth && m_iHealth >= GetMaxHealth() ) || m_Shared.InCondUber() )
	{
		// If we were just fully healed, remove all decals
		RemoveAllDecals();
	}

	// Detect class changes
	if ( m_iOldPlayerClass != m_PlayerClass.GetClassIndex() )
	{
		OnPlayerClassChange();
	}

	bool bJustSpawned = false;

	if ( m_iOldSpawnCounter != m_iSpawnCounter )
	{
		ClientPlayerRespawn();

		bJustSpawned = true;
		m_bUpdatePlayerAttachments = true;
		m_bUpdateCosmetics = true;
	}
	
	if ( m_bOldResupplied != m_bResupplied )
	{
		m_bUpdateCosmetics = true;
		m_bOldResupplied = m_bResupplied;
	}
	
	if( bCosmeticsDisabled != of_disable_cosmetics.GetBool() )
	{
		m_bUpdateCosmetics = true;
		bCosmeticsDisabled = of_disable_cosmetics.GetBool();
	}
	
	// update the chat bubble on the player (when he is typing a chat message)
	CreateChattingEffect();

	if ( m_bSaveMeParity != m_bOldSaveMeParity )
	{
		// Player has triggered a save me command
		CreateSaveMeEffect();
	}

	if ( m_Shared.InCond( TF_COND_BURNING ) && !m_pBurningSound )
	{
		StartBurningSound();
	}

	// See if we should show or hide nemesis icon for this player
	bool bShouldDisplayNemesisIcon = ShouldShowNemesisIcon();
	if ( bShouldDisplayNemesisIcon != m_bIsDisplayingNemesisIcon )
	{
		ShowNemesisIcon( bShouldDisplayNemesisIcon );
	}

	m_Shared.OnDataChanged();

	
	if ( m_bDisguised != m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		m_flDisguiseEndEffectStartTime = max( m_flDisguiseEndEffectStartTime, gpGlobals->curtime );
	}

	int nNewWaterLevel = GetWaterLevel();

	if ( nNewWaterLevel != m_nOldWaterLevel )
	{
		if ( ( m_nOldWaterLevel == WL_NotInWater ) && ( nNewWaterLevel > WL_NotInWater ) )
		{
			// Set when we do a transition to/from partially in water to completely out
			m_flWaterEntryTime = gpGlobals->curtime;
		}

		// If player is now up to his eyes in water and has entered the water very recently (not just bobbing eyes in and out), play a bubble effect.
		if ( ( nNewWaterLevel == WL_Eyes ) && ( gpGlobals->curtime - m_flWaterEntryTime ) < 0.5f ) 
		{
			CNewParticleEffect *pEffect = ParticleProp()->Create( "water_playerdive", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pEffect, 1, NULL, PATTACH_WORLDORIGIN, NULL, WorldSpaceCenter() );
		}		
		// If player was up to his eyes in water and is now out to waist level or less, play a water drip effect
		else if ( m_nOldWaterLevel == WL_Eyes && ( nNewWaterLevel < WL_Eyes ) && !bJustSpawned )
		{
			CNewParticleEffect *pWaterExitEffect = ParticleProp()->Create( "water_playeremerge", PATTACH_ABSORIGIN_FOLLOW );
			ParticleProp()->AddControlPoint( pWaterExitEffect, 1, this, PATTACH_ABSORIGIN_FOLLOW );
			m_bWaterExitEffectActive = true;
		}
	}

	if ( IsLocalPlayer() )
	{
		if ( updateType == DATA_UPDATE_CREATED )
		{
			SetupHeadLabelMaterials();
			GetClientVoiceMgr()->SetHeadLabelOffset( 50 );
		}

		if ( m_iOldTeam != GetTeamNumber() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeteam" );
			if ( event )
			{
				gameeventmanager->FireEventClientSide( event );
			}
		}

		if ( !IsPlayerClass(m_iOldClass) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeclass" );
			if ( event )
			{
				event->SetInt( "updateType", updateType );
				gameeventmanager->FireEventClientSide( event );
			}
		}


		if ( m_iOldClass == TF_CLASS_SPY && 
		   ( m_bDisguised != m_Shared.InCond( TF_COND_DISGUISED ) || m_iOldDisguiseClass != m_Shared.GetDisguiseClass() ) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changedisguise" );
			if ( event )
			{
				event->SetBool( "disguised", m_Shared.InCond( TF_COND_DISGUISED ) );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		// If our metal amount changed, send a game event
		int iCurrentMetal = GetAmmoCount( TF_AMMO_METAL );	

		if ( iCurrentMetal != m_iPreviousMetal )
		{
			//msg
			IGameEvent *event = gameeventmanager->CreateEvent( "player_account_changed" );
			if ( event )
			{
				event->SetInt( "old_account", m_iPreviousMetal );
				event->SetInt( "new_account", iCurrentMetal );
				gameeventmanager->FireEventClientSide( event );
			}

			m_iPreviousMetal = iCurrentMetal;
		}

	}

	// Some time in this network transmit we changed the size of the object array.
	// recalc the whole thing and update the hud
	if ( m_bUpdateObjectHudState )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "building_info_changed" );
		if ( event )
		{
			event->SetInt( "building_type", -1 );
			gameeventmanager->FireEventClientSide( event );
		}
	
		m_bUpdateObjectHudState = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitInvulnerableMaterial( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	const char *pszMaterial = NULL;

	int iVisibleTeam = GetTeamNumber();
	// if this player is disguised and on the other team, use disguise team
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !InSameTeam( pLocalPlayer ) )
	{
		iVisibleTeam = m_Shared.GetDisguiseTeam();
	}

	switch ( iVisibleTeam )
	{
	case TF_TEAM_BLUE:	
		pszMaterial = "models/effects/invulnfx_blue.vmt";
		break;
	case TF_TEAM_RED:	
		pszMaterial = "models/effects/invulnfx_red.vmt";
		break;
	case TF_TEAM_MERCENARY:	
		pszMaterial = "models/effects/invulnfx_mercenary.vmt";
		break;
	default:
		break;
	}

	if ( pszMaterial )
	{
		m_InvulnerableMaterial.Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
	else
	{
		m_InvulnerableMaterial.Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::StartBurningSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( !m_pBurningSound )
	{
		CLocalPlayerFilter filter;
		m_pBurningSound = controller.SoundCreate( filter, entindex(), "Player.OnFire" );
	}

	controller.Play( m_pBurningSound, 0.0, 100 );
	controller.SoundChangeVolume( m_pBurningSound, 1.0, 0.1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::StopBurningSound( void )
{
	if ( m_pBurningSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBurningSound );
		m_pBurningSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnAddTeleported( void )
{
	if ( !m_pTeleporterEffect )
	{
		char *pEffect = NULL;

		switch( GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			pEffect = "player_recent_teleport_blue";
			break;
		case TF_TEAM_RED:
			pEffect = "player_recent_teleport_red";
			break;
		case TF_TEAM_MERCENARY:
			pEffect = "player_recent_teleport_mercenary";
			break;
		default:
			break;
		}

		if ( pEffect )
		{
			m_pTeleporterEffect = ParticleProp()->Create( pEffect, PATTACH_ABSORIGIN_FOLLOW );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnRemoveTeleported( void )
{
	if ( m_pTeleporterEffect )
	{
		ParticleProp()->StopEmission( m_pTeleporterEffect );
		m_pTeleporterEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::GetGlowEffectColor( float *r, float *g, float *b )
{
	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			*r = 0.62; *g = 0.21; *b = 0.13;
			break;
		case TF_TEAM_BLUE:
			*r = 0.3; *g = 0.42; *b = 0.5;
			break;

		case TF_TEAM_MERCENARY:
				{
					Vector Color = m_vecPlayerColor;				
					*r = Color.x;
					*g = Color.y;
					*b = Color.z;
				}
			break; 

		default:
			*r = 0.76; *g = 0.76; *b = 0.76;
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnAddCritBoosted( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnRemoveCritBoosted( void )
{

}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::OnPlayerClassChange( void )
{
	//if ( !m_bPlayingMusic )
	//{
	//	PlayMapMusic();
	//}

	// execute class specific .cfgs if we have spawned
	//if ( IsPlayer() )
	if ( IsLocalPlayer() )
	{
		char szCmd[128];
		Q_snprintf( szCmd, sizeof(szCmd), "exec %s.cfg \n", GetPlayerClass()->GetName() );
		engine->ExecuteClientCmd( szCmd );
	}

	// Init the anim movement vars
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5 );

	/*
	// clear out any magazines
	int i;

	C_FadingPhysPropClientside *pCandidate;
	for ( i = 0; i < g_Mags.Count(); i++ )
	{
		pCandidate = g_Mags[i];
		Assert( !pCandidate->IsEffectActive( EF_NORECEIVESHADOW ) );

		g_Mags.FastRemove( i );

		pCandidate->AddEffects( EF_NORECEIVESHADOW );

		pCandidate->StartFadeOut( 0.1 );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitPhonemeMappings()
{
	CStudioHdr *pStudio = GetModelPtr();
	if ( pStudio )
	{
		char szBasename[MAX_PATH];
		Q_StripExtension( pStudio->pszName(), szBasename, sizeof( szBasename ) );
		char szExpressionName[MAX_PATH];
		Q_snprintf( szExpressionName, sizeof( szExpressionName ), "%s/phonemes/phonemes", szBasename );
		if ( FindSceneFile( szExpressionName ) )
		{
			SetupMappings( szExpressionName );	
		}
		else
		{
			BaseClass::InitPhonemeMappings();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ResetFlexWeights( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr || pStudioHdr->numflexdesc() == 0 )
		return;

	// Reset the flex weights to their starting position.
	LocalFlexController_t iController;
	for ( iController = LocalFlexController_t(0); iController < pStudioHdr->numflexcontrollers(); ++iController )
	{
		SetFlexWeight( iController, 0.0f );
	}

	// Reset the prediction interpolation values.
	m_iv_flexWeight.Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_TFPlayer::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Initialize the gibs.
	InitPlayerGibs();

	InitializePoseParams();

	// Init flexes, cancel any scenes we're playing
	ClearSceneEvents( NULL, false );

	// Reset the flex weights.
	ResetFlexWeights( hdr );

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->OnNewModel();
	}

	if ( hdr )
	{
		InitPhonemeMappings();
	}

	if ( m_hSpyMask )
	{
		m_hSpyMask->UpdateVisibility();
	}	

	m_bUpdatePlayerAttachments = true;
	m_bUpdateCosmetics = true;

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Update clientside player attachments
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdatePlayerAttachedModels( void )
{
	if ( IsAlive() && GetTeamNumber() >= FIRST_GAME_TEAM && !IsPlayerClass(TF_CLASS_UNDEFINED) ) //If we spawned in, continue
	{
		UpdatePartyHat();
		UpdateGameplayAttachments();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the party hat players wear in birthday mode
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdatePartyHat( void )
{
	if ( TFGameRules() && TFGameRules()->IsBirthday() && ( m_Shared.WearsHat( 0 ) || GetPlayerClass()->GetClassIndex() != TF_CLASS_MERCENARY ) ) // If the game is in Birthday mode and we don't already wear anything give us a cool hat
	{
		if ( m_hPartyHat )
		{
			m_hPartyHat->Release(); //Remove the hat so if its not valid anymore we don't wear it
		}
		if ( IsLocalPlayer() &&  !::input->CAM_IsThirdPerson() ) // If we're the local player and not in third person, bail
			return;
		m_hPartyHat = C_PlayerAttachedModel::Create( BDAY_HAT_MODEL, this, LookupAttachment("partyhat"), vec3_origin, PAM_PERMANENT, 0, 0, false );
												  // Model name, object it gets attached to, attachment name,
		// C_PlayerAttachedModel::Create can return NULL!
		if ( m_hPartyHat )
		{
			int iVisibleTeam = GetTeamNumber();
			if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
			{
				iVisibleTeam = m_Shared.GetDisguiseTeam();
			}
			m_hPartyHat->m_nSkin = iVisibleTeam - 2; //Set the proper skin for each team
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Update the party hat players wear in birthday mode
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateSpyMask( void )
{
	C_PlayerAttachedModel *pMask = m_hSpyMask.Get();

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( !pMask )
		{
			pMask = C_PlayerAttachedModel::Create( "models/player/spy_mask.mdl", this, LookupAttachment( "partyhat" ), vec3_origin, PAM_PERMANENT, 0, EF_BONEMERGE, true );

			if ( !pMask )
			{
				pMask->Release();

				return;
			}

			pMask->SetOwnerEntity( this );
			pMask->FollowEntity( this );

			// this might cause some bugs...
			pMask->UpdateVisibility();

			m_hSpyMask = pMask;
		}
	}
	else if ( pMask )
	{
		pMask->Release();

		// nuke from orbit
		m_hSpyMask = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attachments used for gameplay IE Shield powerup go here
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateGameplayAttachments( void )
{
	if ( IsAlive() && GetTeamNumber() >= FIRST_GAME_TEAM && !IsPlayerClass(TF_CLASS_UNDEFINED) )
	{
		if ( m_hShieldEffect )
			m_hShieldEffect->Release();
		if ( m_Shared.InCond( TF_COND_SHIELD ) && ( !IsLocalPlayer() || ( IsLocalPlayer() &&  ::input->CAM_IsThirdPerson() ) ) )
		{
			m_hShieldEffect = C_PlayerAttachedModel::Create( DM_SHIELD_MODEL, this, LookupAttachment("partyhat"), vec3_origin, PAM_PERMANENT, 0, EF_BONEMERGE , false );
			if ( m_hShieldEffect )
			{
				int iVisibleTeam = GetTeamNumber();
				if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
				{
					iVisibleTeam = m_Shared.GetDisguiseTeam();
				}
				m_hShieldEffect->m_nSkin = iVisibleTeam - 2;
			}
		}
	}
}
void C_TFPlayer::UpdateWearables( void )
{
	DevMsg("UpdateWearables: Triggered Update Wearables\n");
	for( int i = 0; i < GetNumBodyGroups(); i++ )
	{
		SetBodygroup( i, 0 );
	}	
	for ( int i = 0; i < m_hCosmetic.Count(); i++ )
	{
		if ( m_hCosmetic[i] )
			m_hCosmetic[i].Get()->Release();
	}
	m_hCosmetic.Purge();

	if( of_disable_cosmetics.GetBool() )
		return;

	if( m_iCosmetics.Count() > 32 || m_iCosmetics.Count() < 0 )
	{
		DevMsg("UpdateWearables: Mismatching cosmetic count\n");
		return;
	}

	for( int i = 0; i < m_iCosmetics.Count(); i++ )
	{
		KeyValues *pCosmetic = GetCosmetic( m_iCosmetics[i] );
		if( !pCosmetic )
		{
			DevMsg("UpdateWearables: Cant find cosmetic with ID %d\n", m_iCosmetics[i]);
			continue;
		}
		KeyValues* pBodygroups = pCosmetic->FindKey("Bodygroups");
		if( pBodygroups )
		{
			for ( KeyValues *sub = pBodygroups->GetFirstValue(); sub; sub = sub->GetNextValue() )
			{
				int m_Bodygroup = FindBodygroupByName( sub->GetName() );

				if ( m_Bodygroup >= 0 )
					SetBodygroup( m_Bodygroup, sub->GetInt() );
			}
		}

		if( Q_strcmp( pCosmetic->GetString( "Model" ), "BLANK" ) )
		{
			CosmeticHandle handle = C_PlayerAttachedModel::Create( pCosmetic->GetString( "Model" , "models/empty.mdl" ), this, LookupAttachment("partyhat"), vec3_origin, PAM_PERMANENT, 0, EF_BONEMERGE, false );	

			if( handle )
			{
				int iVisibleTeam = GetTeamNumber();
				if (m_Shared.InCond(TF_COND_DISGUISED) && IsEnemyPlayer())
				{
					iVisibleTeam = m_Shared.GetDisguiseTeam();
				}
				
				iVisibleTeam = iVisibleTeam - 2;
				handle->m_nSkin = iVisibleTeam < 0 ? 0 : iVisibleTeam;

				m_hCosmetic.AddToTail(handle);
			}
		}
		else
		{
			DevMsg("UpdateWearables: Blank model\n");
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: Is this player an enemy to the local player
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsEnemyPlayer( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return false;

	switch( pLocalPlayer->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		return ( GetTeamNumber() == TF_TEAM_BLUE );
	case TF_TEAM_BLUE:
		return ( GetTeamNumber() == TF_TEAM_RED );
	case TF_TEAM_MERCENARY:
		return ( GetTeamNumber() == TF_TEAM_MERCENARY && !IsLocalPlayer() );
	default:
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Displays a nemesis icon on this player to the local player
//-----------------------------------------------------------------------------
void C_TFPlayer::ShowNemesisIcon( bool bShow )
{
	if ( bShow )
	{
		const char *pszEffect = NULL;
		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			pszEffect = "particle_nemesis_red";
			break;
		case TF_TEAM_BLUE:
			pszEffect = "particle_nemesis_blue";
			break;
		case TF_TEAM_MERCENARY:
			pszEffect = "particle_nemesis_mercenary";
			break;
		default:
			return;	// shouldn't get called if we're not on a team; bail out if it does
		}
		m_Shared.UpdateParticleColor( ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "head" ) );
	}
	else
	{
		// stop effects for both team colors (to make sure we remove effects in event of team change)
		ParticleProp()->StopParticlesNamed( "particle_nemesis_red", true );
		ParticleProp()->StopParticlesNamed( "particle_nemesis_blue", true );
		ParticleProp()->StopParticlesNamed( "particle_nemesis_mercenary", true );
	}
	m_bIsDisplayingNemesisIcon = bShow;
}

#define	TF_TAUNT_PITCH	0
#define TF_TAUNT_YAW	1
#define TF_TAUNT_DIST	2

#define TF_TAUNT_MAXYAW		135
#define TF_TAUNT_MINYAW		-135
#define TF_TAUNT_MAXPITCH	90
#define TF_TAUNT_MINPITCH	0
#define TF_TAUNT_IDEALLAG	4.0f

static Vector TF_TAUNTCAM_HULL_MIN( -9.0f, -9.0f, -9.0f );
static Vector TF_TAUNTCAM_HULL_MAX( 9.0f, 9.0f, 9.0f );

static ConVar tf_tauntcam_yaw( "tf_tauntcam_yaw", "0", FCVAR_CHEAT );
static ConVar tf_tauntcam_pitch( "tf_tauntcam_pitch", "0", FCVAR_CHEAT );
static ConVar tf_tauntcam_dist( "tf_tauntcam_dist", "110", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOnTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;

	if ( tf_taunt_first_person.GetBool() )
		return;

	// Save the old view angles.
	engine->GetViewAngles( m_angTauntEngViewAngles );
	prediction->GetViewAngles( m_angTauntPredViewAngles );

	m_TauntCameraData.m_flPitch = tf_tauntcam_pitch.GetFloat();
	m_TauntCameraData.m_flYaw =  tf_tauntcam_yaw.GetFloat();
	m_TauntCameraData.m_flDist = tf_tauntcam_dist.GetFloat();
	m_TauntCameraData.m_flLag = 4.0f;
	m_TauntCameraData.m_vecHullMin.Init( -9.0f, -9.0f, -9.0f );
	m_TauntCameraData.m_vecHullMax.Init( 9.0f, 9.0f, 9.0f );

	QAngle vecCameraOffset( tf_tauntcam_pitch.GetFloat(), tf_tauntcam_yaw.GetFloat(), tf_tauntcam_dist.GetFloat() );

	g_ThirdPersonManager.SetDesiredCameraOffset( Vector( tf_tauntcam_dist.GetFloat(), 0.0f, 0.0f ) );
	g_ThirdPersonManager.SetOverridingThirdPerson( true );
	::input->CAM_ToThirdPerson();
	ThirdPersonSwitch( true );

	::input->CAM_SetCameraThirdData( &m_TauntCameraData, vecCameraOffset );

	if ( m_hItem )
	{
		m_hItem->UpdateVisibility();
	}
	UpdatePlayerAttachedModels();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TurnOffTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;	

	Vector vecOffset = g_ThirdPersonManager.GetCameraOffsetAngles();

	tf_tauntcam_pitch.SetValue( vecOffset[PITCH] - m_angTauntPredViewAngles[PITCH] );
	tf_tauntcam_yaw.SetValue( vecOffset[YAW] - m_angTauntPredViewAngles[YAW] );

	g_ThirdPersonManager.SetDesiredCameraOffset( vec3_origin );
	g_ThirdPersonManager.SetOverridingThirdPerson( false );
	::input->CAM_ToFirstPerson();
	ThirdPersonSwitch( false );
	::input->CAM_SetCameraThirdData( NULL, vec3_angle );

	// Reset the old view angles.
	engine->SetViewAngles( m_angTauntEngViewAngles );
	prediction->SetViewAngles( m_angTauntPredViewAngles );

	// Force the feet to line up with the view direction post taunt.
	m_PlayerAnimState->m_bForceAimYaw = true;

	if ( GetViewModel() )
	{
		GetViewModel()->UpdateVisibility();
	}

	if ( m_hItem )
	{
		m_hItem->UpdateVisibility();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::HandleTaunting( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Clear the taunt slot.
	if ( !m_bWasTaunting && m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		m_bWasTaunting = true;

		// Handle the camera for the local player.
		if ( pLocalPlayer )
		{
			TurnOnTauntCam();
		}
	}

	if ( m_bWasTaunting && !m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		m_bWasTaunting = false;

		// Clear the vcd slot.
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );

		// Handle the camera for the local player.
		if ( pLocalPlayer )
		{
			TurnOffTauntCam();
		}
	}
}

void C_TFPlayer::ClientThink()
{
	// Pass on through to the base class.
	BaseClass::ClientThink();

	UpdateIDTarget();

	UpdateLookAt();
	
	// Handle invisibility.
	m_Shared.InvisibilityThink();

	m_Shared.ConditionThink();

	// Clear our healer, it'll be reset by the medigun client think if we're being healed
	m_hHealer = NULL;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( TFGameRules() && TFGameRules()->IsESCGamemode() && pLocalPlayer->GetTeamNumber() == GetTeamNumber() && IsPlayerClass( TF_CLASS_CIVILIAN ) )
	{
		m_bGlowEnabled = true;
		UpdateGlowEffect();
	}
	else
	{
		m_bGlowEnabled = false;
		DestroyGlowEffect();
	}

	// Ugh, this check is getting ugly

	// Start smoke if we're not invisible or disguised
	if ( !m_bRetroMode && IsPlayerClass( TF_CLASS_SPY ) && IsAlive() &&									// only on spy model when not with TFC model
		( !m_Shared.InCond( TF_COND_DISGUISED ) || !IsEnemyPlayer() ) &&	// disguise doesn't show for teammates
		GetPercentInvisible() <= 0 &&										// don't start if invis
		( pLocalPlayer != this ) && 										// don't show to local player
		!( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalPlayer->GetObserverTarget() == this ) )	// not if we're spectating this player first person
	{
		if ( !m_bCigaretteSmokeActive )
		{
			int iSmokeAttachment = LookupAttachment( "cig_smoke" );
			ParticleProp()->Create( "cig_smoke", PATTACH_POINT_FOLLOW, iSmokeAttachment );
			m_bCigaretteSmokeActive = true;
		}
	}
	else	// stop the smoke otherwise if its active
	{
		if ( m_bCigaretteSmokeActive )
		{
			ParticleProp()->StopParticlesNamed( "cig_smoke", false );
			m_bCigaretteSmokeActive = false;
		}
	}
	
	if ( m_bWaterExitEffectActive && !IsAlive() )
	{
		ParticleProp()->StopParticlesNamed( "water_playeremerge", false );
		m_bWaterExitEffectActive = false;
	}

	if ( m_bUpdatePlayerAttachments )
	{
		UpdatePlayerAttachedModels();
		m_bUpdatePlayerAttachments = false;
	}
	
	if ( m_bUpdateCosmetics )
	{
		UpdateWearables();
		m_bUpdateCosmetics = false;
	}
	
	if ( m_pSaveMeEffect )
	{
		// Kill the effect if either
		// a) the player is dead
		// b) the enemy disguised spy is now invisible

		if ( !IsAlive() ||
			( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() && ( GetPercentInvisible() > 0 ) ) )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pSaveMeEffect );
			m_pSaveMeEffect = NULL;
		}
	}

	if ( m_pChattingEffect )
	{
		// Kill the effect if either
		// a) the player is dead
		// b) the enemy disguised spy is now invisible

		if ( !IsAlive() && ( GetPercentInvisible() > 0 ) )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pChattingEffect );
			m_pChattingEffect = NULL;
		}
	}
}


class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, CBaseEntity *pOwner )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam ), m_hOwner( pOwner )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam ) || pEntity == m_hOwner )
		{
			return false;
		}

		return true;
	}
	CBaseEntity *m_hOwner;
	int m_iIgnoreTeam;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateLookAt( void )
{
	bool bFoundViewTarget = false;

	Vector vForward;
	AngleVectors( GetLocalAngles(), &vForward );

	Vector vMyOrigin =  GetAbsOrigin();

	Vector vecLookAtTarget = vec3_origin;

	for( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if ( !pEnt || !pEnt->IsPlayer() )
			continue;

		if ( !pEnt->IsAlive() )
			continue;

		if ( pEnt == this )
			continue;

		Vector vDir = pEnt->GetAbsOrigin() - vMyOrigin;

		if ( vDir.Length() > 300 ) 
			continue;

		VectorNormalize( vDir );

		if ( DotProduct( vForward, vDir ) < 0.0f )
			continue;

		vecLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if ( bFoundViewTarget == false )
	{
		// no target, look forward
		vecLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	// orient eyes
	m_viewtarget = vecLookAtTarget;

	
	// blinking
	if (m_blinkTimer.IsElapsed() )
	{
		m_blinktoggle = !m_blinktoggle;
		m_blinkTimer.Start( RandomFloat( 1.5f, 4.0f ) );
	}
	

	/*
	// Figure out where we want to look in world space.
	QAngle desiredAngles;
	Vector to = vecLookAtTarget - EyePosition();
	VectorAngles( to, desiredAngles );

	// Figure out where our body is facing in world space.
	QAngle bodyAngles( 0, 0, 0 );
	bodyAngles[YAW] = GetLocalAngles()[YAW];

	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];

	// Set the head's yaw.
	float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] );
	desired = clamp( -desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

	// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );

	SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

	// Set the head's yaw.
	desired = AngleNormalize( desiredAngles[PITCH] );
	desired = clamp( desired, m_headPitchMin, m_headPitchMax );

	m_flCurrentHeadPitch = ApproachAngle( -desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
	m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
	SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Try to steer away from any players and objects we might interpenetrate
//-----------------------------------------------------------------------------
#define TF_AVOID_MAX_RADIUS_SQR		5184.0f			// Based on player extents and max buildable extents.
#define TF_OO_AVOID_MAX_RADIUS_SQR	0.00019f

ConVar tf_max_separation_force ( "tf_max_separation_force", "256", FCVAR_CHEAT );

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

void C_TFPlayer::AvoidPlayers( CUserCmd *pCmd )
{
	// Turn off the avoid player code.
	if ( !tf_avoidteammates.GetBool() )
		return;

	// Don't test if the player doesn't exist or is dead.
	if ( IsAlive() == false )
		return;

	C_Team *pTeam = ( C_Team * )GetTeam();
	if ( !pTeam )
		return;

	// Up vector.
	static Vector vecUp( 0.0f, 0.0f, 1.0f );

	Vector vecTFPlayerCenter = GetAbsOrigin();
	Vector vecTFPlayerMin = GetPlayerMins();
	Vector vecTFPlayerMax = GetPlayerMaxs();
	float flZHeight = vecTFPlayerMax.z - vecTFPlayerMin.z;
	vecTFPlayerCenter.z += 0.5f * flZHeight;
	VectorAdd( vecTFPlayerMin, vecTFPlayerCenter, vecTFPlayerMin );
	VectorAdd( vecTFPlayerMax, vecTFPlayerCenter, vecTFPlayerMax );

	// Find an intersecting player or object.
	int nAvoidPlayerCount = 0;
	C_TFPlayer *pAvoidPlayerList[MAX_PLAYERS];

	C_TFPlayer *pIntersectPlayer = NULL;
	CBaseObject *pIntersectObject = NULL;
	float flAvoidRadius = 0.0f;

	Vector vecAvoidCenter, vecAvoidMin, vecAvoidMax;
	for ( int i = 0; i < pTeam->GetNumPlayers(); ++i )
	{
		C_TFPlayer *pAvoidPlayer = static_cast< C_TFPlayer * >( pTeam->GetPlayer( i ) );
		if ( pAvoidPlayer == NULL )
			continue;
		// Is the avoid player me?
		if ( pAvoidPlayer == this )
			continue;

		// Save as list to check against for objects.
		pAvoidPlayerList[nAvoidPlayerCount] = pAvoidPlayer;
		++nAvoidPlayerCount;

		// Check to see if the avoid player is dormant.
		if ( pAvoidPlayer->IsDormant() )
			continue;

		// Is the avoid player solid?
		if ( pAvoidPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
			continue;

		Vector t1, t2;

		vecAvoidCenter = pAvoidPlayer->GetAbsOrigin();
		vecAvoidMin = pAvoidPlayer->GetPlayerMins();
		vecAvoidMax = pAvoidPlayer->GetPlayerMaxs();
		flZHeight = vecAvoidMax.z - vecAvoidMin.z;
		vecAvoidCenter.z += 0.5f * flZHeight;
		VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
		VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

		if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
		{
			// Need to avoid this player.
			if ( !pIntersectPlayer )
			{
				pIntersectPlayer = pAvoidPlayer;
				break;
			}
		}
	}

	// We didn't find a player - look for objects to avoid.
	if ( !pIntersectPlayer )
	{
		for ( int iPlayer = 0; iPlayer < nAvoidPlayerCount; ++iPlayer )
		{	
			// Stop when we found an intersecting object.
			if ( pIntersectObject )
				break;

			C_TFTeam *pTeam = (C_TFTeam*)GetTeam();

			for ( int iObject = 0; iObject < pTeam->GetNumObjects(); ++iObject )
			{
				CBaseObject *pAvoidObject = pTeam->GetObject( iObject );
				if ( !pAvoidObject )
					continue;

				// Check to see if the object is dormant.
				if ( pAvoidObject->IsDormant() )
					continue;

				// Is the object solid.
				if ( pAvoidObject->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
					continue;

				// If we shouldn't avoid it, see if we intersect it.
				if ( pAvoidObject->ShouldPlayersAvoid() )
				{
					vecAvoidCenter = pAvoidObject->WorldSpaceCenter();
					vecAvoidMin = pAvoidObject->WorldAlignMins();
					vecAvoidMax = pAvoidObject->WorldAlignMaxs();
					VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
					VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

					if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
					{
						// Need to avoid this object.
						pIntersectObject = pAvoidObject;
						break;
					}
				}
			}
		}
	}

	// Anything to avoid?
	if ( !pIntersectPlayer && !pIntersectObject )
	{
		m_Shared.SetSeparation( false );
		m_Shared.SetSeparationVelocity( vec3_origin );
		return;
	}

	// Calculate the push strength and direction.
	Vector vecDelta;

	// Avoid a player - they have precedence.
	if ( pIntersectPlayer )
	{
		VectorSubtract( pIntersectPlayer->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectPlayer->WorldAlignMaxs() - pIntersectPlayer->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}
	// Avoid a object.
	else
	{
		VectorSubtract( pIntersectObject->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectObject->WorldAlignMaxs() - pIntersectObject->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}

	float flPushStrength = RemapValClamped( vecDelta.Length(), flAvoidRadius, 0, 0, tf_max_separation_force.GetInt() ); //flPushScale;

	//Msg( "PushScale = %f\n", flPushStrength );

	// Check to see if we have enough push strength to make a difference.
	if ( flPushStrength < 0.01f )
		return;

	Vector vecPush;
	if ( GetAbsVelocity().Length2DSqr() > 0.1f )
	{
		Vector vecVelocity = GetAbsVelocity();
		vecVelocity.z = 0.0f;
		CrossProduct( vecUp, vecVelocity, vecPush );
		VectorNormalize( vecPush );
	}
	else
	{
		// We are not moving, but we're still intersecting.
		QAngle angView = pCmd->viewangles;
		angView.x = 0.0f;
		AngleVectors( angView, NULL, &vecPush, NULL );
	}

	// Move away from the other player/object.
	Vector vecSeparationVelocity;
	if ( vecDelta.Dot( vecPush ) < 0 )
	{
		vecSeparationVelocity = vecPush * flPushStrength;
	}
	else
	{
		vecSeparationVelocity = vecPush * -flPushStrength;
	}

	// Don't allow the max push speed to be greater than the max player speed.
	float flMaxPlayerSpeed = MaxSpeed();
	float flCropFraction = 1.33333333f;

	if ( ( GetFlags() & FL_DUCKING ) && ( GetGroundEntity() != NULL ) )
	{	
		flMaxPlayerSpeed *= flCropFraction;
	}	

	float flMaxPlayerSpeedSqr = flMaxPlayerSpeed * flMaxPlayerSpeed;

	if ( vecSeparationVelocity.LengthSqr() > flMaxPlayerSpeedSqr )
	{
		vecSeparationVelocity.NormalizeInPlace();
		VectorScale( vecSeparationVelocity, flMaxPlayerSpeed, vecSeparationVelocity );
	}

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;
	Vector currentdir;
	Vector rightdir;

	AngleVectors( vAngles, &currentdir, &rightdir, NULL );

	Vector vDirection = vecSeparationVelocity;

	VectorNormalize( vDirection );

	float fwd = currentdir.Dot( vDirection );
	float rt = rightdir.Dot( vDirection );

	float forward = fwd * flPushStrength;
	float side = rt * flPushStrength;

	//Msg( "fwd: %f - rt: %f - forward: %f - side: %f\n", fwd, rt, forward, side );

	m_Shared.SetSeparation( true );
	m_Shared.SetSeparationVelocity( vecSeparationVelocity );

	pCmd->forwardmove	+= forward;
	pCmd->sidemove		+= side;

	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
	}
	else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
	}

	float flSideScale = 1.0f;
	if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
	{
		flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
	}

	float flScale = min( flForwardScale, flSideScale );
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "Pforwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInputSampleTime - 
//			*pCmd - 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{	
	static QAngle angMoveAngle( 0.0f, 0.0f, 0.0f );
	
	bool bNoTaunt = true;
	if ( m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		// show centerprint message 
		pCmd->forwardmove = 0.0f;
		pCmd->sidemove = 0.0f;
		pCmd->upmove = 0.0f;
		pCmd->buttons = 0;
		pCmd->weaponselect = 0;

		VectorCopy( angMoveAngle, pCmd->viewangles );
		bNoTaunt = false;
	}
	else
	{
		VectorCopy( pCmd->viewangles, angMoveAngle );
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd );

	AvoidPlayers( pCmd );

	// should we create chat bubbles on the player or not?
	//if ( GetChatHud()->GetMessageMode() != MM_NONE )
	if ( GetChatHud() && GetChatHud()->GetMessageMode() != MM_NONE )
	{
		pCmd->buttons |= IN_TYPING;
	}

	return bNoTaunt;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent( event, nData );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetObserverCamOrigin( void )
{
	if ( !IsAlive() )
	{
		if ( m_hFirstGib )
		{
			IPhysicsObject *pPhysicsObject = m_hFirstGib->VPhysicsGetObject();
			if( pPhysicsObject )
			{
				Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
				Vector vecWorld;
				m_hFirstGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &vecWorld );
				return (vecWorld);
			}
			return m_hFirstGib->GetRenderOrigin();
		}

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();
		if ( pRagdoll )
			return pRagdoll->GetRagdollOrigin();
	}

	return BaseClass::GetObserverCamOrigin();	
}

//-----------------------------------------------------------------------------
// Purpose: Consider the viewer and other factors when determining resulting
// invisibility
//-----------------------------------------------------------------------------
float C_TFPlayer::GetEffectiveInvisibilityLevel( void )
{
	float flPercentInvisible = GetPercentInvisible();

	// If this is a teammate of the local player or viewer is observer,
	// dont go above a certain max invis
	if ( !IsEnemyPlayer() || ( GetTeamNumber() == TF_TEAM_MERCENARY && m_Shared.InCond( TF_COND_INVIS_POWERUP ) ) )
	{
		float flMax = tf_teammate_max_invis.GetFloat();
		if ( flPercentInvisible > flMax )
		{
			flPercentInvisible = flMax;
		}
	}
	else
	{
		// If this player just killed me, show them slightly
		// less than full invis in the deathcam and freezecam

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer )
		{
			int iObserverMode = pLocalPlayer->GetObserverMode();

			if ( ( iObserverMode == OBS_MODE_FREEZECAM || iObserverMode == OBS_MODE_DEATHCAM ) && 
				pLocalPlayer->GetObserverTarget() == this )
			{
				float flMax = tf_teammate_max_invis.GetFloat();
				if ( flPercentInvisible > flMax )
				{
					flPercentInvisible = flMax;
				}
			}
		}
	}

	return flPercentInvisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::DrawModel( int flags )
{
	// If we're a dead player with a fresh ragdoll, don't draw
	if ( m_nRenderFX == kRenderFxRagdoll )
		return 0;

	// Don't draw the model at all if we're fully invisible
	if ( GetEffectiveInvisibilityLevel() >= 1.0f )
		return 0;

	CMatRenderContextPtr pRenderContext( materials );
	bool bDoEffect = false;

	float flAmountToChop = 0.0;
	if ( m_Shared.InCond( TF_COND_DISGUISING ) )
	{
		flAmountToChop = ( gpGlobals->curtime - m_flDisguiseEffectStartTime ) *
			( 1.0 / TF_TIME_TO_DISGUISE );
	}
	else
		if ( m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			float flETime = gpGlobals->curtime - m_flDisguiseEffectStartTime;
			if ( ( flETime > 0.0 ) && ( flETime < TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT ) )
			{
				flAmountToChop = 1.0 - ( flETime * ( 1.0/TF_TIME_TO_SHOW_DISGUISED_FINISHED_EFFECT ) );
			}
		}

	bDoEffect = ( flAmountToChop > 0.0 ) && ( ! IsLocalPlayer() );
#if ( SHOW_DISGUISE_EFFECT == 0  )
	bDoEffect = false;
#endif
	bDoEffect = false;
	if ( bDoEffect )
	{
		Vector vMyOrigin =  GetAbsOrigin();
		BoxDeformation_t mybox;
		mybox.m_ClampMins = vMyOrigin - Vector(100,100,100);
		mybox.m_ClampMaxes = vMyOrigin + Vector(500,500,72 * ( 1 - flAmountToChop ) );
		pRenderContext->PushDeformation( &mybox );
	}

	int ret = BaseClass::DrawModel( flags );

	if ( bDoEffect )
		pRenderContext->PopDeformation();
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::ProcessMuzzleFlashEvent()
{
	CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// Reenable when the weapons have muzzle flash attachments in the right spot.
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( this == pLocalPlayer && !bInToolRecordingMode )
		return; // don't show own world muzzle flash for localplayer

	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		// also don't show in 1st person spec mode
		if ( pLocalPlayer->GetObserverTarget() == this )
			return;
	}

	C_TFWeaponBase *pWeapon = m_Shared.GetActiveTFWeapon();
	if ( !pWeapon )
		return;

	pWeapon->ProcessMuzzleFlashEvent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetForcedIDTarget( int iTarget )
{
	m_iForcedIDTarget = iTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Update this client's targetid entity
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateIDTarget()
{
	if ( !IsLocalPlayer() )
		return;

	// don't show IDs if mp_fadetoblack is on
	if ( GetTeamNumber() > TEAM_SPECTATOR && mp_fadetoblack.GetBool() && !IsAlive() )
	{
		m_iIDEntIndex = 0;
		return;
	}

	if ( m_iForcedIDTarget )
	{
		m_iIDEntIndex = m_iForcedIDTarget;
		return;
	}

	// If we're in deathcam, ID our killer
	if ( (GetObserverMode() == OBS_MODE_DEATHCAM || GetObserverMode() == OBS_MODE_CHASE) && GetObserverTarget() && GetObserverTarget() != GetLocalTFPlayer() )
	{
		m_iIDEntIndex = GetObserverTarget()->entindex();
		return;
	}

	// Clear old target and find a new one
	m_iIDEntIndex = 0;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), MAX_TRACE_LENGTH, MainViewForward(), vecEnd );
	VectorMA( MainViewOrigin(), 10,   MainViewForward(), vecStart );

	// If we're in observer mode, ignore our observer target. Otherwise, ignore ourselves.
	if ( IsObserver() )
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, GetObserverTarget(), COLLISION_GROUP_NONE, &tr );
	}
	else
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	}

	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;

		if ( pEntity && ( pEntity != this ) )
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display appropriate hints for the target we're looking at
//-----------------------------------------------------------------------------
void C_TFPlayer::DisplaysHintsForTarget( C_BaseEntity *pTarget )
{
	// If the entity provides hints, ask them if they have one for this player
	ITargetIDProvidesHint *pHintInterface = dynamic_cast<ITargetIDProvidesHint*>(pTarget);
	if ( pHintInterface )
	{
		pHintInterface->DisplayHintTo( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetRenderTeamNumber( void )
{
	return m_nSkin;
}

static Vector WALL_MIN(-WALL_OFFSET,-WALL_OFFSET,-WALL_OFFSET);
static Vector WALL_MAX(WALL_OFFSET,WALL_OFFSET,WALL_OFFSET);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcDeathCamView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	CBaseEntity	* killer = GetObserverTarget();

	// Swing to face our killer within half the death anim time
	float interpolation = ( gpGlobals->curtime - m_flDeathTime ) / (TF_DEATH_ANIMATION_TIME * 0.5);
	interpolation = clamp( interpolation, 0.0f, 1.0f );
	interpolation = SimpleSpline( interpolation );

	m_flObserverChaseDistance += gpGlobals->frametime*48.0f;
	m_flObserverChaseDistance = clamp(m_flObserverChaseDistance, CHASE_CAM_DISTANCE_MIN, CHASE_CAM_DISTANCE_MAX);

	QAngle aForward = eyeAngles = EyeAngles();
	Vector origin = EyePosition();			

	IRagdoll *pRagdoll = GetRepresentativeRagdoll();
	if ( pRagdoll )
	{
		origin = pRagdoll->GetRagdollOrigin();
		origin.z += VEC_DEAD_VIEWHEIGHT.z; // look over ragdoll, not through
		
		if ( cl_fp_ragdoll.GetBool() && m_hRagdoll.Get() )
		{
			// pointer to the ragdoll
			C_TFRagdoll *pRagdoll = ( C_TFRagdoll* )m_hRagdoll.Get();

			// gets its origin and angles
			int iAttachment = pRagdoll->LookupAttachment( "eyes" );

			// if no eyes attachment is found, fallback to head attachment
			if ( iAttachment <= 0 )
			{
				pRagdoll->GetAttachment( pRagdoll->LookupAttachment( "head" ), eyeOrigin, eyeAngles );
			}
			else
			{
				pRagdoll->GetAttachment( pRagdoll->LookupAttachment( "eyes" ), eyeOrigin, eyeAngles );
			}

			Vector vForward;
			AngleVectors( eyeAngles, &vForward );

			return;
		}

		if ( cl_fp_ragdoll.GetBool() )
		{
			eyeOrigin = vec3_origin;
			eyeAngles = vec3_angle;
		}
	}

	if ( killer && (killer != this) ) 
	{
		Vector vKiller = killer->EyePosition() - origin;
		QAngle aKiller; VectorAngles( vKiller, aKiller );
		InterpolateAngles( aForward, aKiller, eyeAngles, interpolation );
	};

	Vector vForward; AngleVectors( eyeAngles, &vForward );

	VectorNormalize( vForward );

	VectorMA( origin, -m_flObserverChaseDistance, vForward, eyeOrigin );

	trace_t trace; // clip against world
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if (trace.fraction < 1.0)
	{
		eyeOrigin = trace.endpos;
		m_flObserverChaseDistance = VectorLength(origin - eyeOrigin);
	}

	fov = GetFOV();
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	return;
}

float C_TFPlayer::GetMinFOV() const
{
	// Min FOV for Sniper Rifle
	return 20;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const QAngle& C_TFPlayer::EyeAngles()
{
	if ( IsLocalPlayer() && g_nKillCamMode == OBS_MODE_NONE )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &color - 
//-----------------------------------------------------------------------------
void C_TFPlayer::GetTeamColor( Color &color )
{
	color[3] = 255;

	if ( GetTeamNumber() == TF_TEAM_RED )
	{
		color[0] = 159;
		color[1] = 55;
		color[2] = 34;
	}
	else if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		color[0] = 76;
		color[1] = 109;
		color[2] = 129;
	}
	else if ( GetTeamNumber() == TF_TEAM_MERCENARY)
	{
		color[0] = 128;
		color[1] = 0;
		color[2] = 128;		
	}
	else
	{
		color[0] = 255;
		color[1] = 255;
		color[2] = 255;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bCopyEntity - 
// Output : C_BaseAnimating *
//-----------------------------------------------------------------------------
C_BaseAnimating *C_TFPlayer::BecomeRagdollOnClient()
{
	// Let the C_TFRagdoll take care of this.
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll* C_TFPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_TFRagdoll *pRagdoll = static_cast<C_TFRagdoll*>( m_hRagdoll.Get() );
		if ( !pRagdoll )
			return NULL;

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitPlayerGibs( void )
{
	// Clear out the gib list and create a new one.
	m_aGibs.Purge();
	BuildGibList( m_aGibs, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );

	if ( TFGameRules() && TFGameRules()->IsBirthday() )
	{
		for ( int i = 0; i < m_aGibs.Count(); i++ )
		{
			if ( RandomFloat(0,1) < 0.75 )
			{
				Q_strncpy( m_aGibs[i].modelName, g_pszBDayGibs[ RandomInt(0,ARRAYSIZE(g_pszBDayGibs)-1) ] , sizeof(m_aGibs[i].modelName) );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			&vecImpactVelocity - 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale )
{
	// Make sure we have Gibs to create.
	if ( m_aGibs.Count() == 0 )
		return;

	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );

	Vector vecBreakVelocity = vecVelocity;
	vecBreakVelocity.z += tf_playergib_forceup.GetFloat();
	VectorNormalize( vecBreakVelocity );
	vecBreakVelocity *= tf_playergib_force.GetFloat();

	// Cap the impulse.
	float flSpeed = vecBreakVelocity.Length();
	if ( flSpeed > tf_playergib_maxspeed.GetFloat() )
	{
		VectorScale( vecBreakVelocity, tf_playergib_maxspeed.GetFloat() / flSpeed, vecBreakVelocity );
	}

	breakablepropparams_t breakParams( vecOrigin, GetRenderAngles(), vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;//

	// Break up the player.
	m_hSpawnedGibs.Purge();
	m_hFirstGib = CreateGibsFromList( m_aGibs, GetModelIndex(), NULL, breakParams, this, -1 , false, true, &m_hSpawnedGibs );

	DropPartyHat( breakParams, vecBreakVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::DropPartyHat( breakablepropparams_t &breakParams, Vector &vecBreakVelocity )
{
	if ( m_hPartyHat )
	{
		breakmodel_t breakModel;
		Q_strncpy( breakModel.modelName, BDAY_HAT_MODEL, sizeof(breakModel.modelName) );
		breakModel.health = 1;
		breakModel.fadeTime = RandomFloat(5,10);
		breakModel.fadeMinDist = 0.0f;
		breakModel.fadeMaxDist = 0.0f;
		breakModel.burstScale = breakParams.defBurstScale;
		breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
		breakModel.isRagdoll = false;
		breakModel.isMotionDisabled = false;
		breakModel.placementName[0] = 0;
		breakModel.placementIsBone = false;
		breakModel.offset = GetAbsOrigin() - m_hPartyHat->GetAbsOrigin();
		BreakModelCreateSingle( this, &breakModel, m_hPartyHat->GetAbsOrigin(), m_hPartyHat->GetAbsAngles(), vecBreakVelocity, breakParams.angularVelocity, m_hPartyHat->m_nSkin, breakParams );

		m_hPartyHat->Release();
	}
}

//-----------------------------------------------------------------------------
// Purpose: How many buildables does this player own
//-----------------------------------------------------------------------------
int	C_TFPlayer::GetObjectCount( void )
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObject( int index )
{
	return m_aObjects[index].Get();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObjectOfType( int iObjectType, int iAltMode )
{
	int iCount = m_aObjects.Count();

	for ( int i=0;i<iCount;i++ )
	{
		C_BaseObject *pObj = m_aObjects[i].Get();

		if ( !pObj )
			continue;

		if ( pObj->IsDormant() || pObj->IsMarkedForDeletion() )
			continue;

		if ( pObj->GetType() == iObjectType && pObj->GetAltMode() == iAltMode )
		{
			return pObj;
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( ( ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ) && tf_avoidteammates.GetBool() ) ||
		collisionGroup == TFCOLLISION_GROUP_ROCKETS )
	{	
		if ( TFGameRules()->IsCoopEnabled() ) 
			return false;

		switch( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			if ( !( contentsMask & CONTENTS_REDTEAM ) )
				return false;
			break;

		case TF_TEAM_BLUE:
			if ( !( contentsMask & CONTENTS_BLUETEAM ) )
				return false;
			break;
		case TF_TEAM_MERCENARY:
			if ( !( contentsMask & CONTENTS_MERCENARYTEAM ) )
				return false;
			break;			
		}
	}
	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

float C_TFPlayer::GetPercentInvisible( void )
{
	return m_Shared.GetPercentInvisible();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetSkin()
{
	C_TFPlayer *pLocalPlayer = GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return 0;

	int iVisibleTeam = GetTeamNumber();

	// if this player is disguised and on the other team, use disguise team
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
	{
		iVisibleTeam = m_Shared.GetDisguiseTeam();
	}

	int nSkin;

	switch( iVisibleTeam )
	{
	case TF_TEAM_RED:
		nSkin = 0;
		break;

	case TF_TEAM_BLUE:
		nSkin = 1;
		break;
	case TF_TEAM_MERCENARY:
		if ( of_tennisball.GetBool() && IsPlayerClass( TF_CLASS_MERCENARY ) )
			nSkin = 6;
		else
			nSkin = 4;
		break;
	default:
		nSkin = 0;
		break;
	}

	// 3 and 4 are invulnerable
	if ( m_Shared.InCondUber() )
	{
		switch( iVisibleTeam )
		{
			case TF_TEAM_RED:
				nSkin = 2;
				break;

			case TF_TEAM_BLUE:
				nSkin = 3;
				break;
			case TF_TEAM_MERCENARY:
				if ( of_tennisball.GetBool() )
					nSkin = 7;
				else
					nSkin = 5;
				break;
			default:
				nSkin = 2;
			break;
		}
	}

	return nSkin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iClass - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsPlayerClass( int iClass )
{
	C_TFPlayerClass *pClass = GetPlayerClass();
	if ( !pClass )
		return false;

	return ( pClass->GetClassIndex() == iClass );
}

//-----------------------------------------------------------------------------
// Purpose: Don't take damage decals while stealthed
//-----------------------------------------------------------------------------
void C_TFPlayer::AddDecal( const Vector& rayStart, const Vector& rayEnd,
							const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal )
{
	if ( m_Shared.InCondInvis() )
	{
		return;
	}

	if ( m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		return;
	}

	if ( m_Shared.InCondUber() )
	{ 
		Vector vecDir = rayEnd - rayStart;
		VectorNormalize(vecDir);
		g_pEffects->Ricochet( rayEnd - (vecDir * 8), -vecDir );
		return;
	}

	// don't decal from inside the player
	if ( tr.startsolid )
	{
		return;
	}

	BaseClass::AddDecal( rayStart, rayEnd, decalCenter, hitbox, decalIndex, doTrace, tr, maxLODToDecal );
}

/*
//-----------------------------------------------------------------------------
// Get the map music
//-----------------------------------------------------------------------------
void C_TFPlayer::LoadMapMusic( IBaseFileSystem *pFileSystem )
{
	char mapname[ MAX_MAP_NAME ];
	Q_FileBase( engine->GetLevelName(), mapname, sizeof( mapname) );

	const char *mapFilename = NULL;

	if ( mapname && *mapname )
	{
		mapFilename = VarArgs( "maps/%s_music.txt", mapname );
	}

	KeyValues *pMusicValues = new KeyValues( "Music descriptions" );

	if ( !pMusicValues->LoadFromFile( pFileSystem, mapFilename, "GAME" ) )
	{
		DevMsg( "Can't open %s for music info.", mapFilename );
		pMusicValues->deleteThis();
		m_bPlayingMusic = true;
		return;
	}

	KeyValues *pMusicSub = pMusicValues->FindKey( "music_data" );

	if ( !pMusicSub )
	{
		pMusicValues->deleteThis();
		m_bPlayingMusic = true;
		return;
	}

	m_pzMusicLink = ReadAndAllocStringValue( pMusicSub, "SoundFile", mapFilename );
	m_fMusicDuration = pMusicSub->GetInt( "SoundDurationSeconds", 1 );

}

//-----------------------------------------------------------------------------
// Play map music
//-----------------------------------------------------------------------------
void C_TFPlayer::PlayMapMusic( void )
{
	if ( !m_bPlayingMusic )
	{
		if ( IsLocalPlayer() )
		{
			char szCmd[128];
			Q_snprintf( szCmd, sizeof(szCmd), "playmusicsound %s", m_pzMusicLink );
			engine->ExecuteClientCmd( szCmd );
		}

		m_bPlayingMusic = true;
	}
}

//-----------------------------------------------------------------------------
// Play map music
//-----------------------------------------------------------------------------
void C_TFPlayer::ReplayMapMusic( void )
{
	if ( IsLocalPlayer() )
	{
		char szCmd[128];
		Q_snprintf( szCmd, sizeof(szCmd), "playmusicsound %s", m_pzMusicLink );
		engine->ExecuteClientCmd( szCmd );
	}

	if ( m_fMusicDuration > 0.1f && gpGlobals->curtime + m_fMusicDuration )
	{
			ReplayMapMusic();
	}
}
*/

//-----------------------------------------------------------------------------
// Called every time the player respawns
//-----------------------------------------------------------------------------
void C_TFPlayer::ClientPlayerRespawn( void )
{
	if ( IsLocalPlayer() )
	{
		// Dod called these, not sure why
		//MoveToLastReceivedPosition( true );
		//ResetLatched();

		// Reset the camera.
		if ( m_bWasTaunting )
		{
			TurnOffTauntCam();
		}

		ResetToneMapping(1.0);

		// Release the duck toggle key
		KeyUp( &in_ducktoggle, NULL );
		
		RefreshDesiredCosmetics( GetPlayerClass()->GetClassIndex() );
		RefreshDesiredWeapons( GetPlayerClass()->GetClassIndex() );
	}
	
	// don't draw the respawn particle in first person or if the client has them disabled
	if ( of_respawn_particles.GetBool() )
	{
		if ( TFGameRules() && TFGameRules()->IsDMGamemode() && ( !InFirstPersonView() || !IsLocalPlayer() || of_first_person_respawn_particles.GetBool() ) )
		{
			char pEffectName[32];
			pEffectName[0] = '\0';
			if ( m_Shared.GetSpawnEffects() < 10 )
				Q_snprintf( pEffectName, sizeof( pEffectName ), "dm_respawn_0%d", m_Shared.GetSpawnEffects() );
			else
				Q_snprintf( pEffectName, sizeof( pEffectName ), "dm_respawn_%d", m_Shared.GetSpawnEffects() );
			if ( pEffectName[0] != '\0' )
				m_Shared.UpdateParticleColor( ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN ) );
		}
	}
	
	UpdateVisibility();

	m_hFirstGib = NULL;
	m_hSpawnedGibs.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreateSaveMeEffect( void )
{
	// Don't create them for the local player
	// if ( IsLocalPlayer() && !ShouldDrawLocalPlayer() )
	if ( IsLocalPlayer() )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// If I'm disguised as the enemy, play to all players
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && m_Shared.GetDisguiseTeam() != GetTeamNumber() )
	{
		// play to all players
	}
	else
	{
		// only play to teammates
		if ( pLocalPlayer && pLocalPlayer->GetTeamNumber() != GetTeamNumber() )
			return;
	}

	if ( m_pSaveMeEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_pSaveMeEffect );
		m_pSaveMeEffect = NULL;
	}

	m_pSaveMeEffect = ParticleProp()->Create( "speech_mediccall", PATTACH_POINT_FOLLOW, "head" );

	if ( m_pSaveMeEffect )
	{
		// make the medic call as red the victim's health
		float flHealth = clamp( ( float ) GetHealth() / ( float ) GetMaxHealth(), 0.01f, 1.0f );

		m_pSaveMeEffect->SetControlPoint( 1, Vector( flHealth ) );
	}

	// If the local player has a medigun, add this player to our list of medic callers
	if ( pLocalPlayer && pLocalPlayer->IsAlive() == true )
	{
		CTFWeaponBase *pWpn = (CTFWeaponBase *)Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
		CTFWeaponBase *pWpn2 = (CTFWeaponBase *)Weapon_OwnsThisID( TFC_WEAPON_MEDKIT );

		if ( pWpn != NULL)
		{
			Vector vecPos;
			if ( GetAttachmentLocal( LookupAttachment( "head" ), vecPos ) )
			{
				vecPos += Vector(0,0,18);	// Particle effect is 18 units above the attachment
				CTFMedicCallerPanel::AddMedicCaller( this, 5.0, vecPos );
			}
		}
		else if ( pWpn2 != NULL)
		{
			Vector vecPos;
			if ( GetAttachmentLocal( LookupAttachment( "head" ), vecPos ) )
			{
				vecPos += Vector(0,0,18);	// Particle effect is 18 units above the attachment
				CTFMedicCallerPanel::AddMedicCaller( this, 5.0, vecPos );
			}
		}
		else
		{
			return;
		}
	}
}

void C_TFPlayer::CreateChattingEffect(void)
{
	// Don't create them for the local player
	// if ( IsLocalPlayer() && !ShouldDrawLocalPlayer() )
	if ( IsLocalPlayer() )
		return;

	// If I'm disguised as the enemy, don't create
	// if ( !m_Shared.InCond( TF_COND_DISGUISED ) && m_bChatting )
	if ( ( !m_Shared.InCondInvis() ) && m_bChatting && IsAlive() )
	{
		if ( !m_pChattingEffect ) 
		{
			// this uses the unused particle
			m_pChattingEffect = ParticleProp()->Create( "speech_typing", PATTACH_POINT_FOLLOW, "head" );
		}
	}
	// kill the chat bubble if we aren't typing anymore
	else
	{
		if ( m_pChattingEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pChattingEffect );
			m_pChattingEffect = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsOverridingViewmodel( void )
{
	C_TFPlayer *pPlayer = this;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && 
		 pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = assert_cast<C_TFPlayer*>(pLocalPlayer->GetObserverTarget());
	}

	if ( pPlayer->m_Shared.InCondUber() )
		return true;

	return BaseClass::IsOverridingViewmodel();
}

//-----------------------------------------------------------------------------
// Purpose: Draw my viewmodel in some special way
//-----------------------------------------------------------------------------
int	C_TFPlayer::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int ret = 0;

	C_TFPlayer *pPlayer = this;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && 
		pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = assert_cast<C_TFPlayer*>(pLocalPlayer->GetObserverTarget());
	}

	if ( pPlayer->m_Shared.InCondUber() )
	{
		// Force the invulnerable material
		modelrender->ForcedMaterialOverride( *pPlayer->GetInvulnMaterialRef() );
		ret = pViewmodel->DrawOverriddenViewmodel( flags );
		
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetHealer( C_TFPlayer *pHealer, float flChargeLevel )
{
	// We may be getting healed by multiple healers. Show the healer
	// who's got the highest charge level.
	if ( m_hHealer )
	{
		if ( m_flHealerChargeLevel > flChargeLevel )
			return;
	}

	m_hHealer = pHealer;
	m_flHealerChargeLevel = flChargeLevel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float C_TFPlayer::MedicGetChargeLevel( void )
{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

		if ( pWpn == NULL )
			return 0;

		CWeaponMedigun *pWeapon = dynamic_cast <CWeaponMedigun*>( pWpn );

		if ( pWeapon )
			return pWeapon->GetChargeLevel();
		else
			return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CanShowClassMenu( void )
{
	if ( TFGameRules()->IsESCGamemode() && m_PlayerClass.GetClassIndex() == TF_CLASS_CIVILIAN  
				&& TFGameRules()->GetMaxHunted( GetTeamNumber() ) != 0 && TFGameRules()->GetMaxHunted( GetTeamNumber() ) >= TFGameRules()->GetHuntedCount( GetTeamNumber() ) )
		return false;

	if ( this )
		return ( GetTeamNumber() > LAST_SHARED_TEAM );

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::InitializePoseParams( void )
{
	/*
	m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );
	*/

	CStudioHdr *hdr = GetModelPtr();
	Assert( hdr );
	if ( !hdr )
		return;

	for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector C_TFPlayer::GetChaseCamViewOffset( CBaseEntity *target )
{
	if ( target->IsBaseObject() )
		return Vector(0,0,64);

	return BaseClass::GetChaseCamViewOffset( target );
}

//-----------------------------------------------------------------------------
// Purpose: Called from PostDataUpdate to update the model index
//-----------------------------------------------------------------------------
void C_TFPlayer::ValidateModelIndex( void )
{
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && IsEnemyPlayer() )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( m_Shared.GetDisguiseClass() );
		m_nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );
	}
	else
	{
		C_TFPlayerClass *pClass = GetPlayerClass();
		if ( pClass )
		{
			m_nModelIndex = modelinfo->GetModelIndex( pClass->GetModelName() );
		}
	}

	BaseClass::ValidateModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
void C_TFPlayer::Simulate( void )
{
	//Frame updates
	//if ( this == C_BasePlayer::GetLocalPlayer() )
	if ( IsLocalPlayer() )
	{
		//Update the flashlight
		Flashlight();
	}

	// TF doesn't do step sounds based on velocity, instead using anim events
	// So we deliberately skip over the base player simulate, which calls them.
	BaseClass::BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == 7001 )
	{
		// Force a footstep sound
		m_flStepSoundTime = 0;
		Vector vel;
		EstimateAbsVelocity( vel );
		UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
	}
	else if ( event == AE_WPN_HIDE )
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( false );
		}
	}
	else if ( event == AE_WPN_UNHIDE )
	{
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->SetWeaponVisible( true );
		}
	}
	else if ( event == AE_CL_BODYGROUP_SET_VALUE )
	{
		CTFWeaponBase *pTFWeapon = GetActiveTFWeapon();
		
		if ( pTFWeapon )
		{
			// hacky!
			C_BaseAnimating *pWeapon = pTFWeapon->GetOwnModel();

			if ( pWeapon )
			{
				pTFWeapon->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
			}
		}
	}

	else if ( event == TF_AE_CIGARETTE_THROW )
	{
		CEffectData data;
		int iAttach = LookupAttachment( options );
		GetAttachment( iAttach, data.m_vOrigin, data.m_vAngles );

		data.m_vAngles = GetRenderAngles();

		data.m_hEntity = ClientEntityList().EntIndexToHandle( entindex() );
		DispatchEffect( "TF_ThrowCigarette", data );
		return;
	}
	else
		BaseClass::FireEvent( origin, angles, event, options );
}

void C_TFPlayer::FireGameEvent( IGameEvent *event )
{
	if( C_TFPlayer::GetLocalTFPlayer() && C_TFPlayer::GetLocalTFPlayer() == this )
		return;

	const char *eventname = event->GetName();

	if ( !Q_strcmp( "player_jump", eventname ))
	{
		if( !of_jumpsound.GetBool() )
			return;
		
		if ( event->GetInt("playerid") != entindex() )
			return;
		
		if ( GetPlayerClass()->GetClassIndex() > 9 || of_jumpsound.GetInt() == 2 )
			EmitSound( GetPlayerClass()->GetJumpSound() );
	}
}


// Shadows

ConVar cl_blobbyshadows( "cl_blobbyshadows", "0", FCVAR_CLIENTDLL );
ShadowType_t C_TFPlayer::ShadowCastType( void ) 
{
	// Removed the GetPercentInvisible - should be taken care off in BindProxy now.
	if ( !IsVisible() /*|| GetPercentInvisible() > 0.0f*/ )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	if ( m_nRenderFX == kRenderFxRagdoll )
		return SHADOWS_NONE;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// if we're first person spectating this player
	if ( pLocalPlayer && 
		pLocalPlayer->GetObserverTarget() == this &&
		pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		return SHADOWS_NONE;		
	}

	if( cl_blobbyshadows.GetBool() )
		return SHADOWS_SIMPLE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

float g_flFattenAmt = 4;
void C_TFPlayer::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Don't let the render bounds change when we're using blobby shadows, or else the shadow
		// will pop and stretch.
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();
	}
	else
	{
		GetRenderBounds( mins, maxs );

		// We do this because the normal bbox calculations don't take pose params into account, and 
		// the rotation of the guy's upper torso can place his gun a ways out of his bbox, and 
		// the shadow will get cut off as he rotates.
		//
		// Thus, we give it some padding here.
		mins -= Vector( g_flFattenAmt, g_flFattenAmt, 0 );
		maxs += Vector( g_flFattenAmt, g_flFattenAmt, 0 );
	}
}


void C_TFPlayer::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	// TODO POSTSHIP - this hack/fix goes hand-in-hand with a fix in CalcSequenceBoundingBoxes in utils/studiomdl/simplify.cpp.
	// When we enable the fix in CalcSequenceBoundingBoxes, we can get rid of this.
	//
	// What we're doing right here is making sure it only uses the bbox for our lower-body sequences since,
	// with the current animations and the bug in CalcSequenceBoundingBoxes, are WAY bigger than they need to be.
	C_BaseAnimating::GetRenderBounds( theMins, theMaxs );
}


bool C_TFPlayer::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{ 
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Blobby shadows should sit directly underneath us.
		pDirection->Init( 0, 0, -1 );
		return true;
	}
	else
	{
		return BaseClass::GetShadowCastDirection( pDirection, shadowType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is the nemesis of the local player
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsNemesisOfLocalPlayer()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		// return whether this player is dominating the local player
		return m_Shared.IsPlayerDominated( pLocalPlayer->entindex() );
	}		
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether we should show the nemesis icon for this player
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldShowNemesisIcon()
{
	// we should show the nemesis effect on this player if he is the nemesis of the local player,
	// and is not dead, cloaked or disguised
	if ( IsNemesisOfLocalPlayer() && g_PR && g_PR->IsConnected( entindex() ) )
	{
		bool bStealthed = m_Shared.InCondInvis();
		bool bDisguised = m_Shared.InCond( TF_COND_DISGUISED );
		if ( IsAlive() && !bStealthed && !bDisguised )
			return true;
	}
	return false;
}

bool C_TFPlayer::IsWeaponLowered( void )
{
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();

	if ( !pWeapon )
		return false;

	CTFGameRules *pRules = TFGameRules();

	// Lower losing team's weapons in bonus round
	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() || ( pRules->GetWinningTeam() == TF_TEAM_MERCENARY && !m_Shared.IsTopThree() ) ) )
		return true;

	// Hide all view models after the game is over
	if ( pRules->State_Get() == GR_STATE_GAME_OVER )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
		return StartGestureSceneEvent( info, scene, event, actor, pTarget );
	default:
		return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFPlayer::StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	// Get the (gesture) sequence.
	info->m_nSequence = LookupSequence( event->GetParameters() );
	if ( info->m_nSequence < 0 )
		return false;

	// Player the (gesture) sequence.
	m_PlayerAnimState->AddVCDSequenceToGestureSlot( GESTURE_SLOT_VCD, info->m_nSequence );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFPlayer::GetNumActivePipebombs( void )
{
	if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		CTFPipebombLauncher *pWeapon = dynamic_cast < CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );

		if ( pWeapon )
		{
			return pWeapon->GetPipeBombCount();
		}
	}

	return 0;
}

bool C_TFPlayer::IsAllowedToSwitchWeapons( void )
{
	if ( IsWeaponLowered() == true )
		return false;

	return BaseClass::IsAllowedToSwitchWeapons();
}

IMaterial *C_TFPlayer::GetHeadLabelMaterial( void )
{
	if ( g_pHeadLabelMaterial[0] == NULL )
		SetupHeadLabelMaterials();

	if ( GetTeamNumber() == TF_TEAM_RED )
	{
		return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_RED];
	}
	else if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_BLUE];
	}
	else
	{
		return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_MERCENARY];
	}
	return BaseClass::GetHeadLabelMaterial();
}

void SetupHeadLabelMaterials( void )
{
	for ( int i = 0; i < 3; i++ )
	{
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->DecrementReferenceCount();
			g_pHeadLabelMaterial[i] = NULL;
		}

		g_pHeadLabelMaterial[i] = materials->FindMaterial( pszHeadLabelNames[i], TEXTURE_GROUP_VGUI );
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->IncrementReferenceCount();
		}
	}
}

void C_TFPlayer::ComputeFxBlend( void )
{
	BaseClass::ComputeFxBlend();

	if ( GetPlayerClass()->IsClass( TF_CLASS_SPY ) )
	{
		float flInvisible = GetPercentInvisible();
		if ( flInvisible != 0.0f )
		{
			// Tell our shadow
			ClientShadowHandle_t hShadow = GetShadowHandle();
			if ( hShadow != CLIENTSHADOW_INVALID_HANDLE )
			{
				g_pClientShadowMgr->SetFalloffBias( hShadow, flInvisible * 255 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	HandleTaunting();
	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

static void cc_tf_crashclient()
{
	C_TFPlayer *pPlayer = NULL;
	pPlayer->ComputeFxBlend();
}
static ConCommand tf_crashclient( "tf_crashclient", cc_tf_crashclient, "Crashes this client for testing.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::ForceUpdateObjectHudState( void )
{
	m_bUpdateObjectHudState = true;
}

#include "c_obj_sentrygun.h"


static void cc_tf_debugsentrydmg()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	pPlayer->UpdateIDTarget();
	int iTarget = pPlayer->GetIDTarget();
	if ( iTarget > 0 )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( iTarget );

		C_ObjectSentrygun *pSentry = dynamic_cast< C_ObjectSentrygun * >( pEnt );

		if ( pSentry )
		{
			pSentry->DebugDamageParticles();
		}
	}
}
static ConCommand tf_debugsentrydamage( "tf_debugsentrydamage", cc_tf_debugsentrydmg, "", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: HL1's view bob, roll and idle effects.
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcVehicleView(IClientVehicle* pVehicle, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov)
{
	BaseClass::CalcVehicleView(pVehicle, eyeOrigin, eyeAngles, zNear, zFar, fov);

	if (pVehicle != nullptr)
	{
		if (pVehicle->GetVehicleEnt() != nullptr)
		{
			Vector Velocity;
			pVehicle->GetVehicleEnt()->EstimateAbsVelocity(Velocity);

			if (Velocity.Length() == 0)
			{
				IdleScale += gpGlobals->frametime * 0.05;
				if (IdleScale > 1.0)
					IdleScale = 1.0;
			}
			else
			{
				IdleScale -= gpGlobals->frametime;
				if (IdleScale < 0.0)
					IdleScale = 0.0;
			}
			CalcViewIdle(eyeAngles);
		}
	}
}

void C_TFPlayer::CalcPlayerView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	BaseClass::CalcPlayerView(eyeOrigin, eyeAngles, fov);

	Vector Velocity;
	EstimateAbsVelocity(Velocity);
	if ( m_Shared.InCond ( TF_COND_AIMING ) || !of_idleview.GetBool() )
		IdleScale = 0.0;
	
	if (Velocity.Length() == 0 && !m_Shared.InCond( TF_COND_AIMING ) && of_idleview.GetBool() )
	{
		IdleScale += gpGlobals->frametime * 0.05;
		if (IdleScale > 1.0)
			IdleScale = 1.0;
	}
	else
	{
		IdleScale -= gpGlobals->frametime;
		if (IdleScale < 0.0)
			IdleScale = 0.0;
	}

	CalcViewBob(eyeOrigin);
	CalcViewIdle(eyeAngles);
}

ConVar cl_hl1_rollspeed("cl_hl1_rollspeed", "600.0", FCVAR_USERINFO | FCVAR_ARCHIVE ); // 300.0
ConVar cl_hl1_rollangle("cl_hl1_rollangle", "0.15", FCVAR_USERINFO | FCVAR_ARCHIVE ); // 0.65

void C_TFPlayer::CalcViewRoll( QAngle& eyeAngles )
{
	if (GetMoveType() == MOVETYPE_NOCLIP)
		return;

	float Side = CalcRoll(GetAbsAngles(), GetAbsVelocity(), cl_hl1_rollangle.GetFloat(), cl_hl1_rollspeed.GetFloat()) * 4.0;
	eyeAngles[ROLL] += Side;

	if (GetHealth() <= 0)
	{
		eyeAngles[ROLL] = 80;
		return;
	}
}

ConVar of_viewbobcycle("of_viewbobcycle", "0.8", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar of_viewbob("of_viewbob", "0.01", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar of_viewbobup("of_viewbobup", "0.2", FCVAR_USERINFO | FCVAR_ARCHIVE );

void C_TFPlayer::CalcViewBob( Vector& eyeOrigin )
{
	float Cycle;
	Vector Velocity;

	if (GetGroundEntity() == nullptr || gpGlobals->curtime == BobLastTime ||  m_Shared.InCond ( TF_COND_AIMING ) || !of_viewbobcycle.GetBool() )
	{
		eyeOrigin.z += ViewBob;
		return;
	}

	BobLastTime = gpGlobals->curtime;
	BobTime += gpGlobals->frametime;

	Cycle = BobTime - (int)(BobTime / of_viewbobcycle.GetFloat()) * of_viewbobcycle.GetFloat();
	Cycle /= of_viewbobcycle.GetFloat();

	if (Cycle < of_viewbobup.GetFloat())
		Cycle = M_PI * Cycle / of_viewbobup.GetFloat();
	else
		Cycle = M_PI + M_PI * (Cycle - of_viewbobup.GetFloat()) / (1.0 - of_viewbobup.GetFloat());

	EstimateAbsVelocity(Velocity);
	Velocity.z = 0;

	ViewBob = sqrt(Velocity.x * Velocity.x + Velocity.y * Velocity.y) * of_viewbob.GetFloat();
	ViewBob = ViewBob * 0.3 + ViewBob * 0.7 * sin(Cycle);
	ViewBob = min(ViewBob, 4);
	ViewBob = max(ViewBob, -7);

	eyeOrigin.z += ViewBob;
}

ConVar cl_hl1_iyaw_cycle("cl_hl1_iyaw_cycle", "2.0", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar cl_hl1_iroll_cycle("cl_hl1_iroll_cycle", "0.5", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar cl_hl1_ipitch_cycle("cl_hl1_ipitch_cycle", "1.0", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar cl_hl1_iyaw_level("cl_hl1_iyaw_level", "0.3", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar cl_hl1_iroll_level("cl_hl1_iroll_level", "0.1", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar cl_hl1_ipitch_level("cl_hl1_ipitch_level", "0.3", FCVAR_USERINFO | FCVAR_ARCHIVE );

void C_TFPlayer::CalcViewIdle(QAngle& eyeAngles)
{
	eyeAngles[ROLL] += IdleScale * sin(gpGlobals->curtime * cl_hl1_iroll_cycle.GetFloat()) * cl_hl1_iroll_level.GetFloat();
	eyeAngles[PITCH] += IdleScale * sin(gpGlobals->curtime * cl_hl1_ipitch_cycle.GetFloat()) * cl_hl1_ipitch_level.GetFloat();
	eyeAngles[YAW] += IdleScale * sin(gpGlobals->curtime * cl_hl1_iyaw_cycle.GetFloat()) * cl_hl1_iyaw_level.GetFloat();
}

int C_TFPlayer::GetAccount() const
{
	return m_iAccount;
}

void C_TFPlayer::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );

	if ( cl_first_person_uses_world_model.GetBool() )
		BuildFirstPersonMeathookTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed, "bip_head" );
}