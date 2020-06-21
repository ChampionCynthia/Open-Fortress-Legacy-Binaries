//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>
#include "tf_tips.h"
#include "renderparm.h"
#include "animation.h"
#include "tf_controls.h"
#include "cvartogglecheckbutton.h"
#include "datacache/imdlcache.h"

#include "of_colorpanel.h"

#include "engine/IEngineSound.h"
#include "basemodelpanel.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include <convar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Slider.h>
#include "fmtstr.h"

#include "tier0/dbg.h"

using namespace vgui;

// "Simple" function to convert Hue Saturation and Brightness to RGB
// Hue goes from 0 to 360
// Saturation and brightness goes from 0 to 1 ( float value which represents 0% to 100% )
// Maybe we should try to do this with inline assembly code later?
// Definetly beneficial for such a common function

typedef struct {
    double r;       // a fraction between 0 and 1
    double g;       // a fraction between 0 and 1
    double b;       // a fraction between 0 and 1
} rgb;

typedef struct {
    double h;       // angle in degrees
    double s;       // a fraction between 0 and 1
    double v;       // a fraction between 0 and 1
} hsv;

static rgb   hsv2rgb(hsv in);
static hsv   rgb2hsv(rgb in);

hsv rgb2hsv(rgb in)
{
    hsv         out;
    double      min, max, delta;

    min = in.r < in.g ? in.r : in.g;
    min = min  < in.b ? min  : in.b;

    max = in.r > in.g ? in.r : in.g;
    max = max  > in.b ? max  : in.b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( in.r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( in.g - in.b ) / delta;        // between yellow & magenta
    else
    if( in.g >= max )
        out.h = 2.0 + ( in.b - in.r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( in.r - in.g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}

rgb hsv2rgb(hsv in)
{
    double      hh, p, q, t, ff;
    long        i;
    rgb         out;

    if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
        out.r = in.v;
        out.g = in.v;
        out.b = in.v;
        return out;
    }
    hh = in.h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = in.v * (1.0 - in.s);
    q = in.v * (1.0 - (in.s * ff));
    t = in.v * (1.0 - (in.s * (1.0 - ff)));

    switch(i) {
    case 0:
        out.r = in.v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = in.v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = in.v;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = in.v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = in.v;
        break;
    case 5:
    default:
        out.r = in.v;
        out.g = p;
        out.b = q;
        break;
    }
    return out;     
}

Color HSBtoRGB( float iH, float iS, float iB )
{
	hsv in;
	in.h = iH;
	in.s = iS;
	in.v = iB;
	rgb out = hsv2rgb( in );
	
	Color vecCol;
	vecCol.SetColor((int)(out.r * 255.0f),(int)(out.g * 255.0f),(int)(out.b * 255.0f), 255);
	return vecCol;
}

extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

DECLARE_BUILD_FACTORY( CTFColorPanel );
DECLARE_BUILD_FACTORY( CTFColorSlider );

ConVar of_use_rgb("of_use_rgb", "0", FCVAR_CLIENTDLL | FCVAR_USERINFO | FCVAR_ARCHIVE );

CTFColorPanel::CTFColorPanel(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	pHue = new CTFColorSlider(this ,"Hue");
	pSaturation = new CTFColorSlider(this ,"Saturation");
	pBrightness = new CTFColorSlider(this ,"Brightness");

	pHueEntry = new TextEntry(this, "HueEntry");
	pHueEntry->AddActionSignalTarget(this);	
	
	pSaturationEntry = new TextEntry(this, "SaturationEntry");
	pSaturationEntry->AddActionSignalTarget(this);	
	
	pBrightnessEntry = new TextEntry(this, "BrightnessEntry");
	pBrightnessEntry->AddActionSignalTarget(this);

	pRed = new CTFColorSlider(this ,"Red");
	pGreen = new CTFColorSlider(this ,"Green");
	pBlue = new CTFColorSlider(this ,"Blue");
	
	pRedEntry = new TextEntry(this, "RedEntry");
	pRedEntry->AddActionSignalTarget(this);
	
	pGreenEntry = new TextEntry(this, "GreenEntry");
	pGreenEntry->AddActionSignalTarget(this);
	
	pBlueEntry = new TextEntry(this, "BlueEntry");
	pBlueEntry->AddActionSignalTarget(this);
	
	pHexEntry = new TextEntry(this, "HexEntry");
	pHexEntry->AddActionSignalTarget(this);
	
	cHueS = Color( 255, 255, 255, 255 );
	cHueB = Color( 255, 255, 255, 255 );
	cHueBnoS = Color( 255, 255, 255, 255 );
	
	iCurrRed = 0;
	iCurrGreen = 0;	
	iCurrBlue = 0;	

    pRGBToggle = new CCvarToggleCheckButton(
        this,
        "RGBToggle",
        "Use RGB",
        "of_use_rgb");
	
	bReset = false;
	bRGBOn = false;
	bUpdateHexValue = true;
	
	SetScheme( "ClientScheme" );
}

void CTFColorPanel::OnColorChanged( bool bTriggerChangeMessage, bool bUpdateHexValue )
{
	if( !bReset )
		return;
	
	if( !of_use_rgb.GetBool() )
	{
		if( !pHue || !pSaturation || !pBrightness )
			return;

		float iH = pHue->GetValue();
		float iS = pSaturation->GetValue() / 100.0f;
		float iB = pBrightness->GetValue() / 100.0f;
		
		Color RGB = HSBtoRGB( iH, iS, iB );
		cHueB = HSBtoRGB( iH, 1, iB );
		cHueBnoS = HSBtoRGB( iH, 0, iB );
		cHueS = HSBtoRGB( iH, iS, 1 );

		int iRed, iGreen, iBlue, iAlpha;
		RGB.GetColor(iRed, iGreen, iBlue, iAlpha);
		
		engine->ExecuteClientCmd( VarArgs("of_color_r %d", iRed) );
		engine->ExecuteClientCmd( VarArgs("of_color_g %d", iGreen) );
		engine->ExecuteClientCmd( VarArgs("of_color_b %d", iBlue) );
		
		iCurrRed = iRed;
		iCurrGreen = iGreen;
		iCurrBlue = iBlue;
	}
	else
	{
		if( !pRed || !pGreen || !pBlue )
			return;

		engine->ExecuteClientCmd( VarArgs("of_color_r %d", pRed->GetValue()) );
		engine->ExecuteClientCmd( VarArgs("of_color_g %d", pGreen->GetValue()) );
		engine->ExecuteClientCmd( VarArgs("of_color_b %d", pBlue->GetValue()) );

		iCurrRed = pRed->GetValue();
		iCurrGreen = pGreen->GetValue();
		iCurrBlue = pBlue->GetValue();
	}
	
	if( bTriggerChangeMessage )
	{
		OnControlModified( pHue );
		OnControlModified( pSaturation );
		OnControlModified( pBrightness );

		OnControlModified( pRed );
		OnControlModified( pGreen );
		OnControlModified( pBlue );			
	}
	
	if( bUpdateHexValue )
	{
		char szRedHex[3];
		Q_snprintf( szRedHex, sizeof(szRedHex), of_color_r.GetInt() > 15 ? "%X" : "0%X", of_color_r.GetInt() );
		
		char szGreenHex[3];
		Q_snprintf( szGreenHex, sizeof(szGreenHex), of_color_g.GetInt() > 15 ? "%X" : "0%X", of_color_g.GetInt() );
		
		char szBlueHex[3];
		Q_snprintf( szBlueHex, sizeof(szBlueHex), of_color_b.GetInt() > 15 ? "%X" : "0%X", of_color_b.GetInt() );	
		
		pHexEntry->SetText(VarArgs("#%s%s%s", szRedHex, szGreenHex, szBlueHex ));		
	}
}

static Color iFade[] = 
{
	Color( 255, 0, 0, 255 ),
	Color( 255, 255, 0, 255 ),
	Color( 0, 255, 0, 255 ),
	Color( 0, 255, 255, 255 ),
	Color( 0, 0, 255, 255 ),
	Color( 255, 0, 255, 255 ),
	Color( 255, 0, 0, 255 ),
};

void CTFColorPanel::RecalculateColorValues()
{
	if( pRed && pGreen && pBlue && pRedEntry && pGreenEntry && pBlueEntry )
	{
		pRed->SetVisible( of_use_rgb.GetBool() );
		pGreen->SetVisible( of_use_rgb.GetBool() );
		pBlue->SetVisible( of_use_rgb.GetBool() );
		
		pRedEntry->SetVisible( of_use_rgb.GetBool() );
		pGreenEntry->SetVisible( of_use_rgb.GetBool() );
		pBlueEntry->SetVisible( of_use_rgb.GetBool() );
	}
	
	if( pHue && pSaturation && pBrightness && pHueEntry && pSaturationEntry && pBrightnessEntry )
	{
		pHue->SetVisible( !of_use_rgb.GetBool() );
		pSaturation->SetVisible( !of_use_rgb.GetBool() );
		pBrightness->SetVisible( !of_use_rgb.GetBool() );
		
		pHueEntry->SetVisible( !of_use_rgb.GetBool() );
		pSaturationEntry->SetVisible( !of_use_rgb.GetBool() );
		pBrightnessEntry->SetVisible( !of_use_rgb.GetBool() );
	}
	
	Panel *pHueLabel = FindChildByName( "HueLabel" );
	Panel *pSaturationLabel = FindChildByName( "SaturationLabel" );
	Panel *pBrightnessLabel = FindChildByName( "BrightnessLabel" );

	if( pHueLabel && pSaturationLabel && pBrightnessLabel )
	{
		pHueLabel->SetVisible( !of_use_rgb.GetBool() );
		pSaturationLabel->SetVisible( !of_use_rgb.GetBool() );
		pBrightnessLabel->SetVisible( !of_use_rgb.GetBool() );
	}
	
	Panel *pRedLabel = FindChildByName( "RedLabel" );
	Panel *pGreenLabel = FindChildByName( "GreenLabel" );
	Panel *pBlueLabel = FindChildByName( "BlueLabel" );

	if( pRedLabel && pGreenLabel && pBlueLabel )
	{
		pRedLabel->SetVisible( of_use_rgb.GetBool() );
		pGreenLabel->SetVisible( of_use_rgb.GetBool() );
		pBlueLabel->SetVisible( of_use_rgb.GetBool() );
	}
	
	bRGBOn = of_use_rgb.GetBool();

	if( bRGBOn )
	{
		bReset = false;

		pRed->SetValue(of_color_r.GetInt(), false);
		pGreen->SetValue(of_color_g.GetInt(), false);
		pBlue->SetValue(of_color_b.GetInt(), false);

		char buf[64];
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_r.GetInt() );
		pRedEntry->SetText(buf);                               
											   
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_g.GetInt() );
		pGreenEntry->SetText(buf);                        
											   
		Q_snprintf(buf, sizeof(buf), "%d", (int)of_color_b.GetInt() );
		pBlueEntry->SetText(buf);

		bReset = true;
	}
	else
	{
		bReset = false;
		rgb in;
		
		in.r = of_color_r.GetFloat() / 255;
		in.g = of_color_g.GetFloat() / 255;
		in.b = of_color_b.GetFloat() / 255;

		hsv out = rgb2hsv( in );
		
		float iH = out.h;
		float iS = out.s;
		float iB = out.v;
		
		cHueB = HSBtoRGB( iH, 1, iB );
		cHueBnoS = HSBtoRGB( iH, 0, iB );
		cHueS = HSBtoRGB( iH, iS, 1 );
		
		char buf[64];
		Q_snprintf(buf, sizeof(buf), "%d", (int)out.h);
		pHueEntry->SetText(buf);
		
		Q_snprintf(buf, sizeof(buf), "%d",(int)(out.s * 100));
		pSaturationEntry->SetText(buf);
		
		Q_snprintf(buf, sizeof(buf), "%d",(int)(out.v * 100));
		pBrightnessEntry->SetText(buf);
		
		pHue->SetValue(out.h, false);
		pSaturation->SetValue(out.s * 100, false);
		pBrightness->SetValue(out.v * 100, false);
		
		bReset = true;
	}
	
	char szRedHex[3];
	Q_snprintf( szRedHex, sizeof(szRedHex), of_color_r.GetInt() > 15 ? "%X" : "0%X", of_color_r.GetInt() );
	
	char szGreenHex[3];
	Q_snprintf( szGreenHex, sizeof(szGreenHex), of_color_g.GetInt() > 15 ? "%X" : "0%X", of_color_g.GetInt() );

	char szBlueHex[3];
	Q_snprintf( szBlueHex, sizeof(szBlueHex), of_color_b.GetInt() > 15 ? "%X" : "0%X", of_color_b.GetInt() );	

	pHexEntry->SetText(VarArgs("#%s%s%s", szRedHex, szGreenHex, szBlueHex ));
	
	iCurrRed = of_color_r.GetInt();
	iCurrGreen = of_color_g.GetInt();
	iCurrBlue = of_color_b.GetInt();
}

