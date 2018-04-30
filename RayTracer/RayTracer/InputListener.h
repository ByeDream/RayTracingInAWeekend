#pragma once

#include <map>

class InputListener
{
public:
	InputListener() = default;
	~InputListener() = default;

	void								RegisterKey(UINT8 keyCode);

	BOOL								WhenPressKey(UINT8 keyCode);
	BOOL								WhenReleaseKey(UINT8 keyCode);
	BOOL								WhenHoldKey(UINT8 keyCode);

	void								NotifyKeyDown(UINT8 keyCode);
	void								NotifyKeyUp(UINT8 keyCode);
	void								Clear();
private:
	struct KeyState
	{
		BOOL m_keyDown{ FALSE };
		BOOL m_press{ FALSE };
		BOOL m_release{ FALSE };
		BOOL m_hold{ FALSE };
	};

	std::map<UINT8, KeyState>				m_keyStatus;
};