//
// Copyright (C) 2011-2012 Andrey Sibiryov <me@kobology.ru>
//
// Licensed under the BSD 2-Clause License (the "License");
// you may not use this file except in compliance with the License.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <boost/filesystem/fstream.hpp>

#include "cocaine/context.hpp"

#include "cocaine/io.hpp"
#include "cocaine/logging.hpp"

#include "cocaine/storages/files.hpp"

using namespace cocaine;
using namespace cocaine::storages;

namespace fs = boost::filesystem;

const float defaults::heartbeat_timeout = 30.0f;
const float defaults::suicide_timeout = 600.0f;
const unsigned int defaults::pool_limit = 10;
const unsigned int defaults::queue_limit = 100;
const unsigned int defaults::io_bulk_size = 100;
const char defaults::slave[] = "cocaine-slave";
const char defaults::plugin_path[] = "/usr/lib/cocaine";
const char defaults::ipc_path[] = "/var/run/cocaine";
const char defaults::spool_path[] = "/var/spool/cocaine";

namespace {
    void validate_path(const fs::path& path) {
        if(!fs::exists(path)) {
            throw configuration_error_t("the specified path '" + path.string() + "' does not exist");
        } else if(fs::exists(path) && !fs::is_directory(path)) {
            throw configuration_error_t("the specified path '" + path.string() + "' is not a directory");
        }
    }
}

config_t::config_t(const std::string& path):
    config_path(path)
{
    if(!fs::exists(config_path)) {
        throw configuration_error_t("the configuration file doesn't exist");
    }

    fs::ifstream stream(config_path);

    if(!stream) {
        throw configuration_error_t("unable to open the configuration file");
    }

    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;

    if(!reader.parse(stream, root)) {
        throw configuration_error_t("the configuration file is corrupted");
    }

    // Validation
    // ----------

    if(root.get("version", 0).asUInt() != 1) {
        throw configuration_error_t("the configuration version is invalid");
    }

    // Paths
    // -----

    plugin_path = root["paths"].get("plugins", defaults::plugin_path).asString();
    
    validate_path(plugin_path);

    spool_path = root["paths"].get("spool", defaults::spool_path).asString();
    
    validate_path(spool_path);

    ipc_path = root["paths"].get("ipc", defaults::ipc_path).asString();
    
    validate_path(ipc_path);

    // Storage configuration
    // ---------------------

    if(!root["storages"].isObject() || root["storages"].empty()) {
        throw configuration_error_t("no storages has been configured");
    }

    Json::Value::Members storage_names(root["storages"].getMemberNames());

    for(Json::Value::Members::const_iterator it = storage_names.begin();
        it != storage_names.end();
        ++it)
    {
        plugin_config_t config = {
            *it,
            root["storages"][*it]["args"]
        };

        storage_info_t info = {
            root["storages"][*it]["type"].asString(),
            config
        };

        storages.insert(
            std::make_pair(
                *it,
                info
            )
        );
    }

    if(storages.find("core") == storages.end()) {
        throw configuration_error_t("mandatory 'core' storage has not been configured");
    }


    // IO configuration
    // ----------------

    char hostname[HOSTNAME_MAX_LENGTH];

    if(gethostname(hostname, HOSTNAME_MAX_LENGTH) == 0) {
        runtime.hostname = hostname;
    } else {
        throw system_error_t("failed to determine the hostname");
    }   
}

context_t::context_t(config_t config_, boost::shared_ptr<logging::sink_t> sink):
    config(config_),
    m_sink(sink)
{
    if(!m_sink) {
        m_sink.reset(new logging::void_sink_t());
    }

    // Initialize the component repository.
    m_repository.reset(new repository_t(*this));

    // Register the builtin components.
    m_repository->insert<file_storage_t>("files");
}

context_t::~context_t() { }

zmq::context_t& context_t::io() {
    boost::lock_guard<boost::mutex> lock(m_mutex);
    
    if(!m_io.get()) {
        m_io.reset(new zmq::context_t(1));
    }

    return *m_io;
}

boost::shared_ptr<logging::logger_t>
context_t::log(const std::string& name) {
    return m_sink->get(name);
}
