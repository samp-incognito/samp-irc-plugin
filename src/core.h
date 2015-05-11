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

#ifndef CORE_H
#define CORE_H

#include "common.h"
#include "data.h"

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <sdk/plugin.h>

#include <map>
#include <queue>
#include <set>

class Core
{
public:
	Core();

	boost::mutex mutex;
	boost::asio::io_service io_service;
	boost::asio::io_service::work work;

	std::set<AMX*> interfaces;
	std::queue<Data::Message> messages;

	std::map<int, SharedClient> clients;
	GroupMap groups;
};

extern boost::scoped_ptr<Core> core;

#endif
