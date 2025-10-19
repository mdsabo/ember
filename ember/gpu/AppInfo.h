#pragma once

namespace ember::gpu {

    struct EngineFeatures {
        bool vk_layer_validation = false;
        bool vk_layer_api_dump = false;
    };

    struct AppInfo {
        const char* name;
        uint32_t version;
        EngineFeatures features;
    };

}