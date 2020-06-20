//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_pipebomblauncher.h"

// Client specific.
#ifdef CLIENT_DLL
// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "prediction.h"
#include "iefx.h"
#include "dlight.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#else
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "IEffects.h"
#include "props.h"
#include "func_respawnroom.h"

#endif

#define TF_WEAPON_PIPEBOMB_TIMER		3.0f //Seconds

#define TF_WEAPON_PIPEBOMB_GRAVITY		0.5f
#define TF_WEAPON_PIPEBOMB_FRICTION		0.8f
#define TF_WEAPON_PIPEBOMB_ELASTICITY	0.45f

#define TF_WEAPON_PIPEBOMB_TIMER_DMG_REDUCTION		0.6

extern ConVar tf_grenadelauncher_max_chargetime;
ConVar tf_grenadelauncher_chargescale( "tf_grenadelauncher_chargescale", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED );
ConVar tf_grenadelauncher_livetime( "tf_grenadelauncher_livetime", "0.8", FCVAR_CHEAT | FCVAR_REPLICATED );

#ifdef CLIENT_DLL
extern ConVar of_muzzlelight;
#else
ConVar of_stabilize_grenades("of_stabilize_grenades", "0", FCVAR_REPLICATED, "Testing convar, only used to reduce crashes if they happen.");
#endif

#ifndef CLIENT_DLL
ConVar tf_grenadelauncher_min_contact_speed( "tf_grenadelauncher_min_contact_speed", "100" );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )

BEGIN_NETWORK_TABLE( CTFGrenadePipebombProjectile, DT_TFProjectile_Pipebomb )
#ifdef CLIENT_DLL
RecvPropInt( RECVINFO( m_bTouched ) ),
RecvPropInt( RECVINFO( m_iType ) ),
#else
SendPropBool( SENDINFO( m_bTouched ) ),
SendPropInt( SENDINFO( m_iType ), 2 ),
#endif
END_NETWORK_TABLE()

static const char *s_PipebombModel;

