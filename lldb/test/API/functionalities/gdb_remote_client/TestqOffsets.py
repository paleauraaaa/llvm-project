import lldb
from lldbsuite.test.lldbtest import *
from lldbsuite.test.decorators import *
from lldbsuite.test.gdbclientutils import *
from lldbsuite.test.lldbgdbclient import GDBRemoteTestBase


class TestqOffsets(GDBRemoteTestBase):
    class Responder(MockGDBServerResponder):
        def qOffsets(self):
            return "Text=470000;Data=470000"

    def test(self):
        self.server.responder = TestqOffsets.Responder()
        target = self.createTarget("qOffsets.yaml")
        text = target.modules[0].FindSection(".text")
        self.assertEqual(text.GetLoadAddress(target), lldb.LLDB_INVALID_ADDRESS)

        process = self.connect(target)
        self.assertEqual(text.GetLoadAddress(target), 0x471000)

    @skipIfXmlSupportMissing
    def test_library_list_preserves_executable_load_address(self):
        class LibraryResponder(TestqOffsets.Responder):
            def __init__(self, filename):
                super().__init__()
                self.filename = filename

            def qSupported(self, client_supported):
                return (
                    super().qSupported(client_supported)
                    + ";qXfer:libraries:read+"
                )

            def qXferRead(self, obj, annex, offset, length):
                if obj == "libraries":
                    return (
                        '<library-list><library name="%s">'
                        '<section address="0x470000"/>'
                        "</library></library-list>" % self.filename,
                        False,
                    )
                return None, False

            def qfThreadInfo(self):
                return "m1"

            def haltReason(self):
                return "T05thread:1;library:;"

        target = self.createTarget("qOffsets.yaml")
        self.server.responder = LibraryResponder(
            self.getBuildArtifact("qOffsets")
        )
        text = target.modules[0].FindSection(".text")
        process = self.connect(target)
        lldbutil.expect_state_changes(
            self, self.dbg.GetListener(), process, [lldb.eStateStopped]
        )
        self.assertNotEqual(text.GetLoadAddress(target), lldb.LLDB_INVALID_ADDRESS)
