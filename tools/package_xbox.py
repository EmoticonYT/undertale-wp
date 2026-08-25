#!/usr/bin/env python3
"""
Xbox Dev Mode (UWP) AppX Packager for Butterscotch (Undertale)
Creates an x64 UWP .appx package ready for Xbox Device Portal deployment.
"""

import os
import sys
import zipfile
import hashlib
import base64
import shutil
import xml.etree.ElementTree as ET

BLOCK_SIZE = 64 * 1024  # 64 KB AppX block size

def compute_block_hashes(filepath):
    hashes = []
    with open(filepath, "rb") as f:
        while True:
            block = f.read(BLOCK_SIZE)
            if not block:
                break
            h = hashlib.sha256(block).digest()
            hashes.append(base64.b64encode(h).decode("ascii"))
    return hashes

def build_block_map(files_map, output_blockmap_path):
    root = ET.Element("BlockMap", {
        "xmlns": "http://schemas.microsoft.com/appx/2010/blockmap",
        "HashMethod": "http://www.w3.org/2001/04/xmlenc#sha256"
    })

    for rel_path, full_path in sorted(files_map.items()):
        file_size = os.path.getsize(full_path)
        hashes = compute_block_hashes(full_path)
        
        file_elem = ET.SubElement(root, "File", {
            "Name": rel_path.replace("/", "\\"),
            "Size": str(file_size),
            "LfhSize": "30"
        })
        for b_hash in hashes:
            b_elem = ET.SubElement(file_elem, "Block", {"Hash": b_hash})

    tree = ET.ElementTree(root)
    tree.write(output_blockmap_path, encoding="utf-8", xml_declaration=True)

def package_appx(project_dir, binary_path, output_appx_path):
    print("=== Packaging Xbox Dev Mode UWP AppX ===")
    staging_dir = os.path.join(project_dir, "build_xbox_staging")
    if os.path.exists(staging_dir):
        shutil.rmtree(staging_dir)
    os.makedirs(staging_dir, exist_ok=True)

    files_to_pack = {}

    # 1. Manifest
    manifest_src = os.path.join(project_dir, "xbox", "Package.appxmanifest")
    manifest_dst = os.path.join(staging_dir, "AppxManifest.xml")
    shutil.copy2(manifest_src, manifest_dst)
    files_to_pack["AppxManifest.xml"] = manifest_dst

    # 2. Binary
    if os.path.exists(binary_path):
        binary_dst = os.path.join(staging_dir, "Butterscotch.exe")
        shutil.copy2(binary_path, binary_dst)
        files_to_pack["Butterscotch.exe"] = binary_dst
    else:
        print(f"[NOTE] Binary not found at {binary_path}. Generating placeholder for test.")
        binary_dst = os.path.join(staging_dir, "Butterscotch.exe")
        with open(binary_dst, "wb") as f:
            f.write(b"MZ\x90\x00" + b"\x00"*1024)
        files_to_pack["Butterscotch.exe"] = binary_dst

    # 3. Assets
    assets_src_dir = os.path.join(project_dir, "xbox", "Assets")
    assets_dst_dir = os.path.join(staging_dir, "Assets")
    os.makedirs(assets_dst_dir, exist_ok=True)
    for root_dir, _, files in os.walk(assets_src_dir):
        for f in files:
            src_f = os.path.join(root_dir, f)
            rel_asset = os.path.relpath(src_f, project_dir + "/xbox")
            dst_f = os.path.join(staging_dir, rel_asset)
            os.makedirs(os.path.dirname(dst_f), exist_ok=True)
            shutil.copy2(src_f, dst_f)
            files_to_pack[rel_asset.replace("\\", "/")] = dst_f

    # 4. Undertale Game Data
    data_dir = os.path.join(project_dir, "UndertaleData")
    if os.path.exists(data_dir):
        print("Bundling UndertaleData resources...")
        for root_dir, _, files in os.walk(data_dir):
            for f in files:
                if f.endswith(".dll") or f.endswith(".xex") or f == "butterscotch":
                    continue
                src_f = os.path.join(root_dir, f)
                rel_f = os.path.relpath(src_f, project_dir)
                dst_f = os.path.join(staging_dir, rel_f)
                os.makedirs(os.path.dirname(dst_f), exist_ok=True)
                shutil.copy2(src_f, dst_f)
                files_to_pack[rel_f.replace("\\", "/")] = dst_f

    # 5. Generate BlockMap
    blockmap_path = os.path.join(staging_dir, "AppxBlockMap.xml")
    build_block_map(files_to_pack, blockmap_path)
    files_to_pack["AppxBlockMap.xml"] = blockmap_path

    # 6. Create ZIP (APPX)
    os.makedirs(os.path.dirname(os.path.abspath(output_appx_path)), exist_ok=True)
    if os.path.exists(output_appx_path):
        os.remove(output_appx_path)

    print(f"Compressing into {output_appx_path}...")
    with zipfile.ZipFile(output_appx_path, "w", zipfile.ZIP_DEFLATED) as zip_out:
        for rel_name, abs_path in sorted(files_to_pack.items()):
            zip_out.write(abs_path, rel_name)

    print(f"Successfully created: {output_appx_path} ({os.path.getsize(output_appx_path) / (1024*1024):.2f} MB)")
    shutil.rmtree(staging_dir)

if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    bin_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(base_dir, "build", "butterscotch.exe")
    out_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(base_dir, "dist", "Undertale_Xbox.appx")
    package_appx(base_dir, bin_path, out_path)