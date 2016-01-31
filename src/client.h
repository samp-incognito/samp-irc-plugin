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

#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <map>
#include <string>
#include <queue>
#include <set>

class Client : public boost::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_service &io_service);

	void sendAsync(const std::string &buffer);
	bool socketOpen();
	void startAsync();
	void stopAsync();

	int connectAttempts;
	int connectDelay;
	int connectTimeout;
	int receiveTimeout;
	bool respawn;

	bool connected;
	int botID;
	int groupID;
	bool quitting;

	std::string nickname;
	std::string realname;
	std::string username;

	std::string localAddress;
	std::string remoteAddress;
	unsigned short remotePort;

	std::string serverPassword;

	UserMap users;
private:
	void handleConnect(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator iterator);
	void handleRead(const boost::system::error_code &error, std::size_t transferredBytes);
	void handleResolve(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator iterator);
	void handleWrite(const boost::system::error_code &error);

	void handleConnectTimer(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator iterator);
	void handleConnectTimeoutTimer(const boost::system::error_code &error);
	void handleReceiveTimeoutTimer(const boost::system::error_code &error);
	void handleResolveTimer(const boost::system::error_code &error);

	void startRead();

	void startConnectTimer(boost::asio::ip::tcp::resolver::iterator iterator);
	void startConnectTimeoutTimer();
	void startReceiveTimeoutTimer();
	void startResolveTimer();

	void parseBuffer(const std::string &buffer);

	enum Commands
	{
		Nick,
		Quit,
		Join,
		Part,
		Topic,
		Invite,
		Kick,
		Mode,
		Privmsg,
		Notice,
		Ping
	};

	enum Replies
	{
		RPL_WELCOME = 1,
		RPL_NAMREPLY = 353,
		RPL_ENDOFNAMES = 366
	};

	boost::asio::ip::tcp::socket clientSocket;
	boost::asio::ip::tcp::resolver resolver;

	boost::asio::deadline_timer connectTimer;
	boost::asio::deadline_timer connectTimeoutTimer;
	boost::asio::deadline_timer receiveTimeoutTimer;
	boost::asio::deadline_timer resolveTimer;

	std::string connectedAddress;
	unsigned short connectedPort;

	int currentConnectAttempts;
	std::set<std::string> pendingChannels;
	std::queue<std::string> pendingMessages;
	char receivedData[MAX_BUFFER];
	std::string sentData;
	std::map<std::string, int> serverCommands;
	bool timedOut;
	bool writeInProgress;
};

#endif