void CTFColorPanel::PaintBackground()
{
	if( iCurrRed != of_color_r.GetInt() ||
	iCurrGreen != of_color_g.GetInt() ||
	iCurrBlue != of_color_b.GetInt() )
		bReset = false;
	
	if( !bReset )
		bRGBOn = !of_use_rgb.GetBool();

	if( bRGBOn != of_use_rgb.GetBool() )
	{	
		RecalculateColorValues();
	}

	if( !of_use_rgb.GetBool() )
	{
		if( !pHue )
			return;

		int wide, tall;
		pHue->GetSize( wide, tall );
		int x,y;
		pHue->GetPos( x, y );

		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;
		
		for( int i = 0; i < 6; i++ )
		{
			int lastwide = x + ( wide * ( ( 1.0f / 6.0f ) * (i) ) );
			int curwide = x + ( wide * ( ( 1.0f / 6.0f ) * (1 + i) ) );

			vgui::surface()->DrawSetColor( iFade[i] );
			vgui::surface()->DrawFilledRectFade( lastwide, y, curwide , y + tall, 255, 0, true );
			
			vgui::surface()->DrawSetColor( iFade[i+1] );
			vgui::surface()->DrawFilledRectFade( lastwide, y, curwide , y + tall, 0, 255, true );
		}

		if( !pSaturation )
			return;

		pSaturation->GetSize( wide, tall );
		pSaturation->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( cHueBnoS );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );
		
		vgui::surface()->DrawSetColor( cHueB );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 55, 255, true );
		
		if( !pBrightness )
			return;

		pBrightness->GetSize( wide, tall );
		pBrightness->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( 0, 0, 0, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( cHueS );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
	}
	else
	{
		int wide, tall;
		int x,y;
		
		int iRed = of_color_r.GetInt();
		int iGreen = of_color_g.GetInt();
		int iBlue = of_color_b.GetInt();

		if( !pRed )
			return;

		pRed->GetSize( wide, tall );
		pRed->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( 0, iGreen, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( 255, iGreen, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
		
		if( !pGreen )
			return;

		pGreen->GetSize( wide, tall );
		pGreen->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;
		
		vgui::surface()->DrawSetColor( Color( iRed, 0, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( iRed, 255, iBlue, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
		
		if( !pBlue )
			return;

		pBlue->GetSize( wide, tall );
		pBlue->GetPos( x, y );
		wide -= 31;
		x += 7;
		y += 5;
		tall -= 10;

		vgui::surface()->DrawSetColor( Color( iRed, iGreen, 0, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 255, 0, true );

		vgui::surface()->DrawSetColor( Color( iRed, iGreen, 255, 255 ) );
		vgui::surface()->DrawFilledRectFade( x, y, x + wide , y + tall, 0, 255, true );
	}
}

void CTFColorPanel::OnThink()
{
	BaseClass::OnThink();
}

void CTFColorPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFColorPanel::OnTextChanged(Panel *panel)
{
	if( !bReset )
		return;

    if( panel == pHueEntry )
    {
        char buf[64];
        pHueEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0 && iValue <= 360)
        {
            pHue->SetValue(iValue, false);
        }
    }
    else if( panel == pSaturationEntry )
    {
        char buf[64];
        pSaturationEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 100)
        {
            pSaturation->SetValue(iValue, false);
        }
    }
    else if( panel == pBrightnessEntry )
    {
        char buf[64];
        pBrightnessEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 100)
        {
            pBrightness->SetValue(iValue, false);
        }
    }
	else if( panel == pRedEntry )
    {
        char buf[64];
        pRedEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0.0  && iValue <= 255)
        {
            pRed->SetValue(iValue, false);
        }
    }
    else if( panel == pGreenEntry )
    {
        char buf[64];
        pGreenEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0 && iValue <= 255)
        {
            pGreen->SetValue(iValue, false);
        }
    }
    else if( panel == pBlueEntry )
    {
        char buf[64];
        pBlueEntry->GetText(buf, 64);

        int iValue = atoi(buf);
        if (iValue >= 0 && iValue <= 255)
        {
            pBlue->SetValue(iValue, false);
        }
    }
	else if( panel == pHexEntry )
	{
		bUpdateHexValue = false;

		int i = 0;
        char buf[64];
        pHexEntry->GetText(buf, 64);
		strupr(buf);
		if( buf[0] == '#' )
			i++;
		
		char szRedVal[2];
		szRedVal[0] = buf[i];
		szRedVal[1] = buf[i+1];
		
		char szGreenVal[2];
		szGreenVal[0] = buf[i+2];
		szGreenVal[1] = buf[i+3];	
		
		char szBlueVal[2];
		szBlueVal[0] = buf[i+4];
		szBlueVal[1] = buf[i+5];	
		
		int iRed = -1;
		sscanf(szRedVal, "%X", &iRed);

		int iGreen = -1;
		sscanf(szGreenVal, "%X", &iGreen);

		int iBlue = -1;
		sscanf(szBlueVal, "%X", &iBlue);
		
		if( of_use_rgb.GetBool() )
		{
			pRed->SetValue(iRed, false);
			OnControlModified( pRed );
			
			pGreen->SetValue(iGreen, false);
			OnControlModified( pGreen );
			
			pBlue->SetValue(iBlue, false);
			OnControlModified( pBlue );	
		}
		else
		{
			rgb in;
			in.r = iRed ? (float)iRed / 255.0f : 0;
			in.g = iGreen ? (float)iGreen / 255.0f : 0;
			in.b = iBlue ? (float)iBlue / 255.0f : 0;
			hsv out = rgb2hsv( in );
			
			pHue->SetValue(out.h, false);
			OnControlModified( pHue );
			
			pSaturation->SetValue(out.s * 100, false);
			OnControlModified( pSaturation );
			
			pBrightness->SetValue(out.v * 100, false);
			OnControlModified( pBrightness );	
		}
		
		bUpdateHexValue = true;
	}
}

