#! /usr/bin/env python3

import subprocess
import json
from pathlib import Path
import shutil
from sys import exit
from os import execlp

from types import SimpleNamespace
from pprint import pprint
from argparse import ArgumentParser
from dataclasses import dataclass, asdict


def run(args):
    """Execute command and return its stdout output."""
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
    """A CircuitPython composite USB device."""

    vendor: str
    model: str
    serial: str

    # Path to partition device.
    partition_path: str = None

    # Path to serial device.
    serial_path: str = None


def get_device_info(path):
    """
    Extract device attributes from udevadm.

    Returns None if the device is not a USB device.
    """
    info = {}
    args = f"udevadm info --query=property --name {path}".split()
    for line in run(args).splitlines():
        key, value = line.split("=", maxsplit=1)
        info[key] = value
    if info.get("ID_BUS", None) != "usb":
        return None
    return info


def all_devices():
    """Finds all USB CircuitPython devices."""

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

    # Find CIRCUITPY partition devices.
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

    # Find serial devices.
    for path in Path("/dev/serial/by-id/").iterdir():
        info = get_device_info(path)
        if info is None:
            continue
        device = find_or_add_device(info)
        device.serial_path = str(path)

    return devices


def get_mountpoint(partition_path):
    """Find mountpoint of given partition device. Returns empty string if not mounted."""
    args = f"lsblk {partition_path} --output mountpoint --noheadings".split()
    return run(args).strip()


def mount_if_needed(partition_path):
    """Mounts the given partition device if needed, and returns the mountpoint."""
    mountpoint = get_mountpoint(partition_path)
    if mountpoint:
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


def copy_tree(source_dir, dest_dir):
    """Copy contents of source directory into destination directory."""
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


def list_command(devices):
    """list subcommand."""
    print("Matching devices:")
    pprint(devices)


def upload_command(device, source_dirs):
    """upload subcommand."""
    print("Uploading to device: ")
    pprint(device)
    for source_dir in source_dirs:
        if not source_dir.exists():
            exit(f"Path '{source_dir}' does not exist.")
        if not source_dir.is_dir():
            exit(f"Path '{source_dir}' is not a directory.")

    dest_dir = Path(mount_if_needed(device.partition_path))

    for source_dir in source_dirs:
        copy_tree(source_dir, dest_dir)
    print("Upload complete.")


def serial_command(device):
    """serial subcommand."""
    print("Launching minicom for ")
    pprint(device)
    execlp("minicom", "minicom", "-D", device.serial_path)


def main():
    parser = ArgumentParser()
    parser.add_argument(
        "--vendor",
        type=str,
        default="",
        help="Filter to devices whose vendor contains this string.",
    )
    parser.add_argument(
        "--model",
        type=str,
        default="",
        help="Filter to devices whose model contains this string.",
    )
    parser.add_argument(
        "--serial",
        type=str,
        default="",
        help="Filter to devices whose serial contains this string.",
    )

    subparsers = parser.add_subparsers(required=True, dest="command")

    subparsers.add_parser("list")
    upload_parser = subparsers.add_parser("upload")
    upload_parser.add_argument("source_dir", type=Path, nargs="+")
    subparsers.add_parser("serial")

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

    match args.command:
        case "list":
            list_command(devices)
        case "upload":
            upload_command(unique_device(devices), args.source_dir)
        case "serial":
            serial_command(unique_device(devices))


if __name__ == "__main__":
    main()
