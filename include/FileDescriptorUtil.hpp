#ifndef FILEDESCRIPTORUTIL_HPP
#define FILEDESCRIPTORUTIL_HPP

#ifdef __APPLE__
#include <fcntl.h>
#include <unistd.h>

class FileDescriptorUtil {
public:
    static bool setNonBlockingMode(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            return false;
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            return false;
        }
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
            return false;
        }
        return true;
    }
};

#endif // __APPLE__
#endif // FILEDESCRIPTORUTIL_HPP