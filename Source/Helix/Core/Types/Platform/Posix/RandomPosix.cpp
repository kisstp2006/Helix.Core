#include <Helix/Core/Types/Guid.h>

#include <fcntl.h>
#include <unistd.h>

#if HX_PLATFORM_WEB
#include <sys/random.h>
#endif

namespace helix
{
    void FillRandomBytes(void* buffer, usize size)
    {
        u8* out = static_cast<u8*>(buffer);
#if HX_PLATFORM_WEB
        while (size)
        {
            usize chunk = Min<usize>(size, 256);
            HX_VERIFY_MSG(getentropy(out, chunk) == 0, "getentropy failed");
            out += chunk;
            size -= chunk;
        }
#else
        int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
        HX_VERIFY_MSG(fd >= 0, "cannot open /dev/urandom");
        while (size)
        {
            ssize_t n = read(fd, out, size);
            if (n <= 0)
            {
                HX_VERIFY_MSG(false, "reading /dev/urandom failed");
                break;
            }
            out += n;
            size -= usize(n);
        }
        close(fd);
#endif
    }
}
