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

            valgrind
            gf

            sdl3

            vulkan-headers
            vulkan-loader
            vulkan-validation-layers
            vulkan-tools
            vulkan-tools-lunarg

            ktx-tools

            shaderc
            shader-slang

            renderdoc
          ];

          shellHook = ''
            export VK_LAYER_PATH=${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d
            export VK_ADD_LAYER_PATH=$VK_LAYER_PATH
            export LD_LIBRARY_PATH=${pkgs.vulkan-loader}/lib:${pkgs.renderdoc}/lib:$LD_LIBRARY_PATH
          '';

          hardeningDisable = [ "fortify" ];
        };
      }
    );
}
