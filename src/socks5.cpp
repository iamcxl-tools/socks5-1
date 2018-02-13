/*******************************************************************************
 *
 * socks5
 * A C++11 socks5 proxy server based on Libevent 
 *
 * Copyright 2018 Senlin Zhan. All rights reserved.
 *
 ******************************************************************************/

#include "cipher.hpp"
#include "config.hpp"
#include "server.hpp"

#include <gflags/gflags.h>
#include <glog/logging.h>

// check whether the port is in range [1, 65535]
static bool isValidPort(const char* flagname, gflags::int32 value)
{
    return (value > 0 && value < 65536);
}

static bool isValidSecretKey(const char *flagname, const std::string &value)
{
    // FIXME: key length may change
    return value.size() == Cryptor::KEY_SIZE;
} 

static bool isValidMode(const char *flagname, const std::string &value)
{
    return value == "client" || value == "server";
}

DEFINE_string(host, "localhost", "Listening host");

DEFINE_int32(port, 6060, "Listening port");
DEFINE_validator(port, &isValidPort);

DEFINE_string(username, "", "Username for login <optional>");
DEFINE_string(password, "", "Password for login <optional>");

DEFINE_string(key, "", "Secret key");
DEFINE_validator(key, &isValidSecretKey);

DEFINE_string(mode, "", "Working mode <client or server>");
DEFINE_validator(mode, &isValidMode);

int main(int argc, char *argv[])
{ 
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    Config config(
        FLAGS_host, static_cast<unsigned short>(FLAGS_port),
        FLAGS_username, FLAGS_password, FLAGS_key, FLAGS_mode
    );    
    
    LOG(WARNING) << "Socks5 options: "
                 << "Listening host = " << config.host() << ", "
                 << "Listening port = " << config.port() << ", "
                 << "Secret key = " << config.key();

    if (config.useUserPassAuth())
    {
        LOG(WARNING) << "Enable Username/Password authentication: "
                     << "username = " << config.username()
                     << ", password = " << config.password();
    }

    Server server(config);
    server.run();
    
    return 0;
}
