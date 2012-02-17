import qbs.fileinfo 1.0 as FileInfo

Module {
  FileTagger {
    pattern: "*.proto"
    fileTags: ["proto"]
  }

  Rule {
    inputs: ["proto"]
    
    Artifact {
      id: foo
      fileName: "GeneratedFiles/" + product.name + "/" + input.baseName + ".pb.cc"
      fileTags: ["cpp"]
    }

    Artifact {
      fileName: "GeneratedFiles/" + product.name + "/" + input.baseName + ".pb.h"
    }

    prepare: {
      var cmd = new Command("protoc", [
        "--proto_path",
        FileInfo.path(input.fileName),
        "--cpp_out",
        FileInfo.path(outputs.cpp[0].fileName),
        input.fileName,
      ]);
      cmd.description = "protoc " + FileInfo.fileName(input.fileName);
      cmd.highlight = "codegen";
      return cmd;
    }
  }

  Depends {name: "cpp"}
  cpp.includePaths: [product.buildDirectory + "/GeneratedFiles/" + product.name]
  cpp.dynamicLibraries: ["protobuf"]
}