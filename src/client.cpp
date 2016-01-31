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

#include "client.h"

#include "core.h"
#include "main.h"

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

#include <limits>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

Client::Client(boost::asio::io_service &io_service) :
	clientSocket(io_service),
	resolver(io_service),
	connectTimer(io_service),
	connectTimeoutTimer(io_service),
	receiveTimeoutTimer(io_service),
	resolveTimer(io_service)
{
	const char *commands[] =
	{
		"NICK",
		"QUIT",
		"JOIN",
		"PART",
		"TOPIC",
		"INVITE",
		"KICK",
		"MODE",
		"PRIVMSG",
		"NOTICE",
		"PING"
	};
	for (std::size_t i = 0; i < sizeof(commands) / sizeof(const char*); ++i)
	{
		serverCommands.insert(std::make_pair(commands[i], i));
	}
	connectAttempts = 5;
	connectDelay = 20;
	connectTimeout = 10;
	connected = false;
	currentConnectAttempts = 1;
	receiveTimeout = std::numeric_limits<int>::max();
	respawn = true;
	quitting = false;
	timedOut = false;
	writeInProgress = false;
}

void Client::handleConnect(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator iterator)
{
	boost::mutex::scoped_lock lock(core->mutex);
	if (!error)
	{
		connectedAddress = iterator->endpoint().address().to_string();
		connectedPort = iterator->endpoint().port();
		if (!serverPassword.empty())
		{
			sendAsync(boost::str(boost::format("PASS %1%\r\n") % serverPassword));
		}
		sendAsync(boost::str(boost::format("USER %1% 0 * :%2%\r\nNICK %3%\r\n") % username % realname % nickname));
		startRead();
		connectTimeoutTimer.cancel();
	}
	else
	{
		if (!socketOpen())
		{
			Data::Message message;
			message.array.push_back(Data::OnConnectAttemptFail);
			message.array.push_back(iterator->endpoint().port());
			message.array.push_back(botID);
			message.buffer.push_back("Connection attempt timed out");
			message.buffer.push_back(iterator->endpoint().address().to_string());
			core->messages.push(message);
			startConnectTimer(iterator);
		}
		else
		{
			Data::Message message;
			message.array.push_back(Data::OnConnectAttemptFail);
			message.array.push_back(iterator->endpoint().port());
			message.array.push_back(botID);
			message.buffer.push_back(error.message());
			message.buffer.push_back(iterator->endpoint().address().to_string());
			core->messages.push(message);
			stopAsync();
			startConnectTimer(iterator);
		}
	}
}

void Client::handleRead(const boost::system::error_code &error, std::size_t transferredBytes)
{
	boost::mutex::scoped_lock lock(core->mutex);
	if (!error)
	{
		std::string messageBuffer = receivedData;
		messageBuffer.resize(transferredBytes);
		boost::algorithm::erase_all(messageBuffer, "\r");
		boost::algorithm::erase_last(messageBuffer, "\n");
		std::vector<std::string> splitBuffer;
		boost::algorithm::split(splitBuffer, messageBuffer, boost::algorithm::is_any_of("\n"));
		for (std::vector<std::string>::iterator i = splitBuffer.begin(); i != splitBuffer.end(); ++i)
		{
			if (!i->empty())
			{
				Data::Message message;
				message.array.push_back(Data::OnReceiveRaw);
				message.array.push_back(botID);
				if (i->length() > MAX_BUFFER / 8)
				{
					std::string tempBuffer = *i;
					tempBuffer.resize(MAX_BUFFER / 8);
					message.buffer.push_back(tempBuffer);
				}
				else
				{
					message.buffer.push_back(*i);
				}
				core->messages.push(message);
				parseBuffer(*i);
			}
		}
		startReceiveTimeoutTimer();
		startRead();
	}
	else
	{
		std::string reason;
		if (timedOut)
		{
			reason = "Connection timed out";
			timedOut = false;
		}
		else
		{
			reason = error.message();
		}
		Data::Message message;
		message.array.push_back(Data::OnDisconnect);
		message.array.push_back(connectedPort);
		message.array.push_back(botID);
		message.buffer.push_back(reason);
		message.buffer.push_back(connectedAddress);
		core->messages.push(message);
		if (!quitting)
		{
			stopAsync();
			if (respawn)
			{
				startAsync();
			}
		}
	}
}

