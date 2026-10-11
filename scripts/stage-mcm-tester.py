#!/usr/bin/env python3
"""Stage a self-contained, untested Quick Attach MCM-owner ZIP. Never deploy."""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from zipfile import ZipFile,ZIP_DEFLATED
ROOT=Path(__file__).resolve().parents[1]
PARENT="Data/MCM/Config/K2040_Mods/config.json"
CHILD="Data/MCM/Config/K2040_Quick_Attach_Menu/"
DLL="Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll"
PLUGIN_INI="Data/F4SE/Plugins/K2040_Quick_Attach_Menu.ini"
VIEW="Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/"
EXPECTED="8eccc0c1313d1ec982e6094650835e7f83b42e03c56d8435a81ff55c3aba49eb"

def gather(dist=ROOT/"dist"):
    parent=dist/PARENT
    raw=parent.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=EXPECTED:
        raise ValueError("parent MCM differs from the canonical K2040 template")
    if raw != (ROOT/PARENT).read_bytes():
        raise ValueError("staged and source parent differ")
    if json.loads(raw)["pages"]!=[]:
        raise ValueError("parent may not contain individual mod pages")
    source={}
    for name in (PARENT, CHILD+"config.json", CHILD+"keybinds.json",CHILD+"settings.ini",DLL,PLUGIN_INI):
        path=dist/name
        if not path.is_file() or path.is_symlink():
            raise ValueError("missing or invalid staged file: "+name)
        source[name]=path.read_bytes()
    config=json.loads(source[CHILD+"config.json"])
    keys=json.loads(source[CHILD+"keybinds.json"])
    if config["modName"]!="K2040_Quick_Attach_Menu" or config.get("ownerModName")!="K2040_Mods":
        raise ValueError("child page ownership mismatch")
    if [v["id"] for v in config["pages"][0]["content"] if v.get("type")=="hotkey"]!=["openQuickMenu","openMenuBuilder"]:
        raise ValueError("expected Quick Attach hotkey actions")
    if keys["modName"]!="K2040_Quick_Attach_Menu":
        raise ValueError("keyboard registry must remain child-owned")
    data=source[DLL]
    if len(data)<256 or data[:2]!=b"MZ":
        raise ValueError("missing PE x64 DLL")
    header=struct.unpack_from("<I",data,0x3c)[0]
    if header+32>len(data) or data[header:header+4]!=b"PE\x00\x00":
        raise ValueError("invalid PE image")
    machine=struct.unpack_from("<H",data,header+4)[0]
    flags=struct.unpack_from("<H",data,header+22)[0]
    if machine!=0x8664 or not(flags&0x2000):
        raise ValueError("expected Windows x64 DLL")
    view=dist/VIEW
    if not view.is_dir():raise ValueError("missing PrismaUI views")
    for file in view.rglob("*"):
        if not file.is_file():continue
        if file.is_symlink() or file.suffix not in (".html",".js",".css",".json"):
            raise ValueError("unknown or symlinked PrismaUI asset: "+str(file))
        name=VIEW+file.relative_to(view).as_posix()
        source[name]=file.read_bytes()
    if len([name for name in source if name.startswith(VIEW)])<6:
        raise ValueError("missing PrismaUI view assets")
    return source

def stage(files,path):
    path.parent.mkdir(parents=True,exist_ok=True)
    with ZipFile(path,"x",compression=ZIP_DEFLATED,compresslevel=9) as z:
        for name,contents in sorted(files.items()):
            z.writestr(name,contents)
    with ZipFile(path) as z:
        assert z.testzip() is None
        assert set(z.namelist())==set(files)
        for name,contents in files.items():
            assert z.read(name)==contents
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",required=True,type=Path)
    args=parser.parse_args()
    files=gather()
    digest=stage(files,args.output)
    print("Staged",len(files),"files:",args.output)
    print("SHA-256:",digest)
    print("No game or Amethyst files deployed. Windows x64 export/runtime QA remains separate.")

if __name__=="__main__":main()
