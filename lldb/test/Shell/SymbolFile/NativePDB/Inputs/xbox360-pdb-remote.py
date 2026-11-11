# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import pathlib
import struct
import subprocess
import sys
import tempfile

sys.path.insert(
    0, str(pathlib.Path(__file__).resolve().parents[5] / "packages" / "Python")
)
from lldbsuite.test.gdbclientutils import (
    MockGDBServer,
    MockGDBServerResponder,
    TCPServerSocket,
)


class PDBResponder(MockGDBServerResponder):
    def __init__(self, executable):
        super().__init__()
        data = executable.read_bytes()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        sections = struct.unpack_from("<H", data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", data, pe + 20)[0]
        optional = pe + 24
        self.base = struct.unpack_from("<I", data, optional + 28)[0]
        self.pc = self.base + struct.unpack_from("<I", data, optional + 16)[0]
        image_size, header_size = struct.unpack_from("<II", data, optional + 56)
        self.image = bytearray(image_size)
        self.image[:header_size] = data[:header_size]
        for i in range(sections):
            section = optional + optional_size + i * 40
            rva, size, offset = struct.unpack_from("<III", data, section + 12)
            self.image[rva : rva + size] = data[offset : offset + size]
        self.breakpoint = None
        self.resumed = False

    def qHostInfo(self):
        return (
            "ptrsize:4;endian:big;triple:"
            + "powerpc-ibm-xbox360".encode().hex()
            + ";"
        )

    def qProcessInfo(self):
        return "pid:1;" + self.qHostInfo()

    def qRegisterInfo(self, num):
        if num >= 34:
            return "E01"
        name = f"r{num}" if num < 32 else ("pc" if num == 32 else "lr")
        size = 32 if num == 32 else 64
        offset = num * 8 if num <= 32 else 260
        generic = {1: "sp", 32: "pc", 33: "ra"}.get(num)
        return (
            f"name:{name};bitsize:{size};offset:{offset};encoding:uint;"
            "format:hex;set:GPR;"
            + (f"generic:{generic};" if generic else "")
        )

    def readRegister(self, num):
        if num == 32:
            return f"{self.pc:08x}"
        if num == 1:
            return "0000000000012000"
        return "0000000000000000"

    def readRegisters(self):
        return "".join(self.readRegister(i) for i in range(34))

    def qC(self):
        return "QC1"

    def qfThreadInfo(self):
        return "m1"

    def haltReason(self):
        return f"T05thread:1;20:{self.pc:08x};"

    def readMemory(self, addr, length):
        offset = addr - self.base
        if 0 <= offset and offset + length <= len(self.image):
            return self.image[offset : offset + length].hex()
        if 0x10000 <= addr and addr + length <= 0x14000:
            return "00" * length
        return "E01"

    def writeMemory(self, addr, data_hex):
        data = bytes.fromhex(data_hex)
        offset = addr - self.base
        if offset < 0 or offset + len(data) > len(self.image):
            return "E01"
        self.image[offset : offset + len(data)] = data
        return "OK"

    def setBreakpoint(self, packet):
        kind, address, size = packet[1:].split(",")
        assert kind == "0" and int(size, 16) == 4, packet
        if packet.startswith("Z"):
            self.breakpoint = int(address, 16)
        return "OK"

    def respond(self, packet):
        if packet == "c" or packet.startswith("vCont;c"):
            assert self.breakpoint is not None
            self.pc = self.breakpoint
            self.resumed = True
            return self.haltReason()
        return super().respond(packet)


executable = pathlib.Path(sys.argv[1]).resolve()
server = MockGDBServer(TCPServerSocket())
server.responder = PDBResponder(executable)
server.start()
commands = [
    "settings set interpreter.prompt-on-quit false",
    "settings set stop-disassembly-display never",
    f'target create "{executable}"',
    "breakpoint set -f xbox360-pdb.cpp -l 101",
    "gdb-remote " + server.get_connect_address(),
    f'target modules load -f "{executable}" --slide 0',
    "continue",
    "breakpoint list -v",
    "thread backtrace",
    "expression -j false -- game::actor.id",
    "expression -j false -- game::actor.team",
    "expression -j false -- game::actor.team = 3",
    "expression -j false -- game::actor.team",
    "expression -j false -- game::actor.enabled",
    "expression -j false -- game::Actor::count",
    "expression -j false -- game::Actor::count = 11",
    "expression -j false -- game::Actor::count",
    "process detach",
    "quit",
]
try:
    with tempfile.TemporaryDirectory() as directory:
        command_file = pathlib.Path(directory) / "commands"
        command_file.write_text("\n".join(commands) + "\n")
        completed = subprocess.run(
            sys.argv[2:]
            + ["-b", "-o", f'command source -e true "{command_file}"'],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=60,
        )
    print(completed.stdout)
    if completed.returncode:
        print(server.responder.packetLog)
    assert completed.returncode == 0
    assert "error:" not in completed.stdout
    assert "stop reason = breakpoint 1.1" in completed.stdout
    assert "xbox360-pdb.cpp:101" in completed.stdout
    assert "resolved = true" in completed.stdout
    assert "hit count = 1" in completed.stdout
    for value in [
        "(int) $0 = 42",
        "$1 = 5",
        "$2 = 3",
        "$3 = 3",
        "$4 = 1",
        "$5 = 9",
        "$6 = 11",
        "$7 = 11",
    ]:
        assert value in completed.stdout, value
    assert server.responder.resumed
finally:
    server.stop()
