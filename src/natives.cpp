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

#include "natives.h"

#include "client.h"
#include "core.h"
#include "main.h"

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <sdk/plugin.h>

#include <map>
#include <string>

cell AMX_NATIVE_CALL Natives::IRC_Connect(AMX *amx, cell *params)
{
	CHECK_PARAMS(8, "IRC_Connect");
	boost::mutex::scoped_lock lock(core->mutex);
	char *remoteAddress = NULL;
	amx_StrParam(amx, params[1], remoteAddress);
	if (remoteAddress == NULL)
	{
		return 0;
	}
	unsigned short remotePort = static_cast<unsigned short>(params[2]);
	char *nickname = NULL;
	amx_StrParam(amx, params[3], nickname);
	if (nickname == NULL)
	{
		return 0;
	}
	char *realname = NULL;
	amx_StrParam(amx, params[4], realname);
	if (realname == NULL)
	{
		return 0;
	}
	char *username = NULL;
	amx_StrParam(amx, params[5], username);
	if (username == NULL)
	{
		return 0;
	}
	bool ssl = static_cast<int>(params[6]) != 0;
	if (ssl)
	{
		logprintf("*** IRC_Connect: SSL disabled");
		return 0;
	}
	char *localAddress = NULL;
	amx_StrParam(amx, params[7], localAddress);
	int botID = 1;
	for (std::map<int, SharedClient>::iterator c = core->clients.begin(); c != core->clients.end(); ++c)
	{
		if (c->first != botID)
		{
			break;
		}
		++botID;
	}
	char *serverPassword = NULL;
	amx_StrParam(amx, params[8], serverPassword);
	SharedClient client(new Client(core->io_service));
	client->botID = botID;
	client->groupID = 0;
	client->localAddress = (localAddress ? localAddress : "");
	client->nickname = nickname;
	client->realname = realname;
	client->remoteAddress = remoteAddress;
	client->remotePort = remotePort;
	client->serverPassword = (serverPassword ? serverPassword : "");
	client->username = username;
	client->startAsync();
	return static_cast<cell>(botID);
}

cell AMX_NATIVE_CALL Natives::IRC_Quit(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "IRC_Quit");
	boost::mutex::scoped_lock lock(core->mutex);
	char *message = NULL;
	amx_StrParam(amx, params[2], message);
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->quitting = true;
		if (c->second->connected)
		{
			c->second->sendAsync(boost::str(boost::format("QUIT :%1%\r\n") % (message ? message : "")));
			c->second->stopAsync();
		}
		else
		{
			c->second->stopAsync();
		}
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_JoinChannel(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_JoinChannel");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	char *key = NULL;
	amx_StrParam(amx, params[3], key);
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("JOIN %1% %2%\r\n") % channel % (key ? key : "")));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_PartChannel(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_PartChannel");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[3], message);
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("PART %1% :%2%\r\n") % channel % (message ? message : "")));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_ChangeNick(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "IRC_ChangeNick");
	boost::mutex::scoped_lock lock(core->mutex);
	char *nickname = NULL;
	amx_StrParam(amx, params[2], nickname);
	if (nickname == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("NICK %1%\r\n") % nickname));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_SetMode(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_SetMode");
	boost::mutex::scoped_lock lock(core->mutex);
	char *target = NULL;
	amx_StrParam(amx, params[2], target);
	if (target == NULL)
	{
		return 0;
	}
	char *mode = NULL;
	amx_StrParam(amx, params[3], mode);
	if (mode == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("MODE %1% %2%\r\n") % target % mode));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_Say(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_Say");
	boost::mutex::scoped_lock lock(core->mutex);
	char *target = NULL;
	amx_StrParam(amx, params[2], target);
	if (target == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[3], message);
	if (message == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("PRIVMSG %1% :%2%\r\n") % target % message));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_Notice(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_Notice");
	boost::mutex::scoped_lock lock(core->mutex);
	char *target = NULL;
	amx_StrParam(amx, params[2], target);
	if (target == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[3], message);
	if (message == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("NOTICE %1% :%2%\r\n") % target % message));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_IsUserOnChannel(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_IsUserOnChannel");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	char *user = NULL;
	amx_StrParam(amx, params[3], user);
	if (user == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		UserMap::iterator f = c->second->users.find(user);
		if (f != c->second->users.end())
		{
			std::map<std::string, std::string>::iterator g = f->second.find(channel);
			if (g != f->second.end())
			{
				return 1;
			}
		}
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_InviteUser(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_InviteUser");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	char *user = NULL;
	amx_StrParam(amx, params[3], user);
	if (user == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("INVITE %1% %2%\r\n") % user % channel));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_KickUser(AMX *amx, cell *params)
{
	CHECK_PARAMS(4, "IRC_KickUser");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	char *user = NULL;
	amx_StrParam(amx, params[3], user);
	if (user == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[4], message);
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("KICK %1% %2% :%3%\r\n") % channel % user % (message ? message : "")));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_GetUserChannelMode(AMX *amx, cell *params)
{
	CHECK_PARAMS(4, "IRC_GetUserChannelMode");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	char *user = NULL;
	amx_StrParam(amx, params[3], user);
	if (user == NULL)
	{
		return 0;
	}
	std::string mode;
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		UserMap::iterator f = c->second->users.find(user);
		if (f != c->second->users.end())
		{
			std::map<std::string, std::string>::iterator g = f->second.find(channel);
			if (g != f->second.end())
			{
				mode = g->second;
			}
		}
	}
	if (mode.empty())
	{
		mode = "-";
	}
	cell *destination = NULL;
	if (!amx_GetAddr(amx, params[4], &destination))
	{
		amx_SetString(destination, mode.c_str(), 0, 0, mode.length() + 1);
	}
	return 1;
}

