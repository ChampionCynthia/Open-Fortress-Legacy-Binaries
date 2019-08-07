//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_PARSE_H
#define TF_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_parse.h"
#include "networkvar.h"
#include "tf_shareddefs.h"

//=============================================================================
//
// TF Weapon Info
//


struct WeaponData_t
{
	int		m_nDamage;
	int		m_nInstagibDamage;
	int		m_nBulletsPerShot;
	float	m_flRange;
	float	m_flSpread;
	float	m_flPunchAngle;
	float	m_flTimeFireDelay;				// Time to delay between firing
	float	m_flTimeIdle;					// Time to idle after firing
	float	m_flTimeIdleEmpty;				// Time to idle after firing last bullet in clip
	float	m_flTimeReloadStart;			// Time to start into a reload (ie. shotgun)
	float	m_flTimeReload;					// Time to reload
	bool	m_bDrawCrosshair;				// Should the weapon draw a crosshair
	int		m_iProjectile;					// The type of projectile this mode fires
	int		m_iAmmoPerShot;					// How much ammo each shot consumes
	float	m_flProjectileSpeed;			// Start speed for projectiles (nail, etc.); NOTE: union with something non-projectile
	float	m_flSmackDelay;					// how long after swing should damage happen for melee weapons
	float 	m_flMeleeRange;
	bool	m_bUseRapidFireCrits;
	bool	m_bCenterfireProjectile;
	
	float	m_flBurstFireDelay;				// Time to delay between firing
	int		m_nBurstSize;

	void Init( void )
	{
		m_nDamage = 0;
		m_nInstagibDamage = 9999;
		m_nBulletsPerShot = 0;
		m_flRange = 0.0f;
		m_flSpread = 0.0f;
		m_flPunchAngle = 0.0f;
		m_flTimeFireDelay = 0.0f;
		m_flTimeIdle = 0.0f;
		m_flTimeIdleEmpty = 0.0f;
		m_flTimeReloadStart = 0.0f;
		m_flTimeReload = 0.0f;
		m_iProjectile = TF_PROJECTILE_NONE;
		m_iAmmoPerShot = 0;
		m_flProjectileSpeed = 0.0f;
		m_flSmackDelay = 0.0f;
		m_bUseRapidFireCrits = false;
		
		m_flBurstFireDelay = 0.0f;
	};
};

class CTFWeaponInfo : public FileWeaponInfo_t
{
public:

	DECLARE_CLASS_GAMEROOT( CTFWeaponInfo, FileWeaponInfo_t );
	
	CTFWeaponInfo();
	~CTFWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	WeaponData_t const &GetWeaponData( int iWeapon ) const	{ return m_WeaponData[iWeapon]; }

public:

	WeaponData_t	m_WeaponData[2];

	int		m_iWeaponType;
	
	// Grenade.
	bool	m_bGrenade;
	float	m_flDamageRadius;
	float	m_flPrimerTime;
	bool	m_bLowerWeapon;
	bool	m_bSuppressGrenTimer;
	bool 	m_bExplodeOnImpact;
	char 	m_nProjectileModel[MAX_WEAPON_STRING];  // Will be adapted to work in rockets once we need that
	
	// Skins
	bool	m_bHasTeamSkins_Viewmodel;
	bool	m_bHasTeamSkins_Worldmodel;
	// Eye candy
	bool	m_bUsesCritAnimation;
	bool	m_bGibOnOverkill;
	bool	m_bGibOnHeadshot;	
	// Used exclusivley for the Berserk powerup for now so that you don't loose it in gun game
	bool	m_bNeverStrip;
	// Bomblet functions for the dynamite pack, may be used in the BFG later
	bool 	m_bDropBomblets;
	int 	m_iBombletAmount;
	float	m_flBombletTimer;
	float	m_flBombletMultiplier;
	int 	m_iBombletLevel;	
	// Misc
	bool 	m_bAllowDrop;
	float	m_flPickupMultiplier;
	bool	m_bDropOnNoAmmo;
	bool 	m_bDisableSecondaryAttack;
	int 	m_iCost;
	bool	m_bBuyable;
	// Mag
	char    m_szMagModel[128];
	int     m_iMagBodygroup;                // Body group of the magazine refer to the mag uneject function
	bool    m_bDropsMag;


	//Viewmodels
	char					szScoutViewModel[MAX_WEAPON_STRING];
	char					szSoldierViewModel[MAX_WEAPON_STRING];
	char					szPyroViewModel[MAX_WEAPON_STRING];
	char					szDemomanViewModel[MAX_WEAPON_STRING];
	char					szHeavyViewModel[MAX_WEAPON_STRING];
	char					szEngineerViewModel[MAX_WEAPON_STRING];
	char					szMedicViewModel[MAX_WEAPON_STRING];
	char					szSniperViewModel[MAX_WEAPON_STRING];
	char					szSpyViewModel[MAX_WEAPON_STRING];
	char					szMercenaryViewModel[MAX_WEAPON_STRING];	
	char					szCivilianViewModel[MAX_WEAPON_STRING];	

	
	// Muzzle flash
	char	m_szMuzzleFlashModel[128];
	float	m_flMuzzleFlashModelDuration;
	char	m_szMuzzleFlashParticleEffect[128];

	// Tracer
	char	m_szTracerEffect[128];

	// Eject Brass
	bool	m_bDoInstantEjectBrass;
	char	m_szBrassModel[128];

	// Explosion Effect
	char	m_szExplosionSound[128];
	char	m_szExplosionEffect[128];
	char	m_szExplosionPlayerEffect[128];
	char	m_szExplosionWaterEffect[128];

	// Explosion Effect
	char	m_szExplosionSoundClassic[128];
	char	m_szExplosionEffectClassic[128];
	char	m_szExplosionPlayerEffectClassic[128];
	char	m_szExplosionWaterEffectClassic[128];	
	
	bool	m_bDontDrop;
};

#endif // TF_WEAPON_PARSE_H
