import subprocess
import json
import sys
from pathlib import Path
import shutil


def run(args):
    process = subprocess.run(args, capture_output=True, text=True)
    try:
        process.check_returncode()
    except subprocess.CalledProcessError as e:
        print(f"{args[0]} exited with status {process.returncode}")
        if process.stdout:
            print(f"stdout:\n{process.stdout}")
        if process.stderr:
            print(f"stderr:\n{process.stderr}")
        raise
    return process.stdout


def circuitpy_drive():
    command = "lsblk -o name,label,mountpoint -p --json -l"
    devices = json.loads(run(command.split()))
    for device in devices["blockdevices"]:
        if device["label"] == "CIRCUITPY":
            return device["name"], device["mountpoint"]
    return None, None


def mount(device):
    mount_stdout = run(f"udisksctl mount --block-device {device}".split())
    print(f"udisksctl: {mount_stdout}")


def main():
    device, mountpoint = circuitpy_drive()
    if device is None:
        print("Could not find CIRCUITPY drive.")
        sys.exit(1)
    if mountpoint:
        print(f"{device} is already mounted at {mountpoint}")
    else:
        mount(device)

    device, mountpoint = circuitpy_drive()
    if mountpoint is None:
        print(f"Can't find mountpoint of {device}")
        sys.exit(1)

    this_file = Path(__file__)
    source_dir = this_file.parent
    dest_dir = Path(mountpoint)

    for source in this_file.parent.rglob("*"):
        if source == this_file or source.name[0] == ".":
            # print(f'Skipping {source}')
            continue
        rel_path = source.relative_to(source_dir)
        dest = dest_dir / rel_path
        dest.parent.mkdir(parents=True, exist_ok=True)
        if dest.exists():
            # Round source timestamp to 2s resolution to match FAT drive.
            source_mtime = (source.stat().st_mtime // 2) * 2
            dest_mtime = dest.stat().st_mtime
            if source_mtime == dest_mtime:
                continue
        print(f"Copying {rel_path}")
        shutil.copy2(source, dest)

    print("Push completed.")


if __name__ == "__main__":
    main()
