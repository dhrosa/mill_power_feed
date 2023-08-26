import subprocess
import json
from pathlib import Path
import shutil
from sys import exit

from types import SimpleNamespace
from pprint import pprint
from argparse import ArgumentParser
from dataclasses import dataclass


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


def all_devices():
    command = "lsblk --output path,label,mountpoint,serial,model,vendor --json --tree"

    def hook(obj):
        return SimpleNamespace(**obj)

    return json.loads(run(command.split()), object_hook=hook).blockdevices


def circuitpy_device(device_filter):
    def has_circuitpy_child(d):
        for c in getattr(d, "children", []):
            if c.label == "CIRCUITPY":
                return True
        return False

    devices = [d for d in all_devices() if has_circuitpy_child(d) and device_filter(d)]
    if len(devices) == 0:
        exit("No CircuitPython devices found.")
        return 1
    if len(devices) > 1:
        pprint(devices)
        exit("Ambiguous choice of CircuitPython device.")
    parent = devices[0]
    child = parent.children[0]
    return parent, child


@dataclass
class Filter:
    vendor: str
    model: str
    serial: str

    def __call__(self, device):
        return all(
            (
                self.vendor in device.vendor,
                self.model in device.model,
                self.serial in device.serial,
            )
        )


def mount(device_path):
    mount_stdout = run(f"udisksctl mount --block-device {device_path}".split())
    print(f"udisksctl: {mount_stdout}")


def main():
    parser = ArgumentParser()
    parser.add_argument("--vendor", type=str, default="")
    parser.add_argument("--model", type=str, default="")
    parser.add_argument("--serial", type=str, default="")

    args = parser.parse_args()
    device_filter = Filter(args.vendor, args.model, args.serial)
    parent, child = circuitpy_device(device_filter)

    print("Selected device:")
    pprint(parent)

    if child.mountpoint:
        print(f"Device already mounted at {child.mountpoint}")
    else:
        mount(child.path)
        parent, child = circuitpy_device(device_filter)

    if not child.mountpoint:
        exit("CIRCUITPY drive not mounted somehow.")

    this_file = Path(__file__)
    source_dir = this_file.parent
    dest_dir = Path(child.mountpoint)

    for source in this_file.parent.rglob("*"):
        if source == this_file or source.name[0] == "." or source.is_dir():
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
