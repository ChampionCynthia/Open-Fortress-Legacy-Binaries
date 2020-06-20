//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "gamemovement.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "in_buttons.h"
#include "movevars_shared.h"
#include "collisionutils.h"
#include "debugoverlay_shared.h"
#include "baseobject_shared.h"
#include "particle_parse.h"
#include "baseobject_shared.h"
#include "coordsize.h"

#include "func_ladder.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "c_world.h"
#include "c_team.h"

#define CTeam C_Team

#else
#include "tf_player.h"
#include "team.h"
#include "shareddefs.h"
#endif

ConVar	tf_maxspeed("tf_maxspeed", "720", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar	mp_maxairspeed("mp_maxairspeed", "30", FCVAR_NOTIFY | FCVAR_REPLICATED);
ConVar	tf_showspeed("tf_showspeed", "0", FCVAR_REPLICATED);
ConVar	tf_avoidteammates("tf_avoidteammates", "1", FCVAR_REPLICATED | FCVAR_CHEAT);
ConVar  tf_solidobjects("tf_solidobjects", "1", FCVAR_REPLICATED | FCVAR_CHEAT);
ConVar	tf_clamp_back_speed("tf_clamp_back_speed", "0.9", FCVAR_REPLICATED);
ConVar  tf_clamp_back_speed_min("tf_clamp_back_speed_min", "100", FCVAR_REPLICATED);
ConVar	of_shield_charge_speed("of_shield_charge_speed", "720", FCVAR_REPLICATED);

ConVar 	of_bunnyhop("of_bunnyhop", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggle bunnyhoping.\n-1: Mercenary Only\n 0: None\n 1: All Classes except Zombies\n 2: All Classes including Zombies");
ConVar 	of_jumpbuffer("of_jumpbuffer", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggle jump buffering\nOverrides of_bunnyhop when non 0.\n-1: Merc Only\n 0: None\n 1: All Classes except Zombies\n 2: All Classes including Zombies");
ConVar 	of_crouchjump("of_crouchjump", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Allows enables/disables crouch jumping.");
ConVar 	of_bunnyhop_max_speed_factor("of_bunnyhop_max_speed_factor", "1.2", FCVAR_NOTIFY | FCVAR_REPLICATED, "Max Speed achievable with bunnyhoping.");
ConVar 	of_jump_velocity("of_jump_velocity", "268.3281572999747", FCVAR_NOTIFY | FCVAR_REPLICATED, "The velocity applied when a player jumps.");
ConVar  of_speedcap("of_speedcap", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Maximum speed limit");
ConVar  of_ramp_jump("of_ramp_jump", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "0: off\n1: Quake Style ramp jumps\n2: Source ramp jumps");
ConVar  of_ramp_up_multiplier("of_ramp_up_multiplier", "0.7", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar  of_ramp_down_multiplier("of_ramp_down_multiplier", "0.7", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar  of_ramp_min_speed("of_ramp_min_speed", "50", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimal speed you need to be for ramp jumps to take effect.");
ConVar  of_zombie_lunge_speed("of_zombie_lunge_speed", "800", FCVAR_ARCHIVE | FCVAR_NOTIFY, "How much velocity, in units, to apply to a zombie lunge.");

#if defined (CLIENT_DLL)
ConVar 	of_jumpsound("of_jumpsound", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Hough", true, 0, true, 2);
#endif

#define TF_MAX_SPEED   720

#define TF_WATERJUMP_FORWARD  30
#define TF_WATERJUMP_UP       300
//ConVar	tf_waterjump_up( "tf_waterjump_up", "300", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_CHEAT );
//ConVar	tf_waterjump_forward( "tf_waterjump_forward", "30", FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_CHEAT );

static ConVar sv_autoladderdismount("sv_autoladderdismount", "1", FCVAR_REPLICATED, "Automatically dismount from ladders when you reach the end (don't have to +USE).");
static ConVar sv_ladderautomountdot("sv_ladderautomountdot", "0.4", FCVAR_REPLICATED, "When auto-mounting a ladder by looking up its axis, this is the tolerance for looking now directly along the ladder axis.");

static ConVar sv_ladder_useonly("sv_ladder_useonly", "0", FCVAR_REPLICATED, "If set, ladders can only be mounted by pressing +USE");

extern ConVar of_zombie_lunge_delay;

#define USE_DISMOUNT_SPEED 100

struct LadderMove_t;
class CInfoLadderDismount;

struct NearbyDismount_t
{
	CInfoLadderDismount		*dismount;
	float					distSqr;
};

class CTFGameMovement : public CGameMovement
{
public:
	DECLARE_CLASS(CTFGameMovement, CGameMovement);

	CTFGameMovement();

	virtual void PlayerMove();
	virtual void ShieldChargeMove();
	virtual unsigned int PlayerSolidMask(bool brushOnly = false);
	virtual void ProcessMovement(CBasePlayer *pBasePlayer, CMoveData *pMove);
	virtual bool CanAccelerate();
	virtual bool CheckJumpButton();
	bool CheckLunge();
	virtual bool CheckWater(void);
	virtual void WaterMove(void);
	virtual void FullWalkMove();
	virtual void WalkMove(bool CSliding = false);
	virtual void AirMove(void);
	virtual float GetAirSpeedCap(void);
	virtual void FullTossMove(void);
	virtual void CategorizePosition(void);
	virtual void Duck(void);

	virtual Vector GetPlayerViewOffset(bool ducked) const;

	virtual void	TracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm);
	virtual CBaseHandle	TestPlayerPosition(const Vector& pos, int collisionGroup, trace_t& pm);
	virtual void	StepMove(Vector &vecDestination, trace_t &trace);
	virtual bool	GameHasLadders() const;
	virtual void SetGroundEntity(trace_t *pm);
	virtual void PlayerRoughLandingEffects(float fvol);
protected:
	virtual void CheckWaterJump(void);
	void		 FullWalkMoveUnderwater();
	virtual void HandleDuckingSpeedCrop();
	virtual void AirAccelerate(Vector& wishdir, float wishspeed, float accel, bool q1accel = true);
	void		 Friction(bool CSliding);

private:

	bool		CheckWaterJumpButton(void);
	void		AirDash(void);
	void		PreventBunnyJumping();
	float		CheckRamp(float flMul, int rampMode);
	void		CheckCSlideSound(bool CSliding);

private:

	Vector		m_vecWaterPoint;
	CTFPlayer  *m_pTFPlayer;
};


// Expose our interface.
static CTFGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = (IGameMovement *)&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement);

// ---------------------------------------------------------------------------------------- //
// CTFGameMovement.
// ---------------------------------------------------------------------------------------- //

CTFGameMovement::CTFGameMovement()
{
	m_pTFPlayer = NULL;
}

//---------------------------------------------------------------------------------------- 
// Purpose: moves the player
//----------------------------------------------------------------------------------------
void CTFGameMovement::PlayerMove()
{
	// call base class to do movement
	BaseClass::PlayerMove();

	//Optional global speed cap
	float speedCap = of_speedcap.GetFloat();
	if (speedCap)
	{
		Vector2D horVel(mv->m_vecVelocity.x, mv->m_vecVelocity.y);
		if (horVel.Length() > speedCap)
		{
			Vector2DNormalize(horVel);
			Vector2Scale(horVel, speedCap, horVel);
			mv->m_vecVelocity.x = horVel.x;
			mv->m_vecVelocity.y = horVel.y;
		}
	}

	// handle player's interaction with water
	int nNewWaterLevel = m_pTFPlayer->GetWaterLevel();
	if (m_nOldWaterLevel != nNewWaterLevel)
	{
		if (WL_NotInWater == m_nOldWaterLevel)
		{
			// The player has just entered the water.  Determine if we should play a splash sound.
			bool bPlaySplash = false;

			Vector vecVelocity = m_pTFPlayer->GetAbsVelocity();
			if (vecVelocity.z <= -200.0f)
			{
				// If the player has significant downward velocity, play a splash regardless of water depth.  (e.g. Jumping hard into a puddle)
				bPlaySplash = true;
			}
			else
			{
				// Look at the water depth below the player.  If it's significantly deep, play a splash to accompany the sinking that's about to happen.
				Vector vecStart = m_pTFPlayer->GetAbsOrigin();
				Vector vecEnd = vecStart;
				vecEnd.z -= 20;	// roughly thigh deep
				trace_t tr;
				// see if we hit anything solid a little bit below the player
				UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, m_pTFPlayer, COLLISION_GROUP_NONE, &tr);
				if (tr.fraction >= 1.0f)
				{
					// some amount of water below the player, play a splash
					bPlaySplash = true;
				}
			}

			if (bPlaySplash)
			{
				m_pTFPlayer->EmitSound("Physics.WaterSplash");
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGameMovement::ShieldChargeMove(void)
{
	mv->m_flForwardMove = of_shield_charge_speed.GetFloat();
	mv->m_flMaxSpeed = of_shield_charge_speed.GetFloat();
	mv->m_flSideMove = 0.0f;
	mv->m_flUpMove = 0.0f;

	// HACK HACK: This prevents Jump and crouch inputs, try to do it via functions instead
	mv->m_nButtons &= ~IN_DUCK;
	mv->m_nButtons &= ~IN_JUMP;
}

Vector CTFGameMovement::GetPlayerViewOffset(bool ducked) const
{
	Vector DuckedView = VEC_DUCK_VIEW_SCALED(m_pTFPlayer);
	if (m_pTFPlayer->GetClassEyeHeight().z < 68)
		DuckedView.z = m_pTFPlayer->GetClassEyeHeight().z / 1.5 * player->GetModelScale();
	return ducked ? DuckedView : (m_pTFPlayer->GetClassEyeHeight());
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
unsigned int CTFGameMovement::PlayerSolidMask(bool brushOnly)
{
	unsigned int uMask = 0;

	if (m_pTFPlayer)
	{
		switch (m_pTFPlayer->GetTeamNumber())
		{
		case TF_TEAM_RED:
			uMask = CONTENTS_BLUETEAM + CONTENTS_MERCENARYTEAM;
			break;

		case TF_TEAM_BLUE:
			uMask = CONTENTS_REDTEAM + CONTENTS_MERCENARYTEAM;
			break;
		case TF_TEAM_MERCENARY:
			uMask = CONTENTS_REDTEAM + CONTENTS_BLUETEAM + CONTENTS_MERCENARYTEAM;
			break;
		}
	}

	return (uMask | BaseClass::PlayerSolidMask(brushOnly));
}

//-----------------------------------------------------------------------------
// Purpose: Overridden to allow players to run faster than the maxspeed
//-----------------------------------------------------------------------------
void CTFGameMovement::ProcessMovement(CBasePlayer *pBasePlayer, CMoveData *pMove)
{
	// Verify data.
	Assert(pBasePlayer);
	Assert(pMove);
	if (!pBasePlayer || !pMove)
		return;

	// Reset point contents for water check.
	ResetGetPointContentsCache();

	// Cropping movement speed scales mv->m_fForwardSpeed etc. globally
	// Once we crop, we don't want to recursively crop again, so we set the crop
	// flag globally here once per usercmd cycle.
	m_iSpeedCropped = SPEED_CROPPED_RESET;

	// Get the current TF player.
	m_pTFPlayer = ToTFPlayer(pBasePlayer);
	player = m_pTFPlayer;
	mv = pMove;

	// The max speed is currently set to the demoman charge - if this changes we need to change this!
	mv->m_flMaxSpeed = tf_maxspeed.GetFloat() <= 0.0f ? 100000.0f : tf_maxspeed.GetFloat();

	// Run the command.
	if (m_pTFPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE))
		ShieldChargeMove();

	PlayerMove();
	FinishMove();
}


bool CTFGameMovement::CanAccelerate()
{
	// Only allow the player to accelerate when in certain states.
	int nCurrentState = m_pTFPlayer->m_Shared.GetState();
	if (nCurrentState == TF_STATE_ACTIVE)
	{
		return player->GetWaterJumpTime() == 0;
	}
	else if (player->IsObserver())
	{
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we are in water.  If so the jump button acts like a
// swim upward key.
//-----------------------------------------------------------------------------
bool CTFGameMovement::CheckWaterJumpButton(void)
{
	// See if we are water jumping.  If so, decrement count and return.
	if (player->m_flWaterJumpTime)
	{
		player->m_flWaterJumpTime -= gpGlobals->frametime;
		if (player->m_flWaterJumpTime < 0)
		{
			player->m_flWaterJumpTime = 0;
		}

		return false;
	}

	// In water above our waist.
	if (player->GetWaterLevel() >= 2)
	{
		// Swimming, not jumping.
		SetGroundEntity(NULL);

		// We move up a certain amount.
		if (player->GetWaterType() == CONTENTS_WATER)
		{
			mv->m_vecVelocity[2] = 100;
		}
		else if (player->GetWaterType() == CONTENTS_SLIME)
		{
			mv->m_vecVelocity[2] = 80;
		}

		// Play swiming sound.
		if (player->m_flSwimSoundTime <= 0)
		{
			// Don't play sound again for 1 second.
			player->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return false;
	}

	return true;
}

void CTFGameMovement::AirDash(void)
{
	// Apply approx. the jump velocity added to an air dash.
	Assert(sv_gravity.GetFloat() == 800.0f);
	float flDashZ = 268.3281572999747f;

	// Get the wish direction.
	Vector vecForward, vecRight;
	AngleVectors(mv->m_vecViewAngles, &vecForward, &vecRight, NULL);
	vecForward.z = 0.0f;
	vecRight.z = 0.0f;
	VectorNormalize(vecForward);
	VectorNormalize(vecRight);

	// Copy movement amounts
	float flForwardMove = mv->m_flForwardMove;
	float flSideMove = mv->m_flSideMove;

	// Find the direction,velocity in the x,y plane.
	Vector vecWishDirection(((vecForward.x * flForwardMove) + (vecRight.x * flSideMove)),
		((vecForward.y * flForwardMove) + (vecRight.y * flSideMove)),
		0.0f);

	// Update the velocity on the scout.
	mv->m_vecVelocity = vecWishDirection;
	mv->m_vecVelocity.z += flDashZ;

	m_pTFPlayer->m_Shared.SetAirDash(true);
	m_pTFPlayer->m_Shared.AddAirDashCount();

	// Play the gesture.
	m_pTFPlayer->DoAnimationEvent(PLAYERANIMEVENT_DOUBLEJUMP);
}

// Only allow bunny jumping up to 1.2x server / player maxspeed setting
//#define BUNNYJUMP_MAX_SPEED_FACTOR 1.2f

ConVar of_bunnyhopfade("of_bunnyhopfade", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "When on, instead of instantly slowing down upon a bunnyhop, you gradually loose your speed.");

void CTFGameMovement::PreventBunnyJumping()
{
	// Speed at which bunny jumping is limited
	float maxscaledspeed = of_bunnyhop_max_speed_factor.GetFloat() * player->m_flMaxspeed;
	if (maxscaledspeed <= 0.0f)
		return;

	// Current player speed
	float spd = mv->m_vecVelocity.Length();
	if (spd <= maxscaledspeed)
		return;
	// 320        400     0.8
	if (of_bunnyhopfade.GetBool())
	{
		// From testing 40 seems to be the best bunnyhop fade float
		float fraction = (maxscaledspeed / of_bunnyhopfade.GetFloat());

		Vector vecSub = mv->m_vecVelocity / fraction;

		Vector vecTargetVelocity = mv->m_vecVelocity - vecSub;
		// If doing the fade would make us slower than the max speed
		// do the reular prevention, putting us at the exact bunnyhop speed limit instead
		if (vecTargetVelocity.Length() < maxscaledspeed)
		{
			// Apply this cropping fraction to velocity
			float fraction = (maxscaledspeed / spd);

			mv->m_vecVelocity *= fraction;
		}
		else // otherwise, go through with the gradient
		{
			mv->m_vecVelocity = vecTargetVelocity;
		}
	}
	else
	{
		// Apply this cropping fraction to velocity
		float fraction = (maxscaledspeed / spd);

		mv->m_vecVelocity *= fraction;
	}
}

bool CTFGameMovement::CheckJumpButton()
{
	// Are we dead?  Then we cannot jump.
	if (player->pl.deadflag)
		return false;

	// Check to see if we are in water.
	if (!CheckWaterJumpButton())
		return false;

	// Cannot jump while taunting
	if (m_pTFPlayer->m_Shared.InCond(TF_COND_TAUNTING))
		return false;

	// Check to see if the player is a scout.
	bool bCanAirDash = m_pTFPlayer->GetPlayerClass()->CanAirDash();
	bool bAirDash = false;
	bool bOnGround = player->GetGroundEntity() != NULL;

	//jumping cvars
	bool CrouchJump = of_crouchjump.GetBool() && !of_cslide.GetBool();

	// Cannot jump while ducked.
	if (player->GetFlags() & FL_DUCKING)
	{
		// Let a scout do it.
		bool bAllow = (bCanAirDash && !bOnGround) || (CrouchJump && bOnGround);
		if (!bAllow)
			return false;
	}

	// Cannot jump while in the unduck transition.
	if ((!CrouchJump && player->m_Local.m_bDucking && player->GetFlags() & FL_DUCKING) ||
		player->m_Local.m_flDuckJumpTime > 0.0f)
		return false;

	//Jump buffer shenanigans
	int JumpBuffer = of_jumpbuffer.GetInt();
	if (JumpBuffer)
	{
		//people without buffer get to jump like when bunnyhop is off
		if ((m_pTFPlayer->m_Shared.IsZombie() && JumpBuffer != 2) ||										//player is zombie and jump buffer is not 2
			(JumpBuffer == -1 && m_pTFPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_MERCENARY))		//jump buffer is -1 and player is not a Merc
		{
			if (mv->m_nOldButtons & IN_JUMP)
				return false;
		}
		else
		{
			JumpBuffer = 1;
		}
	}
	else //jump buffering excludes the regular OF jumping routing, only evaluate is of_jumpbuffer == 0
	{
		// Cannot jump again until the jump button has been released.
		int bHop = of_bunnyhop.GetInt();
		if (mv->m_nOldButtons & IN_JUMP)
		{
			if (!bOnGround ||																				//not on ground
				!bHop ||																					//nobody can bhop
				(bHop == -1 && m_pTFPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_MERCENARY) ||		//only Merc allowed
				(m_pTFPlayer->m_Shared.IsZombie() && bHop != 2))											//everybody jump!
				return false;
		}
	}

	// In air, so ignore jumps (unless you are a scout).
	if (!bOnGround)
	{
		if (bCanAirDash && m_pTFPlayer->m_Shared.GetAirDashCount() < m_pTFPlayer->GetPlayerClass()->MaxAirDashCount())
			bAirDash = true;
		else
			return false;
	}

	// Check for an air dash.
	if (bAirDash)
	{
		AirDash();
		return true;
	}

	PreventBunnyJumping();

	// Start jump animation and player sound (specific TF animation and flags).
	m_pTFPlayer->DoAnimationEvent(PLAYERANIMEVENT_JUMP);
	m_pTFPlayer->m_Shared.SetJumping(true);

	if (gpGlobals->curtime >= m_pTFPlayer->m_Shared.m_flStepSoundDelay)
		player->PlayStepSound((Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true);
	m_pTFPlayer->m_Shared.m_flStepSoundDelay = gpGlobals->curtime + 0.25f;

	// Set the player as in the air.
	SetGroundEntity(NULL);

	// Check the surface the player is standing on to see if it impacts jumping.
	float flGroundFactor = 1.0f;
	if (player->m_pSurfaceData)
		flGroundFactor = player->m_pSurfaceData->game.jumpFactor;

	Assert(sv_gravity.GetFloat() == 800.0f);
	float flMul = of_jump_velocity.GetFloat() * flGroundFactor;

	//Calculate vertical velocity
	float flStartZ = mv->m_vecVelocity[2];

	if (player->m_Local.m_bDucking || player->GetFlags() & FL_DUCKING) //crouch jump
	{
		mv->m_vecVelocity[2] = flMul;
	}
	else
	{
		//Trimping
		int rampMode = of_ramp_jump.GetInt();
		if (rampMode)
			flMul = CheckRamp(flMul, rampMode);

		mv->m_vecVelocity[2] += flMul;
	}

	// Apply gravity.
	FinishGravity();

	// Save the output data for the physics system to react to if need be.
	mv->m_outJumpVel.z += mv->m_vecVelocity[2] - flStartZ;
	mv->m_outStepHeight += 0.15f;

	if (gpGlobals->curtime >= m_pTFPlayer->m_Shared.m_flJumpSoundDelay)
	{
#ifdef GAME_DLL
		IGameEvent *event = gameeventmanager->CreateEvent( "player_jump" );
		if ( event )
		{
			event->SetInt("playerid", m_pTFPlayer->entindex());
			gameeventmanager->FireEvent(event);
		}
#else
		if ((of_jumpsound.GetBool() && m_pTFPlayer->GetPlayerClass()->GetClassIndex() > 9) || of_jumpsound.GetInt() == 2)
			m_pTFPlayer->EmitSound(m_pTFPlayer->GetPlayerClass()->GetJumpSound());
#endif
	}
	m_pTFPlayer->m_Shared.m_flJumpSoundDelay = gpGlobals->curtime + 0.5f;

	// Flag that we jumped and don't jump again until it is released.
	m_pTFPlayer->m_Shared.SetBlockJump(JumpBuffer == 1 ? true : false); //jump successful, set the buffer
	return true;
}

float CTFGameMovement::CheckRamp(float flMul, int rampMode)
{
	if (rampMode == 1) //Quake style
	{
		mv->m_vecVelocity[2] = max(0, m_pTFPlayer->m_Shared.GetRampJumpVel()); //set velocity to what it was before touching the ground

		if (mv->m_vecVelocity[2]) //player has positive vertical velocity
		{
			float endflMul = flMul * of_ramp_up_multiplier.GetFloat();
			if (mv->m_vecVelocity[2] + endflMul > flMul) //only demultiply if the overall resulting velZ is higher than the non-demultiplied jump velZ
				flMul = endflMul;
		}
	}
	else //source trimping
	{
		// Take the lateral velocity
		Vector vecVelocity = mv->m_vecVelocity * Vector(1.0f, 1.0f, 0.0f);
		float flHorizontalSpeed = vecVelocity.Length();

		if (flHorizontalSpeed < of_ramp_min_speed.GetFloat()) //not enough speed, abort
			return flMul;

		//Try find the floor
		trace_t pm;
		Vector vecStart = mv->GetAbsOrigin();
		Vector vecStop = vecStart - Vector(0, 0, 60.0f);
		TracePlayerBBox(vecStart, vecStop, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm);

		// no floor, abort
		if (pm.fraction == 1.0f)
			return flMul;

		//Calculate dot product of horizontal velocity and surface
		VectorNormalize(vecVelocity);
		float flDotProduct = DotProduct(vecVelocity, pm.plane.normal);
		float absDot = abs(flDotProduct);

		//not enough trimping power
		if (absDot < 0.15f)
			return flMul;

		if (flDotProduct < 0) //going up a slope
		{
			//increase jump power according to the dot product and horizontal velocity
			flMul += absDot * flHorizontalSpeed * of_ramp_up_multiplier.GetFloat();
		}
		else
		{
			float ramp_multi = of_ramp_down_multiplier.GetFloat();
			//jump velocity must be demultiplied and horizontal speed must be increased
			flMul *= absDot / ramp_multi;
			for (int i = 0; i < 2; i++)
				mv->m_vecVelocity[i] += absDot * mv->m_vecVelocity[i] * ramp_multi;
		}
	}

	return flMul;
}

bool CTFGameMovement::CheckLunge()
{
	// Check to see if we are in water.
	if (player->GetWaterLevel() >= 2)
		return false;

	// Cannot lunge jump while taunting
	if (m_pTFPlayer->m_Shared.InCond(TF_COND_TAUNTING))
		return false;

	// Check to see if the player is a scout.
	bool bOnGround = (player->GetGroundEntity() != NULL);

	// In air, so ignore jumps
	if (!bOnGround)
		return false;

	// Start jump animation and player sound (specific TF animation and flags).
	m_pTFPlayer->DoAnimationEvent(PLAYERANIMEVENT_JUMP);
	player->PlayStepSound((Vector &)mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true);
	m_pTFPlayer->m_Shared.SetJumping(true);

	// Set the player as in the air.
	SetGroundEntity(NULL);

	// Check the surface the player is standing on to see if it impacts jumping.
	float flGroundFactor = 1.0f;
	if (player->m_pSurfaceData)
	{
		flGroundFactor = player->m_pSurfaceData->game.jumpFactor;
	}

	// Get the start and endpoints.
	Vector vecMuzzlePos = player->Weapon_ShootPosition();
	Vector forward;
	player->EyeVectors(&forward);
	Vector vecEndPos = vecMuzzlePos + (forward * MAX_TRACE_LENGTH);

	trace_t	trace;
	UTIL_TraceLine(vecMuzzlePos, vecEndPos, (MASK_SHOT & ~CONTENTS_WINDOW), player, COLLISION_GROUP_NONE, &trace);

	Vector vecTarget = trace.endpos;
	Vector vecDir = vecTarget - player->GetAbsOrigin();
	VectorNormalize(vecDir);
	float flSpeed = of_zombie_lunge_speed.GetFloat();
	vecDir *= flSpeed;

	Vector vecTmpStart = mv->m_vecVelocity;

	mv->m_vecVelocity = vecDir * flGroundFactor;

	// Apply gravity.
	FinishGravity();

	player->EmitSound("Player.ZombieLunge");
	m_pTFPlayer->m_Shared.SetNextLungeTime(gpGlobals->curtime + of_zombie_lunge_delay.GetFloat());

	// Save the output data for the physics system to react to if need be.
	mv->m_outJumpVel += mv->m_vecVelocity - vecTmpStart;
	mv->m_outStepHeight += 0.15f;

	// Flag that we jumped and don't jump again until it is released.
	mv->m_nOldButtons |= IN_ATTACK2;
	m_pTFPlayer->m_Shared.SetBlockJump(true);
	return true;
}

bool CTFGameMovement::CheckWater(void)
{
	Vector vecPlayerMin = GetPlayerMins();
	Vector vecPlayerMax = GetPlayerMaxs();

	Vector vecPoint((mv->GetAbsOrigin().x + (vecPlayerMin.x + vecPlayerMax.x) * 0.5f),
		(mv->GetAbsOrigin().y + (vecPlayerMin.y + vecPlayerMax.y) * 0.5f),
		(mv->GetAbsOrigin().z + vecPlayerMin.z + 1));


	// Assume that we are not in water at all.
	int wl = WL_NotInWater;
	int wt = CONTENTS_EMPTY;

	// Check to see if our feet are underwater.
	int nContents = GetPointContentsCached(vecPoint, 0);
	if (nContents & MASK_WATER)
	{
		// Clear our jump flag, because we have landed in water.
		m_pTFPlayer->m_Shared.SetJumping(false);

		// Set water type and level.
		wt = nContents;
		wl = WL_Feet;

		float flWaistZ = mv->GetAbsOrigin().z + (vecPlayerMin.z + vecPlayerMax.z) * 0.5f + 12.0f;

		// Now check eyes
		vecPoint.z = mv->GetAbsOrigin().z + player->GetViewOffset()[2];
		nContents = GetPointContentsCached(vecPoint, 1);
		if (nContents & MASK_WATER)
		{
			// In over our eyes
			wl = WL_Eyes;
			VectorCopy(vecPoint, m_vecWaterPoint);
			m_vecWaterPoint.z = flWaistZ;
		}
		else
		{
			// Now check a point that is at the player hull midpoint (waist) and see if that is underwater.
			vecPoint.z = flWaistZ;
			nContents = GetPointContentsCached(vecPoint, 2);
			if (nContents & MASK_WATER)
			{
				// Set the water level at our waist.
				wl = WL_Waist;
				VectorCopy(vecPoint, m_vecWaterPoint);
			}
		}
	}

	player->SetWaterLevel(wl);
	player->SetWaterType(wt);

	// If we just transitioned from not in water to water, record the time for splashes, etc.
	if ((WL_NotInWater == m_nOldWaterLevel) && (wl >  WL_NotInWater))
	{
		m_flWaterEntryTime = gpGlobals->curtime;
	}

	return (wl > WL_Feet);
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::WaterMove(void)
{
	int i;
	float	wishspeed;
	Vector	wishdir;
	Vector	start, dest;
	Vector  temp;
	trace_t	pm;
	float speed, newspeed, addspeed, accelspeed;

	// Determine movement angles.
	Vector vecForward, vecRight, vecUp;
	AngleVectors(mv->m_vecViewAngles, &vecForward, &vecRight, &vecUp);

	// Calculate the desired direction and speed.
	Vector vecWishVelocity;
	int iAxis;
	for (iAxis = 0; iAxis < 3; ++iAxis)
	{
		vecWishVelocity[iAxis] = (vecForward[iAxis] * mv->m_flForwardMove) + (vecRight[iAxis] * mv->m_flSideMove);
	}

	// Check for upward velocity (JUMP).
	if (mv->m_nButtons & IN_JUMP)
	{
		if (player->GetWaterLevel() == WL_Eyes)
			vecWishVelocity[2] += mv->m_flClientMaxSpeed;
	}
	// Sinking if not moving.
	else if (!mv->m_flForwardMove && !mv->m_flSideMove && !mv->m_flUpMove)
	{
		vecWishVelocity[2] -= 60;
	}
	// Move up based on view angle.
	else
	{
		vecWishVelocity[2] += mv->m_flUpMove;
	}

	// Copy it over and determine speed
	VectorCopy(vecWishVelocity, wishdir);
	wishspeed = VectorNormalize(wishdir);

	// Cap speed.
	if (wishspeed > mv->m_flMaxSpeed)
	{
		VectorScale(vecWishVelocity, mv->m_flMaxSpeed / wishspeed, vecWishVelocity);
		wishspeed = mv->m_flMaxSpeed;
	}

	// Slow us down a bit.
	wishspeed *= 0.8;

	// Water friction
	VectorCopy(mv->m_vecVelocity, temp);
	speed = VectorNormalize(temp);
	if (speed)
	{
		newspeed = speed - gpGlobals->frametime * speed * sv_friction.GetFloat() * player->m_surfaceFriction;
		if (newspeed < 0.1f)
		{
			newspeed = 0;
		}

		VectorScale(mv->m_vecVelocity, newspeed / speed, mv->m_vecVelocity);
	}
	else
	{
		newspeed = 0;
	}

	// water acceleration
	if (wishspeed >= 0.1f)  // old !
	{
		addspeed = wishspeed - newspeed;
		if (addspeed > 0)
		{
			VectorNormalize(vecWishVelocity);
			accelspeed = sv_accelerate.GetFloat() * wishspeed * gpGlobals->frametime * player->m_surfaceFriction;
			if (accelspeed > addspeed)
			{
				accelspeed = addspeed;
			}

			for (i = 0; i < 3; i++)
			{
				float deltaSpeed = accelspeed * vecWishVelocity[i];
				mv->m_vecVelocity[i] += deltaSpeed;
				mv->m_outWishVel[i] += deltaSpeed;
			}
		}
	}

	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	// Now move
	// assume it is a stair or a slope, so press down from stepheight above
	VectorMA(mv->GetAbsOrigin(), gpGlobals->frametime, mv->m_vecVelocity, dest);

	TracePlayerBBox(mv->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm);
	if (pm.fraction == 1.0f)
	{
		VectorCopy(dest, start);
		if (player->m_Local.m_bAllowAutoMovement)
		{
			start[2] += player->m_Local.m_flStepSize + 1;
		}

		TracePlayerBBox(start, dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm);

		if (!pm.startsolid && !pm.allsolid)
		{
#if 0
			float stepDist = pm.endpos.z - mv->GetAbsOrigin().z;
			mv->m_outStepHeight += stepDist;
			// walked up the step, so just keep result and exit

			Vector vecNewWaterPoint;
			VectorCopy(m_vecWaterPoint, vecNewWaterPoint);
			vecNewWaterPoint.z += (dest.z - mv->GetAbsOrigin().z);
			bool bOutOfWater = !(enginetrace->GetPointContents(vecNewWaterPoint) & MASK_WATER);
			if (bOutOfWater && (mv->m_vecVelocity.z > 0.0f) && (pm.fraction == 1.0f))
			{
				// Check the waist level water positions.
				trace_t traceWater;
				UTIL_TraceLine(vecNewWaterPoint, m_vecWaterPoint, CONTENTS_WATER, player, COLLISION_GROUP_NONE, &traceWater);
				if (traceWater.fraction < 1.0f)
				{
					float flFraction = 1.0f - traceWater.fraction;

					//					Vector vecSegment;
					//					VectorSubtract( mv->GetAbsOrigin(), dest, vecSegment );
					//					VectorMA( mv->GetAbsOrigin(), flFraction, vecSegment, mv->GetAbsOrigin() );
					float flZDiff = dest.z - mv->GetAbsOrigin().z;
					float flSetZ = mv->GetAbsOrigin().z + (flFraction * flZDiff);
					flSetZ -= 0.0325f;

					VectorCopy(pm.endpos, mv->GetAbsOrigin());
					mv->GetAbsOrigin().z = flSetZ;
					VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
					mv->m_vecVelocity.z = 0.0f;
				}

			}
			else
			{
				VectorCopy(pm.endpos, mv->GetAbsOrigin());
				VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
			}

			return;
#endif
			float stepDist = pm.endpos.z - mv->GetAbsOrigin().z;
			mv->m_outStepHeight += stepDist;
			// walked up the step, so just keep result and exit
			mv->SetAbsOrigin(pm.endpos);
			VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
			return;
		}

		// Try moving straight along out normal path.
		TryPlayerMove();
	}
	else
	{
		if (!player->GetGroundEntity())
		{
			TryPlayerMove();
			VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
			return;
		}

		StepMove(dest, pm);
	}

	VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::WalkMove(bool CSliding)
{
	// Get the movement angles.
	Vector vecForward, vecRight, vecUp;
	AngleVectors(mv->m_vecViewAngles, &vecForward, &vecRight, &vecUp);
	vecForward.z = 0.0f;
	vecRight.z = 0.0f;
	VectorNormalize(vecForward);
	VectorNormalize(vecRight);

	// Copy movement amounts
	float flForwardMove = mv->m_flForwardMove;
	float flSideMove = mv->m_flSideMove;

	// Find the direction,velocity in the x,y plane.
	Vector vecWishDirection(vecForward.x * flForwardMove + vecRight.x * flSideMove,
							vecForward.y * flForwardMove + vecRight.y * flSideMove,
							0.0f);

	// Accelerate in the x,y plane.
	float flWishSpeed;
	if (CSliding)
	{
		VectorNormalize(vecWishDirection);
		flWishSpeed = min(mv->m_vecVelocity.Length(), of_cslidestopspeed.GetFloat());
		mv->m_vecVelocity.z = 0;
		AirAccelerate(vecWishDirection, flWishSpeed, of_cslideaccelerate.GetFloat(), false);
	}
	else
	{
		// Calculate the speed and direction of movement, then clamp the speed.
		flWishSpeed = VectorNormalize(vecWishDirection);
		flWishSpeed = clamp(flWishSpeed, 0.f, mv->m_flMaxSpeed);
		mv->m_vecVelocity.z = 0;
		Accelerate(vecWishDirection, flWishSpeed, sv_accelerate.GetFloat());

		// Clamp the players speed in x,y.
		/* no we don't so we can circle jump
		float flNewSpeed = VectorLength(mv->m_vecVelocity);
		if (flNewSpeed > mv->m_flMaxSpeed)
		{
			float flScale = (mv->m_flMaxSpeed / flNewSpeed);
			mv->m_vecVelocity.x *= flScale;
			mv->m_vecVelocity.y *= flScale;
		}*/
	}
	Assert(mv->m_vecVelocity.z == 0.0f);

	/* no make it quake like
	// Now reduce their backwards speed to some percent of max, if they are travelling backwards
	// unless they are under some minimum, to not penalize deployed snipers or heavies
	if (tf_clamp_back_speed.GetFloat() < 1.0 && VectorLength(mv->m_vecVelocity) > tf_clamp_back_speed_min.GetFloat())
	{
		float flDot = DotProduct(vecForward, mv->m_vecVelocity);

		// are we moving backwards at all?
		if (flDot < 0)
		{
			Vector vecBackMove = vecForward * flDot;
			Vector vecRightMove = vecRight * DotProduct(vecRight, mv->m_vecVelocity);

			// clamp the back move vector if it is faster than max
			float flBackSpeed = VectorLength(vecBackMove);
			float flMaxBackSpeed = (mv->m_flMaxSpeed * tf_clamp_back_speed.GetFloat());

			if (flBackSpeed > flMaxBackSpeed)
			{
				vecBackMove *= flMaxBackSpeed / flBackSpeed;
			}

			// reassemble velocity	
			mv->m_vecVelocity = vecBackMove + vecRightMove;

			// Clamp the players speed in x, y.
			float flNewSpeed = VectorLength(mv->m_vecVelocity);
			if (flNewSpeed > mv->m_flMaxSpeed)
			{
				float flScale = (mv->m_flMaxSpeed / flNewSpeed);
				mv->m_vecVelocity.x *= flScale;
				mv->m_vecVelocity.y *= flScale;
			}
		}
	}
	*/

	// Add base velocity to the player's current velocity - base velocity = velocity from conveyors, etc.
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	// Calculate the current speed and return if we are not really moving.
	float flSpeed = VectorLength(mv->m_vecVelocity);
	if (flSpeed < 1.0f)
	{
		// I didn't remove the base velocity here since it wasn't moving us in the first place.
		mv->m_vecVelocity.Init();
		return;
	}

	// Calculate the destination.
	Vector vecDestination;
	vecDestination.x = mv->GetAbsOrigin().x + (mv->m_vecVelocity.x * gpGlobals->frametime);
	vecDestination.y = mv->GetAbsOrigin().y + (mv->m_vecVelocity.y * gpGlobals->frametime);
	vecDestination.z = mv->GetAbsOrigin().z;

	// Try moving to the destination.
	trace_t trace;
	TracePlayerBBox(mv->GetAbsOrigin(), vecDestination, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);
	if (trace.fraction == 1.0f)
	{
		// Made it to the destination (remove the base velocity).
		mv->SetAbsOrigin(trace.endpos);
		VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

		// Save the wish velocity.
		mv->m_outWishVel += (vecWishDirection * flWishSpeed);

		// Try and keep the player on the ground.
		// NOTE YWB 7/5/07: Don't do this here, our version of CategorizePosition encompasses this test
		// StayOnGround();

		return;
	}

	// Now try and do a step move.
	StepMove(vecDestination, trace);

	// Remove base velocity.
	Vector baseVelocity = player->GetBaseVelocity();
	VectorSubtract(mv->m_vecVelocity, baseVelocity, mv->m_vecVelocity);

	// Save the wish velocity.
	mv->m_outWishVel += (vecWishDirection * flWishSpeed);

	// Try and keep the player on the ground.
	// NOTE YWB 7/5/07: Don't do this here, our version of CategorizePosition encompasses this test
	// StayOnGround();

#if 0
	// Debugging!!!
	Vector vecTestVelocity = mv->m_vecVelocity;
	vecTestVelocity.z = 0.0f;
	float flTestSpeed = VectorLength(vecTestVelocity);
	if (baseVelocity.IsZero() && (flTestSpeed > (mv->m_flMaxSpeed + 1.0f)))
	{
		Msg("Step Max Speed < %f\n", flTestSpeed);
	}

	if (tf_showspeed.GetBool())
	{
		Msg("Speed=%f\n", flTestSpeed);
	}

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CTFGameMovement::AirAccelerate(Vector& wishdir, float wishspeed, float accel, bool q1accel)
{
	float addspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	if (player->pl.deadflag || player->m_flWaterJumpTime)
		return;

	// Cap speed, this is the only thing to edit to allow
	// Q3 style strafejumping, if Q3 movement is on it is ignored
	if (q1accel)
		wishspd = min(wishspd, GetAirSpeedCap());

	// Determine veer amount
	currentspeed = mv->m_vecVelocity.Dot(wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration and cap it
	float accelspeed = min(accel * wishspeed * gpGlobals->frametime * player->m_surfaceFriction, addspeed);

	// Adjust pmove vel.
	VectorAdd(mv->m_vecVelocity, accelspeed * wishdir, mv->m_vecVelocity);
	VectorAdd(mv->m_outWishVel, accelspeed * wishdir, mv->m_outWishVel);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::AirMove(void)
{
	int			movementmode;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector		forward, right, up;

	AngleVectors(mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;

	// Zero out z components of movement vectors
	forward[2] = right[2] = wishdir[2] = 0;

	for (int i = 0; i < 3; i++)       // Determine x and y parts of velocity
		wishdir[i] = forward[i] * fmove + right[i] * smove;

	// Determine magnitude of speed of move
	wishspeed = VectorNormalize(wishdir);
	wishspeed = min(wishspeed, mv->m_flMaxSpeed);

	//Accelerate
	movementmode = of_movementmode.GetInt();	//get the value only once

	if (!movementmode) //default OF
	{
		AirAccelerate(wishdir, wishspeed, sv_airaccelerate.GetFloat());
	}
	else if (movementmode == 1) //Q3
	{
		AirAccelerate(wishdir, wishspeed, of_q3airaccelerate.GetFloat(), false);
	}
	else //CPMA
	{
		bool CPMA = !fmove && smove;
		float airaccel = CPMA ? sv_airaccelerate.GetFloat() : of_q3airaccelerate.GetFloat();
		AirAccelerate(wishdir, wishspeed, airaccel, CPMA);
	}

	// Add in any base velocity to the current velocity.
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	TryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);
}

float CTFGameMovement::GetAirSpeedCap(void)
{
	if (m_pTFPlayer->m_Shared.InCond(TF_COND_SHIELD_CHARGE) && mp_maxairspeed.GetFloat() < of_shield_charge_speed.GetFloat())
		return of_shield_charge_speed.GetFloat();
	return mp_maxairspeed.GetFloat();
}

extern void TracePlayerBBoxForGround(const Vector& start, const Vector& end, const Vector& minsSrc,
	const Vector& maxsSrc, IHandleEntity *player, unsigned int fMask,
	int collisionGroup, trace_t& pm);

//-----------------------------------------------------------------------------
// This filter checks against buildable objects.
//-----------------------------------------------------------------------------
class CTraceFilterObject : public CTraceFilterSimple
{
public:
	DECLARE_CLASS(CTraceFilterObject, CTraceFilterSimple);

	CTraceFilterObject(const IHandleEntity *passentity, int collisionGroup);
	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask);
};

CTraceFilterObject::CTraceFilterObject(const IHandleEntity *passentity, int collisionGroup) :
BaseClass(passentity, collisionGroup)
{

}

bool CTraceFilterObject::ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
{
	CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);

	if (pEntity && pEntity->IsBaseObject())
	{
		CBaseObject *pObject = static_cast<CBaseObject *>(pEntity);

		Assert(pObject);

		if (pObject && pObject->ShouldPlayersAvoid())
		{
			if (pObject->GetOwner() == GetPassEntity())
				return true;
		}
	}

	return CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask);
}

CBaseHandle CTFGameMovement::TestPlayerPosition(const Vector& pos, int collisionGroup, trace_t& pm)
{
	if (tf_solidobjects.GetBool() == false)
		return BaseClass::TestPlayerPosition(pos, collisionGroup, pm);

	Ray_t ray;
	ray.Init(pos, pos, GetPlayerMins(), GetPlayerMaxs());

	CTraceFilterObject traceFilter(mv->m_nPlayerHandle.Get(), collisionGroup);
	enginetrace->TraceRay(ray, PlayerSolidMask(), &traceFilter, &pm);

	if ((pm.contents & PlayerSolidMask()) && pm.m_pEnt)
	{
		return pm.m_pEnt->GetRefEHandle();
	}
	else
	{
		return INVALID_EHANDLE_INDEX;
	}
}

//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
void CTFGameMovement::TracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm)
{
	if (tf_solidobjects.GetBool() == false)
		return BaseClass::TracePlayerBBox(start, end, fMask, collisionGroup, pm);

	Ray_t ray;
	ray.Init(start, end, GetPlayerMins(), GetPlayerMaxs());

	CTraceFilterObject traceFilter(mv->m_nPlayerHandle.Get(), collisionGroup);

	enginetrace->TraceRay(ray, fMask, &traceFilter, &pm);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
//-----------------------------------------------------------------------------
void CTFGameMovement::CategorizePosition(void)
{
	// Observer.
	if (player->IsObserver())
		return;

	// Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
	player->m_surfaceFriction = 1.0f;

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.
	CheckWater();

	// If standing on a ladder we are not on ground.
	if (player->GetMoveType() == MOVETYPE_LADDER)
	{
		SetGroundEntity(NULL);
		return;
	}

	// Check for a jump.
	if (mv->m_vecVelocity.z > 250.0f)
	{
		SetGroundEntity(NULL);
		return;
	}

	// Calculate the start and end position.
	Vector vecStartPos = mv->GetAbsOrigin();
	Vector vecEndPos(mv->GetAbsOrigin().x, mv->GetAbsOrigin().y, (mv->GetAbsOrigin().z - 2.0f));

	// NOTE YWB 7/5/07:  Since we're already doing a traceline here, we'll subsume the StayOnGround (stair debouncing) check into the main traceline we do here to see what we're standing on
	bool bUnderwater = (player->GetWaterLevel() >= WL_Eyes);
	bool bMoveToEndPos = false;
	if (player->GetMoveType() == MOVETYPE_WALK &&
		player->GetGroundEntity() != NULL && !bUnderwater)
	{
		// if walking and still think we're on ground, we'll extend trace down by stepsize so we don't bounce down slopes
		vecEndPos.z -= player->GetStepSize();
		bMoveToEndPos = true;
	}

	trace_t trace;
	TracePlayerBBox(vecStartPos, vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);

	// Steep plane, not on ground.
	if (trace.plane.normal.z < 0.7f)
	{
		// Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on.
		TracePlayerBBoxForGround(vecStartPos, vecEndPos, GetPlayerMins(), GetPlayerMaxs(), mv->m_nPlayerHandle.Get(), PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);
		if (trace.plane.normal[2] < 0.7f)
		{
			// Too steep.
			SetGroundEntity(NULL);
			if ((mv->m_vecVelocity.z > 0.0f) &&
				(player->GetMoveType() != MOVETYPE_NOCLIP))
			{
				player->m_surfaceFriction = 0.25f;
			}
		}
		else
		{
			SetGroundEntity(&trace);
		}
	}
	else
	{
		// YWB:  This logic block essentially lifted from StayOnGround implementation
		if (bMoveToEndPos &&
			!trace.startsolid &&				// not sure we need this check as fraction would == 0.0f?
			trace.fraction > 0.0f &&			// must go somewhere
			trace.fraction < 1.0f) 			// must hit something
		{
			float flDelta = fabs(mv->GetAbsOrigin().z - trace.endpos.z);
			// HACK HACK:  The real problem is that trace returning that strange value 
			//  we can't network over based on bit precision of networking origins
			if (flDelta > 0.5f * COORD_RESOLUTION)
			{
				Vector org = mv->GetAbsOrigin();
				org.z = trace.endpos.z;
				mv->SetAbsOrigin(org);
			}
		}
		SetGroundEntity(&trace);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::CheckWaterJump(void)
{
	Vector	flatforward;
	Vector	flatvelocity;
	float curspeed;

	// Jump button down?
	bool bJump = (mv->m_nButtons & IN_JUMP) != 0;

	Vector forward, right;
	AngleVectors(mv->m_vecViewAngles, &forward, &right, NULL);  // Determine movement angles

	// Already water jumping.
	if (player->m_flWaterJumpTime)
		return;

	// Don't hop out if we just jumped in
	if (mv->m_vecVelocity[2] < -180)
		return; // only hop out if we are moving up

	// See if we are backing up
	flatvelocity[0] = mv->m_vecVelocity[0];
	flatvelocity[1] = mv->m_vecVelocity[1];
	flatvelocity[2] = 0;

	// Must be moving
	curspeed = VectorNormalize(flatvelocity);

#if 1
	// Copy movement amounts
	float fmove = mv->m_flForwardMove;
	float smove = mv->m_flSideMove;

	for (int iAxis = 0; iAxis < 2; ++iAxis)
	{
		flatforward[iAxis] = forward[iAxis] * fmove + right[iAxis] * smove;
	}
#else
	// see if near an edge
	flatforward[0] = forward[0];
	flatforward[1] = forward[1];
#endif
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	// Are we backing into water from steps or something?  If so, don't pop forward
	if (curspeed != 0.0 && (DotProduct(flatvelocity, flatforward) < 0.0) && !bJump)
		return;

	Vector vecStart;
	// Start line trace at waist height (using the center of the player for this here)
	vecStart = mv->GetAbsOrigin() + (GetPlayerMins() + GetPlayerMaxs()) * 0.5;

	Vector vecEnd;
	VectorMA(vecStart, TF_WATERJUMP_FORWARD/*tf_waterjump_forward.GetFloat()*/, flatforward, vecEnd);

	trace_t tr;
	TracePlayerBBox(vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr);
	if (tr.fraction < 1.0)		// solid at waist
	{
		IPhysicsObject *pPhysObj = tr.m_pEnt->VPhysicsGetObject();
		if (pPhysObj)
		{
			if (pPhysObj->GetGameFlags() & FVPHYSICS_PLAYER_HELD)
				return;
		}

		float ViewOffset = (player->GetViewOffset().z < 68.0f) ? 68.0f : player->GetViewOffset().z;
		vecStart.z = mv->GetAbsOrigin().z + ViewOffset + WATERJUMP_HEIGHT;
		VectorMA(vecStart, TF_WATERJUMP_FORWARD/*tf_waterjump_forward.GetFloat()*/, flatforward, vecEnd);
		VectorMA(vec3_origin, -50.0f, tr.plane.normal, player->m_vecWaterJumpVel);

		TracePlayerBBox(vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr);
		if (tr.fraction == 1.0)		// open at eye level
		{
			// Now trace down to see if we would actually land on a standable surface.
			VectorCopy(vecEnd, vecStart);
			vecEnd.z -= 1024.0f;
			TracePlayerBBox(vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr);
			if ((tr.fraction < 1.0f) && (tr.plane.normal.z >= 0.7))
			{
				mv->m_vecVelocity[2] = TF_WATERJUMP_UP/*tf_waterjump_up.GetFloat()*/;		// Push up
				player->AddFlag(FL_WATERJUMP);
				m_pTFPlayer->m_Shared.SetBlockJump(true);
				player->m_flWaterJumpTime = 2000.0f;	// Do this for 2 seconds
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::Duck(void)
{
	// Don't allowing ducking in water.
	if ((player->GetWaterLevel() >= WL_Feet && player->GetGroundEntity() == NULL) || player->GetWaterLevel() >= WL_Eyes)
	{
		mv->m_nButtons &= ~IN_DUCK;
	}

	BaseClass::Duck();
}
void CTFGameMovement::FullWalkMoveUnderwater()
{
	if (player->GetWaterLevel() == WL_Waist)
		CheckWaterJump();

	// If we are falling again, then we must not trying to jump out of water any more.
	if ((mv->m_vecVelocity.z < 0.0f) && player->m_flWaterJumpTime)
	{
		player->m_flWaterJumpTime = 0.0f;
	}

	//Jumping stuff
	if (mv->m_nButtons & IN_JUMP)
	{
		if (!m_pTFPlayer->m_Shared.IsJumpBlocked())
			CheckJumpButton();
	}
	else
	{
		m_pTFPlayer->m_Shared.SetBlockJump(false);
	}

	// Perform regular water movement
	WaterMove();

	// Redetermine position vars
	CategorizePosition();

	// If we are on ground, no downward velocity.
	if (player->GetGroundEntity() != NULL)
		mv->m_vecVelocity[2] = 0;
}

void CTFGameMovement::HandleDuckingSpeedCrop(void)
{
	BaseClass::HandleDuckingSpeedCrop();

	// no moving while crouched in loser state
	if ((m_iSpeedCropped & SPEED_CROPPED_DUCK && m_pTFPlayer->m_Shared.IsLoser()))
	{
		// Suppress regular motion
		mv->m_flForwardMove = 0.0f;
		mv->m_flSideMove = 0.0f;
		mv->m_flUpMove = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CTFGameMovement::Friction(bool CSliding)
{
	float	speed, newspeed, control;
	float	friction;
	float	drop;

	// If we are in water jump cycle, don't apply friction
	if (player->m_flWaterJumpTime)
		return;

	// Calculate speed
	speed = VectorLength(mv->m_vecVelocity);

	// If too slow, return
	if (speed < 0.1f || player->GetGroundEntity() == NULL)
		return;

	// apply ground friction
	friction = (CSliding ? of_cslidefriction.GetFloat() : sv_friction.GetFloat()) * player->m_surfaceFriction;

	// Bleed off some speed, but if we have less than the bleed
	// threshold, bleed the threshold amount.
	control = speed < sv_stopspeed.GetFloat() ? sv_stopspeed.GetFloat() : speed;

	// Add the amount to the drop amount.
	drop = control * friction * gpGlobals->frametime;

	// scale the velocity
	newspeed = max(speed - drop, 0) / speed;
	VectorScale(mv->m_vecVelocity, newspeed, mv->m_vecVelocity);
	mv->m_outWishVel -= (1.f - newspeed) * mv->m_vecVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::FullWalkMove()
{
	//deny jump and cslide at the start of the match when you can't move
	bool CanMove = int(mv->m_flClientMaxSpeed) != 1;

	if (!InWater())
		StartGravity();

	// If we are leaping out of the water, just update the counters.
	if (player->m_flWaterJumpTime)
	{
		// Try to jump out of the water (and check to see if we still are).
		WaterJump();
		TryPlayerMove();
		CheckWater();
		return;
	}

	// If we are swimming in the water, see if we are nudging against a place we can jump up out
	//  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0
	if (InWater())
	{
		FullWalkMoveUnderwater();
		return;
	}

	//Jumping stuff
	if (mv->m_nButtons & IN_JUMP && CanMove)
	{
		if (!m_pTFPlayer->m_Shared.IsJumpBlocked())
			CheckJumpButton();
	}
	else
	{
		m_pTFPlayer->m_Shared.SetBlockJump(false);
	}

	//Zombie lunge
	if (m_pTFPlayer->m_Shared.DoLungeCheck())
		CheckLunge();

	// Make sure velocity is valid.
	CheckVelocity();

	//Ground and air movement
	bool CSliding = false;
	if (player->GetGroundEntity() != NULL)
	{
		//check if player can CSlide
		CSliding = of_cslide.GetBool() &&												//crouch sliding is enabled
				   CanMove &&															//player allowed to move
				   !m_pTFPlayer->GetWaterLevel() &&		 								//player is not in water
				   (player->m_Local.m_bDucking || player->m_Local.m_bDucked) &&			//player is ducked/ducking
				   (mv->m_flForwardMove || mv->m_flSideMove) &&							//player is moving
				   gpGlobals->curtime <= m_pTFPlayer->m_Shared.GetCSlideDuration();		//there is crouch slide charge to spend

		Friction(CSliding);
		WalkMove(CSliding);

		//If not using CSlide right away clear it
		if (!CSliding)
			m_pTFPlayer->m_Shared.SetCSlideDuration(0);

		//If not using ramp jump vel right away clear it
		m_pTFPlayer->m_Shared.SetRampJumpVel(0);
	}
	else
	{
		AirMove();
	}

	m_pTFPlayer->m_Shared.SetCSlide(CSliding);

	// Set final flags.
	CategorizePosition();

	// Add any remaining gravitational component if we are not in water.
	if (!InWater())
		FinishGravity();

	//Post gravity settings
	if (player->GetGroundEntity() != NULL)
	{
		mv->m_vecVelocity[2] = 0;

		if (!IsDead() && m_pTFPlayer->m_Shared.IsJumping())
			m_pTFPlayer->m_Shared.SetJumping(false);
	}
	else
	{
		//Determine ramp jump vel and crouch slide duration
		m_pTFPlayer->m_Shared.SetRampJumpVel(mv->m_vecVelocity[2]);
		m_pTFPlayer->m_Shared.SetCSlideDuration(gpGlobals->curtime - mv->m_vecVelocity[2] / 200.f);
	}

	// Handling falling.
	CheckFalling();

	// Make sure velocity is valid.
	CheckVelocity();

	//Cslide sound turn on/off
	CheckCSlideSound(CSliding);
}

void CTFGameMovement::CheckCSlideSound(bool CSliding)
{
	if (CSliding) //always go here if cslide is happening
	{
		if (!player->m_bIsCSliding)
		{
			player->EmitSound("Player.Slide");
			player->m_bIsCSliding = true;
		}
	}
	else if (player->m_bIsCSliding)
	{
		player->StopSound("Player.Slide");
		player->m_bIsCSliding = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::FullTossMove(void)
{
	trace_t pm;
	Vector move;

	// add velocity if player is moving 
	if ((mv->m_flForwardMove != 0.0f) || (mv->m_flSideMove != 0.0f) || (mv->m_flUpMove != 0.0f))
	{
		Vector forward, right, up;
		float fmove, smove;
		Vector wishdir;
		float wishspeed;

		AngleVectors(mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

		// Copy movement amounts
		fmove = mv->m_flForwardMove;
		smove = mv->m_flSideMove;

		VectorNormalize(forward);  // Normalize remainder of vectors.
		VectorNormalize(right);    // 

		for (int i = 0; i < 3; i++)       // Determine x and y parts of velocity
			wishdir[i] = forward[i] * fmove + right[i] * smove;

		wishdir[2] += mv->m_flUpMove;

		// Determine maginitude of speed of move
		wishspeed = VectorNormalize(wishdir);
		wishspeed = min(wishspeed, mv->m_flMaxSpeed);

		// Set pmove velocity
		Accelerate(wishdir, wishspeed, sv_accelerate.GetFloat());
	}

	if (mv->m_vecVelocity[2] > 0)
		SetGroundEntity(NULL);

	// If on ground and not moving, return.
	if (player->GetGroundEntity() != NULL)
	{
		if (VectorCompare(player->GetBaseVelocity(), vec3_origin) &&
			VectorCompare(mv->m_vecVelocity, vec3_origin))
			return;
	}

	CheckVelocity();

	// add gravity
	if (player->GetMoveType() == MOVETYPE_FLYGRAVITY)
		AddGravity();

	// move origin
	// Base velocity is not properly accounted for since this entity will move again after the bounce without
	// taking it into account
	VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	CheckVelocity();

	VectorScale(mv->m_vecVelocity, gpGlobals->frametime, move);
	VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

	PushEntity(move, &pm);	// Should this clear basevelocity

	CheckVelocity();

	if (pm.allsolid)
	{
		// entity is trapped in another solid
		SetGroundEntity(&pm);
		mv->m_vecVelocity.Init();
		return;
	}

	if (pm.fraction != 1.0f)
	{
		PerformFlyCollisionResolution(pm, move);
	}

	// Check for in water
	CheckWater();
}

//-----------------------------------------------------------------------------
// Purpose: Does the basic move attempting to climb up step heights.  It uses
//          the mv->GetAbsOrigin() and mv->m_vecVelocity.  It returns a new
//          new mv->GetAbsOrigin(), mv->m_vecVelocity, and mv->m_outStepHeight.
//-----------------------------------------------------------------------------
void CTFGameMovement::StepMove(Vector &vecDestination, trace_t &trace)
{
	trace_t saveTrace;
	saveTrace = trace;

	Vector vecEndPos;
	VectorCopy(vecDestination, vecEndPos);

	Vector vecPos, vecVel;
	VectorCopy(mv->GetAbsOrigin(), vecPos);
	VectorCopy(mv->m_vecVelocity, vecVel);

	bool bLowRoad = false;
	bool bUpRoad = true;

	// First try the "high road" where we move up and over obstacles
	if (player->m_Local.m_bAllowAutoMovement)
	{
		// Trace up by step height
		VectorCopy(mv->GetAbsOrigin(), vecEndPos);
		vecEndPos.z += player->m_Local.m_flStepSize + DIST_EPSILON;
		TracePlayerBBox(mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);
		if (!trace.startsolid && !trace.allsolid)
		{
			mv->SetAbsOrigin(trace.endpos);
		}

		// Trace over from there
		TryPlayerMove();

		// Then trace back down by step height to get final position
		VectorCopy(mv->GetAbsOrigin(), vecEndPos);
		vecEndPos.z -= player->m_Local.m_flStepSize + DIST_EPSILON;
		TracePlayerBBox(mv->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);
		// If the trace ended up in empty space, copy the end over to the origin.
		if (!trace.startsolid && !trace.allsolid)
		{
			mv->SetAbsOrigin(trace.endpos);
		}

		// If we are not on the standable ground any more or going the "high road" didn't move us at all, then we'll also want to check the "low road"
		if ((trace.fraction != 1.0f &&
			trace.plane.normal[2] < 0.7) || VectorCompare(mv->GetAbsOrigin(), vecPos))
		{
			bLowRoad = true;
			bUpRoad = false;
		}
	}
	else
	{
		bLowRoad = true;
		bUpRoad = false;
	}

	if (bLowRoad)
	{
		// Save off upward results
		Vector vecUpPos, vecUpVel;
		if (bUpRoad)
		{
			VectorCopy(mv->GetAbsOrigin(), vecUpPos);
			VectorCopy(mv->m_vecVelocity, vecUpVel);
		}

		// Take the "low" road
		mv->SetAbsOrigin(vecPos);
		VectorCopy(vecVel, mv->m_vecVelocity);
		VectorCopy(vecDestination, vecEndPos);
		TryPlayerMove(&vecEndPos, &saveTrace);

		// Down results.
		Vector vecDownPos, vecDownVel;
		VectorCopy(mv->GetAbsOrigin(), vecDownPos);
		VectorCopy(mv->m_vecVelocity, vecDownVel);

		if (bUpRoad)
		{
			float flUpDist = (vecUpPos.x - vecPos.x) * (vecUpPos.x - vecPos.x) + (vecUpPos.y - vecPos.y) * (vecUpPos.y - vecPos.y);
			float flDownDist = (vecDownPos.x - vecPos.x) * (vecDownPos.x - vecPos.x) + (vecDownPos.y - vecPos.y) * (vecDownPos.y - vecPos.y);

			// decide which one went farther
			if (flUpDist >= flDownDist)
			{
				mv->SetAbsOrigin(vecUpPos);
				VectorCopy(vecUpVel, mv->m_vecVelocity);

				// copy z value from the Low Road move
				mv->m_vecVelocity.z = vecDownVel.z;
			}
		}
	}

	float flStepDist = mv->GetAbsOrigin().z - vecPos.z;
	if (flStepDist > 0)
	{
		mv->m_outStepHeight += flStepDist;
	}
}

bool CTFGameMovement::GameHasLadders() const
{
	return true;
}

void CTFGameMovement::SetGroundEntity(trace_t *pm)
{
	BaseClass::SetGroundEntity(pm);
	if (pm && pm->m_pEnt)
	{
		m_pTFPlayer->m_Shared.SetAirDash(false);
		m_pTFPlayer->m_Shared.SetAirDashCount(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGameMovement::PlayerRoughLandingEffects(float fvol)
{
	if (m_pTFPlayer && m_pTFPlayer->IsPlayerClass(TF_CLASS_SCOUT))
	{
		// Scouts don't play rumble unless they take damage.
		if (fvol < 1.0)
		{
			fvol = 0;
		}
	}

	BaseClass::PlayerRoughLandingEffects(fvol);
}

/*
if (of_ramp_jump.GetBool())
{
	bool bTrimped = false;
	bool bDownTrimped = false;

	trace_t pm;

	// Adjusted for bboxes.
	// TODO: Look at this later
	Vector vecStart = mv->GetAbsOrigin(); // + Vector(0, 0, GetPlayerMins()[2] + 1.0f);
	Vector vecStop = vecStart - Vector(0, 0, 60.0f);

	TracePlayerBBox(vecStart, vecStop, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm);

	// Found the floor
	if (pm.fraction != 1.0f)
	{
		// Take the lateral velocity
		Vector vecVelocity = mv->m_vecVelocity * Vector(1.0f, 1.0f, 0.0f);
		float flHorizontalSpeed = vecVelocity.Length();

		if (flHorizontalSpeed > 0)
			vecVelocity /= flHorizontalSpeed;

		float flDotProduct = DotProduct(vecVelocity, pm.plane.normal);
		float flRampSlideDotProduct = DotProduct(mv->m_vecVelocity, pm.plane.normal);

		// They have to be at least moving a bit
		if (flHorizontalSpeed > of_ramp_min_speed.GetFloat())
		{
			// Don't do anything for flat ground or downwardly sloping (relative to motion)
			// Changed to 0.15f to make it a bit less trimpy on only slightly uneven ground
			//if (flDotProduct < -0.15f || flDotProduct > 0.15f)
			if (flDotProduct < -0.15f)
			{
				float flForwardMul = flMul + (-flDotProduct * flHorizontalSpeed * of_ramp_up_forward_multiplier.GetFloat());
				// This is one way to do it
				flMul += -flDotProduct * flHorizontalSpeed * of_ramp_up_multiplier.GetFloat(); //0.6f;
				DevMsg("[S] Trimp %f! Dotproduct:%f. Horizontal speed:%f. Rampslide dot.p.:%f\n", flMul, flDotProduct, flHorizontalSpeed, flRampSlideDotProduct);

				bTrimped = true;

				mv->m_vecVelocity[0] *= flForwardMul;
				mv->m_vecVelocity[1] *= flForwardMul;
				// This is another that'll give some different height results
				// UNDONE: Reverted back to the original way for now
				//Vector reflect = mv->m_vecVelocity + (-2.0f * pm.plane.normal * DotProduct(mv->m_vecVelocity, pm.plane.normal));
				//float flSpeedAmount = clamp((flLength - 400.0f) / 800.0f, 0, 1.0f);
				//flMul += reflect.z * flSpeedAmount;
			}
		}
		// trigger downwards trimp at any speed
		if (flHorizontalSpeed > 50.0f)
		{
			if (flDotProduct > 0.15f) // AfterShock: travelling downwards onto a downward ramp - give boost horizontally
			{
				// This is one way to do it
				//mv->m_vecVelocity[1] += -flDotProduct * mv->m_vecVelocity[2] * sv_trimpmultiplier.GetFloat(); //0.6f;
				//mv->m_vecVelocity[0] += -flDotProduct * mv->m_vecVelocity[2] * sv_trimpmultiplier.GetFloat(); //0.6f;
				//mv->m_vecVelocity[1] += -flDotProduct * flMul * sv_trimpmultiplier.GetFloat(); //0.6f;
				//mv->m_vecVelocity[0] += -flDotProduct * flMul * sv_trimpmultiplier.GetFloat(); //0.6f;
				DevMsg("[S] Down Trimp %f! Dotproduct:%f, upwards vel:%f, vel 1:%f, vel 0:%f\n", flMul, flDotProduct, mv->m_vecVelocity[2], mv->m_vecVelocity[1], mv->m_vecVelocity[0]);

				bDownTrimped = true;

				flMul *= (1.0f / of_ramp_down_multiplier.GetFloat());
			}
		}
	}
}*/