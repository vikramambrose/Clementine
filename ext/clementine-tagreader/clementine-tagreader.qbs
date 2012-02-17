import qbs.base 1.0
import "../../cubes/Definitions.js" as Definitions

Application {
  name: "clementine-tagreader"

  Depends {name: "cpp"}
  Depends {
    name: "Qt"
    submodules: [
      "core",
      "network",
    ]
  }

  Depends {name: "libclementine-common"}
  Depends {name: "libclementine-tagreader"}

  files: [
    //"fmpsparser.cpp",
    "main.cpp",
    //"tagreaderworker.cpp",
  ]
}