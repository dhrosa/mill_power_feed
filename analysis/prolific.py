import pandas as pd
from pandas import DataFrame, Series
from binascii import unhexlify
from dataclasses import dataclass
import struct
import crc


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


def bulk_packets(pdml_path):
    from lxml import etree

    root = etree.parse(pdml_path).getroot()
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


def transfers(df):
    s = df.groupby(["host", "request_number"]).data.agg(Series.tolist)
    return pd.concat(dict(host=s[True], device=s[False]), axis=1)


class ChecksumError(ValueError):
    pass


def extract_payload(data):
    payload = bytes(data[:-2])
    checksum = int.from_bytes(data[-2:], byteorder="little")
    expected_checksum = crc.Calculator(crc.Crc16.MODBUS).checksum(payload)
    if checksum != expected_checksum:
        raise ChecksumError(f"{checksum=:04X} vs {expected_checksum=:04X}")
    return payload


def check_eq(label, expected, actual):
    if expected != actual:
        raise ValueError("f{label} {expected=} {actual=}")


@dataclass
class ReadRequest:
    address: int
    offset: int
    count: int

    @staticmethod
    def parse(values):
        payload = extract_payload(values)
        address, function, offset, count = struct.unpack(">BBHH", payload)
        check_eq("function", function, 3)
        return ReadRequest(address, offset, count)


@dataclass
class WriteSingleRequest:
    address: int
    offset: int
    value: int

    @staticmethod
    def parse(data):
        payload = extract_payload(data)
        address, function, offset, value = struct.unpack(">BBHH", payload)
        check_eq("function", function, 6)
        return WriteSingleRequest(address, offset, value)


def parse_request(data):
    function = data[1]
    if function == 3:
        return ReadRequest.parse(data)
    if function == 6:
        return WriteSingleRequest.parse(data)
    return data


def parse_response(data):
    function = data[1]
    if function == 3:
        return ReadResponse.parse(data)
    if function == 6:
        return WriteSingleRequest.parse(data)
    return data


@dataclass
class ReadResponse:
    address: int
    values: list[int]

    @staticmethod
    def parse(data):
        payload = extract_payload(data)
        address, function, expected_count = struct.unpack_from(">3B", payload)
        payload = payload[3:]
        check_eq("function", function, 3)
        check_eq("value count", expected_count, len(payload))
        values = memoryview(payload).cast("H")
        return ReadResponse(address, list(values))


def main():
    import argparse
    from pathlib import Path

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "input_pdml", type=Path, help="Path to Wireshark USB capture PDML file."
    )

    args = parser.parse_args()

    pd.set_option("display.max_rows", 0)
    pd.set_option("display.max_columns", 0)
    pd.set_option("display.max_colwidth", None)
    df = bulk_packets(args.input_pdml).pipe(transfers)
    df.host = df.host.map(parse_request)
    df.device = df.device.map(parse_response)

    print(df)


if __name__ == "__main__":
    main()
