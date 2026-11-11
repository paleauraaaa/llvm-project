# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import json
import pathlib
import subprocess
import sys
import tempfile

sys.path.insert(
    0, str(pathlib.Path(__file__).resolve().parents[4] / "packages" / "Python")
)
from lldbsuite.test.gdbclientutils import (
    MockGDBServer,
    MockGDBServerResponder,
    TCPServerSocket,
)


class ExceptionResponder(MockGDBServerResponder):
    def __init__(self, triple, json_stop):
        super().__init__()
        self.triple = triple
        self.json_stop = json_stop
        self.stop = 0
        self.thread = 2
        self.reasons = [
            ("exception", "guest access violation", 11),
            ("exception", "guest breakpoint exception", 5),
            ("trace", "", 5),
            ("", "", 4),
            ("exception", "", 5),
        ]
        words = [0xC0000005, 1, 0x10000, 0x82001000, 2, 1, 0xDEADBEEF]
        self.record = b"".join(word.to_bytes(4, "big") for word in words).ljust(
            512, b"\0"
        )

    def qHostInfo(self):
        size = 8 if self.triple.startswith("powerpc64") else 4
        return f"ptrsize:{size};endian:big;triple:" + self.triple.encode().hex() + ";"

    def qProcessInfo(self):
        return "pid:1;" + self.qHostInfo()

    def qRegisterInfo(self, num):
        regs = [
            ("pc", 32, 0, "pc"),
            ("r1", 64, 4, "sp"),
            ("r3", 64, 12, "arg1"),
            ("lr", 64, 20, "ra"),
        ]
        if num >= len(regs):
            return "E01"
        name, size, offset, generic = regs[num]
        return (
            f"name:{name};bitsize:{size};offset:{offset};encoding:uint;"
            f"format:hex;set:GPR;generic:{generic};"
        )

    def readRegister(self, num):
        return [
            f"{0x82001000 + self.stop * 4:08x}",
            "0000000000012000",
            "0000000000010000",
            "0000000082001004",
        ][num]

    def readRegisters(self):
        return "".join(self.readRegister(i) for i in range(4))

    def qC(self):
        return "QC2"

    def qfThreadInfo(self):
        return "m1,2"

    def selectThread(self, op, thread_id):
        if thread_id in (1, 2):
            self.thread = thread_id
        return "OK"

    def haltReason(self):
        reason, description, signal = self.reasons[self.stop]
        pc = self.readRegister(0)
        reply = f"T{signal:02x}thread:2;00:{pc};"
        if self.json_stop:
            info = [{
                "tid": 2,
                "reason": reason,
                "description": description,
                "signal": signal,
                "registers": {"0": pc},
            }]
            # A different primary stop thread exercises lazy JSON stop decoding.
            return (
                f"T0bthread:1;00:{pc};jstopinfo:"
                + json.dumps(info).encode().hex() + ";"
            )
        if reason:
            reply += f"reason:{reason};"
        if description:
            reply += "description:" + description.encode().hex() + ";"
        return reply

    def respond(self, packet):
        if packet == "s" or packet == "c" or packet.startswith("vCont;"):
            self.stop += 1
            assert self.stop < len(self.reasons)
            return self.haltReason()
        return super().respond(packet)

    def readMemory(self, addr, length):
        if 0x82001000 <= addr < 0x82002000:
            return (bytes.fromhex("60000000") * ((length + 3) // 4))[:length].hex()
        if 0x10000 <= addr and addr + length <= 0x10000 + len(self.record):
            return self.record[addr - 0x10000 : addr - 0x10000 + length].hex()
        return "E01"


triple = sys.argv[1]
json_stop = sys.argv[2] == "json"
server = MockGDBServer(TCPServerSocket())
server.responder = ExceptionResponder(triple, json_stop)
server.start()
commands = [
    "settings set interpreter.prompt-on-quit false",
    "gdb-remote " + server.get_connect_address(),
    "thread exception",
    "thread info -s",
    "thread exception-record 0x10000",
    "thread exception-record 0x10001",
    "thread exception-record 0x20000",
    "process handle -s false -n false -p false SIGTRAP",
    "thread step-inst",
    "thread exception",
    "thread info -s",
    "process handle -s true -n true -p false SIGTRAP",
    "process handle -s true -n true -p false SIGILL",
    "thread step-inst",
    "thread info -s",
    "continue",
    "thread info -s",
    "continue",
    "thread exception",
    "thread info -s",
    "process detach",
    "quit",
]
if json_stop:
    commands = [
        "settings set interpreter.prompt-on-quit false",
        "gdb-remote " + server.get_connect_address(),
        "thread select 2",
        "thread exception",
        "thread info -s",
        "process detach",
        "quit",
    ]
try:
    with tempfile.TemporaryDirectory() as directory:
        command_file = pathlib.Path(directory) / "commands"
        command_file.write_text("\n".join(commands) + "\n")
        completed = subprocess.run(
            sys.argv[3:] + [
                "-b", "-o", f'command source -e false "{command_file}"'
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=60,
        )
    print(completed.stdout)
    assert completed.returncode == 0
    assert "first_chance" not in completed.stdout
    assert "context_address" not in completed.stdout
    if json_stop:
        assert "thread #2: guest access violation" in completed.stdout
        assert completed.stdout.count('"type": "xbox360"') == 2
        assert '"thread_id": 2' in completed.stdout
        assert '"signal": 11' in completed.stdout
        assert "error:" not in completed.stdout
        sys.exit(0)
    assert "thread #1: guest access violation" in completed.stdout
    assert "thread #1: guest breakpoint exception" in completed.stdout
    assert "thread #1: exception" in completed.stdout
    if "xbox360" in triple:
        assert completed.stdout.count("error:") == 2
        assert "write at 0xdeadbeef (noncontinuable)" in completed.stdout
        assert "invalid Xbox 360 exception record address" in completed.stdout
        assert "cannot read Xbox 360 exception record" in completed.stdout
        # Metadata belongs only to exception events, not subsequent trace/signal
        # stops. Both the command and thread-info paths dump it.
        assert completed.stdout.count('"type": "xbox360"') == 6
        assert '"signal": 11' in completed.stdout
        assert '"signal": 5' in completed.stdout
        assert '"signal": 4' not in completed.stdout
        assert '"pc": 2181042176' in completed.stdout
    else:
        assert completed.stdout.count("error:") == 3
        assert '"type": "xbox360"' not in completed.stdout
        assert "not supported by this ABI" in completed.stdout
    assert server.responder.stop == 4
finally:
    server.stop()
