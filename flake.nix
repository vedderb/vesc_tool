{
  description = "Packages VESC Tool into a flake.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
    flake-utils.url = "github:numtide/flake-utils";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    bldcSrc = {
      url = "github:vedderb/bldc/release_6_05";
      flake = false;
    };
  };

  # TODO: Add support for building on/for other systems.
  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      treefmt-nix,
      bldcSrc,
    }@inputs:
    flake-utils.lib.eachSystem [ "x86_64-linux" ] (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };
        treefmtEval = treefmt-nix.lib.evalModule pkgs ./treefmt.nix;
        selfPkgs = import ./pkgs {
          inherit pkgs bldcSrc;
          src = self;
        };
      in
      {
        packages = selfPkgs // {
          default = selfPkgs.vesc-tool;
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
      overlays.default = (nixpkgs.lib.makeOverridable (import ./overlay.nix)) {
        inherit bldcSrc;
        src = self;
      };
      # For development in the nix repl
      inherit self;
    };
}
