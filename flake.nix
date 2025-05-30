{
  description = "Packages VESC Tool into a flake.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.05";
    # gcc-arm-embedded-7 has been removed from nixpkgs since 25.05 since it's
    # old and unmaintained. However, this project still uses that version, so we
    # include an older version of nixpkgs to access it.
    nixpkgsOld.url = "github:nixos/nixpkgs/nixos-24.11";
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
      nixpkgsOld,
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
          gcc-arm-embedded-7 = nixpkgsOld.legacyPackages.${system}.gcc-arm-embedded-7;
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
        inherit bldcSrc nixpkgsOld;
        src = self;
      };
      # For development in the nix repl
      inherit self;
    };
}
