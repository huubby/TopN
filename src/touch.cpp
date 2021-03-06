#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "log.h"
#include "touch.h"

uint16_t type_2_port(port_type_t type)
{
    switch (type) {
        case TCP_PORT_80:
            return 80;
        case TCP_PORT_443:
            return 443;
        case TCP_PORT_8080:
            return 8080;
        default:
            break;
    }

    return 0;
}

bool
is_addr_reachable(uint32_t ip_num, port_type_t type, uint32_t timeout_in_ms)
{
    uint16_t port = type_2_port(type);
    if (port == 0)
        return false;
    ip_num = htonl(ip_num);
    char ip[16];
    if (!inet_ntop(AF_INET, &ip_num, ip, sizeof(ip))) {
        return false;
    }

    int res, valopt;
    struct sockaddr_in addr;
    long arg;
    fd_set myset;
    struct timeval tv;
    socklen_t lon;

    int soc = socket(AF_INET, SOCK_STREAM, 0);
    if (soc == -1) {
        LOG(LOG_LEVEL_ERROR, "Creating socket failed, %s", strerror(errno));
        return false;
    }

    bool result = true;
    arg = fcntl(soc, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    if (-1 == fcntl(soc, F_SETFL, arg)) {
        LOG(LOG_LEVEL_WARNING, "Set socket to non-block failed");
        result = false;
        goto end;
    }

    bzero((char*)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    errno = 0;
    res = connect(soc, (struct sockaddr*)&addr, sizeof(addr));
    if (res < 0) {
        if (errno == EINPROGRESS) {
            tv.tv_sec = timeout_in_ms/1000;
            tv.tv_usec = 1000*timeout_in_ms;
            FD_ZERO(&myset);
            FD_SET(soc, &myset);
            if (select(soc+1, NULL, &myset, NULL, &tv) > 0) {
                lon = sizeof(int);
                getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
                if (valopt) {
                    LOG(LOG_LEVEL_WARNING
                            , "Error in connect() %d - %s, (%s:%d)"
                            , valopt, strerror(valopt), ip, port);
                    result = false;
                    goto end;
                }
            } else {
                LOG(LOG_LEVEL_WARNING
                        , "Connecting timeout, %s, (%s:%d)"
                        , strerror(errno), ip, port);
                result = false;
                goto end;
            }
        } else {
            LOG(LOG_LEVEL_WARNING
                    , "Error connecting, %s, (%s:%d)"
                    , strerror(errno), ip, port);
            result = false;
            goto end;
        }
    }

end:
    close(soc);

    return result;
}
