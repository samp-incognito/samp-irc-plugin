/*
 * Copyright (C) 2016 Incognito
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <a_samp>

// Enum

enum
{
	E_IRC_CONNECT_ATTEMPTS,
	E_IRC_CONNECT_DELAY,
	E_IRC_CONNECT_TIMEOUT,
	E_IRC_RECEIVE_TIMEOUT,
	E_IRC_RESPAWN
}

// Natives

native IRC_Connect(const server[], port, const nickname[], const realname[], const username[], bool:ssl = false, const localip[] = "", const serverpassword[] = "");
native IRC_Quit(botid, const message[] = "");
native IRC_JoinChannel(botid, const channel[], const key[] = "");
native IRC_PartChannel(botid, const channel[], const message[] = "");
native IRC_ChangeNick(botid, const nick[]);
native IRC_SetMode(botid, const target[], const mode[]);
native IRC_Say(botid, const target[], const message[]);
native IRC_Notice(botid, const target[], const message[]);
native IRC_IsUserOnChannel(botid, const channel[], const user[]);
native IRC_InviteUser(botid, const channel[], const user[]);
native IRC_KickUser(botid, const channel[], const user[], const message[] = "");
native IRC_GetUserChannelMode(botid, const channel[], const user[], dest[]);
native IRC_GetChannelUserList(botid, const channel[], dest[], maxlength = sizeof dest);
native IRC_SetChannelTopic(botid, const channel[], const topic[]);
native IRC_RequestCTCP(botid, const user[], const message[]);
native IRC_ReplyCTCP(botid, const user[], const message[]);
native IRC_SendRaw(botid, const message[]);
native IRC_CreateGroup();
native IRC_DestroyGroup(groupid);
native IRC_AddToGroup(groupid, botid);
native IRC_RemoveFromGroup(groupid, botid);
native IRC_GroupSay(groupid, const target[], const message[]);
native IRC_GroupNotice(groupid, const target[], const message[]);
native IRC_SetIntData(botid, data, value);

// Callbacks

forward IRC_OnConnect(botid, const ip[], port);
forward IRC_OnDisconnect(botid, const ip[], port, const reason[]);
forward IRC_OnConnectAttempt(botid, const ip[], port);
forward IRC_OnConnectAttemptFail(botid, const ip[], port, const reason[]);
forward IRC_OnJoinChannel(botid, const channel[]);
forward IRC_OnLeaveChannel(botid, const channel[], const message[]);
forward IRC_OnInvitedToChannel(botid, const channel[], const invitinguser[], const invitinghost[]);
forward IRC_OnKickedFromChannel(botid, const channel[], const oppeduser[], const oppedhost[], const message[]);
forward IRC_OnUserDisconnect(botid, const user[], const host[], const message[]);
forward IRC_OnUserJoinChannel(botid, const channel[], const user[], const host[]);
forward IRC_OnUserLeaveChannel(botid, const channel[], const user[], const host[], const message[]);
forward IRC_OnUserKickedFromChannel(botid, const channel[], const kickeduser[], const oppeduser[], const oppedhost[], const message[]);
forward IRC_OnUserNickChange(botid, const oldnick[], const newnick[], const host[]);
forward IRC_OnUserSetChannelMode(botid, const channel[], const user[], const host[], const mode[]);
forward IRC_OnUserSetChannelTopic(botid, const channel[], const user[], const host[], const topic[]);
forward IRC_OnUserSay(botid, const recipient[], const user[], const host[], const message[]);
forward IRC_OnUserNotice(botid, const recipient[], const user[], const host[], const message[]);
forward IRC_OnUserRequestCTCP(botid, const user[], const host[], const message[]);
forward IRC_OnUserReplyCTCP(botid, const user[], const host[], const message[]);
forward IRC_OnReceiveNumeric(botid, numeric, const message[]);
forward IRC_OnReceiveRaw(botid, const message[]);

// Helper Functions

stock IRC_IsVoice(botid, const channel[], const user[])
{
	new mode[2];
	IRC_GetUserChannelMode(botid, channel, user, mode);
	switch (mode[0])
	{
		case '+', '%', '@', '&', '!', '*', '~', '.':
		{
			return 1;
		}
	}
	return 0;
}

stock IRC_IsHalfop(botid, const channel[], const user[])
{
	new mode[2];
	IRC_GetUserChannelMode(botid, channel, user, mode);
	switch (mode[0])
	{
		case '%', '@', '&', '!', '*', '~', '.':
		{
			return 1;
	
		}
	}
	return 0;
}

stock IRC_IsOp(botid, const channel[], const user[])
{
	new mode[2];
	IRC_GetUserChannelMode(botid, channel, user, mode);
	switch (mode[0])
	{
		case '@', '&', '!', '*', '~', '.':
		{
			return 1;
	
		}
	}
	return 0;
}

stock IRC_IsAdmin(botid, const channel[], const user[])
{
	new mode[2];
	IRC_GetUserChannelMode(botid, channel, user, mode);
	switch (mode[0])
	{
		case '&', '!', '*', '~', '.':
		{
			return 1;
	
		}
	}
	return 0;
}

stock IRC_IsOwner(botid, const channel[], const user[])
{
	new mode[2];
	IRC_GetUserChannelMode(botid, channel, user, mode);
	switch (mode[0])
	{
		case '~', '.':
		{
			return 1;
	
		}
	}
	return 0;
}

// Channel Command System

#define CHANNEL_PREFIX '#'
#define COMMAND_PREFIX '!'

#define IRCCMD:%1(%2) \
	forward irccmd_%1(%2); \
	public irccmd_%1(%2)

#define irccmd(%1,%2,%3,%4,%5,%6) \
	IRCCMD:%1(%2, %3, %4, %5, %6)

#if !defined isnull
	#define isnull(%1) \
		((!(%1[0])) || (((%1[0]) == '\1') && (!(%1[1]))))
#endif

static bool:IRC_g_OUS = false;

public OnFilterScriptInit()
{
	IRC_g_OUS = funcidx("IRC_OUS") != -1;
	if (funcidx("IRC_OnFilterScriptInit") != -1)
	{
		return CallLocalFunction("IRC_OnFilterScriptInit", "");
	}
	return 1;
}

#if defined _ALS_OnFilterScriptInit
	#undef OnFilterScriptInit
#else
	#define _ALS_OnFilterScriptInit
#endif
#define OnFilterScriptInit IRC_OnFilterScriptInit

forward IRC_OnFilterScriptInit();

public OnGameModeInit()
{
	IRC_g_OUS = funcidx("IRC_OUS") != -1;
	if (funcidx("IRC_OnGameModeInit") != -1)
	{
		return CallLocalFunction("IRC_OnGameModeInit", "");
	}
	return 1;
}

#if defined _ALS_OnGameModeInit
	#undef OnGameModeInit
#else
	#define _ALS_OnGameModeInit
#endif
#define OnGameModeInit IRC_OnGameModeInit

forward IRC_OnGameModeInit();

public IRC_OnUserSay(botid, const recipient[], const user[], const host[], const message[])
{
	if (recipient[0] == CHANNEL_PREFIX && message[0] == COMMAND_PREFIX)
	{
		new function[32], pos = 0;
		while (message[++pos] > ' ')
		{
			function[pos - 1] = tolower(message[pos]);
			if (pos > (sizeof(function) - 1))
			{
				break;
			}
		} 
		format(function, sizeof(function), "irccmd_%s", function);
		while (message[pos] == ' ')
		{
			pos++;
		}
		if (!message[pos])
		{
			CallLocalFunction(function, "dssss", botid, recipient, user, host, "\1");
		}
		else
		{
			CallLocalFunction(function, "dssss", botid, recipient, user, host, message[pos]);
		}
	}
	if (IRC_g_OUS)
	{
		return CallLocalFunction("IRC_OUS", "dssss", botid, recipient, user, host, message);
	}
	return 1;
}

#if defined _ALS_IRC_OnUserSay
	#undef IRC_OnUserSay
#else
	#define _ALS_IRC_OnUserSay
#endif
#define IRC_OnUserSay IRC_OUS

forward IRC_OUS(botid, const recipient[], const user[], const host[], const message[]);
