add_lldb_library(lldbPluginABIPowerPC PLUGIN
  ABIPowerPC.cpp
  ABISysV_ppc.cpp
  ABISysV_ppc64.cpp
  ABIXbox360.cpp

  LINK_LIBS
    lldbCore
    lldbSymbol
    lldbTarget
    lldbPluginTypeSystemClang
    lldbValueObject
  CLANG_LIBS
    clangAST
  LINK_COMPONENTS
    Support
    TargetParser
  )
