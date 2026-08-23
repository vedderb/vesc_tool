{
  src,
  bldc-fw,
  # One of "original", "platinum", "gold", "silver", "bronze", or "free"
  kind ? "free",

  lib,
  cmake,
  copyDesktopItems,
  libsForQt5,
  makeDesktopItem,
  stdenv,
  tree,
}:

let
  firstToUpper =
    str:
    (lib.toUpper (builtins.substring 0 1 str)) + (builtins.substring 1 (builtins.stringLength str) str);
  kindTitleCase = firstToUpper kind;
  executableName = "vesc_tool${if kind == "original" then "" else "_${kind}"}";
  releaseConfig = if stdenv.hostPlatform.isDarwin then "release_macos" else "release_lin";
  qmakePlatformArgs = lib.optionalString stdenv.hostPlatform.isDarwin (
    "-after QMAKE_APPLE_DEVICE_ARCHS=${stdenv.hostPlatform.darwinArch}"
  );
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

in
stdenv.mkDerivation {
  pname = executableName;
  version = src.shortRev or src.dirtyShortRev or src.rev or src.dirtyRev or "unknown";

  meta = with lib; {
    description = "VESC Tool ${kind}, an IDE for controlling and configuring VESC-compatible motor controllers and other devices.";
    platforms = platforms.linux ++ platforms.darwin;
  };

  desktopItems = [
    (makeDesktopItem {
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
    qmake -config release "CONFIG += ${releaseConfig} build_${kind}" ${qmakePlatformArgs}
  '';
  buildPhase = ''
    mkdir -p ./res/firmwares/
    cp -r ${bldc-fw}/* ./res/firmwares/

    ls -la res/firmwares

    make -j$NIX_BUILD_CORES
  '';
  installPhase =
    if stdenv.hostPlatform.isDarwin then
      ''
        runHook preInstall

        mkdir -p $out/Applications
        cp -R "build/macos/VESC Tool.app" $out/Applications/

        runHook postInstall
      ''
    else
      ''
        runHook preInstall

        mkdir -p \
          $out/bin \
          $out/share/icons/hicolor/scalable/apps

        cp build/lin/vesc_tool_* $out/bin/${executableName}
        cp ${iconPath} $out/share/icons/hicolor/scalable/apps/vesc_tool_${kind}.svg
        echo $desktopItems

        runHook postInstall
      '';

  buildInputs = [ libsForQt5.qtbase ];

  nativeBuildInputs = [
    cmake
    libsForQt5.qtbase
    libsForQt5.qtquickcontrols2
    libsForQt5.qtgamepad
    libsForQt5.qtconnectivity
    libsForQt5.qtpositioning
    libsForQt5.qtserialport
    libsForQt5.qtgraphicaleffects
    libsForQt5.wrapQtAppsHook

    # Make the desktop icon work
    copyDesktopItems
    tree
  ];
}
