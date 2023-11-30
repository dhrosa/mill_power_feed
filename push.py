#! /usr/bin/env python3

import subprocess
import json
from pathlib import Path
import shutil
from sys import exit

from types import SimpleNamespace
from pprint import pprint
from argparse import ArgumentParser
from dataclasses import dataclass, asdict


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


@dataclass
class Device:
    vendor: str
    model: str
    serial: str

    partition_path: str = None
    serial_path: str = None


def get_device_info(path):
    info = {}
    args = f"udevadm info --query=property --name {path}".split()
    for line in run(args).splitlines():
        key, value = line.split("=", maxsplit=1)
        info[key] = value
    if info.get("ID_BUS", None) != "usb":
        return None
    return info


def all_devices():
    devices = []

    def find_or_add_device(info):
        vendor = info["ID_USB_VENDOR"]
        model = info["ID_USB_MODEL"]
        serial = info["ID_USB_SERIAL_SHORT"]

        for device in devices:
            if (
                device.vendor == vendor
                and device.model == model
                and device.serial == serial
            ):
                return device
        device = Device(vendor, model, serial)
        devices.append(device)
        return device

    for path in Path("/dev/disk/by-id/").iterdir():
        info = get_device_info(path)
        if (
            info is None
            or info["DEVTYPE"] != "partition"
            or info["ID_FS_LABEL"] != "CIRCUITPY"
        ):
            continue
        device = find_or_add_device(info)
        device.partition_path = str(path)

    for path in Path("/dev/serial/by-id/").iterdir():
        info = get_device_info(path)
        if info is None:
            continue
        device = find_or_add_device(info)
        device.serial_path = str(path)

    return devices


def get_mountpoint(partition_path):
    args = f"lsblk {partition_path} --output mountpoint --noheadings".split()
    return run(args).strip()


def mount_if_needed(partition_path):
    mountpoint = get_mountpoint(partition_path)
    if mountpoint:
        print(f"Device already mounted at {mountpoint}.")
        return mountpoint
    mount_stdout = run(
        f"udisksctl mount --block-device {partition_path} --options noatime".split()
    )
    print(f"udisksctl: {mount_stdout}")
    mountpoint = get_mountpoint(partition_path)
    if mountpoint:
        return mountpoint
    exit(f"{partition_path} somehow not mounted.")


def unique_device(devices):
    """
    Returns the single device in the list. If there isn't strictly one device, we exit the process with an error.
    """
    if len(devices) == 0:
        exit("No CircuitPython devices found.")
    if len(devices) > 1:
        pprint(devices)
        exit("Ambiguous choice of CircuitPython device.")
    return devices[0]


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

    devices = [d for d in all_devices() if device_filter(d)]
    if args.command == "list":
        print("Matching devices:")
        pprint(devices)
        exit()

    for source_dir in args.source_dir:
        if not source_dir.exists():
            exit(f"Path '{source_dir}' does not exist.")
        if not source_dir.is_dir():
            exit(f"Path '{source_dir}' is not a directory.")

    device = unique_device(devices)

    print("Selected device:")
    pprint(device)

    dest_dir = Path(mount_if_needed(device.partition_path))

    for source_dir in args.source_dir:
        push_tree(source_dir, dest_dir)
    print("Push complete.")


if __name__ == "__main__":
    main()
