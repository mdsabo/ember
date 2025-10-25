#include <SDL3/SDL_main.h>

#include <memory>

#include "MeshViewer.h"
#include "ember/util/Log.h"

int main(int argc, char* argv[]) {
    ember::util::set_global_logger(std::make_unique<ember::util::PrintfLogger>());

    // if (argc < 2) {
    //     error(MESHVIEWER_LOG, "Missing file path arg!");
    //     exit(-1);
    // };

    MeshViewer("C:\\Users\\slabo\\Code\\ember\\tools\\meshviewer\\assets\\cube.fbx").run();
    return 0;
}