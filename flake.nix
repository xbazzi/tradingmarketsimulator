{
  description = "C and C++";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_latest;
      in
      {
        devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            clang
            llvm.clang
            llvm.clang-tools
            pkg-config
            bear
            gdb
          ];

          buildInputs = with pkgs; [
            gcc
            boost
            gtest
            gbenchmark
            doxygen
            graphviz
            valgrind
            openssl
          ];
          shell = pkgs.zsh;
          shellHook = ''
            echo "Welcome to the MarketMakerSimulator flake dev shell"
            export CC=gcc
            export CXX=g++
          '';
        };
      }
    );
}
