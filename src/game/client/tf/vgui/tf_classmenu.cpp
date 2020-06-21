//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_classmenu.h"

#include <KeyValues.h>
#include <filesystem.h>
#include <vgui_controls/Button.h>
#include <vgui/IVGui.h>

#include "hud.h" // for gEngfuncs
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "c_tf_playerresource.h"

#include "tf_controls.h"
#include "vguicenterprint.h"
#include "of_imagemouseoverbutton.h"
#include "IconPanel.h"

#include "IGameUIFuncs.h" // for key bindings

#include "tf_hud_notification_panel.h"

extern IGameUIFuncs *gameuifuncs; // for key binding details

using namespace vgui;

ConVar _cl_classmenuopen( "_cl_classmenuopen", "0", 0, "internal cvar used to tell server when class menu is open" );

// menu buttons are not in the same order as the defines
static int iRemapIndexToClass[TF_CLASS_MENU_BUTTONS] = 
{
	0,
	TF_CLASS_SCOUT,
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,
	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,
	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY,
	0,
	TF_CLASS_MERCENARY,
	TF_CLASS_RANDOM,
	TF_CLASS_CIVILIAN,
	TF_CLASS_JUGGERNAUT,
};

int GetIndexForClass( int iClass )
{
	int iIndex = 0;

	for ( int i = 0 ; i < TF_CLASS_MENU_BUTTONS ; i++ )
	{
		if ( iRemapIndexToClass[i] == iClass )
		{
			iIndex = i;
			break;
		}
	}

	return iIndex;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFClassMenu::CTFClassMenu( IViewPort *pViewPort ) : CClassMenu( pViewPort )
{
	m_mouseoverButtons.RemoveAll();

	m_iClassMenuKey = BUTTON_CODE_INVALID;
	m_iCurrentClassIndex = TF_CLASS_HEAVYWEAPONS;

	m_pClassInfoPanel = new CTFClassInfoPanel( this, "ClassInfoPanel" );
	LoadControlSettings( "Resource/UI/ClassInfoPanel.res" );

	Q_memset( m_pClassButtons, 0, sizeof( m_pClassButtons ) );


	char tempName[MAX_PATH];
	for ( int i = 0 ; i < CLASS_COUNT_IMAGES ; ++i )
	{
		Q_snprintf( tempName, sizeof( tempName ), "countImage%d", i );
		m_ClassCountImages[i] = new CTFImagePanel( this, tempName );
	}

	m_pCountLabel = NULL;
	
	EditablePanel *pOffense = new EditablePanel( this, "OffensePanel" );

	m_pClassButtons[TF_CLASS_SCOUT] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pOffense, "scout", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_SOLDIER] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pOffense, "soldier", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_PYRO] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pOffense, "pyro", m_pClassInfoPanel, this );

	EditablePanel *pDefense = new EditablePanel( this, "DefensePanel" );
	
	m_pClassButtons[TF_CLASS_DEMOMAN] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pDefense, "demoman", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_HEAVYWEAPONS] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pDefense, "heavyweapons", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_ENGINEER] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pDefense, "engineer", m_pClassInfoPanel, this );
	
	EditablePanel *pSupport = new EditablePanel( this, "SupportPanel" );
	
	m_pClassButtons[TF_CLASS_MEDIC] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pSupport, "medic", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_SNIPER] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pSupport, "sniper", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_SPY] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pSupport, "spy", m_pClassInfoPanel, this );
	
	EditablePanel *pMisc = new EditablePanel( this, "MiscPanel" );
	
	m_pClassButtons[TF_CLASS_MERCENARY] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pMisc, "mercenary", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_CIVILIAN] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pMisc, "civilian", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_RANDOM] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pMisc, "randompc", m_pClassInfoPanel, this );
	m_pClassButtons[TF_CLASS_JUGGERNAUT] = new CTFImageMouseOverButton<CTFClassInfoPanel>( pMisc, "juggernaut", m_pClassInfoPanel, this );

	for( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ )
	{
		if( m_pClassButtons[i] )
			m_pClassButtons[i]->AddActionSignalTarget( this );
	}
	
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ClassMenu.res" );
	
	for( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ ) 
	{
		if( !m_pClassButtons[i] )
			continue;

		CTFImageMouseOverButton<CTFClassInfoPanel> *button = m_pClassButtons[i];

		if ( button )
		{
			button->SetPreserveArmedButtons( true );
			button->SetUpdateDefaultButtons( true );
		}
	}
}

