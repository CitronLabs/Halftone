module;

export module com.citronlabs.Halftone.App:Info;


export namespace Halftone::App {

    struct Info {
        Info() = delete;

        inline static const char
            * Name        = "Halftone",
            * Version     = "1.0.0",
            * Engine      = "Chroma",
            * Organization = "CitronLabs";

        inline static constexpr unsigned
            VersionMajor = 1,
            VersionMinor = 0,
            VersionPatch = 0;
    };
};
