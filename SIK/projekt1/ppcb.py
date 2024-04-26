import os
import struct
import enum
from typing import Union
from datetime import datetime


class PacketType(enum.IntEnum):
    CONN = 1
    CONNACC = 2
    CONRJT = 3
    DATA = 4
    ACC = 5
    RJT = 6
    RCVD = 7

    def __str__(self):
        return self.name


class ConnType(enum.IntEnum):
    TCP = 1
    UDP = 2
    UDPR = 3

    def __str__(self):
        return self.name


class Packet:
    def __init__(self, packet_type: Union[PacketType, int], session_id: int):
        self.packet_type = packet_type
        self.session_id = session_id

    @staticmethod
    def read(file):
        packet_type = struct.unpack("!B", file.read(1))[0]
        try:
            packet_type = PacketType(packet_type)
        except ValueError:
            pass
        session_id = struct.unpack("!Q", file.read(8))[0]
        packet = Packet(packet_type, session_id)
        if packet.packet_type == PacketType.CONN:
            conn_packet = ConnPacket.read(file, packet)
            return conn_packet
        elif packet.packet_type == PacketType.DATA:
            data_packet = DataPacket.read(file, packet)
            return data_packet
        elif packet.packet_type == PacketType.ACC or packet.packet_type == PacketType.RJT:
            acc_rjt_packet = AccRjtPacket.read(file, packet)
            return acc_rjt_packet
        return packet

    def __str__(self):
        return f"Packet(packet_type={self.packet_type}, session_id={self.session_id})"


class ConnPacket(Packet):
    def __init__(self, packet_type: PacketType, session_id: int, conn_type: ConnType, length: int):
        super().__init__(packet_type, session_id)
        self.conn_type = conn_type
        self.length = length

    @staticmethod
    def read(file, packet):
        conn_type = struct.unpack("!B", file.read(1))[0]
        try:
            conn_type = ConnType(conn_type)
        except ValueError:
            pass
        length = struct.unpack("!Q", file.read(8))[0]
        return ConnPacket(packet.packet_type, packet.session_id, conn_type, length)

    def __str__(self):
        return (f"ConnPacket(packet_type={self.packet_type}, session_id={self.session_id}, conn_type={self.conn_type}, "
                f"length={self.length})")


class DataPacket(Packet):
    def __init__(self, packet_type: PacketType, session_id: int, packet_id: int, data_length: int):
        super().__init__(packet_type, session_id)
        self.packet_id = packet_id
        self.data_length = data_length

    @staticmethod
    def read(file, packet):
        packet_id = struct.unpack("!Q", file.read(8))[0]
        data_length = struct.unpack("!L", file.read(4))[0]
        return DataPacket(packet.packet_type, packet.session_id, packet_id, data_length)

    def __str__(self):
        return (f"DataPacket(packet_type={self.packet_type}, session_id={self.session_id}, packet_id={self.packet_id}, "
                f"data_length={self.data_length})")


class AccRjtPacket(Packet):
    def __init__(self, packet_type: PacketType, session_id: int, packet_id: int):
        super().__init__(packet_type, session_id)
        self.packet_id = packet_id

    @staticmethod
    def read(file, packet):
        packet_id = struct.unpack("!Q", file.read(8))[0]
        return AccRjtPacket(packet.packet_type, packet.session_id, packet_id)

    def __str__(self):
        return f"AccRjtPacket(packet_type={self.packet_type}, session_id={self.session_id}, packet_id={self.packet_id})"


class Entry:
    def __init__(self, time: datetime, _dir: str, packet: Packet):
        self.time = time
        self.dir = _dir
        self.packet = packet

    def __str__(self):
        return f"Entry(time={self.time}, dir={self.dir}, packet={self.packet})"

    @staticmethod
    def read(file):
        time = struct.unpack("=Q", file.read(8))[0]
        time = datetime.fromtimestamp(int(time))
        _dir = struct.unpack("=b", file.read(1))[0]
        _dir = chr(_dir)
        packet = Packet.read(file)
        return Entry(time, _dir, packet)


def read(file_path: str):
    with open(file_path, "rb") as file:
        end = os.fstat(file.fileno()).st_size
        while file.tell() < end:
            try:
                yield Entry.read(file)
            except ValueError | struct.error:
                break


if __name__ == "__main__":
    for entry in read("/tmp/server.ppcb"):
        print(entry)
