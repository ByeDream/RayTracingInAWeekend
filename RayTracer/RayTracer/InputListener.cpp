#include "stdafx.h"
#include "InputListener.h"

using namespace std;

void InputListener::RegisterKey(UINT8 keyCode)
{
	m_keyStatus[keyCode] = KeyState{};
}
	 
BOOL InputListener::WhenPressKey(UINT8 keyCode)
{
	BOOL ret = FALSE;
	auto keyState = m_keyStatus.find(keyCode);
	if (keyState != m_keyStatus.end())
	{
		ret = keyState->second.m_press;
	}
	return ret;
}

BOOL InputListener::WhenReleaseKey(UINT8 keyCode)
{
	BOOL ret = FALSE;
	auto keyState = m_keyStatus.find(keyCode);
	if (keyState != m_keyStatus.end())
	{
		ret = keyState->second.m_release;
	}
	return ret;
}

BOOL InputListener::WhenHoldKey(UINT8 keyCode)
{
	BOOL ret = FALSE;
	auto keyState = m_keyStatus.find(keyCode);
	if (keyState != m_keyStatus.end())
	{
		ret = keyState->second.m_hold;
	}
	return ret;
}
	 
void InputListener::NotifyKeyDown(UINT8 keyCode)
{
	auto keyState = m_keyStatus.find(keyCode);
	if (keyState != m_keyStatus.end())
	{
		keyState->second.m_press = !keyState->second.m_keyDown;
		keyState->second.m_hold = keyState->second.m_keyDown;
		keyState->second.m_release = FALSE;

		keyState->second.m_keyDown = TRUE;
	}
}

void InputListener::NotifyKeyUp(UINT8 keyCode)
{
	auto keyState = m_keyStatus.find(keyCode);
	if (keyState != m_keyStatus.end())
	{

		keyState->second.m_press = FALSE;
		keyState->second.m_hold = FALSE;
		keyState->second.m_release = keyState->second.m_keyDown;

		keyState->second.m_keyDown = FALSE;
	}
}

void InputListener::Clear()
{
	for (auto keyState = m_keyStatus.begin(); keyState != m_keyStatus.end(); keyState++)
	{
		keyState->second.m_press = FALSE;
		keyState->second.m_release = FALSE;
	}
}