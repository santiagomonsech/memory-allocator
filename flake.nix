{
  description = "C memory allocator development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          name = "memory-allocator";

          packages = with pkgs; [
            gcc
            gnumake
            gdb
            valgrind
            strace
            clang-tools
            cppcheck
            man-pages
            man-pages-posix
          ];

          hardeningDisable = [ "all" ];

          shellHook = ''
            echo "memory-allocator ready"
            echo "  gcc      : $(gcc --version | head -1)"
            echo "  valgrind : $(valgrind --version)"
          '';
        };
      }
    );
}
