#include <Helix/Core/Types/Guid.h>

#include <windows.h>
#include <bcrypt.h>

namespace helix
{
    void FillRandomBytes(void* buffer, usize size)
    {
        NTSTATUS status = BCryptGenRandom(nullptr, static_cast<PUCHAR>(buffer), ULONG(size), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        HX_VERIFY_MSG(status >= 0, "BCryptGenRandom failed");
    }
}
