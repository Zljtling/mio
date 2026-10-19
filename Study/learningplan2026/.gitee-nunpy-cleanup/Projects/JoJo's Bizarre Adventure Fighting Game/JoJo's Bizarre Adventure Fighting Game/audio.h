#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "player_id.h"
#include <Windows.h>
#include <tchar.h>
#include <mmsystem.h>
#include <initializer_list>

#pragma comment(lib, "Winmm.lib")

// 两名玩家各自使用一个设备别名；互换角色或同时选择同一角色也不会互相重播。
inline void get_player_sound_alias(PlayerID id, LPCTSTR sound, TCHAR (&alias)[96])
{
	_stprintf_s(alias, _T("%s_P%d"), sound, id == PlayerID::P1 ? 1 : 2);
}

inline bool load_player_sound(LPCTSTR sound, LPCTSTR path)
{
	bool loaded = true;
	for (PlayerID id : { PlayerID::P1, PlayerID::P2 })
	{
		TCHAR alias[96], command[320];
		get_player_sound_alias(id, sound, alias);
		_stprintf_s(command, _T("open %s type waveaudio alias %s"), path, alias);
		loaded = mciSendString(command, nullptr, 0, nullptr) == 0 && loaded;
	}
	return loaded;
}

inline bool play_player_sound(PlayerID id, LPCTSTR sound, int from = 0, int duration = -1, bool restart = true)
{
	TCHAR alias[96], command[192], mode[32] = {};
	get_player_sound_alias(id, sound, alias);
	if (!restart)
	{
		_stprintf_s(command, _T("status %s mode"), alias);
		if (mciSendString(command, mode, 32, nullptr) == 0 && _tcscmp(mode, _T("playing")) == 0)
			return false; // 高频命中保留正在播放的尾音，避免不断切回首个采样
	}
	if (duration >= 0)
		_stprintf_s(command, _T("play %s from %d to %d"), alias, from, from + duration);
	else
		_stprintf_s(command, _T("play %s from %d"), alias, from);
	return mciSendString(command, nullptr, 0, nullptr) == 0;
}

inline void stop_player_sound(PlayerID id, LPCTSTR sound)
{
	TCHAR alias[96], command[128];
	get_player_sound_alias(id, sound, alias);
	_stprintf_s(command, _T("stop %s"), alias);
	mciSendString(command, nullptr, 0, nullptr);
}

// 同一份列表用于停止和时停暂停，避免漏掉某个玩家的技能尾音。
inline const LPCTSTR* get_player_attack_sounds()
{
	static const LPCTSTR sounds[] = { _T("jotaro_charged_punch"), _T("jotaro_ora_barrage"),
		_T("kakyoin_quill_shoot"), _T("kakyoin_quill_hit"), _T("kakyoin_emerald_cast"),
		_T("kakyoin_emerald_burst"), _T("kakyoin_emerald_cannon_cast"),
		_T("kakyoin_emerald_cannon_loop"), _T("kakyoin_emerald_cannon_hit") };
	return sounds;
}

inline void stop_player_attack_sounds(PlayerID id)
{
	for (int i = 0; i < 9; ++i) stop_player_sound(id, get_player_attack_sounds()[i]);
}

inline unsigned int pause_player_attack_sounds(PlayerID id)
{
	unsigned int mask = 0;
	for (int i = 0; i < 9; ++i)
	{
		TCHAR alias[96], command[128], mode[32] = {};
		get_player_sound_alias(id, get_player_attack_sounds()[i], alias);
		_stprintf_s(command, _T("status %s mode"), alias);
		if (mciSendString(command, mode, 32, nullptr) != 0 || _tcscmp(mode, _T("playing")) != 0) continue;
		_stprintf_s(command, _T("pause %s"), alias);
		if (mciSendString(command, nullptr, 0, nullptr) == 0) mask |= 1u << i;
	}
	return mask;
}

inline void resume_player_attack_sounds(PlayerID id, unsigned int& mask)
{
	for (int i = 0; i < 9; ++i)
	{
		if ((mask & (1u << i)) == 0) continue;
		TCHAR alias[96], command[128], mode[32] = {};
		get_player_sound_alias(id, get_player_attack_sounds()[i], alias);
		_stprintf_s(command, _T("status %s mode"), alias);
		if (mciSendString(command, mode, 32, nullptr) != 0 || _tcscmp(mode, _T("paused")) != 0) continue;
		_stprintf_s(command, _T("resume %s"), alias);
		mciSendString(command, nullptr, 0, nullptr); // 保留暂停前的位置和原先的播放结束点
	}
	mask = 0;
}

#endif // !_AUDIO_H_
