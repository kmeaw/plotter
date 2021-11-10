with import <nixpkgs> {};
linux.overrideAttrs (
    o: {
        nativeBuildInputs = o.nativeBuildInputs ++ [
            pkgconfig
            SDL
            SDL_gfx
        ];
})
