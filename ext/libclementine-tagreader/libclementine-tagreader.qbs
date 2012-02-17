import qbs.base 1.0

StaticLibrary {
  id: libclementinetagreader
  name: "libclementine-tagreader"

  Depends {name: "cpp"}
  Depends {name: "protobuf"}

  ProductModule {
    cpp.includePaths: libclementinetagreader.cpp.includePaths
    cpp.dynamicLibraries: libclementinetagreader.cpp.dynamicLibraries
  }

  files: [
    "tagreadermessages.proto",
  ]
}