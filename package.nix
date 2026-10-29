{
  lib,
  stdenv,
  # Compile-time options from config.h as -D flags, e.g.
  # `matrixclear.override { defines = [ "DIR_LEFT=1" "COLOR=0xff2d2d" ]; }`.
  defines ? [ ],
}:

stdenv.mkDerivation {
  pname = "matrixclear";
  version = "0.1.0";

  src = lib.fileset.toSource {
    root = ./.;
    fileset = lib.fileset.unions [
      ./Makefile
      ./config.h
      ./src
    ];
  };

  # Structured attrs keep makeFlags a real list: otherwise the single
  # CPPFLAGS=... string is word-split and make sees each -D as an option.
  __structuredAttrs = true;

  makeFlags = lib.optional (defines != [ ]) "CPPFLAGS=${lib.concatMapStringsSep " " (d: "-D${d}") defines}";

  installPhase = ''
    runHook preInstall
    install -Dm755 target/release/matrixclear $out/bin/matrixclear
    runHook postInstall
  '';

  meta = {
    description = "Clear the terminal with streams of glyphs";
    license = lib.licenses.gpl3Plus;
    platforms = lib.platforms.linux;
    mainProgram = "matrixclear";
  };
}