cell AMX_NATIVE_CALL Natives::IRC_GetChannelUserList(AMX *amx, cell *params)
{
	CHECK_PARAMS(4, "IRC_GetChannelUserList");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	std::string userList;
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		for (UserMap::iterator u = c->second->users.begin(); u != c->second->users.end(); ++u)
		{
			std::map<std::string, std::string>::iterator f = u->second.find(channel);
			if (f != u->second.end())
			{
				userList += f->second + u->first + " ";
			}
		}
	}
	if (userList.empty())
	{
		userList = "None";
	}
	else
	{
		userList.resize(userList.length() - 1);
	}
	cell *destination = NULL;
	if (!amx_GetAddr(amx, params[3], &destination))
	{
		amx_SetString(destination, userList.c_str(), 0, 0, static_cast<std::size_t>(params[4]));
	}
	return 1;
}

cell AMX_NATIVE_CALL Natives::IRC_SetChannelTopic(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_SetChannelTopic");
	boost::mutex::scoped_lock lock(core->mutex);
	char *channel = NULL;
	amx_StrParam(amx, params[2], channel);
	if (channel == NULL)
	{
		return 0;
	}
	char *topic = NULL;
	amx_StrParam(amx, params[3], topic);
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("TOPIC %1% :%2%\r\n") % channel % (topic ? topic : "")));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_RequestCTCP(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_RequestCTCP");
	boost::mutex::scoped_lock lock(core->mutex);
	char *user = NULL;
	amx_StrParam(amx, params[2], user);
	if (user == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[3], message);
	if (message == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("PRIVMSG %1% :\001%2%\001\r\n") % user % message));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_ReplyCTCP(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_ReplyCTCP");
	boost::mutex::scoped_lock lock(core->mutex);
	char *user = NULL;
	amx_StrParam(amx, params[2], user);
	if (user == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[3], message);
	if (message == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("NOTICE %1% :\001%2%\001\r\n") % user % message));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_SendRaw(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "IRC_SendRaw");
	boost::mutex::scoped_lock lock(core->mutex);
	char *message = NULL;
	amx_StrParam(amx, params[2], message);
	if (message == NULL)
	{
		return 0;
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("%1%\r\n") % message));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_CreateGroup(AMX *amx, cell *params)
{
	boost::mutex::scoped_lock lock(core->mutex);
	int groupID = 1;
	for (GroupMap::iterator g = core->groups.begin(); g != core->groups.end(); ++g)
	{
		if (g->first != groupID)
		{
			break;
		}
		++groupID;
	}
	core->groups.insert(std::make_pair(groupID, std::map<int, bool>()));
	return static_cast<cell>(groupID);
}

