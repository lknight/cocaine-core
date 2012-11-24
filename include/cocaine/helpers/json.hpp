/*
    Copyright (c) 2011-2012 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2012 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>. 
*/

#ifndef COCAINE_HELPERS_JSON
#define COCAINE_HELPERS_JSON

#include "json/json.h"

namespace cocaine { namespace json {

template<class T>
static inline
Json::Value
build(const std::string& key,
      const T& value)
{
    Json::Value object(Json::objectValue);
    object[key] = value;
    return object;
}

static inline
std::string
serialize(const Json::Value& json) {
    return Json::FastWriter().write(json);
}

}} // namespace cocaine::json

#endif