void CTFColorPanel::OnControlModified(Panel *panel)
{
    if( panel == pHue )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pHue->GetValue());
		pHueEntry->SetText(buf);
    }
	else if( panel == pSaturation )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pSaturation->GetValue());
		pSaturationEntry->SetText(buf);
    }
	else if( panel == pBrightness )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pBrightness->GetValue());
		pBrightnessEntry->SetText(buf);
    }
	else if( panel == pRed )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pRed->GetValue());
		pRedEntry->SetText(buf);
    }
	else if( panel == pGreen )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pGreen->GetValue());
		pGreenEntry->SetText(buf);
    }
	else if( panel == pBlue )
    {
		char buf[64];
		Q_snprintf(buf, sizeof( buf ), "%d", pBlue->GetValue());
		pBlueEntry->SetText(buf);
    }
}

CTFColorSlider::CTFColorSlider(Panel *parent, const char *panelName) : Slider(parent, panelName)
{
	pParent = dynamic_cast<CTFColorPanel*>(parent);
	AddActionSignalTarget( this );
	iSliderTextureID = -1;
}

void CTFColorSlider::SetValue(int value, bool bTriggerChangeMessage)
{
	BaseClass::SetValue(value, bTriggerChangeMessage);
	
	if( bTriggerChangeMessage )
		PostActionSignal(new KeyValues("ControlModified"));
	
	bool bUpdateHex = true;
	if( GetParent() )
	{
		CTFColorPanel *pParent = dynamic_cast<CTFColorPanel*>(GetParent());
		bUpdateHex = pParent->ShouldUpdateHex();		
	}
	
	if( pParent )
	{
		pParent->OnColorChanged(bTriggerChangeMessage, bUpdateHex);
	}
}

