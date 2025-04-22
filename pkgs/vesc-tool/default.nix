{
  src,
  bldcSrc,
  # One of "original", "platinum", "gold", "silver", "bronze", or "free"
  kind ? "free",
  # List of bldc board names to be built and included, i.e. a list of bldc "fw_*"
  # Makefile targets without the "fw_" prefix.
  # Can also be the string "all", which builds all standard board firmwares.
  fwBoards ? [ ],

  lib,
  pkgs,
}:

let
  firstToUpper =
    str:
    (lib.toUpper (builtins.substring 0 1 str)) + (builtins.substring 1 (builtins.stringLength str) str);
  kindTitleCase = firstToUpper kind;
  executableName = "vesc_tool${if kind == "original" then "" else "_${kind}"}";
  iconPath =
    {
      "original" = "res/version/neutral_v.svg";
      "free" = "res/version/free_v.svg";
      "copper" = "res/version/copper_v.svg";
      "bronze" = "res/version/bronze_v.svg";
      "silver" = "res/version/silver_v.svg";
      "gold" = "res/version/gold_v.svg";
      "platinum" = "res/version/platinum_v.svg";
    }
    .${kind};

  bldc-fw = pkgs.callPackage ./bldc-fw.nix {
    src = bldcSrc;
    inherit fwBoards;
  };
in
pkgs.stdenv.mkDerivation {
  pname = executableName;
  version = src.shortRev or src.dirtyShortRev or src.rev or src.dirtyRev or "unknown";

  meta = with lib; {
    description = "VESC Tool ${kind}, an IDE for controlling and configuring VESC-compatible motor controllers and other devices.";
    platforms = platforms.linux;
  };

  desktopItems = [
    (pkgs.makeDesktopItem {
      name = "com.vesc-project.";
      exec = executableName;
      icon = "vesc_tool_${kind}.svg";
      comment = "IDE for controlling and configuring VESC-compatible motor controllers and other devices.";
      desktopName = "VESC Tool ${kindTitleCase}";
      genericName = "Integrated Development Environment";
      categories = [ "Development" ];
    })
  ];

  inherit src;

  configurePhase = ''
    qmake -config release "CONFIG += release_lin build_${kind}"
  '';
  buildPhase = ''
    mkdir -p ./res/firmwares/
    cp -r ${bldc-fw}/* ./res/firmwares/

    ls -la res/firmwares

    make -j$NIX_BUILD_CORES
  '';
  installPhase = ''
    runHook preInstall

    mkdir -p \
      $out/bin \
      $out/share/icons/hicolor/scalable/apps

    cp build/lin/vesc_tool_* $out/bin/${executableName}
    cp ${iconPath} $out/share/icons/hicolor/scalable/apps/vesc_tool_${kind}.svg
    echo $desktopItems

    runHook postInstall
  '';

  buildInputs = [ pkgs.libsForQt5.qtbase ];

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.libsForQt5.qtbase
    pkgs.libsForQt5.qtquickcontrols2
    pkgs.libsForQt5.qtgamepad
    pkgs.libsForQt5.qtconnectivity
    pkgs.libsForQt5.qtpositioning
    pkgs.libsForQt5.qtserialport
    pkgs.libsForQt5.qtgraphicaleffects
    pkgs.libsForQt5.wrapQtAppsHook

    # Make the desktop icon work
    pkgs.copyDesktopItems
    pkgs.tree
  ];
}
