{
  src,
  bldcSrc,
  fwBoards ? [ ],
}:

final: prev:
import ./pkgs {
  inherit src bldcSrc fwBoards;
  pkgs = prev;
}
