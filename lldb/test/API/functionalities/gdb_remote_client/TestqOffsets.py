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

    def check_library_stop(self, stop_on_events, reason):
        class LibraryStopResponder(TestqOffsets.Responder):
            def __init__(self):
                super().__init__()
                self.continues = 0
                self.library_reads = 0

            def qSupported(self, client_supported):
                return super().qSupported(client_supported) + ";qXfer:libraries:read+"

            def qXferRead(self, obj, annex, offset, length):
                if obj == "libraries":
                    self.library_reads += 1
                    return "<library-list/>", False
                return None, False

            def qfThreadInfo(self):
                return "m1"

            def haltReason(self):
                return "T05thread:1;"

            def cont(self):
                self.continues += 1
                if self.continues == 1:
                    return "T05thread:1;reason:%s;library:;" % reason
                return "T02thread:1;"

        responder = LibraryStopResponder()
        self.server.responder = responder
        self.runCmd(
            "settings set target.process.stop-on-sharedlibrary-events %s"
            % str(stop_on_events).lower()
        )
        self.dbg.SetAsync(False)
        target = self.createTarget("qOffsets.yaml")
        process = self.connect(target)
        reads_before = responder.library_reads
        self.assertSuccess(process.Continue())
        self.assertGreater(responder.library_reads, reads_before)
        if reason == "exception":
            self.assertEqual(responder.continues, 1)
            self.assertEqual(
                process.GetSelectedThread().GetStopReason(), lldb.eStopReasonException
            )
        elif stop_on_events:
            self.assertEqual(responder.continues, 1)
            self.assertEqual(
                process.GetSelectedThread().GetStopReason(),
                lldb.eStopReasonInstrumentation,
            )
        else:
            self.assertEqual(responder.continues, 2)
            self.assertEqual(
                process.GetSelectedThread().GetStopReason(), lldb.eStopReasonSignal
            )

    @skipIfXmlSupportMissing
    def test_library_event_auto_resume(self):
        self.check_library_stop(False, "library")

    @skipIfXmlSupportMissing
    def test_library_event_stop_setting(self):
        self.check_library_stop(True, "library")

    @skipIfXmlSupportMissing
    def test_exception_with_library_change_does_not_resume(self):
        self.check_library_stop(False, "exception")