cell AMX_NATIVE_CALL Natives::IRC_DestroyGroup(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "IRC_DestroyGroup");
	boost::mutex::scoped_lock lock(core->mutex);
	GroupMap::iterator f = core->groups.find(static_cast<int>(params[1]));
	if (f != core->groups.end())
	{
		core->groups.erase(f);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_AddToGroup(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "IRC_AddToGroup");
	boost::mutex::scoped_lock lock(core->mutex);
	GroupMap::iterator f = core->groups.find(static_cast<int>(params[1]));
	if (f != core->groups.end())
	{
		std::map<int, bool>::iterator g = f->second.find(static_cast<int>(params[2]));
		if (g == f->second.end())
		{
			std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[2]));
			if (c != core->clients.end())
			{
				c->second->groupID = f->first;
			}
			f->second.insert(std::make_pair(static_cast<int>(params[2]), false));
			return 1;
		}
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_RemoveFromGroup(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "IRC_RemoveFromGroup");
	boost::mutex::scoped_lock lock(core->mutex);
	GroupMap::iterator f = core->groups.find(static_cast<int>(params[1]));
	if (f != core->groups.end())
	{
		std::map<int, bool>::iterator g = f->second.find(static_cast<int>(params[2]));
		if (g != f->second.end())
		{
			std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[2]));
			if (c != core->clients.end())
			{
				c->second->groupID = 0;
			}
			f->second.erase(static_cast<int>(params[2]));
			return 1;
		}
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_GroupSay(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_GroupSay");
	boost::mutex::scoped_lock lock(core->mutex);
	char *target = NULL;
	amx_StrParam(amx, params[2], target);
	if (target == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[3], message);
	if (message == NULL)
	{
		return 0;
	}
	int botID = 0;
	GroupMap::iterator f = core->groups.find(static_cast<int>(params[1]));
	if (f != core->groups.end())
	{
		if (f->second.empty())
		{
			return 0;
		}
		for (std::map<int, bool>::iterator g = f->second.begin(); g != f->second.end(); ++g)
		{
			if (!g->second)
			{
				botID = g->first;
				g->second = true;
				break;
			}
		}
		if (!botID)
		{
			for (std::map<int, bool>::iterator g = f->second.begin(); g != f->second.end(); ++g)
			{
				g->second = false;
			}
			std::map<int, bool>::iterator g = f->second.begin();
			botID = g->first;
			g->second = true;
		}
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(botID);
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("PRIVMSG %1% :%2%\r\n") % target % message));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_GroupNotice(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_GroupNotice");
	boost::mutex::scoped_lock lock(core->mutex);
	char *target = NULL;
	amx_StrParam(amx, params[2], target);
	if (target == NULL)
	{
		return 0;
	}
	char *message = NULL;
	amx_StrParam(amx, params[3], message);
	if (message == NULL)
	{
		return 0;
	}
	int botID = 0;
	GroupMap::iterator f = core->groups.find(static_cast<int>(params[1]));
	if (f != core->groups.end())
	{
		if (f->second.empty())
		{
			return 0;
		}
		for (std::map<int, bool>::iterator g = f->second.begin(); g != f->second.end(); ++g)
		{
			if (!g->second)
			{
				botID = g->first;
				g->second = true;
				break;
			}
		}
		if (!botID)
		{
			for (std::map<int, bool>::iterator g = f->second.begin(); g != f->second.end(); ++g)
			{
				g->second = false;
			}
			std::map<int, bool>::iterator g = f->second.begin();
			botID = g->first;
			g->second = true;
		}
	}
	std::map<int, SharedClient>::iterator c = core->clients.find(botID);
	if (c != core->clients.end())
	{
		c->second->sendAsync(boost::str(boost::format("NOTICE %1% :%2%\r\n") % target % message));
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL Natives::IRC_SetIntData(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "IRC_SetIntData");
	boost::mutex::scoped_lock lock(core->mutex);
	std::map<int, SharedClient>::iterator c = core->clients.find(static_cast<int>(params[1]));
	if (c != core->clients.end())
	{
		bool restart = false;
		switch (static_cast<int>(params[2]))
		{
			case Data::ConnectAttempts:
			{
				c->second->connectAttempts = static_cast<int>(params[3]);
				restart = true;
				break;
			}
			case Data::ConnectDelay:
			{
				c->second->connectDelay = static_cast<int>(params[3]);
				restart = true;
				break;
			}
			case Data::ConnectTimeout:
			{
				c->second->connectTimeout = static_cast<int>(params[3]);
				restart = true;
				break;
			}
			case Data::ReceiveTimeout:
			{
				c->second->receiveTimeout = static_cast<int>(params[3]);
				restart = true;
				break;
			}
			case Data::Respawn:
			{
				c->second->respawn = static_cast<int>(params[3]) != 0;
				restart = true;
				break;
			}
			default:
			{
				logprintf("*** IRC_SetIntData: Invalid data specified");
				break;
			}
		}
		if (restart)
		{
			if (!c->second->connected && c->second->socketOpen())
			{
				c->second->stopAsync();
				c->second->startAsync();
				return 1;
			}
		}
	}
	return 0;
}