#ifdef GAME_DLL
static string_t s_iszTrainName;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::CTFGrenadePipebombProjectile()
{
	m_bTouched = false;
	m_flChargeTime = 0.0f;
#ifdef GAME_DLL
	s_iszTrainName  = AllocPooledString( "models/props_vehicles/train_enginecar.mdl" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile::~CTFGrenadePipebombProjectile()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFGrenadePipebombProjectile::GetDamageType( void )
{
	int iDmgType = BaseClass::GetDamageType();

	// If we're a pipebomb, we do distance based damage falloff for just the first few seconds of our life
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		if ( gpGlobals->curtime - m_flCreationTime < 5.0 )
		{
			iDmgType |= DMG_USEDISTANCEMOD;
		}
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::UpdateOnRemove( void )
{
	// Tell our launcher that we were removed
	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher*>( GetOriginalLauncher() );

	if ( pLauncher )
	{
		pLauncher->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}

#ifdef CLIENT_DLL
//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Client specific).
//

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CTFGrenadePipebombProjectile::GetTrailParticleName( void )
{
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			return "stickybombtrail_blue";
		}
		else if ( GetTeamNumber() == TF_TEAM_RED)
		{
			return "stickybombtrail_red";
		}
		else
		{
			return "stickybombtrail_dm";
		}
	}
	else
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			return "pipebombtrail_blue";
		}
		else if ( GetTeamNumber() == TF_TEAM_RED )
		{
			return "pipebombtrail_red";
		}
		else
		{
			return "pipebombtrail_dm";
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_flCreationTime = gpGlobals->curtime;
		CNewParticleEffect *pParticle = ParticleProp()->Create( GetTrailParticleName(), PATTACH_ABSORIGIN_FOLLOW );
		
		C_TFPlayer *pPlayer = ToTFPlayer( GetThrower() );
		m_bPulsed = false;

		if ( m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			CreateLightEffects();
		}

		if ( pPlayer && pParticle )
		{
			pPlayer->m_Shared.UpdateParticleColor( pParticle );
		}

		CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher*>( GetOriginalLauncher() );

		if ( pLauncher )
		{
			pLauncher->AddPipeBomb( this );
		}

		if ( m_bCritical )
		{
			switch( GetTeamNumber() )
			{
			case TF_TEAM_BLUE:

				if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
				{
					ParticleProp()->Create( "critical_grenade_blue", PATTACH_ABSORIGIN_FOLLOW );
				}
				else
				{
					ParticleProp()->Create( "critical_pipe_blue", PATTACH_ABSORIGIN_FOLLOW );
				}
				break;
			case TF_TEAM_RED:
				if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
				{
					ParticleProp()->Create( "critical_grenade_red", PATTACH_ABSORIGIN_FOLLOW );
				}
				else
				{
					ParticleProp()->Create( "critical_pipe_red", PATTACH_ABSORIGIN_FOLLOW );
				}
				break;
			case TF_TEAM_MERCENARY:

				if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
				{
					if ( pPlayer )
						pPlayer->m_Shared.UpdateParticleColor( ParticleProp()->Create( "critical_grenade_dm", PATTACH_ABSORIGIN_FOLLOW ) );
					else
						ParticleProp()->Create( "critical_grenade_dm", PATTACH_ABSORIGIN_FOLLOW );
				}
				else
				{
					if ( pPlayer )
						pPlayer->m_Shared.UpdateParticleColor( ParticleProp()->Create( "critical_pipe_dm", PATTACH_ABSORIGIN_FOLLOW ) );
					else
						ParticleProp()->Create( "critical_pipe_dm", PATTACH_ABSORIGIN_FOLLOW );
				}
				break;
			default:
				break;
			}
		}

	}
	else if ( m_bTouched )
	{
		//ParticleProp()->StopEmission();
	}
}
extern ConVar tf_grenadelauncher_livetime;

void CTFGrenadePipebombProjectile::Simulate( void )
{
	BaseClass::Simulate();

	if ( m_iType != TF_GL_MODE_REMOTE_DETONATE )
		return;

	if ( m_bPulsed == false )
	{
		if ( (gpGlobals->curtime - m_flCreationTime) >= tf_grenadelauncher_livetime.GetFloat() )
		{
			if ( GetTeamNumber() == TF_TEAM_RED )
			{
				ParticleProp()->Create( "stickybomb_pulse_red", PATTACH_ABSORIGIN );
			}
			else if ( GetTeamNumber() == TF_TEAM_BLUE )
			{
				ParticleProp()->Create( "stickybomb_pulse_blue", PATTACH_ABSORIGIN );
			}
			else 
			{
				ParticleProp()->Create( "stickybomb_pulse_dm", PATTACH_ABSORIGIN );
			}
			m_bPulsed = true;
		}
	}
}

void CTFGrenadePipebombProjectile::CreateLightEffects(void)
{
	if ( m_iType != TF_GL_MODE_REMOTE_DETONATE )
	{
	// Handle the dynamic light
	if ( of_muzzlelight.GetBool() )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( GetThrower() );
		dlight_t *dl;
		AddEffects(EF_DIMLIGHT);
		if ( IsEffectActive(EF_DIMLIGHT) )
		{
			dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC + index);
			dl->origin = GetAbsOrigin();
			dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
			switch ( GetTeamNumber() )
			{
				case TF_TEAM_RED:
					if (!m_bCritical) 
					{
						dl->color.r = 255; dl->color.g = 30; dl->color.b = 10; dl->style = 0;
					}
					else 
					{
						dl->color.r = 255; dl->color.g = 10; dl->color.b = 10; dl->style = 1;
					}
					break;
				case TF_TEAM_BLUE:
					if (!m_bCritical) 
					{
						dl->color.r = 10; dl->color.g = 30; dl->color.b = 255; dl->style = 0;
					}
					else 
					{
						dl->color.r = 10; dl->color.g = 10; dl->color.b = 255; dl->style = 1;
					}
					break;
				case TF_TEAM_MERCENARY:
					if (!pPlayer)
						break;
					float r = pPlayer->m_vecPlayerColor.x * 255;
					float g = pPlayer->m_vecPlayerColor.y * 255;
					float b = pPlayer->m_vecPlayerColor.z * 255;
					if ( r < TF_LIGHT_COLOR_CLAMP && g < TF_LIGHT_COLOR_CLAMP && b < TF_LIGHT_COLOR_CLAMP )
					{
						float maxi = max(max(r, g), b);
						maxi = TF_LIGHT_COLOR_CLAMP - maxi;
						r += maxi;
						g += maxi;
						b += maxi;
					}
					if (!m_bCritical) 
					{
						dl->color.r = r; dl->color.g = g ; dl->color.b = b ; dl->style = 0;
					}
					else 
					{
						dl->color.r = r; dl->color.g = g; dl->color.b = b; dl->style = 1;
					}
					break;
			}
			dl->die = gpGlobals->curtime + 0.01f;
			dl->radius = 256.0f;
			dl->decay = 512.0f;
			dl->die = gpGlobals->curtime + 0.001;

			tempents->RocketFlare(GetAbsOrigin() );
		}
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
// Input  : flags - 
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::DrawModel( int flags )
{
	if ( gpGlobals->curtime < ( m_flCreationTime + 0.1 ) )
		return 0;

	return BaseClass::DrawModel( flags );
}

#else

//=============================================================================
//
// TF Pipebomb Grenade Projectile functions (Server specific).
//
#define TF_WEAPON_PIPEGRENADE_MODEL		"models/weapons/w_models/w_grenade_grenadelauncher.mdl"
#define TF_WEAPON_PIPEBOMB_MODEL		"models/weapons/w_models/w_stickybomb.mdl"
#define TF_WEAPON_PIPEBOMB_BOUNCE_SOUND	"Weapon_Grenade_Pipebomb.Bounce"
#define TF_WEAPON_GRENADE_DETONATE_TIME 2.0f
#define TF_WEAPON_GRENADE_XBOX_DAMAGE 112

BEGIN_DATADESC( CTFGrenadePipebombProjectile )
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_projectile_pipe_remote, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_projectile_pipe_remote );

LINK_ENTITY_TO_CLASS( tf_projectile_pipe, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_projectile_pipe );

LINK_ENTITY_TO_CLASS( tf_projectile_pipe_dm, CTFGrenadePipebombProjectile );
PRECACHE_WEAPON_REGISTER( tf_projectile_pipe_dm );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGrenadePipebombProjectile* CTFGrenadePipebombProjectile::Create( const Vector &position, const QAngle &angles, 
																    const Vector &velocity, const AngularImpulse &angVelocity, 
																    CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, bool bRemoteDetonate, CTFWeaponBase *pWeapon )
{
	CTFGrenadePipebombProjectile *pGrenade = static_cast<CTFGrenadePipebombProjectile*>( CBaseEntity::CreateNoSpawn( bRemoteDetonate ? "tf_projectile_pipe_remote" : "tf_projectile_pipe", position, angles, pOwner ) );
	if ( pGrenade )
	{
		// Set the pipebomb mode before calling spawn, so the model & associated vphysics get setup properly
		CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( pWeapon );
		if ( pTFWeapon )
		{
			if ( pTFWeapon->GetTFWpnData().m_nProjectileModel[0] != 0 )
			{
				s_PipebombModel =  pTFWeapon->GetTFWpnData().m_nProjectileModel;	
			}
			else
			{
				s_PipebombModel = NULL;
			}
			if ( pTFWeapon->GetTFWpnData().m_bExplodeOnImpact )
			{
				pGrenade->SetTouch( &CTFGrenadePipebombProjectile::PipebombTouch );
			}
			if ( pTFWeapon->GetTFWpnData().m_flFuseTime != -1 )
			{
				if ( pTFWeapon->GetTFWpnData().m_flFuseTime < -1 )
					pGrenade->SetDetonateTimerLength( FLT_MAX );
				else
					pGrenade->SetDetonateTimerLength( pTFWeapon->GetTFWpnData().m_flFuseTime );
			}
			
		}
		pGrenade->SetPipebombMode( bRemoteDetonate );
		DispatchSpawn( pGrenade );
		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo, pWeapon );
		pGrenade->m_flFullDamage = pGrenade->GetDamage();

		if ( pGrenade->m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			// Some hackery here. Reduce the damage by 25%, so that if we explode on timeout,
			// we'll do less damage. If we explode on contact, we'll restore this to full damage.
			pGrenade->SetDamage( pGrenade->GetDamage() * TF_WEAPON_PIPEBOMB_TIMER_DMG_REDUCTION );
		}
		pGrenade->ApplyLocalAngularVelocityImpulse( angVelocity );
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Spawn()
{
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		// Set this to max, so effectively they do not self-implode.
		if ( GetWeaponID() != TF_WEAPON_GRENADE_MIRVBOMB )
		{		
			SetModel( TF_WEAPON_PIPEBOMB_MODEL );
		}
		if ( !GetDetonateTime() )
		SetDetonateTimerLength( FLT_MAX );
	}
	else
	{
		if ( GetWeaponID() != TF_WEAPON_GRENADE_MIRVBOMB )
		{		
			SetModel( TF_WEAPON_PIPEGRENADE_MODEL );
		}
		if ( !GetDetonateTime() )
		SetDetonateTimerLength( TF_WEAPON_GRENADE_DETONATE_TIME );
		SetTouch( &CTFGrenadePipebombProjectile::PipebombTouch );
#ifdef CLIENT_DLL
		if ( m_iType != TF_GL_MODE_REMOTE_DETONATE )
		{
			CreateLightEffects();
		}
#endif
	}
	if ( s_PipebombModel && GetWeaponID() != TF_WEAPON_GRENADE_MIRVBOMB )
	{
		PrecacheModel( s_PipebombModel );
		SetModel( s_PipebombModel );
	}

	BaseClass::Spawn();

	m_bTouched = false;
	m_flCreationTime = gpGlobals->curtime;

	// We want to get touch functions called so we can damage enemy players
	AddSolidFlags( FSOLID_TRIGGER );

	m_flMinSleepTime = 0;
}

void CTFGrenadePipebombProjectile::Explode( trace_t *pTrace, int bitsDamageType, int bitsCustomDamageType )
{
	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( GetOriginalLauncher() );
	// HACK HACK HACK
	// Please if you somehow know how to fix m_hLauncher not being able to get pulled in tf_weaponbase_grenadeproj.cpp
	// PLEASE fix it, idk ill suck your dick or something, i've been trying to get this to work for over 3 days straight - Kay
	if ( pTFWeapon ) 
		pFuckThisShit = pTFWeapon;
	BaseClass::Explode( pTrace, bitsDamageType, bitsCustomDamageType );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Precache()
{	
	PrecacheModel( TF_WEAPON_PIPEBOMB_MODEL );
	PrecacheModel( TF_WEAPON_PIPEGRENADE_MODEL );
	PrecacheParticleSystem( "stickybombtrail_blue" );
	PrecacheParticleSystem( "stickybombtrail_red" );
	PrecacheParticleSystem( "stickybombtrail_dm" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::SetPipebombMode( bool bRemoteDetonate )
{
	Precache();
	if ( bRemoteDetonate )
	{
		m_iType.Set( TF_GL_MODE_REMOTE_DETONATE );
	}
	else
	{
		SetModel( TF_WEAPON_PIPEBOMB_MODEL );
	}
	if ( s_PipebombModel )
	{
		PrecacheModel( s_PipebombModel );
		SetModel( s_PipebombModel );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::BounceSound( void )
{
	EmitSound( TF_WEAPON_PIPEBOMB_BOUNCE_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Detonate()
{
	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	if ( m_bFizzle )
	{
		g_pEffects->Sparks( GetAbsOrigin() );
		RemoveGrenade();
		return;
	}
	BaseClass::Detonate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::Fizzle( void )
{
	m_bFizzle = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::PipebombTouch( CBaseEntity *pOther )
{
	if ( pOther == GetThrower() )
		return;

	// Verify a correct "other."
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) )
		return;

	// Handle hitting skybox (disappear).
	trace_t pTrace;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &pTrace );

	if ( pTrace.fraction < 1.0 && pTrace.surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}
	
	//If we already touched a surface then we're not exploding on contact anymore.
	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( GetOriginalLauncher() );
	if( pTFWeapon )
	{
		if ( !pTFWeapon->GetTFWpnData().m_bAlwaysEnableTouch && m_bTouched == true )
			return;
	}
	else
	{
		if ( m_bTouched == true )
			return;
	}
	// Blow up if we hit an enemy we can damage
	if ( pOther->GetTeamNumber() && ( pOther->GetTeamNumber() != GetTeamNumber() || pOther->GetTeamNumber() == TF_TEAM_MERCENARY )&& pOther->m_takedamage != DAMAGE_NO )
	{
		// Check to see if this is a respawn room.
		if ( !pOther->IsPlayer() )
		{
			CFuncRespawnRoom *pRespawnRoom = dynamic_cast<CFuncRespawnRoom*>( pOther );
			if ( pRespawnRoom )
			{
				if ( !pRespawnRoom->PointIsWithin( GetAbsOrigin() ) )
					return;
			}
		}

		// Restore damage. See comment in CTFGrenadePipebombProjectile::Create() above to understand this.
		m_flDamage = m_flFullDamage;

		// Save this entity as enemy, they will take 100% damage.
		m_hEnemy = pOther;

		Explode( &pTrace, GetDamageType(), GetCustomDamageType() );
	}

	// Train hack!
	if ( pOther->GetModelName() == s_iszTrainName && ( pOther->GetAbsVelocity().LengthSqr() > 1.0f ) )
	{
		Explode(&pTrace, GetDamageType(), GetCustomDamageType());
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadePipebombProjectile::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	if ( !pHitEntity )
		return;
	
	if ( ExplodeOnImpact() )
	{
		SetThink( &CTFGrenadePipebombProjectile::Detonate );
		SetNextThink( gpGlobals->curtime );
				
		m_bTouched = true;
		return;	
	}
	if ( m_iType == TF_GL_MODE_REGULAR )
	{
		// Blow up if we hit an enemy we can damage
		if ( pHitEntity->GetTeamNumber() && ( pHitEntity->GetTeamNumber() != GetTeamNumber() || pHitEntity->GetTeamNumber() == TF_TEAM_MERCENARY ) && pHitEntity->m_takedamage != DAMAGE_NO )
		{
			// Save this entity as enemy, they will take 100% damage.
			m_hEnemy = pHitEntity;

			SetThink( &CTFGrenadePipebombProjectile::Detonate );
			SetNextThink( gpGlobals->curtime );
		}
		
		m_bTouched = true;
		return;
	}

	// Handle hitting skybox (disappear).
	surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	if ( pprops->game.material == 'X' )
	{
		// uncomment to destroy grenade upon hitting sky brush
		//SetThink( &CTFGrenadePipebombProjectile::SUB_Remove );
		//SetNextThink( gpGlobals->curtime );
		return;
	}

	bool bIsDynamicProp = ( NULL != dynamic_cast<CDynamicProp *>( pHitEntity ) );

	// Pipebombs stick to the world when they touch it
	if ( pHitEntity  && ( pHitEntity->IsWorld() || bIsDynamicProp ) && gpGlobals->curtime > m_flMinSleepTime )
	{
		m_bTouched = true;
		VPhysicsGetObject()->EnableMotion( false );

		// Save impact data for explosions.
		m_bUseImpactNormal = true;
		pEvent->pInternalData->GetSurfaceNormal( m_vecImpactNormal );
		m_vecImpactNormal.Negate();
	}
}

bool CTFGrenadePipebombProjectile::ExplodeOnImpact( void )
{
	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( GetOriginalLauncher() );
	if (pTFWeapon &&
		(pTFWeapon->GetTFWpnData().m_bExplodeOnImpact ||
		(GetWeaponID() == TF_WEAPON_GRENADE_MIRVBOMB && pTFWeapon->GetTFWpnData().m_bBombletImpact)))
		return true;
	else if (pTFWeapon && !of_stabilize_grenades.GetBool() &&
		(pTFWeapon->GetTFWpnData().m_flImpactBeforeTime > 0.0f && m_flSpawnTime + pTFWeapon->GetTFWpnData().m_flImpactBeforeTime >= gpGlobals->curtime))
		return true;
	else if (pTFWeapon && of_stabilize_grenades.GetBool() &&
		(pTFWeapon->GetWeaponID() == TF_WEAPON_GRENADELAUNCHER_MERCENARY && m_flSpawnTime + 0.05f >= gpGlobals->curtime))
		return true;
	else
		return false;
}

ConVar tf_grenade_forcefrom_bullet( "tf_grenade_forcefrom_bullet", "0.8", FCVAR_CHEAT );
ConVar tf_grenade_forcefrom_buckshot( "tf_grenade_forcefrom_buckshot", "0.5", FCVAR_CHEAT );
ConVar tf_grenade_forcefrom_blast( "tf_grenade_forcefrom_blast", "0.08", FCVAR_CHEAT );
ConVar tf_grenade_force_sleeptime( "tf_grenade_force_sleeptime", "1.0", FCVAR_CHEAT );	// How long after being shot will we re-stick to the world.
ConVar tf_pipebomb_force_to_move( "tf_pipebomb_force_to_move", "1500.0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: If we are shot after being stuck to the world, move a bit
//-----------------------------------------------------------------------------
int CTFGrenadePipebombProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !info.GetAttacker() )
	{
		Assert( !info.GetAttacker() );
		return 0;
	}

	bool bSameTeam = ( info.GetAttacker()->GetTeamNumber() == GetTeamNumber() && ( info.GetAttacker()->GetTeamNumber() != TF_TEAM_MERCENARY || info.GetAttacker() == GetThrower() ) );

	if ( m_bTouched && ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT|DMG_BLAST|DMG_SLASH|DMG_CLUB) ) && bSameTeam == false )
	{
		Vector vecForce = info.GetDamageForce();
		if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
		{
			if ( info.GetDamageType() & (DMG_BULLET|DMG_BUCKSHOT|DMG_SLASH|DMG_CLUB) )
			{
				//m_bFizzle = false;
				m_bFizzle = true;
				Detonate();
			}
			else if ( info.GetDamageType() & DMG_BLAST )
			{
				vecForce *= tf_grenade_forcefrom_blast.GetFloat();
			}
		}
		else
		{
			if ( info.GetDamageType() & DMG_BULLET )
			{
				vecForce *= tf_grenade_forcefrom_bullet.GetFloat();
			}
			else if ( info.GetDamageType() & DMG_BUCKSHOT )
			{
				vecForce *= tf_grenade_forcefrom_buckshot.GetFloat();
			}
			else if ( info.GetDamageType() & DMG_BLAST )
			{
				vecForce *= tf_grenade_forcefrom_blast.GetFloat();
			}
		}

		// If the force is sufficient, detach & move the pipebomb
		float flForce = tf_pipebomb_force_to_move.GetFloat();
		if ( vecForce.LengthSqr() > (flForce*flForce) )
		{
			if ( VPhysicsGetObject() )
			{
				VPhysicsGetObject()->EnableMotion( true );
			}

			CTakeDamageInfo newInfo = info;
			newInfo.SetDamageForce( vecForce );

			VPhysicsTakeDamage( newInfo );

			// The pipebomb will re-stick to the ground after this time expires
			m_flMinSleepTime = gpGlobals->curtime + tf_grenade_force_sleeptime.GetFloat();
			m_bTouched = false;

			// It has moved the data is no longer valid.
			m_bUseImpactNormal = false;
			m_vecImpactNormal.Init();

			return 1;
		}
	}

	return 0;
}

#endif

#ifdef GAME_DLL
void CTFGrenadePipebombProjectile::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	if ( m_iType == TF_GL_MODE_REMOTE_DETONATE )
	{
		//Vector vecSrc = pDeflectedBy->GetAbsOrigin();
		Vector vecSrc = pDeflectedBy->WorldSpaceCenter();
		Vector vecDir2 = GetAbsOrigin() - vecSrc;
		VectorNormalize( vecDir2 );

		// hey, at least it works
		CTakeDamageInfo shitty_stickybomb_hack( pDeflectedBy, pDeflectedBy, 1000.0f, DMG_BLAST ); // terrible value
		CalculateExplosiveDamageForce( &shitty_stickybomb_hack, vecDir2, vecSrc );

		if ( VPhysicsGetObject() )
			VPhysicsGetObject()->EnableMotion( true );

		VPhysicsTakeDamage( shitty_stickybomb_hack );

		// The pipebomb will re-stick to the ground after this time expires
		m_flMinSleepTime = gpGlobals->curtime + tf_grenade_force_sleeptime.GetFloat();
		m_bTouched = false;

		// It has moved the data is no longer valid.
		m_bUseImpactNormal = false;
		m_vecImpactNormal.Init();
	}
	else
	{
		BaseClass::Deflected( pDeflectedBy, vecDir );
	}
}
#endif