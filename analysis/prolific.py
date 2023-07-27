import pandas as pd
from pandas import DataFrame, Series
from binascii import unhexlify


def keep_field(proto_name, name):
    parts = name.split(".")
    if proto_name == "fake-field-wrapper" and parts[1] == "capdata":
        return parts[1]
    if proto_name == "usb":
        if parts[1] in (
            "transfer_type",
            "src",
        ):
            return parts[1]
    if proto_name == "frame":
        if parts[1] == "time_relative":
            return "t"
        if parts[1] == "number":
            return "frame_number"
    return None


def field_value(field):
    value = field.get("value")
    if value is None:
        return field.get("show")
    if len(value) == 1:
        value = b"0" + value
    return unhexlify(value)


def packet_to_record(packet):
    columns = {}
    for proto in packet.xpath("./proto"):
        proto_name = proto.get("name")
        for field in proto.xpath(".//field[not(child::node())]"):
            name = keep_field(proto_name, field.get("name"))
            if name is None:
                continue
            columns[name] = field_value(field)
    return columns


def bulk_packets():
    from lxml import etree

    root = etree.parse("prolific.pdml").getroot()
    df = DataFrame.from_records(packet_to_record(p) for p in root)

    # Drop packets with no data.
    df = df.dropna()

    # Map transfer types and filter to bulk packets.
    bulk = (
        df.pop("transfer_type")
        .map(int.from_bytes)
        .map({0: "isochronous", 1: "interrupt", 2: "control", 3: "bulk"})
        .eq("bulk")
    )
    df = df[bulk]

    # Boolean host label.
    df["host"] = df.pop("src").eq("host")

    # Each host->device transfer marks a new request.
    df["request_number"] = df.host.cumsum() - 1

    # Explode to single byte per row.
    df.capdata = df.capdata.map(list)
    df = df.explode("capdata")

    # Label each byte within each frame.
    df["byte_number"] = df.groupby(["request_number", "host"]).request_number.cumcount()

    df["data"] = df.pop("capdata")
    df["data_hex"] = df.data.map("{:02X}".format)
    df["data_bin"] = df.data.map("{:08b}".format)

    # 16-bit interpretation of byte pairs
    data16_lsb = df.groupby(["request_number", "host"]).data.shift(-1)
    df["data16"] = (df.data * 256) + data16_lsb
    return df.reset_index(drop=True)


pd.set_option("display.max_rows", 0)
df = bulk_packets()
print(df)
