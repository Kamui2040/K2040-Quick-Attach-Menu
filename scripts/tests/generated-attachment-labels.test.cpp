#include "AttachmentLabels.h"

#include <cassert>
#include <string>

int main()
{
    using k2040::ChooseGeneratedAttachmentLabel;
    // OMOD FULL wins even when the associated MISC has a different name.
    assert(ChooseGeneratedAttachmentLabel("Hardened Receiver", "Loose Mod #005", "record", "editor") == "Hardened Receiver");
    // No or blank OMOD FULL must still display the linked MISC FULL.
    assert(ChooseGeneratedAttachmentLabel("", "Standard Stock", "record", "editor") == "Standard Stock");
    assert(ChooseGeneratedAttachmentLabel("  \t", "Large Magazine", "record", "editor") == "Large Magazine");
    // No MISC: fall back to humanized record/editor names, then safe default.
    assert(ChooseGeneratedAttachmentLabel("", "", "GunReceiverHardened", "editor") == "GunReceiverHardened");
    assert(ChooseGeneratedAttachmentLabel("", "", " ", "Grip Upgrade") == "Grip Upgrade");
    assert(ChooseGeneratedAttachmentLabel("", "", " ", "\t") == "Unnamed attachment");
    // Preserve UTF-8 text supplied by localized records.
    assert(ChooseGeneratedAttachmentLabel("Präzisionslauf", "Loose Mod", "", "") == "Präzisionslauf");
    return 0;
}
