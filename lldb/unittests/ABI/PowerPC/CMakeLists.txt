add_lldb_unittest(ABIPowerPCTests
  ABIXbox360Test.cpp

  LINK_LIBS
    lldbCore
    lldbHost
    lldbPluginABIPowerPC
    lldbPluginPlatformLinux
    lldbPluginTypeSystemClang
    lldbSymbol
    lldbTarget
    lldbUtility
  LINK_COMPONENTS
    PowerPCDesc
    PowerPCInfo
    Support
  )
