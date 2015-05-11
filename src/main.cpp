/*
 * Copyright (C) 2014 Incognito
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

#include "main.h"

#include "core.h"
#include "natives.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <sdk/plugin.h>

#include <queue>
#include <set>

logprintf_t logprintf;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void **ppPluginData)
{
	core.reset(new Core);
	pAMXFunctions = ppPluginData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t)ppPluginData[PLUGIN_DATA_LOGPRINTF];
	logprintf("\n\n*** IRC Plugin v%s by Incognito loaded ***\n", PLUGIN_VERSION);
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	core.reset();
	logprintf("\n\n*** IRC Plugin v%s by Incognito unloaded ***\n", PLUGIN_VERSION);
}

AMX_NATIVE_INFO natives[] =
{
	{ "IRC_Connect", Natives::IRC_Connect },
	{ "IRC_Quit", Natives::IRC_Quit },
	{ "IRC_JoinChannel", Natives::IRC_JoinChannel },
	{ "IRC_PartChannel", Natives::IRC_PartChannel },
	{ "IRC_ChangeNick", Natives::IRC_ChangeNick },
	{ "IRC_SetMode", Natives::IRC_SetMode },
	{ "IRC_Say", Natives::IRC_Say },
	{ "IRC_Notice", Natives::IRC_Notice },
	{ "IRC_IsUserOnChannel", Natives::IRC_IsUserOnChannel },
	{ "IRC_InviteUser", Natives::IRC_InviteUser },
	{ "IRC_KickUser", Natives::IRC_KickUser },
	{ "IRC_GetUserChannelMode", Natives::IRC_GetUserChannelMode },
	{ "IRC_GetChannelUserList", Natives::IRC_GetChannelUserList },
	{ "IRC_SetChannelTopic", Natives::IRC_SetChannelTopic },
	{ "IRC_RequestCTCP", Natives::IRC_RequestCTCP },
	{ "IRC_ReplyCTCP", Natives::IRC_ReplyCTCP },
	{ "IRC_SendRaw", Natives::IRC_SendRaw },
	{ "IRC_CreateGroup", Natives::IRC_CreateGroup },
	{ "IRC_DestroyGroup", Natives::IRC_DestroyGroup },
	{ "IRC_AddToGroup", Natives::IRC_AddToGroup },
	{ "IRC_RemoveFromGroup", Natives::IRC_RemoveFromGroup },
	{ "IRC_GroupSay", Natives::IRC_GroupSay },
	{ "IRC_GroupNotice", Natives::IRC_GroupNotice },
	{ "IRC_SetIntData", Natives::IRC_SetIntData },
	{ 0, 0 }
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *amx)
{
	core->interfaces.insert(amx);
	return amx_Register(amx, natives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *amx)
{
	core->interfaces.erase(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	if (!core->messages.empty())
	{
		boost::mutex::scoped_lock lock(core->mutex);
		Data::Message message(core->messages.front());
		core->messages.pop();
		lock.unlock();
		for (std::set<AMX*>::iterator a = core->interfaces.begin(); a != core->interfaces.end(); ++a)
		{
			cell amxAddresses[5] = { 0 };
			int amxIndex = 0;
			switch (message.array.at(0))
			{
				case Data::OnConnect:
				{
					if (!amx_FindPublic(*a, "IRC_OnConnect", &amxIndex))
					{
						amx_Push(*a, message.array.at(1));
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_Push(*a, message.array.at(2));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
					}
					break;
				}
				case Data::OnDisconnect:
				{
					if (!amx_FindPublic(*a, "IRC_OnDisconnect", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_Push(*a, message.array.at(2));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
					}
					break;
				}
				case Data::OnConnectAttempt:
				{
					if (!amx_FindPublic(*a, "IRC_OnConnectAttempt", &amxIndex))
					{
						amx_Push(*a, message.array.at(1));
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_Push(*a, message.array.at(2));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
					}
					break;
				}
				case Data::OnConnectAttemptFail:
				{
					if (!amx_FindPublic(*a, "IRC_OnConnectAttemptFail", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_Push(*a, message.array.at(2));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
					}
					break;
				}
				case Data::OnJoinChannel:
				{
					if (!amx_FindPublic(*a, "IRC_OnJoinChannel", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
					}
					break;
				}
				case Data::OnLeaveChannel:
				{
					if (!amx_FindPublic(*a, "IRC_OnLeaveChannel", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
					}
					break;
				}
				case Data::OnInvitedToChannel:
				{
					if (!amx_FindPublic(*a, "IRC_OnInvitedToChannel", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
					}
					break;
				}
				case Data::OnKickedFromChannel:
				{
					if (!amx_FindPublic(*a, "IRC_OnKickedFromChannel", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[3], NULL, message.buffer.at(3).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
						amx_Release(*a, amxAddresses[3]);
					}
					break;
				}
				case Data::OnUserDisconnect:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserDisconnect", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
					}
					break;
				}
				case Data::OnUserJoinChannel:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserJoinChannel", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
					}
					break;
				}
				case Data::OnUserLeaveChannel:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserLeaveChannel", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[3], NULL, message.buffer.at(3).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
						amx_Release(*a, amxAddresses[3]);
					}
					break;
				}
				case Data::OnUserKickedFromChannel:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserKickedFromChannel", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[3], NULL, message.buffer.at(3).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[4], NULL, message.buffer.at(4).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
						amx_Release(*a, amxAddresses[3]);
						amx_Release(*a, amxAddresses[4]);
					}
					break;
				}
				case Data::OnUserNickChange:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserNickChange", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
					}
					break;
				}
				case Data::OnUserSetChannelMode:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserSetChannelMode", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[3], NULL, message.buffer.at(3).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
						amx_Release(*a, amxAddresses[3]);
					}
					break;
				}
				case Data::OnUserSetChannelTopic:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserSetChannelTopic", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[3], NULL, message.buffer.at(3).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
						amx_Release(*a, amxAddresses[3]);
					}
					break;
				}
				case Data::OnUserSay:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserSay", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[3], NULL, message.buffer.at(3).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
						amx_Release(*a, amxAddresses[3]);
					}
					break;
				}
				case Data::OnUserNotice:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserNotice", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[3], NULL, message.buffer.at(3).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
						amx_Release(*a, amxAddresses[3]);
					}
					break;
				}
				case Data::OnUserRequestCTCP:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserRequestCTCP", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
					}
					break;
				}
				case Data::OnUserReplyCTCP:
				{
					if (!amx_FindPublic(*a, "IRC_OnUserReplyCTCP", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[1], NULL, message.buffer.at(1).c_str(), 0, 0);
						amx_PushString(*a, &amxAddresses[2], NULL, message.buffer.at(2).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
						amx_Release(*a, amxAddresses[1]);
						amx_Release(*a, amxAddresses[2]);
					}
					break;
				}
				case Data::OnReceiveNumeric:
				{
					if (!amx_FindPublic(*a, "IRC_OnReceiveNumeric", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Push(*a, message.array.at(2));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
					}
					break;
				}
				case Data::OnReceiveRaw:
				{
					if (!amx_FindPublic(*a, "IRC_OnReceiveRaw", &amxIndex))
					{
						amx_PushString(*a, &amxAddresses[0], NULL, message.buffer.at(0).c_str(), 0, 0);
						amx_Push(*a, message.array.at(1));
						amx_Exec(*a, NULL, amxIndex);
						amx_Release(*a, amxAddresses[0]);
					}
					break;
				}
			}
		}
	}
}
