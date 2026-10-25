#include "WeaponPreviewPath.h"
#include <cassert>
#include <string>

int main()
{
    using k2040::NormalizeWeaponPreviewNifPath;
    assert(NormalizeWeaponPreviewNifPath("Weapons\\10mmPistol\\10mmPistol.nif") ==
        "Meshes/Weapons/10mmPistol/10mmPistol.nif");
    assert(NormalizeWeaponPreviewNifPath("meshes/Weapons/pistol.NIF") == "Meshes/Weapons/pistol.NIF");
    assert(NormalizeWeaponPreviewNifPath("Meshes/Weapons/laser.nif") == "Meshes/Weapons/laser.nif");
    for (const auto& invalid : {"", "../../../private.nif", "Meshes/../secret.nif",
        "Meshes//Weapons/pistol.nif", "/tmp/pistol.nif", "C:/pistol.nif",
        "Data/Weapons/10mm.nif", "Weapons/file.txt", "Weapons/pistol.nif?x=1",
        "Weapons/pistol\n.nif", "Weapons/./pistol.nif", "Weapons/pistol.nif/"}) {
        assert(NormalizeWeaponPreviewNifPath(invalid).empty());
    }
    assert(NormalizeWeaponPreviewNifPath(std::string(241, 'x') + ".nif").empty());
}
