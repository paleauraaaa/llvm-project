# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import pathlib
import subprocess
import sys

sys.path.insert(
    0, str(pathlib.Path(__file__).resolve().parents[4] / "packages" / "Python")
)
from lldbsuite.test.gdbclientutils import (
    MockGDBServer,
    MockGDBServerResponder,
    TCPServerSocket,
)


class XboxTLSResponder(MockGDBServerResponder):
    def __init__(self):
        super().__init__()
        self.thread = 1
        self.failed_runtime_tls_read = False
        self.memory = bytearray(0x4000)
        self.put32(0x10000, 0x13800)
        self.put32(0x10100, 0x11000)
        self.put32(0x11068, 0x12000)
        self.put32(0x10300, 0x11200)
        self.put32(0x11268, 0x13000)
        self.put32(0x12000, 0x12345678)
        self.put32(0x123FC, 0xFEDCBA98)
        self.put32(0x13000, 0xAABBCCDD)

    def put32(self, address, value):
        self.memory[address - 0x10000 : address - 0x10000 + 4] = value.to_bytes(
            4, "big"
        )

    def qHostInfo(self):
        triple = "powerpc-ibm-xbox360".encode().hex()
        return "ptrsize:4;endian:big;triple:" + triple + ";"

    def qProcessInfo(self):
        return "pid:1;" + self.qHostInfo()

    def qRegisterInfo(self, num):
        if num >= 34:
            return "E01"
        name = "r%d" % num if num < 32 else ("pc" if num == 32 else "lr")
        size = 32 if num == 32 else 64
        offset = num * 8 if num <= 32 else 260
        generic = {1: "sp", 13: "tp", 32: "pc", 33: "ra"}.get(num)
        return (
            "name:%s;bitsize:%d;offset:%d;encoding:uint;format:hex;set:GPR;"
            % (name, size, offset)
            + ("generic:%s;" % generic if generic else "")
        )

    def qC(self):
        return "QC1"

    def qfThreadInfo(self):
        return "m1,2"

    def haltReason(self):
        return "T05thread:1;"

    def selectThread(self, op, thread_id):
        if thread_id in (1, 2):
            self.thread = thread_id
        return "OK"

    def readRegister(self, register):
        if register == 1:
            return "0000000000013f00"
        if register == 13:
            return "deadbeef00010000" if self.thread == 1 else "deadbeef00010200"
        if register == 32:
            return "82001000"
        return "0000000000000000"

    def readRegisters(self):
        return "".join(self.readRegister(i) for i in range(34))

    def readMemory(self, addr, length):
        if 0x82001000 <= addr < 0x82002000:
            return (bytes.fromhex("4e800020") * ((length + 3) // 4))[:length].hex()
        if (
            not self.failed_runtime_tls_read
            and addr <= 0x13008 < addr + length
        ):
            self.failed_runtime_tls_read = True
            return "E01"
        if addr < 0x10000 or addr + length > 0x14000:
            return "E01"
        return self.memory[addr - 0x10000 : addr - 0x10000 + length].hex()

    def writeMemory(self, addr, data_hex):
        data = bytes.fromhex(data_hex)
        if addr < 0x10000 or addr + len(data) > 0x14000:
            return "E01"
        self.memory[addr - 0x10000 : addr - 0x10000 + len(data)] = data
        return "OK"


server = MockGDBServer(TCPServerSocket())
server.responder = XboxTLSResponder()
server.start()
commands = [
    "settings set interpreter.prompt-on-quit false",
    "gdb-remote " + server.get_connect_address(),
    "thread tls",
    "thread tls 0",
    "thread tls 255",
    "thread tls 256",
    "thread tls -1",
    "thread tls list 0 2",
    "thread tls list 255",
    "thread tls list 256",
    "thread tls list 0 0",
    "thread tls bind tls",
    "thread tls bind $tls",
    "thread tls bind $tls",
    "thread tls bind $bad_slot 256",
    "thread tls bind $tls_slot 1",
    "expression -j false -- *$tls_slot",
    "expression -j false -- *$tls_slot = 0x11223344",
    "expression -j false -- $tls[0]",
    "expression -j false -- $tls[1] = 0x87654321",
    "thread tls list 1 1",
    "thread tls 1",
    "thread select 2",
    "thread tls list 0 2",
    "thread tls 0",
    "thread tls bind $tls2",
    "expression -j false -- $tls2[0]",
    "expression -j false -- $tls[0]",
    "process detach",
    "quit",
]
try:
    completed = subprocess.run(
        sys.argv[1:],
        input="\n".join(commands) + "\n",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=60,
    )
    print(completed.stdout)
    assert completed.returncode == 0
    assert completed.stdout.count("error:") == 8
    assert completed.stdout.count("305419896") == 2
    for expected in [
        "256 four-byte slots",
        "TLS[0] = 0x12345678",
        "TLS[255] = 0xfedcba98",
        "slot index must be less than 256",
        "starting slot index must be less than 256",
        "slot count must be between 1 and 256",
        "memory read failed",
        "expected a slot index",
        "expected a persistent variable",
        "bound to thread #1",
        "TLS slots [0, 2) for thread #1:",
        "TLS slots [255, 256) for thread #1:",
        "$tls_slot = 0x12004 (TLS[1]; bound to thread #1)",
        "persistent variable already exists",
        "305419896",
        "TLS slots [1, 2) for thread #1:",
        "TLS[1] = 0x87654321",
        "TLS[0] = 0xaabbccdd",
        "bound to thread #2",
        "2864434397",
    ]:
        assert expected in completed.stdout, expected
    assert server.responder.memory[0x2004:0x2008] == bytes.fromhex("87654321")
    assert not any(
        p == "c" or p.startswith("vCont;c") for p in server.responder.packetLog
    )
finally:
    server.stop()
