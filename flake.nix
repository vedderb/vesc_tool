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
  };

  # TODO: Add support for building on/for other systems.
  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      treefmt-nix,
      bldc-fw,
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
      overlays.default = import ./overlay.nix {
        inherit bldc-fw;
        src = self;
      };
      # For development in the nix repl
      inherit self;
    };
}
