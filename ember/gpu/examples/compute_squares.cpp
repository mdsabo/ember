#include "Device.h"

using namespace ember::gpu;

int main() {
    const auto app_info = AppInfo {
        .name = "compute_squares",
        .version = 1,
        .features = EngineFeatures {
            .vk_layer_validation = true,
            .vk_layer_api_dump = true,
        }
    };

    auto device = Device::create(app_info);

    return 0;
}