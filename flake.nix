{
  description = "troglodyte vulkan dev env";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      utils,
    }:

    utils.lib.eachDefaultSystem (
      system:

      let
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_22;
      in
      {
        devShells.default = pkgs.mkShell.override { stdenv = llvm.libcxxStdenv; } {
          nativeBuildInputs = with pkgs; [
            xmake

            cppcheck

            llvm.llvm
            llvm.lldb
            llvm.clang-tools

            pkg-config

            gdb
            gf

            shaderc
            shader-slang

            vulkan-tools
            vulkan-tools-lunarg

            renderdoc
          ];

          buildInputs = with pkgs; [
            tinyobjloader

            sdl3

            vulkan-headers
            vulkan-validation-layers

            vulkan-memory-allocator

            vulkan-volk

            ktx-tools

            glm
          ];

          VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
          VULKAN_SDK = "${pkgs.vulkan-headers}";

          shellHook = ''
            export LD_LIBRARY_PATH="/run/opengl-driver/lib"
          '';

          # export LD_LIBRARY_PATH = "${
          #   pkgs.lib.makeLibraryPath [ pkgs.vulkan-loader ]
          # }''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

          hardeningDisable = [ "fortify" ];
        };
      }
    );
}
