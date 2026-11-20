{
  description = "Packages VESC Tool into a flake.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-26.05";
    flake-utils.url = "github:numtide/flake-utils";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    bldc-fw = {
      url = "github:vedderb/bldc/master";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    # Supplies a Mesa built against the same nixpkgs as VESC Tool, so the app
    # does not load the host's GL driver (which may need a newer glibc).
    nixgl = {
      url = "github:nix-community/nixGL";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.flake-utils.follows = "flake-utils";
    };
  };

  # TODO: Add support for building on/for other systems.
  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      treefmt-nix,
      bldc-fw,
      nixgl,
    }@inputs:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
        treefmtEval = treefmt-nix.lib.evalModule pkgs ./treefmt.nix;
        selfPkgs = import ./pkgs {
          inherit pkgs;
          bldc-fw = bldc-fw.packages.${system}.bldc-fw;
          src = self;
        };
        nixglPkgs = import "${nixgl}/default.nix" {
          inherit pkgs;
          enable32bits = false;
          enableIntelX86Extensions = false;
        };
        # Run the app through nixGLMesa so `nix run` works regardless of the
        # host's NixOS/Mesa version. NVIDIA proprietary users still need
        # nixGLNvidia (impure) instead.
        wrapWithNixGL =
          pkg:
          pkgs.symlinkJoin {
            name = "${pkg.pname}-nixgl-${pkg.version}";
            paths = [ pkg ];
            nativeBuildInputs = [ pkgs.makeWrapper ];
            postBuild = ''
              rm $out/bin/${pkg.pname}
              makeWrapper ${nixglPkgs.nixGLMesa}/bin/nixGLMesa $out/bin/${pkg.pname} \
                --add-flags ${pkg}/bin/${pkg.pname}
            '';
            meta = pkg.meta // {
              mainProgram = pkg.pname;
            };
          };
        vescToolPkgs = pkgs.lib.filterAttrs (n: _: pkgs.lib.hasPrefix "vesc-tool" n) selfPkgs;
        nixglWrappedPkgs = pkgs.lib.optionalAttrs pkgs.stdenv.hostPlatform.isLinux (
          pkgs.lib.mapAttrs' (n: p: pkgs.lib.nameValuePair "${n}-nixgl" (wrapWithNixGL p)) vescToolPkgs
        );
      in
      {
        packages =
          selfPkgs
          // nixglWrappedPkgs
          // {
            default = nixglWrappedPkgs.vesc-tool-nixgl or selfPkgs.vesc-tool;
          };

        # For `nix fmt`
        formatter = treefmtEval.config.build.wrapper;

        checks = {
          # For `nix flake check`
          formatting = treefmtEval.config.build.check self;
        };
      }
    )
    // {
      overlays.default = import ./overlay.nix {
        inherit bldc-fw;
        src = self;
      };
      # For development in the nix repl
      inherit self;
    };
}
