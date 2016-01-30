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

#ifndef NATIVES_H
#define NATIVES_H

#include <sdk/plugin.h>

#define CHECK_PARAMS(m, n) \
	if (params[0] != (m * 4)) \
	{ \
		logprintf("*** %s: Expecting %d parameter(s), but found %d", n, m, params[0] / 4); \
		return 0; \
	}

namespace Natives
{
	cell AMX_NATIVE_CALL IRC_Connect(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_Quit(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_JoinChannel(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_PartChannel(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_ChangeNick(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_SetMode(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_Say(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_Notice(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_IsUserOnChannel(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_InviteUser(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_KickUser(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_GetUserChannelMode(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_GetChannelUserList(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_SetChannelTopic(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_RequestCTCP(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_ReplyCTCP(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_SendRaw(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_CreateGroup(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_DestroyGroup(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_AddToGroup(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_RemoveFromGroup(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_GroupSay(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_GroupNotice(AMX *amx, cell *params);
	cell AMX_NATIVE_CALL IRC_SetIntData(AMX *amx, cell *params);
};

#endif
