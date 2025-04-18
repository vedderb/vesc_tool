{
  src,
  bldcSrc,
  fwBoards ? [ ],
}:

final: prev: rec {
  vesc-tool = prev.callPackage ./pkgs/vesc-tool {
    inherit src bldcSrc fwBoards;
    kind = "original";
  };
  vesc-tool-platinum = vesc-tool.override { kind = "platinum"; };
  vesc-tool-gold = vesc-tool.override { kind = "gold"; };
  vesc-tool-silver = vesc-tool.override { kind = "silver"; };
  vesc-tool-bronze = vesc-tool.override { kind = "bronze"; };
  vesc-tool-copper = vesc-tool.override { kind = "copper"; };
  vesc-tool-free = vesc-tool.override { kind = "free"; };
}
