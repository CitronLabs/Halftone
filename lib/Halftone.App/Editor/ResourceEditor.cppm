module;

export module com.citronlabs.Halftone.App.Editor:ResourceEditor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;

export namespace Halftone::App::Editor
{
    // Tabbed editor for an item's asset packs, build systems, macros and
    // configs, including read-only rows inherited from parent scopes. Used by
    // the Project Browser inspector and the scene editor's Scene Settings.
    // Returns true if the item's own resources were modified.
    bool DrawResourceEditor(Halftone::Project::Item& item);
}
