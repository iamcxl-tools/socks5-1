#include "auth.hpp"

#include <vector>

#include <event2/buffer.h>
#include <event2/bufferevent.h>

Auth::Auth(bufferevent *inConn)
    : inConn_(inConn),
      authMethod_(AUTH_NO_ACCEPTABLE),
      supportMethods_{AUTH_NONE}
{    
}

Auth::Auth(bufferevent *inConn, const std::string &username, const std::string &password)
    : inConn_(inConn),
      authMethod_(AUTH_NO_ACCEPTABLE),
      supportMethods_{AUTH_USER_PASSWORD},
      username_(username),
      password_(password)
{
}

/**
   Handling client authentication

   Returns:
     State::incomplete   the data received by server is incomplete
     State::success      the authentication is successful
     State::failed       the authentication is failed
     State::error        an error occurred

   The protocol used by the client to establish authentication:
   +----+----------+----------+
   |VER | NMETHODS | METHODS  |
   +----+----------+----------+
   | 1  |    1     | 1 to 255 |
   +----+----------+----------+

   Socks5 server response with:
   +----+--------+
   |VER | METHOD |
   +----+--------+
   | 1  |   1    |
   +----+--------+
**/
Auth::State Auth::authenticate()
{
    auto inBuff = bufferevent_get_input(inConn_);
    auto inBuffLength = evbuffer_get_length(inBuff);

    if (inBuffLength < 2)
    {
        return State::incomplete;
    }

    unsigned char data[2];    
    evbuffer_copyout(inBuff, data, 2);

    // check protocol version number
    if (data[0] != SOCKS5_VERSION)
    {
        return State::error;
    }

    // how many kinds of methods
    unsigned char nmethods = data[1];

    if (inBuffLength < 2 + nmethods)
    {
        return State::incomplete;
    }    
    else if (inBuffLength > 2 + nmethods)
    {
        return State::error;
    }

    // remove first two bytes from the input buffer
    evbuffer_drain(inBuff, 2);

    std::vector<unsigned char> methods(nmethods, 0);
    evbuffer_remove(inBuff, methods.data(), nmethods);
    
    /* 
     * FIXME: we only support NO AUTHENTICATION here, 
     * add username/password authentication
     */    
    for (auto method: methods)
    {
        if (supportMethods_.find(method) != supportMethods_.end())
        {
            authMethod_ = method;
            break;
        }
    }

    unsigned char response[2];
    response[0] = SOCKS5_VERSION;
    response[1] = authMethod_;
    
    if (bufferevent_write(inConn_, response, 2) == -1)
    {
        return State::error;
    }

    if (authMethod_ == AUTH_NO_ACCEPTABLE)
    {
        return State::failed;
    }

    return State::success;
}


Auth::State Auth::validateUsernamePassword()
{
    auto inBuff = bufferevent_get_input(inConn_);
    auto inBuffLength = evbuffer_get_length(inBuff);

    if (inBuffLength < 2)
    {
        return State::incomplete;
    }

    // 1024 bytes is enough
    unsigned char data[1024];

    // Get version number and length of username
    evbuffer_copyout(inBuff, data, 2);

    // check protocol version number
    if (data[0] != SOCKS5_VERSION)
    {
        return State::error;
    }

    auto userLength = data[1];        
    if (inBuffLength < 3 + userLength)
    {
        return State::incomplete;
    }

    // Get version number and length of username
    evbuffer_copyout(inBuff, data, 3 + userLength);    

    auto passLength = data[userLength + 2];
    if (inBuffLength < 4 + userLength + passLength)
    {
        return State::incomplete;        
    }
    else if (inBuffLength > 4 + userLength + passLength)
    {
        return State::error;
    }

    evbuffer_remove(inBuff, data, inBuffLength);
    std::string username(&data[3], &data[3 + userLength]);
    std::string password(&data[4 + userLength], &data[inBuffLength]);

    if (username != username_ || password != password_)
    {
        return State::failed;
    }
    
    return State::success;
}
