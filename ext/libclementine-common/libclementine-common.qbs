import qbs.base 1.0
import "../../cubes/Definitions.js" as Definitions

StaticLibrary {
  id: libclementinecommon
  name: "libclementine-common"

  Depends {name: "cpp"}
  Depends {
    name: "Qt"
    submodules: [
      "core",
      "gui",
      "network",
    ]
  }
  cpp.includePaths: [
    Definitions.glib_include_dir,
    Definitions.glib_include_dir_lib,
    Definitions.taglib_include_dir,
    ".",
    "../../src",
    "../../3rdparty/universalchardet"
  ]

  ProductModule {
    cpp.includePaths: libclementinecommon.cpp.includePaths
    cpp.dynamicLibraries: [
      "tag",
      "glib-2.0",
    ]
  }

  files: [
    "core/closure.cpp",
    "core/closure.h",
    "core/encoding.cpp",
    "core/encoding.h",
    "core/logging.cpp",
    "core/logging.h",
    "core/messagehandler.cpp",
    "core/messagehandler.h",
    "core/messagereply.cpp",
    "core/messagereply.h",
    "core/waitforsignal.cpp",
    "core/waitforsignal.h",
    "core/workerpool.cpp",
    "core/workerpool.h",
  ]
}