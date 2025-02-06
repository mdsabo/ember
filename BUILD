#cc_library(
#	name = "lib",
#	srcs = glob(
#		include = ["src/*.cpp"],
#		exclude = ["src/main.cpp"]
#	),
#	hdrs = glob(["include/*.h"]),
#	includes = ["include/"],
#	linkstatic = True,
#    visibility = ["//visibility:public"],
#	deps = ["@glm//:glm"]
#)

cc_binary(
    name = "target",
	srcs = ["src/main.cpp"],
#    deps = [":lib"]
)

cc_test(
	name = "test",
	srcs = glob(["test/*.cpp"]),
	includes = ["test/"],
	deps = ["@catch2//:catch2_main"],
	timeout = "short"
)
