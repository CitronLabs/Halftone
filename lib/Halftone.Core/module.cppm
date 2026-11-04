module;

export module com.citronlabs.Halftone.Core;

export import com.citronlabs.Halftone.Core.JSON;

export import :Types;
export import :Math;
export import :Time;
export import :Log;


// Note: the vendored JSON (lib/Halftone.Core/JSON) is patched to live in
// Halftone::Core, so `Halftone::Core::json` is available to every importer.
