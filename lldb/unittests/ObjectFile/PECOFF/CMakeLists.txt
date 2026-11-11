add_lldb_unittest(ObjectFilePECOFFTests
  TestByteOrder.cpp
  TestPECallFrameInfo.cpp
  TestSectionSize.cpp
  TestXbox360CallFrameInfo.cpp

  LINK_LIBS
    lldbUtilityHelpers
    lldbPluginObjectFilePECOFF
    LLVMTestingSupport
  )