void CTFClassMenu::PerformLayout()
{
	BaseClass::PerformLayout();


	m_pCountLabel = dynamic_cast< CExLabel * >( FindChildByName( "CountLabel" ) );

	if ( m_pCountLabel )
	{
		m_pCountLabel->SizeToContents();
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFImageMouseOverButton<CTFClassInfoPanel> *CTFClassMenu::GetCurrentClassButton()
{
	int iClass = TF_CLASS_HEAVYWEAPONS;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED )
	{
		iClass = pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex();
	}

	m_iCurrentClassIndex = GetIndexForClass( iClass );
	return m_pClassButtons[iClass];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::ShowPanel( bool bShow )
{
	if ( bShow )
	{
		// can't change class if you're on the losing team during the "bonus time" after a team has won the round
		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
			 C_TFPlayer::GetLocalTFPlayer() && 
			 C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TFGameRules()->GetWinningTeam()
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_SPECTATOR 
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_UNASSIGNED 
			 && GetSpectatorMode() == OBS_MODE_NONE )
		{
			SetVisible( false );
			SetMouseInputEnabled( false );

			CHudNotificationPanel *pNotifyPanel = GET_HUDELEMENT( CHudNotificationPanel );
			if ( pNotifyPanel )
			{
				pNotifyPanel->SetupNotifyCustom( "#TF_CantChangeClassNow", "ico_notify_flag_moving", C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() );
			}

			return;
		}

		engine->CheckPoint( "ClassMenu" );

		Activate();
		SetMouseInputEnabled( true );

		m_iClassMenuKey = gameuifuncs->GetButtonCodeForBind( "changeclass" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		CTFImageMouseOverButton<CTFClassInfoPanel> *pInitialButton = GetCurrentClassButton();

		for( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ ) 
		{
			if( !m_pClassButtons[i] )
				continue;
			CTFImageMouseOverButton<CTFClassInfoPanel> *button = m_pClassButtons[i];

			if ( button )
			{
				if ( button == pInitialButton )
				{
					CModelPanel *pModelPanel = dynamic_cast<CModelPanel *>(button->FindChildByName( "classModel" ) );
					if ( pModelPanel )
					{
						int iVisibleTeam = GetTeamNumber() < TF_TEAM_RED ? TF_TEAM_MERCENARY : GetTeamNumber();
						
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
							if ( of_tennisball.GetBool() && Q_stricmp(button->GetName(),"mercenary") )
								pModelPanel->SetSkin( 6 );
							else
								pModelPanel->SetSkin( 4 );
							break;						
						}
						
						iVisibleTeam = GetTeamNumber() < TF_TEAM_RED ? TF_TEAM_MERCENARY : GetTeamNumber();
						pModelPanel->SetAttachmentsSkin( iVisibleTeam - 2 );
					}
					button->ShowPage();
					button->SetArmed( true );
					button->SetAsDefaultButton( 1 );

					g_lastButton = button;
				}
				else
				{
					button->HidePage();
					button->SetArmed( false );
				}
			}
		}
	}
	else
	{
		// turn off all of our ClassInfo panels so the VCDs will stop playing
		for( int i = 0; i < TF_CLASS_MENU_BUTTONS; i++ ) 
		{
			if( !m_pClassButtons[i] )
				continue;
			CTFImageMouseOverButton<CTFClassInfoPanel> *button = m_pClassButtons[i];

			if ( button )
			{
				button->HidePage();
			}
		}

		// everything is off so just reset these for next time
		g_lastButton = NULL;
		g_lastPanel = NULL;

		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnKeyCodePressed( KeyCode code )
{
	m_KeyRepeat.KeyDown( code );

	if ( ( m_iClassMenuKey != BUTTON_CODE_INVALID && m_iClassMenuKey == code ) ||
		code == KEY_XBUTTON_BACK || 
		code == KEY_XBUTTON_B )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer && ( pLocalPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED ) )
		{
			ShowPanel( false );
		}
	}
	else if( code == KEY_SPACE || code == KEY_XBUTTON_A || code == KEY_XBUTTON_RTRIGGER )
	{
		ipanel()->SendMessage( GetFocusNavGroup().GetDefaultButton(), new KeyValues( "PressButton" ), GetVPanel() );
	}
	else if( code == KEY_XBUTTON_RIGHT || code == KEY_XSTICK1_RIGHT )
	{
		int loopCheck = 0;

		do 
		{
			loopCheck++;
			m_iCurrentClassIndex++;
			m_iCurrentClassIndex = ( m_iCurrentClassIndex % TF_CLASS_MENU_BUTTONS );
		} while( ( m_pClassButtons[ iRemapIndexToClass[m_iCurrentClassIndex] ] == NULL ) && ( loopCheck < TF_CLASS_MENU_BUTTONS ) );
		
		CTFImageMouseOverButton<CTFClassInfoPanel> *pButton = m_pClassButtons[ iRemapIndexToClass[m_iCurrentClassIndex] ];
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
	else if( code == KEY_XBUTTON_LEFT || code == KEY_XSTICK1_LEFT )
	{
		int loopCheck = 0;

		do 
		{
			loopCheck++;
			m_iCurrentClassIndex--;
			if ( m_iCurrentClassIndex <= 0 )
			{
				m_iCurrentClassIndex = TF_CLASS_RANDOM;
			}
		} while( ( m_pClassButtons[ iRemapIndexToClass[m_iCurrentClassIndex] ] == NULL ) && ( loopCheck < TF_CLASS_MENU_BUTTONS ) );

		CTFImageMouseOverButton<CTFClassInfoPanel> *pButton = m_pClassButtons[ iRemapIndexToClass[m_iCurrentClassIndex] ];
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}
	}
	else if( code == KEY_XBUTTON_UP || code == KEY_XSTICK1_UP )
	{
		// Scroll class info text up
		if ( g_lastPanel )
		{
			CExRichText *pRichText = dynamic_cast< CExRichText * >( g_lastPanel->FindChildByName( "classInfo" ) );

			if ( pRichText )
			{
				PostMessage( pRichText, new KeyValues("MoveScrollBarDirect", "delta", 1) );
			}
		}
	}
	else if( code == KEY_XBUTTON_DOWN || code == KEY_XSTICK1_DOWN )
	{
		// Scroll class info text up
		if ( g_lastPanel )
		{
			CExRichText *pRichText = dynamic_cast< CExRichText * >( g_lastPanel->FindChildByName( "classInfo" ) );

			if ( pRichText )
			{
				PostMessage( pRichText, new KeyValues("MoveScrollBarDirect", "delta", -1) );
			}
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnKeyCodeReleased( vgui::KeyCode code )
{
	m_KeyRepeat.KeyUp( code );

	BaseClass::OnKeyCodeReleased( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnThink()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( !pLocalPlayer->CanShowClassMenu() )
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
	vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
	if ( code )
	{
		OnKeyCodePressed( code );
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::Update()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() != TF_CLASS_UNDEFINED ) )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", true );
		}
#else
		SetVisibleButton( "CancelButton", true );
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", false );
		}
#else
		SetVisibleButton( "CancelButton", false );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Panel *CTFClassMenu::CreateControlByName( const char *controlName )
{
	if ( !Q_stricmp( "CIconPanel", controlName ) )
	{
		return new CIconPanel( this, "icon_panel" );
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}

//-----------------------------------------------------------------------------
// Catch the mouseover event and set the active class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnShowPage( const char *pagename )
{
}

//-----------------------------------------------------------------------------
// Draw nothing
//-----------------------------------------------------------------------------
void CTFClassMenu::PaintBackground( void )
{
}

//-----------------------------------------------------------------------------
// Do things that should be done often, eg number of players in the 
// selected class
//-----------------------------------------------------------------------------
void CTFClassMenu::OnTick( void )
{
	//When a player changes teams, their class and team values don't get here 
	//necessarily before the command to update the class menu. This leads to the cancel button 
	//being visible and people cancelling before they have a class. check for class == TF_CLASS_UNDEFINED and if so
	//hide the cancel button

	if ( !IsVisible() )
		return;

#ifndef _X360
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Force them to pick a class if they haven't picked one yet.
	if ( pLocalPlayer && pLocalPlayer->m_Shared.GetDesiredPlayerClassIndex() == TF_CLASS_UNDEFINED )
	{
		SetVisibleButton( "CancelButton", false );
	}

	UpdateClassCounts();

#endif

	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::OnClose()
{
	ShowPanel( false );

	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFClassMenu::SetVisible( bool state )
{
	BaseClass::SetVisible( state );

	m_KeyRepeat.Reset();

	if ( state )
	{
		engine->ServerCmd( "menuopen" );			// to the server
		engine->ClientCmd( "_cl_classmenuopen 1" );	// for other panels
	}
	else
	{
		engine->ServerCmd( "menuclosed" );	
		engine->ClientCmd( "_cl_classmenuopen 0" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Console command to select a class
//-----------------------------------------------------------------------------
void CTFClassMenu::Join_Class( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		char cmd[256];
		Q_snprintf( cmd, sizeof( cmd ), "joinclass %s", args.Arg( 1 ) );
		OnCommand( cmd );
		ShowPanel( false );
	}
}

static const char *g_sDialogVariables[] = {
	"",
	"numScout",
	"numSniper",
	"numSoldier",
	

	"numDemoman",
	"numMedic",
	"numHeavy",
	
	"numPyro",
	"numSpy",
	"numEngineer",
	"numMercenary",
	"numCivilian",
	"numJuggernaut",
	"",
};

static const char *g_sClassImagesPlaceholder[] = {
	"",
	""
};

static int g_sClassDefines[] = {
	0,
	TF_CLASS_SCOUT,
    TF_CLASS_SNIPER,
    TF_CLASS_SOLDIER,
	
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HEAVYWEAPONS,
	
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,		
	TF_CLASS_MERCENARY,
	TF_CLASS_CIVILIAN,
	TF_CLASS_JUGGERNAUT,
	0,
};

void CTFClassMenu::UpdateNumClassLabels( int iTeam )
{
#ifndef _X360
	int nTotalCount = 0;

	// count how many of each class there are
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );

	if ( !tf_PR )
		return;

	if ( iTeam < FIRST_GAME_TEAM || iTeam >= TF_TEAM_COUNT ) // invalid team number
		return;

	for( int i = TF_FIRST_NORMAL_CLASS ; i <= TF_CLASS_COUNT_ALL ; i++ )
	{
		int classCount = tf_PR->GetCountForPlayerClass( iTeam, g_sClassDefines[i], true );

		if ( classCount > 0 )
		{
			SetDialogVariable( g_sDialogVariables[i], classCount );
		}
		else
		{
			SetDialogVariable( g_sDialogVariables[i], "" );
		}


		if ( nTotalCount < CLASS_COUNT_IMAGES )
		{
			for ( int j = 0 ; j < classCount ; ++j )
			{
				CTFImagePanel *pImage = m_ClassCountImages[nTotalCount];
				if ( pImage )
				{
					TFPlayerClassData_t *pClassData = GetPlayerClassData(i);
					pImage->SetVisible( true );
					if ( iTeam == TF_TEAM_RED ){
						if ( pClassData->GetClassSelectImageRed() )
							pImage->SetImage( pClassData->GetClassSelectImageRed() );
						else
							pImage->SetImage( g_sClassImagesPlaceholder[0] );
							
					}
					else if ( iTeam == TF_TEAM_BLUE )
					{
						if( pClassData->GetClassSelectImageBlue() )
							pImage->SetImage( pClassData->GetClassSelectImageBlue() );
						else
							pImage->SetImage( g_sClassImagesPlaceholder[0] );
					}
					else if ( iTeam == TF_TEAM_MERCENARY )
					{
						if ( pClassData->GetClassSelectImageMercenary() )
							pImage->SetImage( pClassData->GetClassSelectImageMercenary() );
						else
							pImage->SetImage( g_sClassImagesPlaceholder[0] );
					}
				}

				nTotalCount++;
				if ( nTotalCount >= CLASS_COUNT_IMAGES )
				{
					break;
				}
			}
		}
	}

	if ( nTotalCount == 0 )
	{
		// no classes for our team yet
		if ( m_pCountLabel && m_pCountLabel->IsVisible() )
		{
			m_pCountLabel->SetVisible( false );
		}
	}
	else
	{
		if ( m_pCountLabel && !m_pCountLabel->IsVisible() )
		{
			m_pCountLabel->SetVisible( true );
		}
	}

	// turn off any unused images
	while ( nTotalCount < CLASS_COUNT_IMAGES )
	{
		CTFImagePanel *pImage = m_ClassCountImages[nTotalCount];
		if ( pImage )
		{
			pImage->SetVisible( false );
		}

		nTotalCount++;
	}
#endif
}
