#! /usr/bin/env python3

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


def all_devices(device_filter):
    """
    List all block devices with a CIRCUITPY volume that match the provided predicate.
    """
    # TODO(dhrosa): We should handle the possiblility of multiple volumes.
    command = "lsblk --output path,label,mountpoint,serial,model,vendor --json --tree"

    # Translate dictionary entries to attributes for readability.
    hook = lambda obj: SimpleNamespace(**obj)
    devices = json.loads(run(command.split()), object_hook=hook).blockdevices

    def is_circuitpy_device(device):
        # Search for volumes with CIRCUITPY label.
        for volume in getattr(device, "children", []):
            if volume.label == "CIRCUITPY":
                return True
        return False

    return [d for d in devices if is_circuitpy_device(d) and device_filter(d)]


def unique_device_and_volume(devices):
    """
    Returns the single device and CIRCUITPY volume. If there isn't strictly one device, we exit the process with an error.
    """
    if len(devices) == 0:
        exit("No CircuitPython devices found.")
    if len(devices) > 1:
        pprint(devices)
        exit("Ambiguous choice of CircuitPython device.")
    device = devices[0]
    return device, device.children[0]


def mount(device_path):
    mount_stdout = run(
        f"udisksctl mount --block-device {device_path} --options noatime".split()
    )
    print(f"udisksctl: {mount_stdout}")


def push_tree(source_dir, dest_dir):
    for source in source_dir.rglob("*"):
        if source.name[0] == "." or source.is_dir():
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


def main():
    parser = ArgumentParser()
    parser.add_argument("--vendor", type=str, default="")
    parser.add_argument("--model", type=str, default="")
    parser.add_argument("--serial", type=str, default="")

    subparsers = parser.add_subparsers(required=True, dest="command")

    list_parser = subparsers.add_parser("list")
    push_parser = subparsers.add_parser("push")
    push_parser.add_argument("source_dir", type=Path, nargs="+")

    args = parser.parse_args()

    def device_filter(device):
        """Predicate for devices matching requested filter."""
        return all(
            (
                args.vendor in device.vendor,
                args.model in device.model,
                args.serial in device.serial,
            )
        )

    devices = all_devices(device_filter)
    if args.command == "list":
        print("Matching devices:")
        pprint(devices)
        exit()

    for source_dir in args.source_dir:
        if not source_dir.exists():
            exit(f"Path '{source_dir}' does not exist.")
        if not source_dir.is_dir():
            exit(f"Path '{source_dir}' is not a directory.")

    device, volume = unique_device_and_volume(devices)

    print("Selected device:")
    pprint(device)

    if volume.mountpoint:
        print(f"Device already mounted at {volume.mountpoint}")
    else:
        mount(volume.path)
        device, volume = unique_device_and_volume(all_devices(device_filter))

    if not volume.mountpoint:
        exit("CIRCUITPY drive not mounted somehow.")
    dest_dir = Path(volume.mountpoint)

    for source_dir in args.source_dir:
        push_tree(source_dir, dest_dir)
    print("Push completed.")


if __name__ == "__main__":
    main()
