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

#ifndef COMMON_H
#define COMMON_H

#define MAX_BUFFER (4096)

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

class Client;

typedef boost::shared_ptr<Client> SharedClient;

typedef std::map<int, std::map<int, bool> > GroupMap;
typedef std::map<std::string, std::map<std::string, std::string> > UserMap;

#endif
