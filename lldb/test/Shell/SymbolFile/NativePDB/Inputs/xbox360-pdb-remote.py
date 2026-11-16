# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import os
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
        self.stack = bytearray(0x4000)
        struct.pack_into(">II", self.stack, 0x2058, 456, 123)

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
        if num in (1, 31):
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
            offset = addr - 0x10000
            return self.stack[offset : offset + length].hex()
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
test_default_reader = "--default-reader" in sys.argv
check_globals = "--check-globals" in sys.argv
linker_globals = "--linker-globals" in sys.argv
other_frame = "--other-frame" in sys.argv
lldb_args = [
    arg for arg in sys.argv[2:]
    if arg not in ("--default-reader", "--check-globals", "--linker-globals",
                   "--other-frame")
]
server = MockGDBServer(TCPServerSocket())
server.responder = PDBResponder(executable)
server.start()
commands = [
    "settings set interpreter.prompt-on-quit false",
    "settings set stop-disassembly-display never",
    f'target create "{executable}"',
    *(["image dump symfile"] if test_default_reader else []),
    ("breakpoint set -f pdb-probe-real.c -l 13" if linker_globals else
     "breakpoint set -f xbox360-other-globals.cpp -l 4" if other_frame else
     "breakpoint set -f xbox360-pdb.cpp -l 101"),
    "gdb-remote " + server.get_connect_address(),
    f'target modules load -f "{executable}" --slide 0',
    "continue",
    "breakpoint list -v",
    "thread backtrace",
    *(["frame variable value local"] if test_default_reader else []),
    *(["frame variable -g -a -l", "target variable other_global"]
      if check_globals else []),
    *(["frame variable -g -a -l item", "target variable global_data"]
      if linker_globals else [
        "expression -j false -- game::actor.id",
        "expression -j false -- game::actor.team",
        "expression -j false -- game::actor.team = 3",
        "expression -j false -- game::actor.team",
        "expression -j false -- game::actor.enabled",
        "expression -j false -- game::Actor::count",
        "expression -j false -- game::Actor::count = 11",
        "expression -j false -- game::Actor::count",
    ]),
    "process detach",
    "quit",
]
try:
    with tempfile.TemporaryDirectory() as directory:
        command_file = pathlib.Path(directory) / "commands"
        command_file.write_text("\n".join(commands) + "\n")
        env = os.environ.copy()
        if test_default_reader:
            env.pop("LLDB_USE_NATIVE_PDB_READER", None)
        completed = subprocess.run(
            lldb_args
            + ["-b", "-o", f'command source -e true "{command_file}"'],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=60,
            env=env,
        )
    print(completed.stdout)
    if completed.returncode:
        print(server.responder.packetLog)
    assert completed.returncode == 0
    assert "error:" not in completed.stdout
    assert "stop reason = breakpoint 1.1" in completed.stdout
    assert ("pdb-probe-real.c:13" if linker_globals else
            "xbox360-other-globals.cpp:4" if other_frame else
            "xbox360-pdb.cpp:101") in completed.stdout
    assert "resolved = true" in completed.stdout
    assert "hit count = 1" in completed.stdout
    if test_default_reader:
        assert "SymbolFile native-pdb" in completed.stdout
        assert "(int) value = 123" in completed.stdout
        assert "(volatile int) local = 456" in completed.stdout
    if check_globals:
        globals_output = completed.stdout.split(
            "(lldb) frame variable -g -a -l", 1
        )[1].split("(lldb) target variable other_global", 1)[0]
        if other_frame:
            assert "::other_global = 73" in globals_output
            assert "::other_static = 91" in globals_output
            assert "::game::actor" not in globals_output
        else:
            assert "::game::actor" in globals_output
            assert "::game::Actor::count" in globals_output
            assert "other_global" not in globals_output
            assert "other_static" not in globals_output
        assert "(int) other_global = 73" in completed.stdout
    if linker_globals:
        globals_output = completed.stdout.split(
            "(lldb) frame variable -g -a -l item", 1
        )[1].split("(lldb) target variable global_data", 1)[0]
        assert globals_output.count("::item =") == 1
        assert "(int) global_data = 7" in completed.stdout
    else:
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
