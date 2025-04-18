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
          overlays = [ self.overlays.default ];
        };
        treefmtEval = treefmt-nix.lib.evalModule pkgs ./treefmt.nix;
      in
      {
        packages = {
          inherit (pkgs)
            vesc-tool
            vesc-tool-free
            vesc-tool-copper
            vesc-tool-bronze
            vesc-tool-silver
            vesc-tool-gold
            vesc-tool-platinum
            ;
          bldc-fw = pkgs.callPackage ./pkgs/vesc-tool/bldc-fw.nix { src = bldcSrc; };
          default = pkgs.vesc-tool;
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
