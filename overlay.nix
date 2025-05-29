{
  src,
  bldcSrc,
  fwBoards ? [ ],
  nixpkgsOld,
}:

final: prev:
import ./pkgs {
  inherit src bldcSrc fwBoards;
  pkgs = prev;
  gcc-arm-embedded-7 = nixpkgsOld.legacyPackages.${prev.system}.gcc-arm-embedded-7;
}