void Client::handleResolve(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator iterator)
{
	boost::mutex::scoped_lock lock(core->mutex);
	if (!error)
	{
		currentConnectAttempts = 0;
		startConnectTimer(iterator);
	}
	else
	{
		Data::Message message;
		message.array.push_back(Data::OnConnectAttemptFail);
		message.array.push_back(remotePort);
		message.array.push_back(botID);
		message.buffer.push_back(error.message());
		message.buffer.push_back(remoteAddress);
		core->messages.push(message);
		startResolveTimer();
	}
}

void Client::handleWrite(const boost::system::error_code &error)
{
	boost::mutex::scoped_lock lock(core->mutex);
	writeInProgress = false;
	if (!error)
	{
		if (!pendingMessages.empty())
		{
			sendAsync(pendingMessages.front());
			pendingMessages.pop();
		}
	}
	else
	{
		pendingMessages = std::queue<std::string>();
	}
}

void Client::handleConnectTimer(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator iterator)
{
	boost::mutex::scoped_lock lock(core->mutex);
	if (!error)
	{
		if (iterator != boost::asio::ip::tcp::resolver::iterator())
		{
			Data::Message message;
			message.array.push_back(Data::OnConnectAttempt);
			message.array.push_back(iterator->endpoint().port());
			message.array.push_back(botID);
			message.buffer.push_back(iterator->endpoint().address().to_string());
			core->messages.push(message);
			clientSocket.async_connect(iterator->endpoint(), boost::bind(&Client::handleConnect, shared_from_this(), boost::asio::placeholders::error, iterator));
			startConnectTimeoutTimer();
		}
		else
		{
			stopAsync();
		}
	}
}

void Client::handleConnectTimeoutTimer(const boost::system::error_code &error)
{
	boost::mutex::scoped_lock lock(core->mutex);
	if (!error && !connected)
	{
		stopAsync();
	}
}

void Client::handleReceiveTimeoutTimer(const boost::system::error_code &error)
{
	boost::mutex::scoped_lock lock(core->mutex);
	if (!error && connected)
	{
		stopAsync();
		timedOut = true;
	}
}

void Client::handleResolveTimer(const boost::system::error_code &error)
{
	boost::mutex::scoped_lock lock(core->mutex);
	if (!error)
	{
		stopAsync();
		if (currentConnectAttempts < connectAttempts)
		{
			++currentConnectAttempts;
			startAsync();
		}
	}
}

void Client::sendAsync(const std::string &buffer)
{
	if (writeInProgress)
	{
		pendingMessages.push(buffer);
	}
	else
	{
		sentData = buffer;
		writeInProgress = true;
		boost::asio::async_write(clientSocket, boost::asio::buffer(sentData, sentData.length()), boost::bind(&Client::handleWrite, shared_from_this(), boost::asio::placeholders::error));
	}
}

bool Client::socketOpen()
{
	return clientSocket.is_open();
}

void Client::startAsync()
{
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), remoteAddress, boost::str(boost::format("%1%") % remotePort));
	resolver.async_resolve(query, boost::bind(&Client::handleResolve, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::iterator));
	core->clients.insert(std::make_pair(botID, shared_from_this()));
}

void Client::stopAsync()
{
	if (socketOpen())
	{
		boost::system::error_code error;
		if (connected)
		{
			clientSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			connected = false;
			pendingChannels.clear();
			pendingMessages = std::queue<std::string>();
			users.clear();
			writeInProgress = false;
		}
		clientSocket.close(error);
		connectTimer.cancel(error);
		connectTimeoutTimer.cancel(error);
		receiveTimeoutTimer.cancel(error);
		resolveTimer.cancel(error);
	}
	core->clients.erase(botID);
}

