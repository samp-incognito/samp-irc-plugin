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

#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>

namespace Data
{
	enum Callbacks
	{
		OnConnect,
		OnDisconnect,
		OnConnectAttempt,
		OnConnectAttemptFail,
		OnJoinChannel,
		OnLeaveChannel,
		OnInvitedToChannel,
		OnKickedFromChannel,
		OnUserDisconnect,
		OnUserJoinChannel,
		OnUserLeaveChannel,
		OnUserKickedFromChannel,
		OnUserNickChange,
		OnUserSetChannelMode,
		OnUserSetChannelTopic,
		OnUserSay,
		OnUserNotice,
		OnUserRequestCTCP,
		OnUserReplyCTCP,
		OnReceiveNumeric,
		OnReceiveRaw
	};

	enum Settings
	{
		ConnectAttempts,
		ConnectDelay,
		ConnectTimeout,
		ReceiveTimeout,
		Respawn
	};

	struct Message
	{
		std::vector<int> array;
		std::vector<std::string> buffer;
	};
}

#endif
