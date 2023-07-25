import pandas as pd
from pandas import DataFrame, Series
from binascii import unhexlify


def keep_field(proto_name, name):
    parts = name.split(".")
    if proto_name == "fake-field-wrapper" and parts[1] == "capdata":
        return parts[1]
    if proto_name == "usb":
        if parts[1] == "endpoint_address":
            return "endpoint" if parts[2] == "number" else None
        if parts[1] in (
            "transfer_type",
            "urb_status",
            "request_in",
        ):
            return parts[1]
    if proto_name == "frame":
        if parts[1] == "time_relative":
            return "t"
        if parts[1] in ("number", "interface_name"):
            return parts[1]
    return None


def packet_to_record(packet):
    columns = {}
    for proto in packet.xpath("./proto"):
        proto_name = proto.get("name")
        for field in proto.xpath(".//field[not(child::node())]"):
            name = keep_field(proto_name, field.get("name"))
            if name is None:
                continue
            if value := field.get("value"):
                if len(value) == 1:
                    value = "0" + value
                value = unhexlify(value)
                if name != "capdata":
                    value = int.from_bytes(
                        value, byteorder="little", signed=name == "urb_status"
                    )
            else:
                value = field.get("show")
            columns[name] = value
    return columns


def packets():
    from lxml import etree
    from errno import errorcode

    tree = etree.parse("prolific.pdml")
    root = tree.getroot()

    df = DataFrame.from_records(packet_to_record(p) for p in root)
    # Filter to only single interface.
    df = df[df.interface_name == "usbmon0"].drop(columns="interface_name")
    # Map urb_status errno values.
    df = df.assign(
        urb_status=df.urb_status.map(
            lambda e: "eok" if e == 0 else errorcode.get(-e).lower()
        )
    )
    # Map transfer types
    df = df.assign(
        transfer_type=df.transfer_type.map(
            {0: "isochronous", 1: "interrupt", 2: "control", 3: "bulk"}
        )
    )
    # Shift frame number to index.
    return df.set_index("number")


df = packets()

requests = df[df.request_in.isna()].drop(columns=["request_in", "urb_status"])
responses = df[df.request_in.notna()].set_index("request_in")[["urb_status", "capdata"]]
convos = requests.join(responses, lsuffix="_host", rsuffix="_device")
transfers = convos[convos.transfer_type == "bulk"].drop(
    columns=["transfer_type", "endpoint", "urb_status"]
)

transfers = transfers.fillna({"capdata_device": bytes()})

# Unique index that increments each time there's a host->device transfer.
transfers = transfers.assign(transfer_index=transfers.capdata_host.notna().cumsum())

# Aggregate device->host payloads together per transfer.
transfers = (
    transfers.groupby("transfer_index")
    .agg(
        {"t": "first", "capdata_host": "first", "capdata_device": lambda x: b"".join(x)}
    )
    .tail(-1)
)


def pretty_hex(bs):
    return bs.hex(" ")


print(
    pd.concat(
        dict(
            t=transfers.t,
            host=transfers.capdata_host.map(pretty_hex),
            device=transfers.capdata_device.map(pretty_hex),
        ),
        axis=1,
    ).to_string(justify="left", index=False)
)
