
#include <ember/Ember.h>
#include <ember/EmberMain.h>
#include <ember/util/Log.h>

using namespace ember;

class SceneE1 final : public core::Scene {

};

int main(int argc, char* argv[]) {
    ember::util::set_global_logger(std::make_unique<ember::util::PrintfLogger>());

    const AppInfo app_info {
        .app_name = "e1",
        .version = 0,
        .window = {
            .width = 1920,
            .height = 1080,
        }
    };
    auto ember = Ember(argc, argv, app_info, std::make_unique<SceneE1>());
    return ember.run();
}
