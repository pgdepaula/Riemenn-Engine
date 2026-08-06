/**
 * @file dx11_context.cpp
 * @brief Direct3D 11 Backend Implementation (Conceptual Stub for Runtime API Switching)
 */

#include "engine/rhi/rhi.h"
#include "engine/foundation/log.h"

extern "C" {

/* Conceptual stub for Direct3D 11 RHI initialization */
int ri_gpu_backend_dx11_init(void)
{
    RI_LOG_INFO("[RHI] Direct3D 11 backend selected (stub - pending implementation)");
    return 0;
}

}
