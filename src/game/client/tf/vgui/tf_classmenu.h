//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_CLASSMENU_H
#define TF_CLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <classmenu.h>
#include <vgui_controls/EditablePanel.h>
#include "vgui_controls/KeyRepeat.h"
#include <filesystem.h>
#include <tf_shareddefs.h>
#include "cbase.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "basemodelpanel.h"
#include "of_imagemouseoverbutton.h"
#include "IconPanel.h"
#include <vgui_controls/CheckButton.h>
#include "GameEventListener.h"
#include "c_tf_playerresource.h"

using namespace vgui;

#define CLASS_COUNT_IMAGES	11

extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;
extern ConVar of_tennisball;
//-----------------------------------------------------------------------------
// This is the entire info panel for the specific class
//-----------------------------------------------------------------------------
class CTFClassInfoPanel : public vgui::EditablePanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassInfoPanel, vgui::EditablePanel );

public:
	CTFClassInfoPanel( vgui::Panel *parent, const char *panelName ) : vgui::EditablePanel( parent, panelName )
	{
		ListenForGameEvent( "localplayer_changeteam" );
	}

	virtual void SetVisible( bool state )
	{
		CModelPanel *pModelPanel = dynamic_cast<CModelPanel *>(FindChildByName( "classModel" ) );
		if ( pModelPanel )
		{
			pModelPanel->SetPanelDirty();

			if ( !state )
			{
				// stop the panel from running any VCD data
				pModelPanel->DeleteVCDData();
			}
			Vector vLocalColor;
			vLocalColor.x = of_color_r.GetFloat()/255.0f;
			vLocalColor.y = of_color_g.GetFloat()/255.0f;
			vLocalColor.z = of_color_b.GetFloat()/255.0f;
			pModelPanel->SetModelColor(vLocalColor);
		}

		CExRichText *pRichText = dynamic_cast<CExRichText *>(FindChildByName( "classInfo" ) );
		if ( pRichText )
		{
			pRichText->InvalidateLayout( true, false );
		}

		BaseClass::SetVisible( state );
	}

	virtual void FireGameEvent( IGameEvent * event )
	{
		if ( FStrEq( "localplayer_changeteam", event->GetName() ) )
		{
			CModelPanel *pModelPanel = dynamic_cast<CModelPanel *>(FindChildByName( "classModel" ) );
			if ( pModelPanel )
			{
				C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
				int iVisibleTeam = pPlayer ? pPlayer->GetTeamNumber() : GetTeamNumber();
				
				switch( iVisibleTeam )
				{
					case TF_TEAM_RED:
					pModelPanel->SetSkin( 0 );
					break;
					case TF_TEAM_BLUE:
					pModelPanel->SetSkin( 1 );
					break;
					default:
					case TF_TEAM_MERCENARY:
					// this is really hacky, if the name is merc, do brightskins
					if ( of_tennisball.GetBool() && Q_stricmp(GetName(),"mercenary") )
						pModelPanel->SetSkin( 6 );
					else
						pModelPanel->SetSkin( 4 );
					break;						
				}

				pModelPanel->SetAttachmentsSkin( iVisibleTeam - 2 );
			}
		}
	}
	
	virtual int GetTeamNumber( void )
	{
		CClassMenu *pParent = dynamic_cast<CClassMenu*>(GetParent());
		if( pParent )
			return pParent->GetTeamNumber();
		else
			return TF_TEAM_RED;
	}
};


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CTFClassMenu : public CClassMenu
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassMenu, CClassMenu );

public:
	CTFClassMenu( IViewPort *pViewPort );

	virtual void Update( void );
	virtual Panel *CreateControlByName( const char *controlName );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void SetVisible( bool state );
	virtual void PerformLayout();

	MESSAGE_FUNC_CHARPTR( OnShowPage, "ShowPage", page );
	CON_COMMAND_MEMBER_F( CTFClassMenu, "join_class", Join_Class, "Send a joinclass command", 0 );

	virtual void OnClose();
	virtual void ShowPanel( bool bShow );
	
	virtual bool AffectedByDrawHUD( void ){ return false; }

	virtual const char *GetName( void )
	{ 
		return PANEL_CLASS; 
	}
	
	virtual int GetTeamNumber( void )
	{
		switch( GetLocalPlayerTeam() )
		{
			case TF_TEAM_RED:
			case TF_TEAM_BLUE:
			case TF_TEAM_MERCENARY:
				return GetLocalPlayerTeam();
				break;
			default:
				return TF_TEAM_MERCENARY;
			break;
		}
	}
	virtual void UpdateClassCounts( void ){ UpdateNumClassLabels( GetTeamNumber() ); }
	
protected:
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnKeyCodePressed( KeyCode code );
	virtual CTFImageMouseOverButton<CTFClassInfoPanel> *GetCurrentClassButton();
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnThink();
	virtual void UpdateNumClassLabels( int iTeam );

protected:

	CTFImageMouseOverButton<CTFClassInfoPanel> *m_pClassButtons[TF_CLASS_MENU_BUTTONS];
	CTFClassInfoPanel *m_pClassInfoPanel;

private:

#ifdef _X360
	CTFFooter		*m_pFooter;
#endif

	ButtonCode_t	m_iClassMenuKey;
	int				m_iCurrentClassIndex;
	vgui::CKeyRepeatHandler	m_KeyRepeat;

#ifndef _X360
	CTFImagePanel *m_ClassCountImages[CLASS_COUNT_IMAGES];
	CExLabel *m_pCountLabel;
#endif
};

#endif // TF_CLASSMENU_H