void Client::startRead()
{
	clientSocket.async_read_some(boost::asio::buffer(receivedData, MAX_BUFFER), boost::bind(&Client::handleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Client::startConnectTimer(boost::asio::ip::tcp::resolver::iterator iterator)
{
	if (!localAddress.empty())
	{
		boost::system::error_code error;
		boost::asio::ip::address address(boost::asio::ip::address::from_string(localAddress, error));
		if (error)
		{
			logprintf("*** IRC Plugin: Error using supplied local address: %s", error.message().c_str());
		}
		clientSocket.open(boost::asio::ip::tcp::v4(), error);
		if (error)
		{
			logprintf("*** IRC Plugin: Error opening socket: %s", error.message().c_str());
		}
		boost::asio::ip::tcp::endpoint endpoint(address, 0);
		clientSocket.bind(endpoint, error);
		if (error)
		{
			logprintf("*** IRC Plugin: Error binding local address to socket: %s", error.message().c_str());
		}
	}
	connectTimer.expires_from_now(boost::posix_time::seconds(connectDelay));
	if (currentConnectAttempts < connectAttempts)
	{
		++currentConnectAttempts;
		connectTimer.async_wait(boost::bind(&Client::handleConnectTimer, shared_from_this(), boost::asio::placeholders::error, iterator));
	}
	else
	{
		currentConnectAttempts = 1;
		connectTimer.async_wait(boost::bind(&Client::handleConnectTimer, shared_from_this(), boost::asio::placeholders::error, ++iterator));
	}
	core->clients.insert(std::make_pair(botID, shared_from_this()));
}

void Client::startConnectTimeoutTimer()
{
	connectTimeoutTimer.expires_from_now(boost::posix_time::seconds(connectTimeout));
	connectTimeoutTimer.async_wait(boost::bind(&Client::handleConnectTimeoutTimer, shared_from_this(), boost::asio::placeholders::error));
}

void Client::startReceiveTimeoutTimer()
{
	receiveTimeoutTimer.expires_from_now(boost::posix_time::seconds(receiveTimeout));
	receiveTimeoutTimer.async_wait(boost::bind(&Client::handleReceiveTimeoutTimer, shared_from_this(), boost::asio::placeholders::error));
}

void Client::startResolveTimer()
{
	resolveTimer.expires_from_now(boost::posix_time::seconds(connectTimeout));
	resolveTimer.async_wait(boost::bind(&Client::handleResolveTimer, shared_from_this(), boost::asio::placeholders::error));
}

void Client::parseBuffer(const std::string &buffer)
{
	std::string command, delimitedParameters, host, leading = buffer, trailing, user;
	std::vector<std::string> parameters;
	std::size_t locationOfTrailing = buffer.find(" :");
	if (locationOfTrailing != std::string::npos)
	{
		leading = buffer.substr(0, locationOfTrailing);
		trailing = buffer.substr(locationOfTrailing + 2);
		boost::algorithm::trim(trailing);
	}
	std::list<std::string> splitLeading;
	boost::algorithm::split(splitLeading, leading, boost::algorithm::is_any_of(" "));
	if (!splitLeading.empty())
	{
		if (!splitLeading.front().find(':'))
		{
			std::size_t locationOfHostname = splitLeading.front().find("!");
			if (locationOfHostname != std::string::npos)
			{
				host = splitLeading.front().substr(locationOfHostname + 1);
				user = splitLeading.front().substr(1, locationOfHostname - 1);
			}
			else
			{
				host = "No hostname";
				user = splitLeading.front().substr(1);
			}
			splitLeading.pop_front();
		}
		if (!splitLeading.empty())
		{
			command = splitLeading.front();
			splitLeading.pop_front();
		}
		for (std::list<std::string>::iterator i = splitLeading.begin(); i != splitLeading.end(); ++i)
		{
			delimitedParameters += *i + " ";
			parameters.push_back(*i);
		}
		boost::algorithm::trim(delimitedParameters);
	}
	int numeric = 0;
	std::istringstream numericStream(command);
	if ((numericStream >> numeric).eof())
	{
		switch (numeric)
		{
			case RPL_WELCOME:
			{
					Data::Message message;
					message.array.push_back(Data::OnConnect);
					message.array.push_back(connectedPort);
					message.array.push_back(botID);
					message.buffer.push_back(connectedAddress);
					core->messages.push(message);
					connected = true;
					break;
			}
			case RPL_NAMREPLY:
			{
				if (!parameters.empty() && !trailing.empty())
				{
					std::string channel = parameters.back();
					std::set<std::string>::iterator f = pendingChannels.find(channel);
					if (f == pendingChannels.end())
					{
						UserMap::iterator u = users.begin();
						while (u != users.end())
						{
							u->second.erase(channel);
							if (u->second.empty())
							{
								users.erase(u++);
							}
							else
							{
								++u;
							}
						}
						pendingChannels.insert(channel);
					}
					std::vector<std::string> splitTrailing;
					boost::algorithm::split(splitTrailing, trailing, boost::algorithm::is_any_of(" "));
					for (std::vector<std::string>::iterator i = splitTrailing.begin(); i != splitTrailing.end(); ++i)
					{
						std::string mode;
						switch (i->at(0))
						{
							case '+':
							case '%':
							case '@':
							case '&':
							case '!':
							case '*':
							case '~':
							case '.':
							{
								mode = i->at(0);
								break;
							}
						}
						boost::algorithm::trim_if(*i, boost::algorithm::is_any_of("+%@&!*~."));
						UserMap::iterator f = users.find(*i);
						if (f != users.end())
						{
							f->second.insert(std::make_pair(channel, mode));
						}
						else
						{
							std::map<std::string, std::string> channels;
							channels.insert(std::make_pair(channel, mode));
							users.insert(std::make_pair(*i, channels));
						}
					}
				}
				break;
			}
			case RPL_ENDOFNAMES:
			{
				if (!parameters.empty())
				{
					std::string channel = parameters.back();
					pendingChannels.erase(channel);
				}
				break;
			}
		}
		std::string numericMessage;
		if (!parameters.empty())
		{
			std::size_t locationOfNumericMessage = buffer.find(parameters.at(0));
			if (locationOfNumericMessage != std::string::npos)
			{
				numericMessage = buffer.substr(locationOfNumericMessage + parameters.at(0).length());
				boost::algorithm::trim(numericMessage);
			}
		}
		if (numericMessage.empty())
		{
			numericMessage = "No message";
		}
		Data::Message message;
		message.array.push_back(Data::OnReceiveNumeric);
		message.array.push_back(numeric);
		message.array.push_back(botID);
		message.buffer.push_back(numericMessage);
		core->messages.push(message);
	}
	else
	{
		std::map<std::string, int>::iterator m = serverCommands.find(command);
		if (m != serverCommands.end())
		{
			if (!trailing.empty())
			{
				boost::algorithm::replace_all(trailing, "%", "");
			}
			switch (m->second)
			{
				case Nick:
				{
					if (!host.empty() && !parameters.empty() && !user.empty())
					{
						if (user.compare(nickname) != 0)
						{
							Data::Message message;
							message.array.push_back(Data::OnUserNickChange);
							message.array.push_back(botID);
							message.buffer.push_back(host);
							message.buffer.push_back(parameters.back());
							message.buffer.push_back(user);
							core->messages.push(message);
						}
						else
						{
							nickname = parameters.back();
						}
						UserMap::iterator f = users.find(user);
						if (f != users.end())
						{
							users.insert(std::make_pair(parameters.back(), f->second));
							users.erase(f);
						}
					}
					break;
				}
				case Quit:
				{
					if (!host.empty() && !user.empty())
					{
						if (user.compare(nickname) != 0)
						{
							if (trailing.empty())
							{
								trailing = "No reason";
							}
							Data::Message message;
							message.array.push_back(Data::OnUserDisconnect);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							core->messages.push(message);
							users.erase(user);
						}
					}
					break;
				}
				case Join:
				{
					if (!host.empty() && !trailing.empty() && !user.empty())
					{
						Data::Message message;
						if (!user.compare(nickname))
						{
							message.array.push_back(Data::OnJoinChannel);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
						}
						else
						{
							message.array.push_back(Data::OnUserJoinChannel);
							message.array.push_back(botID);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							message.buffer.push_back(trailing);
						}
						UserMap::iterator f = users.find(user);
						if (f != users.end())
						{
							f->second.insert(std::make_pair(trailing, ""));
						}
						else
						{
							std::map<std::string, std::string> channels;
							channels.insert(std::make_pair(trailing, ""));
							users.insert(std::make_pair(user, channels));
						}
						core->messages.push(message);
					}
					break;
				}
				case Part:
				{
					if (!host.empty() && !parameters.empty() && !user.empty())
					{
						if (trailing.empty())
						{
							trailing = "No reason";
						}
						Data::Message message;
						if (!user.compare(nickname))
						{
							message.array.push_back(Data::OnLeaveChannel);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(parameters.back());
							UserMap::iterator u = users.begin();
							while (u != users.end())
							{
								u->second.erase(parameters.back());
								if (u->second.empty())
								{
									users.erase(u++);
								}
								else
								{
									++u;
								}
							}
						}
						else
						{
							message.array.push_back(Data::OnUserLeaveChannel);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							message.buffer.push_back(parameters.back());
							UserMap::iterator f = users.find(user);
							if (f != users.end())
							{
								f->second.erase(parameters.back());
								if (f->second.empty())
								{
									users.erase(f);
								}
							}
						}
						core->messages.push(message);
					}
					break;
				}
				case Topic:
				{
					if (!host.empty() && !parameters.empty() && !user.empty())
					{
						if (trailing.empty())
						{
							trailing = "No topic";
						}
						if (user.compare(nickname) != 0)
						{
							Data::Message message;
							message.array.push_back(Data::OnUserSetChannelTopic);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							message.buffer.push_back(parameters.back());
							core->messages.push(message);
						}
					}
					break;
				}
				case Invite:
				{
					if (!host.empty() && !trailing.empty() && !user.empty())
					{
						Data::Message message;
						message.array.push_back(Data::OnInvitedToChannel);
						message.array.push_back(botID);
						message.buffer.push_back(host);
						message.buffer.push_back(user);
						message.buffer.push_back(trailing);
						core->messages.push(message);
					}
					break;
				}
				case Kick:
				{
					if (!host.empty() && parameters.size() == 2 && !user.empty())
					{
						if (trailing.empty())
						{
							trailing = "No reason";
						}
						Data::Message message;
						if (!parameters.at(1).compare(nickname))
						{
							message.array.push_back(Data::OnKickedFromChannel);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							message.buffer.push_back(parameters.at(0));
							UserMap::iterator u = users.begin();
							while (u != users.end())
							{
								u->second.erase(parameters.at(0));
								if (u->second.empty())
								{
									users.erase(u++);
								}
								else
								{
									++u;
								}
							}
						}
						else
						{
							message.array.push_back(Data::OnUserKickedFromChannel);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							message.buffer.push_back(parameters.at(1));
							message.buffer.push_back(parameters.at(0));
							UserMap::iterator f = users.find(parameters.at(1));
							if (f != users.end())
							{
								f->second.erase(parameters.at(0));
								if (f->second.empty())
								{
									users.erase(f);
								}
							}
						}
						core->messages.push(message);
					}
					break;
				}
				case Mode:
				{
					if (!host.empty() && !parameters.empty() && !user.empty())
					{
						boost::algorithm::trim(delimitedParameters);
						std::size_t result = delimitedParameters.find_first_of(' ');
						if (result != std::string::npos)
						{
							if (user.compare(nickname) != 0)
							{
								Data::Message message;
								message.array.push_back(Data::OnUserSetChannelMode);
								message.array.push_back(botID);
								message.buffer.push_back(delimitedParameters.substr(result + 1));
								message.buffer.push_back(host);
								message.buffer.push_back(user);
								message.buffer.push_back(parameters.at(0));
								core->messages.push(message);
							}
							if (parameters.at(1).find_first_of("vhoauq") != std::string::npos)
							{
								sendAsync(boost::str(boost::format("NAMES %1%\r\n") % parameters.at(0)));
							}
						}
					}
					break;
				}
				case Privmsg:
				{
					if (!host.empty() && !parameters.empty() && !trailing.empty() && !user.empty())
					{
						if (trailing.at(0) == '\001')
						{
							boost::algorithm::erase_all(trailing, "\001");
							Data::Message message;
							message.array.push_back(Data::OnUserRequestCTCP);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							core->messages.push(message);
						}
						else
						{
							if (parameters.back().at(0) == '#' || parameters.back().at(0) == '&')
							{
								GroupMap::iterator f = core->groups.find(groupID);
								if (f != core->groups.end())
								{
									std::map<int, bool>::iterator g = f->second.begin();
									if (g->first != botID)
									{
										return;
									}
								}
							}
							if (user.compare(nickname) != 0)
							{
								Data::Message message;
								message.array.push_back(Data::OnUserSay);
								message.array.push_back(botID);
								message.buffer.push_back(trailing);
								message.buffer.push_back(host);
								message.buffer.push_back(user);
								message.buffer.push_back(parameters.back());
								core->messages.push(message);
							}
						}
					}
					break;
				}
				case Notice:
				{
					if (!host.empty() && !parameters.empty() && !trailing.empty() && !user.empty())
					{
						if (trailing.at(0) == '\001')
						{
							boost::algorithm::erase_all(trailing, "\001");
							Data::Message message;
							message.array.push_back(Data::OnUserReplyCTCP);
							message.array.push_back(botID);
							message.buffer.push_back(trailing);
							message.buffer.push_back(host);
							message.buffer.push_back(user);
							core->messages.push(message);
						}
						else
						{
							if (user.compare(nickname) != 0)
							{
								Data::Message message;
								message.array.push_back(Data::OnUserNotice);
								message.array.push_back(botID);
								message.buffer.push_back(trailing);
								message.buffer.push_back(host);
								message.buffer.push_back(user);
								message.buffer.push_back(parameters.back());
								core->messages.push(message);
							}
						}
					}
					break;
				}
				case Ping:
				{
					std::string sendBuffer = buffer;
					boost::algorithm::replace_first(sendBuffer, "PING", "PONG");
					sendBuffer.append("\r\n");
					sendAsync(sendBuffer);
					break;
				}
			}
		}
	}
}
