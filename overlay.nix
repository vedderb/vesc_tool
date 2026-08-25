{
  src,
  bldc-fw,
}:

final: prev:
import ./pkgs {
  inherit src;
  bldc-fw = bldc-fw.packages.${prev.system}.bldc-fw;
  pkgs = final;
}
