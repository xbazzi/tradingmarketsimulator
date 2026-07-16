{ pkgs, ... }:
let
  llvm = pkgs.llvmPackages_latest;
in
{
  packages =
    [ llvm.clang llvm.clang-tools ]
    ++ (with pkgs; [
      cmake
      ninja
      clang
      pkg-config
      bear
      gdb
      gcc
      boost
      gtest
      gbenchmark
      doxygen
      graphviz
      valgrind
      openssl
    ]);

  enterShell = ''
    echo "Welcome to the TradingMarketSimulator dev shell"
    export CC=''${CC:-gcc}
    export CXX=''${CXX:-g++}
  '';
}
