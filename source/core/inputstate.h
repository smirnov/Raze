#pragma once

#include <stdint.h>
#include "printf.h"
#include "c_dispatch.h" 
#include "tarray.h"
#include "scancodes.h"
#include "c_bind.h"
#include "c_buttons.h"
#include "d_event.h"
#include "m_joy.h"
#include "gamecvars.h"
#include "packet.h"
#include "vectors.h"

class InputState
{
	uint8_t KeyStatus[NUM_KEYS];
	bool AnyKeyStatus;
	FVector2  g_mousePos;

public:

	bool ShiftPressed()
	{
		return KeyStatus[sc_LeftShift] || KeyStatus[sc_RightShift];
	}

	void AddEvent(const event_t* ev);

	void MouseAddToPos(float x, float y)
	{
		g_mousePos.X += x;
		g_mousePos.Y += y;
	}

	void GetMouseDelta(FVector2& gameinput)
	{
		gameinput = g_mousePos;
		g_mousePos.Zero();
	}

	void ClearAllInput();
	bool CheckAllInput()
	{
		bool res = AnyKeyStatus;
		AnyKeyStatus = false;
		return res;
	}
};

extern InputState inputState;

int32_t handleevents(void);

enum GameFunction_t
{
	gamefunc_Move_Forward,		//
	gamefunc_Move_Backward,		//
	gamefunc_Turn_Left,			//
	gamefunc_Turn_Right,		//
	gamefunc_Strafe,			//
	gamefunc_Fire,
	gamefunc_Open,
	gamefunc_Run,
	gamefunc_Alt_Fire,
	gamefunc_Jump,
	gamefunc_Crouch,
	gamefunc_Look_Up,
	gamefunc_Look_Down,
	gamefunc_Look_Left,
	gamefunc_Look_Right,
	gamefunc_Strafe_Left,		//
	gamefunc_Strafe_Right,		//
	gamefunc_Aim_Up,
	gamefunc_Aim_Down,
	gamefunc_Shrink_Screen, // Automap only
	gamefunc_Enlarge_Screen, // Automap only
	gamefunc_Mouse_Aiming,
	gamefunc_Dpad_Select,
	gamefunc_Dpad_Aiming,
	gamefunc_Toggle_Crouch,
	gamefunc_Quick_Kick,
	gamefunc_AM_PanLeft,
	gamefunc_AM_PanRight,
	gamefunc_AM_PanUp,
	gamefunc_AM_PanDown,
	NUM_ACTIONS
};

void SetupGameButtons();
