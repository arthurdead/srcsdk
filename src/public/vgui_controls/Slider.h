//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SLIDER_H
#define SLIDER_H

#pragma once

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Labeled horizontal slider
//-----------------------------------------------------------------------------
class BaseSlider : public Panel
{
	DECLARE_CLASS_SIMPLE( BaseSlider, Panel );
public:
	BaseSlider(Panel *parent, const char *panelName);
	virtual ~BaseSlider() {}

	virtual void SetButtonOffset(int buttonOffset);
	virtual void OnCursorMoved(int x, int y) = 0;
	virtual void OnMousePressed(MouseCode code) = 0;
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void SetTickCaptions(const wchar_t *left, const wchar_t *right);
	virtual void SetTickCaptions(const char *left, const char *right);
	virtual void SetNumTicks(int ticks);
	virtual void SetThumbWidth( int width );
	virtual void SetInverted( bool bInverted );
	
	// If you click on the slider outside of the nob, the nob jumps
	// to the click position, and if this setting is enabled, the nob
	// is then draggable from the new position until the mouse is released
	virtual void SetDragOnRepositionNob( bool state );
	virtual bool IsDragOnRepositionNob() const;

	// Get if the slider nob is being dragged by user, usually the application
	// should refuse from forcefully setting slider value if it is being dragged
	// by user since the next frame the nob will pop back to mouse position
	virtual bool IsDragged( void ) const;

protected:
	virtual void OnSizeChanged(int wide, int tall);
	virtual void Paint();
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void GetSettings(KeyValues *outResourceData);
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual const char *GetDescription();

	virtual void OnKeyCodeTyped(KeyCode code) = 0;

	virtual void DrawNob() = 0;
	virtual void DrawTicks();
	virtual void DrawTickLabels();

	virtual void GetTrackRect( int &x, int &y, int &w, int &h );

protected:
	virtual void RecomputeNobPosFromValue() = 0;
	virtual void RecomputeValueFromNobPos() = 0;
	
	virtual void SendSliderMovedMessage() = 0;
	virtual void SendSliderDragStartMessage() = 0;
	virtual void SendSliderDragEndMessage() = 0;

	virtual void ClampRange() = 0;

	bool _dragging;
	int _buttonOffset;
	IBorder *_sliderBorder;
	IBorder *_insetBorder;
	float _nobSize;

	TextImage *_leftCaption;
	TextImage *_rightCaption;

	Color m_TickColor;
	Color m_TrackColor;
	Color m_DisabledTextColor1;
	Color m_DisabledTextColor2;

	int		m_nNumTicks;
	bool	m_bIsDragOnRepositionNob : 1;
	bool	m_bUseSubRange : 1;
	bool	m_bInverted : 1;
};

class IntSlider : public BaseSlider
{
	DECLARE_CLASS_SIMPLE( IntSlider, BaseSlider );
public:
	IntSlider(Panel *parent, const char *panelName);
	virtual ~IntSlider() {}

	// interface
	virtual void SetValue(int value, bool bTriggerChangeMessage = true); 
	virtual int  GetValue();
    virtual void SetRange(int min, int max);	 // set to max and min range of rows to display
	virtual void GetRange(int &min, int &max);
	virtual void GetNobPos(int &min, int &max);	// get current Slider position

	// This allows the slider to behave like it's larger than what's actually being drawn
	virtual void SetSliderThumbSubRange( bool bEnable, int nMin = 0, int nMax = 100 );

	virtual int	 EstimateValueAtPos( int localMouseX, int localMouseY );

	virtual void OnCursorMoved(int x, int y);

protected:
	virtual void ApplySettings(KeyValues *inResourceData);

	virtual void RecomputeNobPosFromValue();
	virtual void RecomputeValueFromNobPos();

	virtual void SendSliderMovedMessage();
	virtual void SendSliderDragStartMessage();
	virtual void SendSliderDragEndMessage();

	virtual void DrawNob();

	virtual void ClampRange();

	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnMousePressed(MouseCode code);

	int _nobPos[2];
	int _nobDragStartPos[2];
	int _dragStartPos[2];
	int _range[2];
	int _subrange[ 2 ];
	int _value;		// the position of the Slider, in coordinates as specified by SetRange/SetRangeWindow
};

class FloatSlider : public BaseSlider
{
	DECLARE_CLASS_SIMPLE( FloatSlider, BaseSlider );
public:
	FloatSlider(Panel *parent, const char *panelName);
	virtual ~FloatSlider() {}

	// interface
	virtual void SetValue(float value, bool bTriggerChangeMessage = true); 
	virtual float  GetValue();
    virtual void SetRange(float min, float max);	 // set to max and min range of rows to display
	virtual void GetRange(float &min, float &max);
	virtual void GetNobPos(float &min, float &max);	// get current Slider position

	// This allows the slider to behave like it's larger than what's actually being drawn
	virtual void SetSliderThumbSubRange( bool bEnable, float nMin = 0, float nMax = 100 );

	virtual float	 EstimateValueAtPos( int localMouseX, int localMouseY );

	virtual void OnCursorMoved(int x, int y);

protected:
	virtual void ApplySettings(KeyValues *inResourceData);

	virtual void RecomputeNobPosFromValue();
	virtual void RecomputeValueFromNobPos();

	virtual void SendSliderMovedMessage();
	virtual void SendSliderDragStartMessage();
	virtual void SendSliderDragEndMessage();

	virtual void DrawNob();

	virtual void ClampRange();

	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnMousePressed(MouseCode code);

	float _nobPos[2];
	float _nobDragStartPos[2];
	float _dragStartPos[2];
	float _range[2];
	float _subrange[ 2 ];
	float _value;		// the position of the Slider, in coordinates as specified by SetRange/SetRangeWindow
};

}

#endif // SLIDER_H