void CTFColorSlider::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	SetThumbWidth(15);
	SetDragOnRepositionNob( true );

	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( scheme );
	SetBorder( pScheme->GetBorder( inResourceData->GetString("border") ));

	iSliderWidth = inResourceData->GetInt("thumbwidth", 0);
}

//-----------------------------------------------------------------------------
// Purpose: Draw the nob part of the slider.
//-----------------------------------------------------------------------------
void CTFColorSlider::DrawNob()
{
	// horizontal nob
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );
	Color col = GetFgColor();
#ifdef _X360
	if(HasFocus())
	{
		col = m_DepressedBgColor;
	}
#endif
	surface()->DrawSetColor(Color( 255, 255, 255, 255 ));

	int iPanelWide, iPanelTall;
	int iPanelX, iPanelY;
	GetPos( iPanelX, iPanelY );
	GetSize( iPanelWide, iPanelTall );
//	int nobheight = iPanelTall;

	// Set up the image ID's if they've somehow gone bad.
	if( iSliderTextureID == -1 )
		iSliderTextureID = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( iSliderTextureID, "vgui/loadout/color_slider", true, false );	
	
	vgui::surface()->DrawSetTexture(iSliderTextureID);
	
//	x, y, x + wide , y + tall
	
	surface()->DrawTexturedRect(
		_nobPos[0], 
		0, 
		_nobPos[0] + iSliderWidth, 
		iPanelTall);
	/* border
	if (_sliderBorder)
	{
		_sliderBorder->Paint(
			_nobPos[0], 
			y + tall / 2 - nobheight / 2, 
			_nobPos[1], 
			y + tall / 2 + nobheight / 2);
	}*/
}

void CTFColorSlider::PaintBackground()
{
	BaseClass::BaseClass::PaintBackground();
}