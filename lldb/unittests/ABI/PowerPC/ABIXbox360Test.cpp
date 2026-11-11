//===-- ABIXbox360Test.cpp
//-------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Plugins/ABI/PowerPC/ABIXbox360.h"
#include "Plugins/ABI/PowerPC/ABIPowerPC.h"
#include "Plugins/ABI/PowerPC/ABISysV_ppc.h"
#include "Plugins/ABI/PowerPC/ABISysV_ppc64.h"
#include "Plugins/ExpressionParser/Clang/ClangUtil.h"
#include "Plugins/ObjectFile/PECOFF/ObjectFilePECOFF.h"
#include "Plugins/Platform/Linux/PlatformLinux.h"
#include "Plugins/ScriptInterpreter/None/ScriptInterpreterNone.h"
#include "Plugins/SymbolFile/NativePDB/DWARFLocationExpression.h"
#include "Plugins/TypeSystem/Clang/TypeSystemClang.h"
#include "TestingSupport/SubsystemRAII.h"
#include "TestingSupport/Symbol/ClangTestUtils.h"
#include "TestingSupport/TestUtilities.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/Section.h"
#include "lldb/Core/Value.h"
#include "lldb/Expression/DWARFExpression.h"
#include "lldb/Host/FileSystem.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Symbol/CallFrameInfo.h"
#include "lldb/Symbol/CompilerDeclContext.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/ThreadPlanCallFunction.h"
#include "lldb/Target/UnwindLLDB.h"
#include "lldb/Target/Xbox360RegisterInfo.h"
#include "lldb/Utility/Listener.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/ValueObject/ValueObject.h"
#include "lldb/ValueObject/ValueObjectConstResult.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"
#include <array>
#include <tuple>

using namespace lldb;
using namespace lldb_private;

namespace {
class TestProcess : public Process {
public:
  TestProcess(TargetSP target)
      : Process(target, Listener::MakeListener("Xbox ABI tests")) {}
  ~TestProcess() override { Finalize(/*destructing=*/true); }
  llvm::StringRef GetPluginName() override { return "test"; }
  bool CanDebug(TargetSP, bool) override { return true; }
  Status DoDestroy() override { return {}; }
  void RefreshStateAfterStop() override {}
  bool DoUpdateThreadList(ThreadList &, ThreadList &) override { return false; }

  size_t DoReadMemory(addr_t address, void *buffer, size_t size,
                      Status &error) override {
    last_read = address;
    last_read_size = size;
    if (fail_memory || address < base || address - base > memory.size() ||
        size > memory.size() - (address - base)) {
      error = Status::FromErrorString("unreadable test memory");
      return 0;
    }
    memcpy(buffer, memory.data() + address - base, size);
    return size;
  }
  size_t DoWriteMemory(addr_t address, const void *buffer, size_t size,
                       Status &error) override {
    ++write_count;
    if (fail_memory || address < base || address - base > memory.size() ||
        size > memory.size() - (address - base) ||
        (address <= unwritable_address &&
         unwritable_address - address < size)) {
      error = Status::FromErrorString("unwritable test memory");
      return 0;
    }
    if (write_budget == 0)
      return 0;
    size = std::min(size, write_budget);
    write_budget -= size;
    memcpy(memory.data() + address - base, buffer, size);
    return size;
  }

  static constexpr addr_t base = 0x10000;
  std::array<uint8_t, 0x4000> memory = {};
  bool fail_memory = false;
  addr_t unwritable_address = LLDB_INVALID_ADDRESS;
  addr_t last_read = LLDB_INVALID_ADDRESS;
  size_t last_read_size = 0;
  unsigned write_count = 0;
  size_t write_budget = SIZE_MAX;
};

class TestRegisters : public RegisterContext {
public:
  TestRegisters(Thread &thread) : RegisterContext(thread, 0) {
    ArchSpec arch("powerpc-ibm-xbox360");
    auto registers = GetXbox360RegisterInfo();
    uint32_t offset = 0;
    for (auto &reg : registers) {
      reg.byte_offset = offset;
      offset += reg.byte_size;
    }
    info.SetRegisterInfo(std::move(registers), arch);
    values.resize(info.GetNumRegisters());
  }

  void InvalidateAllRegisters() override {}
  size_t GetRegisterCount() override { return info.GetNumRegisters(); }
  const RegisterInfo *GetRegisterInfoAtIndex(size_t reg) override {
    return info.GetRegisterInfoAtIndex(reg);
  }
  size_t GetRegisterSetCount() override { return register_set.empty() ? 0 : 1; }
  const RegisterSet *GetRegisterSet(size_t index) override {
    return index == 0 && !register_set.empty() ? &all_registers : nullptr;
  }
  void EnableRegisterSet() {
    for (uint32_t i = 0; i < GetRegisterCount(); ++i)
      register_set.push_back(i);
    all_registers = {"all", "all", register_set.size(), register_set.data()};
  }
  ByteOrder GetByteOrder() override { return eByteOrderBig; }
  bool ReadRegister(const RegisterInfo *reg, RegisterValue &value) override {
    if (reg && llvm::StringRef(reg->name) == "r2")
      ++r2_reads;
    if (!reg || unreadable == reg->name)
      return false;
    value = values[reg->kinds[eRegisterKindLLDB]];
    return true;
  }
  bool WriteRegister(const RegisterInfo *reg,
                     const RegisterValue &value) override {
    if (reg && llvm::StringRef(reg->name) == "r2")
      ++r2_writes;
    if (!reg || unwritable == reg->name)
      return false;
    values[reg->kinds[eRegisterKindLLDB]] = value;
    return true;
  }
  void Set(llvm::StringRef name, const RegisterValue &value) {
    ASSERT_TRUE(WriteRegister(GetRegisterInfoByName(name), value));
  }
  uint64_t Get(llvm::StringRef name) {
    return ReadRegisterAsUnsigned(GetRegisterInfoByName(name),
                                  LLDB_INVALID_ADDRESS);
  }

  DynamicRegisterInfo info;
  std::vector<RegisterValue> values;
  std::string unreadable, unwritable;
  unsigned r2_reads = 0, r2_writes = 0;
  std::vector<uint32_t> register_set;
  RegisterSet all_registers = {};
};

class TestThread : public Thread {
public:
  TestThread(Process &process) : Thread(process, 1) {
    registers = std::make_shared<TestRegisters>(*this);
  }
  ~TestThread() override { DestroyThread(); }
  void RefreshStateAfterStop() override {}
  RegisterContextSP GetRegisterContext() override { return registers; }
  RegisterContextSP CreateRegisterContextForFrame(StackFrame *) override {
    return registers;
  }
  StackFrameSP GetStackFrameAtIndex(uint32_t index) override {
    if (frames.empty())
      return Thread::GetStackFrameAtIndex(index);
    return index < frames.size() ? frames[index] : nullptr;
  }
  bool CalculateStopInfo() override { return false; }
  std::shared_ptr<TestRegisters> registers;
  std::vector<StackFrameSP> frames;
};

class TestCallPlan : public ThreadPlanCallFunction {
public:
  TestCallPlan(Thread &thread)
      : ThreadPlanCallFunction(thread, Address(), Options()) {}

  bool Setup(Thread &thread) {
    ABI *abi = nullptr;
    addr_t start, function;
    return ConstructorSetup(thread, abi, start, function);
  }

private:
  static EvaluateExpressionOptions Options() {
    EvaluateExpressionOptions options;
    options.SetTrapExceptions(false);
    return options;
  }
};

class Xbox360ABI : public testing::Test {
  SubsystemRAII<FileSystem, HostInfo, platform_linux::PlatformLinux,
                ObjectFilePECOFF, ScriptInterpreterNone>
      subsystems;

protected:
  void SetUp() override {
    LLVMInitializePowerPCTargetInfo();
    LLVMInitializePowerPCTargetMC();
    ABIPowerPC::Initialize();
    ArchSpec host_arch("x86_64-pc-linux");
    Platform::SetHostPlatform(
        platform_linux::PlatformLinux::CreateInstance(true, &host_arch));
    debugger = Debugger::CreateInstance();
    ArchSpec arch("powerpc-ibm-xbox360");
    auto platform = platform_linux::PlatformLinux::CreateInstance(true, &arch);
    ASSERT_TRUE(platform);
    ASSERT_TRUE(debugger->GetTargetList()
                    .CreateTarget(*debugger, "", arch, eLoadDependentsNo,
                                  platform, target)
                    .Success());
    process = std::make_shared<TestProcess>(target);
    thread = std::make_shared<TestThread>(*process);
    registers = thread->registers;
    registers->Set("r1", RegisterValue(uint64_t(0x11000)));
    abi = ABI::FindPlugin(process, arch);
    ASSERT_TRUE(abi);
    ASSERT_EQ(abi->GetPluginName(), "xbox360");
    ast = std::make_shared<TypeSystemClang>("Xbox ABI tests", arch.GetTriple());
  }

  void TearDown() override {
    ast.reset();
    abi.reset();
    registers.reset();
    thread.reset();
    process.reset();
    target.reset();
    Debugger::Destroy(debugger);
    ABIPowerPC::Terminate();
  }

  CompilerType Type(BasicType type) { return ast->GetBasicType(type); }
  void Add(ValueList &list, CompilerType type) {
    Value value;
    value.SetCompilerType(type);
    list.PushValue(value);
  }
  void Add(ValueList &list, BasicType type) { Add(list, Type(type)); }
  CompilerType VectorType() {
    return ast->GetType(ast->getASTContext().getVectorType(
        ClangUtil::GetQualType(Type(eBasicTypeUnsignedInt)), 4,
        clang::VectorKind::Generic));
  }
  void WriteStack(uint64_t offset, uint64_t value) {
    llvm::support::endian::write64be(process->memory.data() + 0x1000 + offset,
                                     value);
  }

  ValueObjectSP Return(CompilerType type,
                       const CompilerDeclContext *context = nullptr) {
    return abi->GetReturnValueObject(*thread, type, /*persistent=*/false,
                                     context);
  }

  StackFrameSP ReturnFrame(RegisterContextSP context = {}) {
    return std::make_shared<StackFrame>(thread, 1, 1,
                                        context ? context : registers, 0x11000,
                                        addr_t(0x82001000), false, nullptr);
  }

  ValueObjectSP ReturnValue(CompilerType type, llvm::ArrayRef<uint8_t> bytes,
                            ByteOrder order = eByteOrderBig) {
    auto buffer = std::make_shared<DataBufferHeap>(bytes.data(), bytes.size());
    return ValueObjectConstResult::Create(thread.get(), type, ConstString(),
                                          buffer, order, 4);
  }

  Status SetReturn(CompilerType type, llvm::ArrayRef<uint8_t> bytes,
                   const CompilerDeclContext *context = nullptr) {
    auto frame = ReturnFrame();
    auto value = ReturnValue(type, bytes);
    return abi->SetReturnValueObject(frame, value, context);
  }

  void ExpectReturnBytes(CompilerType type, llvm::ArrayRef<uint8_t> expected,
                         const CompilerDeclContext *context = nullptr) {
    auto value = Return(type, context);
    ASSERT_TRUE(value);
    EXPECT_EQ(value->GetCompilerType(), type);
    DataExtractor data;
    Status error;
    ASSERT_EQ(value->GetData(data, error), expected.size());
    ASSERT_TRUE(error.Success()) << error.AsCString();
    ASSERT_EQ(data.GetByteSize(), expected.size());
    EXPECT_EQ(data.GetByteOrder(), eByteOrderBig);
    EXPECT_EQ(data.GetAddressByteSize(), 4u);
    EXPECT_EQ(memcmp(data.GetDataStart(), expected.data(), expected.size()), 0);
  }

  CompilerType Record(llvm::StringRef name, CompilerType field,
                      bool cxx = false, AccessType access = eAccessPublic) {
    auto type = ast->CreateRecordType(
        ast->getASTContext().getTranslationUnitDecl(), OptionalClangModuleID(),
        eAccessPublic, name, llvm::to_underlying(clang::TagTypeKind::Struct),
        cxx ? eLanguageTypeC_plus_plus : eLanguageTypeC);
    TypeSystemClang::StartTagDeclarationDefinition(type);
    ast->AddFieldToRecordType(type, "value", field, access, 0);
    TypeSystemClang::CompleteTagDeclarationDefinition(type);
    return type;
  }

  CompilerDeclContext FunctionContext(CompilerType return_type,
                                      bool instance_method) {
    auto owner = ast->CreateRecordType(
        ast->getASTContext().getTranslationUnitDecl(), OptionalClangModuleID(),
        eAccessPublic, "ReturnOwner",
        llvm::to_underlying(clang::TagTypeKind::Struct),
        eLanguageTypeC_plus_plus);
    TypeSystemClang::StartTagDeclarationDefinition(owner);
    auto function_type =
        ast->CreateFunctionType(return_type, nullptr, 0, false, 0);
    auto *method = ast->AddMethodToCXXRecordType(
        owner.GetOpaqueQualType(), instance_method ? "Instance" : "Static",
        nullptr, function_type, eAccessPublic, /*is_virtual=*/false,
        /*is_static=*/!instance_method, /*is_inline=*/false,
        /*is_explicit=*/false, /*is_attr_used=*/false, /*is_artificial=*/false);
    TypeSystemClang::CompleteTagDeclarationDefinition(owner);
    EXPECT_NE(method, nullptr);
    return ast->CreateDeclContext(method);
  }

  DebuggerSP debugger;
  TargetSP target;
  std::shared_ptr<TestProcess> process;
  std::shared_ptr<TestThread> thread;
  std::shared_ptr<TestRegisters> registers;
  ABISP abi;
  std::shared_ptr<TypeSystemClang> ast;
};
} // namespace

TEST_F(Xbox360ABI, Selection) {
  for (const char *triple : {"powerpc-ibm-xbox360", "powerpc64-ibm-xbox360"}) {
    ArchSpec arch(triple);
    auto selected = ABI::FindPlugin({}, arch);
    ASSERT_TRUE(selected);
    EXPECT_EQ(selected->GetPluginName(), "xbox360");
    EXPECT_FALSE(ABISysV_ppc::CreateInstance({}, arch));
    EXPECT_FALSE(ABISysV_ppc64::CreateInstance({}, arch));
  }
  for (const char *triple :
       {"powerpc-unknown-linux", "powerpc64-unknown-linux"}) {
    ArchSpec arch(triple);
    EXPECT_FALSE(ABIXbox360::CreateInstance({}, arch));
    auto selected = ABI::FindPlugin({}, arch);
    ASSERT_TRUE(selected);
    EXPECT_NE(selected->GetPluginName(), "xbox360");
    EXPECT_FALSE(selected->GetThreadLocalData(*thread, {}, 0).has_value());
  }
}

TEST_F(Xbox360ABI, PreservedRegisters) {
  for (unsigned i = 0; i != 32; ++i) {
    EXPECT_EQ(!abi->RegisterIsVolatile(
                  registers->GetRegisterInfoByName("r" + std::to_string(i))),
              i == 1 || i == 2 || i == 13 || i >= 14);
    EXPECT_EQ(!abi->RegisterIsVolatile(
                  registers->GetRegisterInfoByName("f" + std::to_string(i))),
              i >= 14);
  }
  for (unsigned i = 0; i != 128; ++i) {
    SCOPED_TRACE(i);
    auto *reg = registers->GetRegisterInfoByName("v" + std::to_string(i));
    EXPECT_EQ(!abi->RegisterIsVolatile(reg), i >= 64);
    RegisterInfo alias = *reg;
    std::string name = "vr" + std::to_string(i);
    alias.name = name.c_str();
    alias.alt_name = nullptr;
    EXPECT_EQ(!abi->RegisterIsVolatile(&alias), i >= 64);
  }
  for (unsigned i = 0; i != 8; ++i)
    EXPECT_EQ(!abi->RegisterIsVolatile(
                  registers->GetRegisterInfoByName("cr" + std::to_string(i))),
              i >= 2 && i <= 5);
  for (const char *name :
       {"cr", "lr", "pc", "ctr", "xer", "fpscr", "vscr", "vrsave", "msr"})
    EXPECT_TRUE(abi->RegisterIsVolatile(registers->GetRegisterInfoByName(name)))
        << name;
  EXPECT_TRUE(abi->RegisterIsVolatile(nullptr));
  RegisterInfo unknown = {};
  EXPECT_TRUE(abi->RegisterIsVolatile(&unknown));
}

TEST_F(Xbox360ABI, AugmentsRemoteRegistersWithoutChangingLayout) {
  std::vector<DynamicRegisterInfo::Register> regs(2);
  regs[0].name = ConstString("vr127");
  regs[0].byte_size = 16;
  regs[0].byte_offset = 40;
  regs[0].regnum_remote = 12;
  regs[1].name = ConstString("r3");
  regs[1].byte_size = 8;
  regs[1].byte_offset = 0;
  regs[1].regnum_remote = 99;
  abi->AugmentRegisterInfo(regs);
  ASSERT_EQ(regs.size(), 2u);
  EXPECT_EQ(regs[0].regnum_dwarf, 1395u);
  EXPECT_EQ(regs[0].byte_size, 16u);
  EXPECT_EQ(regs[0].byte_offset, 40u);
  EXPECT_EQ(regs[0].regnum_remote, 12u);
  EXPECT_EQ(regs[1].regnum_generic, LLDB_REGNUM_GENERIC_ARG1);
  EXPECT_EQ(regs[1].byte_offset, 0u);
  EXPECT_EQ(regs[1].regnum_remote, 99u);
}

TEST_F(Xbox360ABI, EightIntegerRegistersAndStack) {
  ValueList args;
  for (unsigned i = 0; i != 10; ++i) {
    Add(args, eBasicTypeUnsignedLongLong);
    if (i < 8)
      registers->Set("r" + std::to_string(i + 3),
                     RegisterValue(0x1234567800000000ull + i));
    else
      WriteStack(16 + i * 8, 0x8765432100000000ull + i);
  }
  ASSERT_TRUE(abi->GetArgumentValues(*thread, args));
  for (unsigned i = 0; i != 10; ++i)
    EXPECT_EQ(args.GetValueAtIndex(i)->GetScalar().ULongLong(),
              (i < 8 ? 0x1234567800000000ull : 0x8765432100000000ull) + i);
}

TEST_F(Xbox360ABI, NarrowArgumentsAndPointers) {
  ValueList args;
  Add(args, eBasicTypeSignedChar);
  Add(args, eBasicTypeUnsignedChar);
  Add(args, eBasicTypeInt);
  Add(args, Type(eBasicTypeVoid).GetPointerType());
  registers->Set("r3", RegisterValue(uint64_t(0x12345678000000ff)));
  registers->Set("r4", RegisterValue(uint64_t(0x12345678000000ff)));
  registers->Set("r5", RegisterValue(uint64_t(0x1234567880000000)));
  registers->Set("r6", RegisterValue(uint64_t(0x1234567882001000)));
  ASSERT_TRUE(abi->GetArgumentValues(*thread, args));
  EXPECT_EQ(args.GetValueAtIndex(0)->GetScalar().SLongLong(), -1);
  EXPECT_EQ(args.GetValueAtIndex(1)->GetScalar().ULongLong(), 255u);
  EXPECT_EQ(args.GetValueAtIndex(2)->GetScalar().SLongLong(), INT32_MIN);
  EXPECT_EQ(args.GetValueAtIndex(3)->GetScalar().ULongLong(), 0x82001000u);
}

TEST_F(Xbox360ABI, MixedArgumentBanks) {
  ValueList args;
  Add(args, eBasicTypeInt);
  Add(args, eBasicTypeFloat);
  Add(args, VectorType());
  Add(args, eBasicTypeUnsignedLongLong);
  Add(args, eBasicTypeDouble);
  Add(args, eBasicTypeUnsignedInt);
  registers->Set("r3", RegisterValue(uint64_t(1)));
  registers->Set("f1", RegisterValue(uint64_t(0x3ff4000000000000)));
  const std::array<uint8_t, 16> vector = {0, 1, 2,  3,  4,  5,  6,  7,
                                          8, 9, 10, 11, 12, 13, 14, 15};
  registers->Set("v1", RegisterValue(vector, eByteOrderBig));
  // FP arguments consume a GPR slot; vectors and vector padding do not.
  registers->Set("r5", RegisterValue(uint64_t(0x1122334455667788)));
  registers->Set("f2", RegisterValue(-3.5));
  registers->Set("r7", RegisterValue(uint64_t(42)));
  ASSERT_TRUE(abi->GetArgumentValues(*thread, args));
  EXPECT_EQ(args.GetValueAtIndex(0)->GetScalar().SInt(), 1);
  EXPECT_EQ(args.GetValueAtIndex(1)->GetScalar().Float(), 1.25f);
  auto &buffer = args.GetValueAtIndex(2)->GetBuffer();
  ASSERT_EQ(buffer.GetByteSize(), vector.size());
  EXPECT_EQ(memcmp(buffer.GetBytes(), vector.data(), vector.size()), 0);
  EXPECT_EQ(args.GetValueAtIndex(3)->GetScalar().ULongLong(),
            0x1122334455667788ull);
  EXPECT_EQ(args.GetValueAtIndex(4)->GetScalar().Double(), -3.5);
  EXPECT_EQ(args.GetValueAtIndex(5)->GetScalar().UInt(), 42u);
}

TEST_F(Xbox360ABI, FloatingPointOverflowAndRightJustifiedStack) {
  ValueList args;
  for (unsigned i = 0; i != 14; ++i) {
    Add(args, eBasicTypeFloat);
    if (i < 13)
      registers->Set("f" + std::to_string(i + 1), RegisterValue(i + 0.5));
  }
  // The four-byte float lives in the low word of slot 13, not its high word.
  WriteStack(16 + 13 * 8, 0xdeadbeef40600000ull); // 3.5f
  Add(args, eBasicTypeSignedChar);
  WriteStack(16 + 14 * 8, 0xaabbccddeeff00feull);
  ASSERT_TRUE(abi->GetArgumentValues(*thread, args));
  for (unsigned i = 0; i != 13; ++i)
    EXPECT_EQ(args.GetValueAtIndex(i)->GetScalar().Float(), i + 0.5f);
  EXPECT_EQ(args.GetValueAtIndex(13)->GetScalar().Float(), 3.5f);
  EXPECT_EQ(args.GetValueAtIndex(14)->GetScalar().SLongLong(), -2);
}

TEST_F(Xbox360ABI, VectorOverflowAndPadding) {
  ValueList args;
  Add(args, eBasicTypeInt);
  registers->Set("r3", RegisterValue(uint64_t(7)));
  const std::array<uint8_t, 16> vector = {0, 1, 2,  3,  4,  5,  6,  7,
                                          8, 9, 10, 11, 12, 13, 14, 15};
  for (unsigned i = 0; i != 14; ++i) {
    Add(args, VectorType());
    if (i < 13)
      registers->Set("v" + std::to_string(i + 1),
                     RegisterValue(vector, eByteOrderBig));
  }
  memcpy(process->memory.data() + 0x1000 + 32 + 13 * 16, vector.data(), 16);
  Add(args, eBasicTypeInt);
  registers->Set("r4", RegisterValue(uint64_t(8)));
  ASSERT_TRUE(abi->GetArgumentValues(*thread, args));
  for (unsigned i = 1; i != 15; ++i) {
    auto &buffer = args.GetValueAtIndex(i)->GetBuffer();
    ASSERT_EQ(buffer.GetByteSize(), vector.size());
    EXPECT_EQ(memcmp(buffer.GetBytes(), vector.data(), 16), 0);
  }
  EXPECT_EQ(args.GetValueAtIndex(15)->GetScalar().SInt(), 8);
}

TEST_F(Xbox360ABI, TrivialCallPlacement) {
  std::array<addr_t, 10> args;
  for (unsigned i = 0; i != args.size(); ++i)
    args[i] = 0x1234567887654300ull + i;
  registers->Set("r2", RegisterValue(uint64_t(0x2222222222222222)));
  registers->Set("r13", RegisterValue(uint64_t(0x1313131313131313)));
  ASSERT_TRUE(
      abi->PrepareTrivialCall(*thread, 0x1100f, 0x82001000, 0x82002000, args));
  addr_t sp = registers->Get("r1");
  EXPECT_EQ(sp, 0x10fa0u); // 16-byte linkage plus ten doubleword slots.
  EXPECT_EQ(registers->Get("pc"), 0x82001000u);
  EXPECT_EQ(registers->Get("lr"), 0x82002000u);
  EXPECT_EQ(registers->Get("r2"), 0x2222222222222222ull);
  EXPECT_EQ(registers->Get("r13"), 0x1313131313131313ull);
  for (unsigned i = 0; i != 8; ++i)
    EXPECT_EQ(registers->Get("r" + std::to_string(i + 3)), args[i]);
  const uint8_t *frame = process->memory.data() + sp - TestProcess::base;
  EXPECT_EQ(llvm::support::endian::read32be(frame), 0x11000u);
  EXPECT_EQ(llvm::support::endian::read32be(frame + 4), 0u);
  EXPECT_EQ(llvm::support::endian::read64be(frame + 80), args[8]);
  EXPECT_EQ(llvm::support::endian::read64be(frame + 88), args[9]);

  ValueList values;
  for (unsigned i = 0; i != args.size(); ++i)
    Add(values, eBasicTypeUnsignedLongLong);
  ASSERT_TRUE(abi->GetArgumentValues(*thread, values));
  for (unsigned i = 0; i != args.size(); ++i)
    EXPECT_EQ(values.GetValueAtIndex(i)->GetScalar().ULongLong(), args[i]);
}

TEST_F(Xbox360ABI, TrivialCallReservesHomeArea) {
  ASSERT_TRUE(
      abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000, {}));
  EXPECT_EQ(registers->Get("r1"), 0x10fb0u);
  EXPECT_EQ(abi->GetRedZoneSize(), 1344u);
}

TEST_F(Xbox360ABI, TrivialCallDoesNotUseTOCOrFunctionDescriptors) {
  for (const char *triple : {"powerpc-ibm-xbox360", "powerpc64-ibm-xbox360"}) {
    SCOPED_TRACE(triple);
    auto selected = ABI::FindPlugin(process, ArchSpec(triple));
    ASSERT_TRUE(selected);
    ASSERT_EQ(selected->GetPluginName(), "xbox360");
    registers->Set("r1", RegisterValue(uint64_t(0x11000)));
    registers->Set("r2", RegisterValue(uint64_t(0x2222222222222222)));
    registers->Set("r12", RegisterValue(uint64_t(0x1212121212121212)));
    registers->Set("r13", RegisterValue(uint64_t(0x1313131313131313)));
    registers->r2_reads = registers->r2_writes = 0;
    registers->unreadable = registers->unwritable = "r2";
    process->memory.fill(0xa5);
    process->last_read = LLDB_INVALID_ADDRESS;

    // Code lies outside the mock's readable memory. Xbox function pointers are
    // entry addresses, not descriptors containing an entry point and a TOC.
    ASSERT_TRUE(selected->PrepareTrivialCall(
        *thread, 0x11000, 0x82001000, 0x82002000, {}));
    EXPECT_EQ(registers->r2_reads, 0u);
    EXPECT_EQ(registers->r2_writes, 0u);
    EXPECT_EQ(process->last_read, LLDB_INVALID_ADDRESS);
    EXPECT_EQ(registers->Get("pc"), 0x82001000u);
    EXPECT_EQ(registers->Get("lr"), 0x82002000u);
    EXPECT_EQ(registers->Get("r12"), 0x1212121212121212ull);
    EXPECT_EQ(registers->Get("r13"), 0x1313131313131313ull);
    const uint8_t *frame = process->memory.data() +
                           registers->Get("r1") - TestProcess::base;
    // SysV PPC64 saves TOC at +24 (ELFv2) or +40 (ELFv1). Both are unused
    // argument home slots here, not reserved linkage slots.
    EXPECT_EQ(llvm::support::endian::read64be(frame + 24), 0u);
    EXPECT_EQ(llvm::support::endian::read64be(frame + 40), 0u);
    registers->unreadable.clear();
    registers->unwritable.clear();
    EXPECT_EQ(registers->Get("r2"), 0x2222222222222222ull);
  }
}

TEST_F(Xbox360ABI, TrivialCallInitializesEntireFrame) {
  for (size_t count : {0u, 1u, 8u, 9u, 10u, 11u}) {
    SCOPED_TRACE(count);
    process->memory.fill(0xa5);
    registers->Set("r1", RegisterValue(uint64_t(0x11000)));
    std::vector<addr_t> args(count, 0x1234567887654321ull);
    ASSERT_TRUE(abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000,
                                        0x82002000, args));
    addr_t sp = registers->Get("r1");
    const uint64_t size = (16 + std::max<size_t>(8, count) * 8 + 15) & ~15ull;
    EXPECT_EQ(sp, 0x11000 - size);
    EXPECT_EQ(sp % 16, 0u);
    const uint8_t *frame = process->memory.data() + sp - TestProcess::base;
    EXPECT_EQ(llvm::support::endian::read32be(frame), 0x11000u);
    for (unsigned i = 4; i < 80; ++i)
      EXPECT_EQ(frame[i], 0u);
    for (size_t i = 8; i < count; ++i)
      EXPECT_EQ(llvm::support::endian::read64be(frame + 16 + i * 8), args[i]);
    for (size_t i = 16 + std::max<size_t>(8, count) * 8; i < size; ++i)
      EXPECT_EQ(frame[i], 0u);
    EXPECT_EQ(frame[-1], 0xa5u);
    EXPECT_EQ(frame[size], 0xa5u);
  }
}

TEST_F(Xbox360ABI, TrivialCallPreservesInterruptedRedZone) {
  process->memory.fill(0xa5);
  registers->Set("r1", RegisterValue(uint64_t(0xdeadbeef00011000)));
  addr_t ceiling = 0x11000 - abi->GetRedZoneSize();
  std::array<addr_t, 9> args = {1, 2, 3, 4, 5, 6, 7, 8, 0x1234567887654321ull};
  ASSERT_TRUE(
      abi->PrepareTrivialCall(*thread, ceiling, 0x82001000, 0x82002000, args));
  addr_t sp = registers->Get("r1");
  EXPECT_EQ(sp, ceiling - 96);
  EXPECT_EQ(llvm::support::endian::read32be(process->memory.data() + sp -
                                            TestProcess::base),
            0x11000u);
  for (addr_t address = ceiling; address < 0x11010; ++address)
    EXPECT_EQ(process->memory[address - TestProcess::base], 0xa5u);

  // The target may expose undefined upper halves of the GPR again on a stop.
  registers->Set("r1", RegisterValue(uint64_t(0xdeadbeef00000000) | sp));
  ValueList values;
  for (unsigned i = 0; i < args.size(); ++i)
    Add(values, eBasicTypeUnsignedLongLong);
  ASSERT_TRUE(abi->GetArgumentValues(*thread, values));
  for (unsigned i = 0; i < args.size(); ++i)
    EXPECT_EQ(values.GetValueAtIndex(i)->GetScalar().ULongLong(), args[i]);
}

TEST_F(Xbox360ABI, TrivialCallRejectsInvalidStackBoundsBeforeWriting) {
  for (addr_t current : {addr_t(0), addr_t(0x11008), LLDB_INVALID_ADDRESS}) {
    SCOPED_TRACE(current);
    registers->Set("r1", RegisterValue(current));
    EXPECT_FALSE(
        abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000, {}));
  }
  registers->Set("r1", RegisterValue(uint64_t(0x11000)));
  registers->unreadable = "r1";
  EXPECT_FALSE(
      abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000, {}));
  registers->unreadable.clear();
  for (addr_t ceiling : {addr_t(0), addr_t(79), addr_t(80), addr_t(0x11010)}) {
    SCOPED_TRACE(ceiling);
    EXPECT_FALSE(
        abi->PrepareTrivialCall(*thread, ceiling, 0x82001000, 0x82002000, {}));
  }
  std::vector<addr_t> too_many(0x11000 / 8, 1);
  EXPECT_FALSE(abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000,
                                       too_many));
  EXPECT_EQ(process->write_count, 0u);
  EXPECT_EQ(registers->Get("r1"), 0x11000u);
}

TEST_F(Xbox360ABI, TrivialCallRequiresWritableHomeArea) {
  registers->Set("pc", RegisterValue(uint32_t(0x82003000)));
  registers->Set("lr", RegisterValue(uint64_t(0x82004000)));
  registers->Set("r3", RegisterValue(uint64_t(99)));
  // The backchain is writable, but the last home slot is not.
  process->unwritable_address = 0x10fff;
  const addr_t argument = 42;
  EXPECT_FALSE(abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000,
                                       {argument}));
  EXPECT_EQ(registers->Get("r1"), 0x11000u);
  EXPECT_EQ(registers->Get("pc"), 0x82003000u);
  EXPECT_EQ(registers->Get("lr"), 0x82004000u);
  EXPECT_EQ(registers->Get("r3"), 99u);
}

TEST_F(Xbox360ABI, TrivialCallRejectsShortFrameWrite) {
  process->write_budget = 4;
  registers->Set("pc", RegisterValue(uint32_t(0x82003000)));
  registers->Set("lr", RegisterValue(uint64_t(0x82004000)));
  EXPECT_FALSE(
      abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000, {}));
  EXPECT_EQ(registers->Get("r1"), 0x11000u);
  EXPECT_EQ(registers->Get("pc"), 0x82003000u);
  EXPECT_EQ(registers->Get("lr"), 0x82004000u);
}

TEST_F(Xbox360ABI, CallPlanNormalizesStackBeforeReservingRedZone) {
  registers->Set("r1", RegisterValue(uint64_t(0xdeadbeef00011000)));
  TestCallPlan plan(*thread);
  // This fixture has no executable entry point, so setup stops after the
  // initial stack accessibility check.
  EXPECT_FALSE(plan.Setup(*thread));
  EXPECT_EQ(plan.GetFunctionStackPointer(), 0x11000 - 1344u);
  const addr_t ceiling = 0x11000 - 1344u;
  // Process memory reads may be expanded to cache-line boundaries.
  ASSERT_LE(process->last_read, ceiling);
  EXPECT_GE(process->last_read_size, ceiling - process->last_read + 4);
}

TEST_F(Xbox360ABI, CallPlanRejectsUnreadableOrUnderflowingStack) {
  for (addr_t sp : {addr_t(0), addr_t(1344), LLDB_INVALID_ADDRESS}) {
    registers->Set("r1", RegisterValue(sp));
    TestCallPlan plan(*thread);
    EXPECT_FALSE(plan.Setup(*thread));
    StreamString error;
    EXPECT_FALSE(plan.ValidatePlan(&error));
    EXPECT_TRUE(error.GetString().contains("Cannot reserve the red zone"));
    EXPECT_EQ(process->last_read, LLDB_INVALID_ADDRESS);
  }
  registers->unreadable = "r1";
  TestCallPlan plan(*thread);
  EXPECT_FALSE(plan.Setup(*thread));
  EXPECT_EQ(process->last_read, LLDB_INVALID_ADDRESS);
}

TEST_F(Xbox360ABI, FailuresAreReported) {
  ValueList args;
  Add(args, eBasicTypeInt);
  args.GetValueAtIndex(0)->GetScalar() = 99;
  registers->unreadable = "r3";
  EXPECT_FALSE(abi->GetArgumentValues(*thread, args));
  EXPECT_EQ(args.GetValueAtIndex(0)->GetScalar().SInt(), 99);
  registers->unreadable.clear();
  Add(args, eBasicTypeInt128);
  EXPECT_FALSE(abi->GetArgumentValues(*thread, args));
  EXPECT_EQ(args.GetValueAtIndex(0)->GetScalar().SInt(), 99);
  EXPECT_FALSE(abi->PrepareTrivialCall(*thread, 0x100000000ull, 0x82001000,
                                       0x82002000, {}));
  EXPECT_FALSE(
      abi->PrepareTrivialCall(*thread, 0x11000, 0x82001001, 0x82002000, {}));
  process->fail_memory = true;
  EXPECT_FALSE(
      abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000, {}));
}

TEST_F(Xbox360ABI, RejectsUnsupportedAndUnreadableArguments) {
  ValueList args;
  Add(args, clang_utils::createRecordWithField(
                *ast, "Aggregate", Type(eBasicTypeUnsignedInt), "value"));
  EXPECT_FALSE(abi->GetArgumentValues(*thread, args));
  args.Clear();
  Add(args, eBasicTypeFloat);
  registers->unreadable = "f1";
  EXPECT_FALSE(abi->GetArgumentValues(*thread, args));
  args.Clear();
  Add(args, VectorType());
  registers->unreadable = "v1";
  EXPECT_FALSE(abi->GetArgumentValues(*thread, args));
  registers->unreadable.clear();
  args.Clear();
  for (unsigned i = 0; i != 9; ++i)
    Add(args, eBasicTypeInt);
  process->fail_memory = true;
  EXPECT_FALSE(abi->GetArgumentValues(*thread, args));
  process->fail_memory = false;
  registers->unwritable = "lr";
  EXPECT_FALSE(
      abi->PrepareTrivialCall(*thread, 0x11000, 0x82001000, 0x82002000, {}));
}

TEST_F(Xbox360ABI, AddressValidation) {
  EXPECT_TRUE(abi->CallFrameAddressIsValid(0x11000));
  EXPECT_FALSE(abi->CallFrameAddressIsValid(0));
  EXPECT_FALSE(abi->CallFrameAddressIsValid(0x11008));
  EXPECT_FALSE(abi->CallFrameAddressIsValid(0x100000000ull));
  EXPECT_TRUE(abi->CodeAddressIsValid(0x82000004));
  EXPECT_FALSE(abi->CodeAddressIsValid(0));
  EXPECT_FALSE(abi->CodeAddressIsValid(0x82000002));
  EXPECT_FALSE(abi->CodeAddressIsValid(0x182000000ull));
}

TEST_F(Xbox360ABI, DoesNotUseSysVUnwindOrReturnRules) {
  UnwindPlan plan(eRegisterKindDWARF);
  EXPECT_TRUE(abi->CreateFunctionEntryUnwindPlan(plan));
  EXPECT_EQ(plan.GetRegisterKind(), eRegisterKindGeneric);
  EXPECT_FALSE(abi->CreateDefaultUnwindPlan(plan));
  StackFrameSP frame;
  ValueObjectSP value;
  EXPECT_TRUE(abi->SetReturnValueObject(frame, value).Fail());
}

TEST_F(Xbox360ABI, IntegerReturnWidthsAndSignedness) {
  for (auto entry : {std::pair{eBasicTypeSignedChar, 1u},
                     {eBasicTypeShort, 2u},
                     {eBasicTypeInt, 4u},
                     {eBasicTypeLongLong, 8u}}) {
    registers->Set("r3", RegisterValue(UINT64_MAX));
    auto value = Return(Type(entry.first));
    ASSERT_TRUE(value);
    bool success = false;
    EXPECT_EQ(value->GetValueAsSigned(0, &success), -1);
    EXPECT_TRUE(success);
    std::array<uint8_t, 8> bytes;
    bytes.fill(0xff);
    ExpectReturnBytes(Type(entry.first),
                      llvm::ArrayRef(bytes).take_front(entry.second));
  }
  registers->Set("r3", RegisterValue(uint64_t(0x12345678876543fe)));
  auto value = Return(Type(eBasicTypeUnsignedChar));
  ASSERT_TRUE(value);
  EXPECT_EQ(value->GetValueAsUnsigned(0), 254u);
  value = Return(Type(eBasicTypeUnsignedInt));
  ASSERT_TRUE(value);
  EXPECT_EQ(value->GetValueAsUnsigned(0), 0x876543feu);
  registers->Set("r3", RegisterValue(uint64_t(0xdeadbeef00000001)));
  ExpectReturnBytes(Type(eBasicTypeBool), {1});
}

TEST_F(Xbox360ABI, UnwindAddressesUseLow32Bits) {
  EXPECT_EQ(abi->FixCodeAddress(0xdeadbeef82001000), 0x82001000u);
  EXPECT_EQ(abi->FixDataAddress(0xdeadbeef00011000), 0x11000u);
  EXPECT_EQ(abi->FixCodeAddress(LLDB_INVALID_ADDRESS), LLDB_INVALID_ADDRESS);
  EXPECT_EQ(abi->FixDataAddress(LLDB_INVALID_ADDRESS), LLDB_INVALID_ADDRESS);
}

TEST_F(Xbox360ABI, CodeViewRegisterAndStackLocations) {
  using llvm::codeview::RegisterId;
  auto module =
      std::make_shared<Module>(FileSpec(), ArchSpec("powerpc-ibm-xbox360"));
  ExecutionContext context(thread.get());
  auto evaluate =
      [&](const DWARFExpression &expression) -> llvm::Expected<Value> {
    DataExtractor data;
    if (!expression.GetExpressionData(data))
      return llvm::createStringError("invalid test location");
    return DWARFExpression::Evaluate(&context, registers.get(), module, data,
                                     nullptr, expression.GetRegisterKind(),
                                     nullptr, nullptr);
  };
  registers->Set("r3", RegisterValue(uint64_t(0x1234567887654321)));
  auto integer = evaluate(
      npdb::MakeEnregisteredLocationExpression(RegisterId::PPC_GPR3, module));
  ASSERT_THAT_EXPECTED(integer, llvm::Succeeded());
  EXPECT_EQ(integer->GetScalar().ULongLong(), 0x1234567887654321ull);
  registers->Set("f1", RegisterValue(1.25));
  auto floating = evaluate(
      npdb::MakeEnregisteredLocationExpression(RegisterId::PPC_FPR1, module));
  ASSERT_THAT_EXPECTED(floating, llvm::Succeeded());
  EXPECT_EQ(floating->GetScalar().Double(), 1.25);
  registers->Set("lr", RegisterValue(uint64_t(0x82001234)));
  auto link = evaluate(
      npdb::MakeEnregisteredLocationExpression(RegisterId::PPC_LR, module));
  ASSERT_THAT_EXPECTED(link, llvm::Succeeded());
  EXPECT_EQ(link->GetScalar().ULongLong(), 0x82001234u);
  for (auto [cv, name] : {std::pair{RegisterId::PPC_GPR1, "r1"},
                          {RegisterId::PPC_GPR31, "r31"},
                          {RegisterId::PPC_GPR30, "r30"},
                          {RegisterId::PPC_GPR13, "r13"}}) {
    registers->Set(name, RegisterValue(uint64_t(0xdeadbeef00011000)));
    for (int32_t offset : {-16, 0, 24}) {
      auto address =
          evaluate(npdb::MakeRegRelLocationExpression(cv, offset, module));
      ASSERT_THAT_EXPECTED(address, llvm::Succeeded());
      EXPECT_EQ(address->GetScalar().ULongLong(), 0x11000 + offset);
    }
  }
  auto pc_expression =
      npdb::MakeEnregisteredLocationExpression(RegisterId::PPC_PC, module);
  EXPECT_EQ(pc_expression.GetRegisterKind(), eRegisterKindGeneric);
  registers->Set("pc", RegisterValue(uint32_t(0x82004321)));
  auto pc = evaluate(pc_expression);
  ASSERT_THAT_EXPECTED(pc, llvm::Succeeded());
  EXPECT_EQ(pc->GetScalar().ULongLong(), 0x82004321u);
  registers->Set("cr", RegisterValue(uint32_t(0x12345678)));
  auto cr = evaluate(
      npdb::MakeEnregisteredLocationExpression(RegisterId::PPC_CR, module));
  ASSERT_THAT_EXPECTED(cr, llvm::Succeeded());
  EXPECT_EQ(cr->GetScalar().ULongLong(), 0x12345678u);
  auto vector =
      npdb::MakeEnregisteredLocationExpression(RegisterId::PPC_VR127, module);
  EXPECT_EQ(vector.GetRegisterKind(), eRegisterKindDWARF);
  const std::array<uint8_t, 16> vector_bytes = {0, 1, 2,  3,  4,  5,  6,  7,
                                                8, 9, 10, 11, 12, 13, 14, 15};
  registers->Set("v127", RegisterValue(vector_bytes, eByteOrderBig));
  auto vector_value = evaluate(vector);
  ASSERT_THAT_EXPECTED(vector_value, llvm::Succeeded());
  EXPECT_EQ(vector_value->GetScalar().UInt128(llvm::APInt(128, 0)),
            llvm::APInt(128, "000102030405060708090a0b0c0d0e0f", 16));
  EXPECT_FALSE(
      npdb::MakeEnregisteredLocationExpression(RegisterId::PPC_MQ, module)
          .IsValid());

  std::map<uint64_t, npdb::MemberValLocation> pieces = {
      {0, {uint16_t(RegisterId::PPC_GPR3), 0}},
      {8, {uint16_t(RegisterId::PPC_GPR4), 0}}};
  std::map<uint64_t, size_t> sizes = {{0, 8}, {8, 8}};
  auto composite = npdb::MakeEnregisteredLocationExpressionForComposite(
      pieces, sizes, 16, module);
  EXPECT_TRUE(composite.IsValid());
  EXPECT_EQ(composite.GetRegisterKind(), eRegisterKindDWARF);
  pieces[8].reg_id = uint16_t(RegisterId::PPC_PC);
  EXPECT_FALSE(npdb::MakeEnregisteredLocationExpressionForComposite(
                   pieces, sizes, 16, module)
                   .IsValid());
}

TEST_F(Xbox360ABI, StaticTLSIsThreadRelativeAndBigEndian) {
  auto file = TestFile::fromYaml(R"(
--- !COFF
OptionalHeader:
  ImageBase: 0x82000000
  SectionAlignment: 4096
  FileAlignment: 512
  Subsystem: IMAGE_SUBSYSTEM_XBOX
header:
  Machine: IMAGE_FILE_MACHINE_PPCBE
  Characteristics: [ IMAGE_FILE_EXECUTABLE_IMAGE ]
sections:
  - Name: .tls
    VirtualAddress: 4096
    VirtualSize: 64
    Characteristics: [ IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_MEM_READ, IMAGE_SCN_MEM_WRITE ]
    SectionData: '00000000'
symbols: []
...
)");
  ASSERT_THAT_EXPECTED(file, llvm::Succeeded());
  auto module = std::make_shared<Module>(file->moduleSpec());
  ASSERT_TRUE(module->GetObjectFile());
  target->SetExecutableModule(module, eLoadDependentsNo);
  auto tls = module->GetSectionList()->FindSectionByName(ConstString(".tls"));
  ASSERT_TRUE(tls);
  // Deliberately relocate the image: TLS is not an ordinary static address.
  ASSERT_TRUE(target->SetSectionLoadAddress(tls, 0x83001000));

  auto second = std::make_shared<TestThread>(*process);
  second->registers->Set("r13", RegisterValue(uint64_t(0xdeadbeef00010010)));
  registers->Set("r13", RegisterValue(uint64_t(0xdeadbeef00010000)));
  EXPECT_EQ(registers->GetThreadPointer(), 0xdeadbeef00010000ull);
  llvm::support::endian::write32be(process->memory.data(), 0x12000);
  llvm::support::endian::write32be(process->memory.data() + 4, 0xdeadbeef);
  llvm::support::endian::write32be(process->memory.data() + 0x10, 0x13000);
  EXPECT_EQ(thread->GetThreadLocalData(module, 0), 0x12000u);
  EXPECT_EQ(thread->GetThreadLocalData(module, 12), 0x1200cu);
  EXPECT_EQ(second->GetThreadLocalData(module, 12), 0x1300cu);
  EXPECT_EQ(thread->GetThreadLocalData(module, 63), 0x1203fu);

  auto expression = npdb::MakeGlobalLocationExpression(
      tls->GetID(), 12, module, /*is_thread_local=*/true);
  ExecutionContext context(thread.get());
  DataExtractor expression_data;
  ASSERT_TRUE(expression.GetExpressionData(expression_data));
  auto result = DWARFExpression::Evaluate(&context, registers.get(), module,
                                          expression_data, nullptr,
                                          eRegisterKindDWARF, nullptr, nullptr);
  ASSERT_THAT_EXPECTED(result, llvm::Succeeded());
  EXPECT_EQ(result->GetValueType(), Value::ValueType::LoadAddress);
  EXPECT_EQ(result->GetScalar().ULongLong(), 0x1200cu);
  ExecutionContext second_context(second.get());
  auto second_result = DWARFExpression::Evaluate(
      &second_context, second->registers.get(), module, expression_data,
      nullptr, eRegisterKindDWARF, nullptr, nullptr);
  ASSERT_THAT_EXPECTED(second_result, llvm::Succeeded());
  EXPECT_EQ(second_result->GetScalar().ULongLong(), 0x1300cu);
  for (uint8_t op : {llvm::dwarf::DW_OP_form_tls_address,
                     llvm::dwarf::DW_OP_GNU_push_tls_address}) {
    const uint8_t bytes[] = {llvm::dwarf::DW_OP_constu, 12, op};
    DataExtractor data(bytes, sizeof(bytes), eByteOrderBig, 4);
    auto address = DWARFExpression::Evaluate(&context, registers.get(), module,
                                             data, nullptr, eRegisterKindDWARF,
                                             nullptr, nullptr);
    ASSERT_THAT_EXPECTED(address, llvm::Succeeded());
    EXPECT_EQ(address->GetScalar().ULongLong(), 0x1200cu);
  }

  auto other = npdb::MakeGlobalLocationExpression(tls->GetID(), 12, module);
  ASSERT_TRUE(other.GetExpressionData(expression_data));
  auto global = DWARFExpression::Evaluate(&context, registers.get(), module,
                                          expression_data, nullptr,
                                          eRegisterKindDWARF, nullptr, nullptr);
  ASSERT_THAT_EXPECTED(global, llvm::Succeeded());
  EXPECT_EQ(global->GetValueType(), Value::ValueType::FileAddress);
  EXPECT_EQ(global->GetScalar().ULongLong(), 0x8200100cu);
  EXPECT_FALSE(npdb::MakeGlobalLocationExpression(tls->GetID(), 64, module,
                                                  /*is_thread_local=*/true)
                   .IsValid());
  EXPECT_FALSE(npdb::MakeGlobalLocationExpression(99, 0, module,
                                                  /*is_thread_local=*/true)
                   .IsValid());

  // The block pointer is read again after a stop, never cached per module.
  const uint8_t replacement[] = {0, 1, 0x30, 0};
  Status error;
  ASSERT_EQ(
      process->WriteMemory(0x10000, replacement, sizeof(replacement), error),
      sizeof(replacement));
  EXPECT_EQ(thread->GetThreadLocalData(module, 12), 0x1300cu);

  EXPECT_EQ(thread->GetThreadLocalData(module, 64), LLDB_INVALID_ADDRESS);
  EXPECT_EQ(thread->GetThreadLocalData(module, LLDB_INVALID_ADDRESS),
            LLDB_INVALID_ADDRESS);
  EXPECT_EQ(thread->GetThreadLocalData({}, 0), LLDB_INVALID_ADDRESS);
  auto different_module = std::make_shared<Module>(file->moduleSpec());
  EXPECT_EQ(thread->GetThreadLocalData(different_module, 0),
            LLDB_INVALID_ADDRESS);
  for (addr_t value :
       {addr_t(0), addr_t(0x10001), addr_t(0x100000), LLDB_INVALID_ADDRESS}) {
    registers->Set("r13", RegisterValue(value));
    EXPECT_EQ(thread->GetThreadLocalData(module, 0), LLDB_INVALID_ADDRESS);
  }
  registers->Set("r13", RegisterValue(uint64_t(0x10000)));
  registers->unreadable = "r13";
  EXPECT_EQ(thread->GetThreadLocalData(module, 0), LLDB_INVALID_ADDRESS);
  registers->unreadable.clear();
  registers->Set("r13", RegisterValue(uint64_t(0x10400)));
  process->fail_memory = true;
  EXPECT_EQ(thread->GetThreadLocalData(module, 0), LLDB_INVALID_ADDRESS);
  process->fail_memory = false;
  registers->Set("r13", RegisterValue(uint64_t(0x10000)));
  for (uint32_t block : {0u, 0x12001u, 0xfffffffcu}) {
    uint8_t bytes[4];
    llvm::support::endian::write32be(bytes, block);
    ASSERT_EQ(process->WriteMemory(0x10000, bytes, sizeof(bytes), error),
              sizeof(bytes));
    EXPECT_EQ(thread->GetThreadLocalData(module, 12), LLDB_INVALID_ADDRESS);
  }
}

TEST_F(Xbox360ABI, PackedUnwindExpressionsReadFourByteLRAndCRSaves) {
  auto file = TestFile::fromYaml(R"(
--- !COFF
OptionalHeader:
  ImageBase: 0x82000000
  SectionAlignment: 4096
  FileAlignment: 512
  Subsystem: IMAGE_SUBSYSTEM_XBOX
  ExceptionTable:
    RelativeVirtualAddress: 8192
    Size: 16
header:
  Machine: IMAGE_FILE_MACHINE_PPCBE
  Characteristics: [ IMAGE_FILE_EXECUTABLE_IMAGE ]
sections:
  - Name: .text
    VirtualAddress: 4096
    VirtualSize: 60
    Characteristics: [ IMAGE_SCN_CNT_CODE, IMAGE_SCN_MEM_READ, IMAGE_SCN_MEM_EXECUTE ]
    SectionData: 7D8802A69181FFF87D8000269181FFF49421FF8048000101382100808181FFF87D8803A64E8000207D8802A69181FFF89421FF80600000004E800020
  - Name: .pdata
    VirtualAddress: 8192
    VirtualSize: 16
    Characteristics: [ IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_MEM_READ ]
    SectionData: 8200100040000A058200102840000503
symbols: []
...
)");
  ASSERT_THAT_EXPECTED(file, llvm::Succeeded());
  auto module = std::make_shared<Module>(file->moduleSpec());
  auto *object = module->GetObjectFile();
  ASSERT_TRUE(object);
  auto cfi = object->CreateCallFrameInfo();
  ASSERT_TRUE(cfi);
  UnwindPlan plan(eRegisterKindDWARF);
  ASSERT_TRUE(cfi->GetUnwindPlan(Address(0x82001014), plan));
  auto row = plan.GetRowForFunctionOffset(20);
  ASSERT_TRUE(row);
  registers->Set("r1", RegisterValue(uint64_t(0xdeadbeef00011000)));
  EXPECT_EQ(abi->FixDataAddress(registers->Get("r1")) +
                row->GetCFAValue().GetOffset(),
            0x11080u);
  // The bytes after the four-byte LR spill must not become part of the LR.
  llvm::support::endian::write32be(process->memory.data() + 0x1078, 0x82001038);
  llvm::support::endian::write32be(process->memory.data() + 0x107c, 0xdeadbeef);
  llvm::support::endian::write32be(process->memory.data() + 0x1074, 0x12345678);
  ExecutionContext context(thread.get());
  Value cfa(Scalar(uint64_t(0x11080)));
  cfa.SetValueType(Value::ValueType::LoadAddress);
  for (auto [reg, expected] : {std::pair{65u, 0x82001038u},
                               {70u, 3u},
                               {71u, 4u},
                               {72u, 5u},
                               {73u, 6u}}) {
    UnwindPlan::Row::AbstractRegisterLocation location;
    ASSERT_TRUE(row->GetRegisterInfo(reg, location));
    ASSERT_TRUE(location.IsDWARFExpression());
    const uint8_t *bytes;
    uint16_t size;
    location.GetDWARFExpr(&bytes, size);
    DataExtractor expression(bytes, size, eByteOrderBig, 4);
    auto result =
        DWARFExpression::Evaluate(&context, registers.get(), {}, expression,
                                  nullptr, eRegisterKindDWARF, &cfa, nullptr);
    ASSERT_THAT_EXPECTED(result, llvm::Succeeded());
    EXPECT_EQ(result->GetScalar().ULongLong(), expected);
  }

  target->GetImages().Append(module);
  auto text = object->GetSectionList()->FindSectionByName(ConstString(".text"));
  ASSERT_TRUE(text);
  ASSERT_TRUE(target->SetSectionLoadAddress(text, 0x82001000));
  registers->Set("lr", RegisterValue(uint64_t(0xdeadbeef82001038)));
  registers->Set("r12", RegisterValue(uint64_t(0xdeadbeef82001038)));
  for (uint32_t offset : {4u, 8u, 20u, 24u, 28u, 32u, 36u}) {
    registers->Set("pc", RegisterValue(uint32_t(0x82001000 + offset)));
    registers->Set("r1", RegisterValue(uint64_t(
        offset < 20 || offset >= 28 ? 0xdeadbeef00011080 : 0xdeadbeef00011000)));
    UnwindLLDB unwinder(*thread);
    addr_t frame_cfa, pc;
    bool zeroth;
    ASSERT_TRUE(unwinder.GetFrameInfoAtIndex(0, frame_cfa, pc, zeroth));
    EXPECT_EQ(frame_cfa, 0x11080u);
    ASSERT_TRUE(unwinder.GetFrameInfoAtIndex(1, frame_cfa, pc, zeroth));
    EXPECT_EQ(pc, 0x82001038u);
    EXPECT_EQ(frame_cfa, 0x11100u);
  }
}

TEST_F(Xbox360ABI, PointerReferenceAndEnumReturns) {
  registers->Set("r3", RegisterValue(uint64_t(0xdeadbeef82001000)));
  ExpectReturnBytes(Type(eBasicTypeInt).GetPointerType(), {0x82, 0, 0x10, 0});
  ExpectReturnBytes(Type(eBasicTypeInt).GetLValueReferenceType(),
                    {0x82, 0, 0x10, 0});
  auto enumeration = ast->CreateEnumerationType(
      "ReturnEnum", ast->getASTContext().getTranslationUnitDecl(),
      OptionalClangModuleID(), Declaration(), Type(eBasicTypeUnsignedShort),
      /*is_scoped=*/true);
  TypeSystemClang::StartTagDeclarationDefinition(enumeration);
  TypeSystemClang::CompleteTagDeclarationDefinition(enumeration);
  ExpectReturnBytes(enumeration, {0x10, 0});
}

TEST_F(Xbox360ABI, WideAndComplexIntegerReturns) {
  registers->Set("r3", RegisterValue(uint64_t(0x0123456789abcdef)));
  registers->Set("r4", RegisterValue(uint64_t(0xfedcba9876543210)));
  ExpectReturnBytes(Type(eBasicTypeUnsignedInt128),
                    {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc,
                     0xba, 0x98, 0x76, 0x54, 0x32, 0x10});
  auto complex_int = ast->GetType(ast->getASTContext().getComplexType(
      ClangUtil::GetQualType(Type(eBasicTypeInt))));
  ExpectReturnBytes(complex_int,
                    {0x89, 0xab, 0xcd, 0xef, 0x76, 0x54, 0x32, 0x10});
}

TEST_F(Xbox360ABI, FloatingPointReturnsUseDoublePrecisionFPRContents) {
  registers->Set("f1", RegisterValue(uint64_t(0x3ff4000000000000)));
  ExpectReturnBytes(Type(eBasicTypeFloat), {0x3f, 0xa0, 0, 0});
  ExpectReturnBytes(Type(eBasicTypeDouble), {0x3f, 0xf4, 0, 0, 0, 0, 0, 0});
  registers->Set("f1", RegisterValue(uint64_t(0x8000000000000000)));
  ExpectReturnBytes(Type(eBasicTypeFloat), {0x80, 0, 0, 0});
  registers->Set("f1", RegisterValue(uint64_t(0x7ff0000000000000)));
  ExpectReturnBytes(Type(eBasicTypeFloat), {0x7f, 0x80, 0, 0});
  registers->Set("f1", RegisterValue(uint64_t(0x7ff8123456789abc)));
  ExpectReturnBytes(Type(eBasicTypeDouble),
                    {0x7f, 0xf8, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc});
}

TEST_F(Xbox360ABI, ComplexFloatingPointReturns) {
  registers->Set("f1", RegisterValue(1.25));
  registers->Set("f2", RegisterValue(-3.5));
  ExpectReturnBytes(Type(eBasicTypeFloatComplex),
                    {0x3f, 0xa0, 0, 0, 0xc0, 0x60, 0, 0});
  ExpectReturnBytes(
      Type(eBasicTypeDoubleComplex),
      {0x3f, 0xf4, 0, 0, 0, 0, 0, 0, 0xc0, 0x0c, 0, 0, 0, 0, 0, 0});
}

TEST_F(Xbox360ABI, VectorReturnsUseV1NotV2) {
  const std::array<uint8_t, 16> bytes = {0, 1, 2,  3,  4,  5,  6,  7,
                                         8, 9, 10, 11, 12, 13, 14, 15};
  registers->Set("v1", RegisterValue(bytes, eByteOrderBig));
  registers->Set("v2", RegisterValue(std::array<uint8_t, 16>{}, eByteOrderBig));
  ExpectReturnBytes(VectorType(), bytes);
  auto float_vector = ast->GetType(ast->getASTContext().getVectorType(
      ClangUtil::GetQualType(Type(eBasicTypeFloat)), 4,
      clang::VectorKind::Generic));
  ExpectReturnBytes(float_vector, bytes);
  auto *reg =
      const_cast<RegisterInfo *>(registers->GetRegisterInfoByName("v1"));
  reg->name = reg->alt_name;
  reg->alt_name = nullptr;
  ExpectReturnBytes(float_vector, bytes);
}

TEST_F(Xbox360ABI, SetIntegerAndPointerReturns) {
  for (auto [basic, bytes, expected] :
       {std::tuple{eBasicTypeSignedChar, std::vector<uint8_t>{0x80},
                   0xffffffffffffff80ull},
        {eBasicTypeUnsignedChar, {0x80}, 0x80ull},
        {eBasicTypeShort, {0x80, 0x01}, 0xffffffffffff8001ull},
        {eBasicTypeUnsignedShort, {0x80, 0x01}, 0x8001ull},
        {eBasicTypeInt, {0x80, 0, 0, 1}, 0xffffffff80000001ull},
        {eBasicTypeUnsignedInt, {0x80, 0, 0, 1}, 0x80000001ull},
        {eBasicTypeUnsignedLongLong,
         {1, 2, 3, 4, 5, 6, 7, 8},
         0x0102030405060708ull}}) {
    ASSERT_TRUE(SetReturn(Type(basic), bytes).Success());
    EXPECT_EQ(registers->Get("r3"), expected);
    ExpectReturnBytes(Type(basic), bytes);
  }
  auto pointer = Type(eBasicTypeInt).GetPointerType();
  ASSERT_TRUE(SetReturn(pointer, {0x82, 0, 0x12, 0x34}).Success());
  EXPECT_EQ(registers->Get("r3"), 0x82001234u);
  auto reference = Type(eBasicTypeInt).GetLValueReferenceType();
  ASSERT_TRUE(SetReturn(reference, {0x82, 0, 0x56, 0x78}).Success());
  EXPECT_EQ(registers->Get("r3"), 0x82005678u);

  auto frame = ReturnFrame();
  auto value =
      ReturnValue(Type(eBasicTypeUnsignedInt), {1, 2, 3, 4}, eByteOrderLittle);
  ASSERT_TRUE(abi->SetReturnValueObject(frame, value).Success());
  EXPECT_EQ(registers->Get("r3"), 0x04030201u);
}

TEST_F(Xbox360ABI, SetSplitIntegerAndComplexReturns) {
  const std::array<uint8_t, 16> wide = {
      1, 2, 3, 4, 5, 6, 7, 8, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88};
  ASSERT_TRUE(SetReturn(Type(eBasicTypeUnsignedInt128), wide).Success());
  EXPECT_EQ(registers->Get("r3"), 0x0102030405060708ull);
  EXPECT_EQ(registers->Get("r4"), 0x8182838485868788ull);
  ExpectReturnBytes(Type(eBasicTypeUnsignedInt128), wide);
  auto frame = ReturnFrame();
  std::vector<uint8_t> little(wide.rbegin(), wide.rend());
  auto value =
      ReturnValue(Type(eBasicTypeUnsignedInt128), little, eByteOrderLittle);
  ASSERT_TRUE(abi->SetReturnValueObject(frame, value).Success());
  EXPECT_EQ(registers->Get("r3"), 0x0102030405060708ull);
  EXPECT_EQ(registers->Get("r4"), 0x8182838485868788ull);

  auto complex = ast->GetType(ast->getASTContext().getComplexType(
      ClangUtil::GetQualType(Type(eBasicTypeInt))));
  ASSERT_TRUE(
      SetReturn(complex, {0, 0, 0, 1, 0xff, 0xff, 0xff, 0xfe}).Success());
  EXPECT_EQ(registers->Get("r3") & UINT32_MAX, 1u);
  EXPECT_EQ(registers->Get("r4") & UINT32_MAX, 0xfffffffeu);
  ExpectReturnBytes(complex, {0, 0, 0, 1, 0xff, 0xff, 0xff, 0xfe});
}

TEST_F(Xbox360ABI, SetFloatingAndComplexReturns) {
  ASSERT_TRUE(SetReturn(Type(eBasicTypeFloat), {0x3f, 0xa0, 0, 0}).Success());
  EXPECT_EQ(registers->Get("f1"), 0x3ff4000000000000ull);
  ExpectReturnBytes(Type(eBasicTypeFloat), {0x3f, 0xa0, 0, 0});
  for (auto bytes : {std::array<uint8_t, 4>{0x80, 0, 0, 0},
                     {0x7f, 0x80, 0, 0},
                     {0x7f, 0xc0, 0, 1}}) {
    ASSERT_TRUE(SetReturn(Type(eBasicTypeFloat), bytes).Success());
    ExpectReturnBytes(Type(eBasicTypeFloat), bytes);
  }
  const std::array<uint8_t, 8> negative_zero = {0x80, 0, 0, 0, 0, 0, 0, 0};
  ASSERT_TRUE(SetReturn(Type(eBasicTypeDouble), negative_zero).Success());
  EXPECT_EQ(registers->Get("f1"), 0x8000000000000000ull);
  ExpectReturnBytes(Type(eBasicTypeDouble), negative_zero);
  const std::array<uint8_t, 8> complex_float = {0x3f, 0xa0, 0, 0,
                                                0xc0, 0x60, 0, 0};
  ASSERT_TRUE(SetReturn(Type(eBasicTypeFloatComplex), complex_float).Success());
  EXPECT_EQ(registers->Get("f1"), 0x3ff4000000000000ull);
  EXPECT_EQ(registers->Get("f2"), 0xc00c000000000000ull);
  ExpectReturnBytes(Type(eBasicTypeFloatComplex), complex_float);
  const std::array<uint8_t, 16> complex_double = {0x3f, 0xf4, 0, 0, 0, 0, 0, 0,
                                                  0xc0, 0x0c, 0, 0, 0, 0, 0, 0};
  ASSERT_TRUE(
      SetReturn(Type(eBasicTypeDoubleComplex), complex_double).Success());
  ExpectReturnBytes(Type(eBasicTypeDoubleComplex), complex_double);
}

TEST_F(Xbox360ABI, SetVectorAndSmallAggregateReturns) {
  const std::array<uint8_t, 16> bytes = {0, 1, 2,  3,  4,  5,  6,  7,
                                         8, 9, 10, 11, 12, 13, 14, 15};
  ASSERT_TRUE(SetReturn(VectorType(), bytes).Success());
  ExpectReturnBytes(VectorType(), bytes);
  auto *reg =
      const_cast<RegisterInfo *>(registers->GetRegisterInfoByName("v1"));
  reg->name = reg->alt_name;
  reg->alt_name = nullptr;
  ASSERT_TRUE(SetReturn(VectorType(), bytes).Success());
  ExpectReturnBytes(VectorType(), bytes);
  for (auto basic : {eBasicTypeUnsignedChar, eBasicTypeUnsignedShort,
                     eBasicTypeUnsignedInt, eBasicTypeUnsignedLongLong}) {
    auto type = Record("ForcedAggregate", Type(basic));
    auto size = type.GetByteSize(thread.get());
    ASSERT_TRUE(size);
    auto data = llvm::ArrayRef(bytes).take_front(*size);
    ASSERT_TRUE(SetReturn(type, data).Success());
    ExpectReturnBytes(type, data);
  }
  auto type = Record("ForcedFloatAggregate", Type(eBasicTypeFloat));
  registers->Set("f1", RegisterValue(uint64_t(0x4000000000000000)));
  ASSERT_TRUE(SetReturn(type, {0x3f, 0xa0, 0, 0}).Success());
  EXPECT_EQ(registers->Get("r3"), 0x3fa00000u);
  EXPECT_EQ(registers->Get("f1"), 0x4000000000000000ull);
}

TEST_F(Xbox360ABI, SetReturnUsesFunctionContextAndRejectsIndirectStorage) {
  auto type = Record("ForcedCXXResult", Type(eBasicTypeUnsignedInt), true);
  auto instance = FunctionContext(type, true);
  auto statik = FunctionContext(type, false);
  ASSERT_TRUE(SetReturn(type, {1, 2, 3, 4}, &statik).Success());
  EXPECT_EQ(registers->Get("r3"), 0x01020304u);
  registers->Set("r3", RegisterValue(uint64_t(TestProcess::base)));
  auto before = process->memory;
  EXPECT_TRUE(SetReturn(type, {1, 2, 3, 4}, &instance).Fail());
  EXPECT_EQ(registers->Get("r3"), TestProcess::base);
  EXPECT_EQ(process->memory, before);
  CompilerDeclContext missing;
  EXPECT_TRUE(SetReturn(type, {1, 2, 3, 4}, &missing).Fail());

  const std::array<uint8_t, 16> bytes = {};
  auto vector_type = VectorType();
  instance = FunctionContext(vector_type, true);
  EXPECT_TRUE(SetReturn(vector_type, bytes, &instance).Fail());
  EXPECT_TRUE(SetReturn(Record("ForcedLarge", vector_type), bytes).Fail());
  auto private_type = Record("ForcedPrivate", Type(eBasicTypeUnsignedInt), true,
                             eAccessPrivate);
  EXPECT_TRUE(SetReturn(private_type, {1, 2, 3, 4}).Fail());
}

TEST_F(Xbox360ABI, SetReturnHonorsPassInReg) {
  auto type = Record("ForcedPassInReg", Type(eBasicTypeFloat), true);
  ClangUtil::GetAsTagDecl(type)->addAttr(
      clang::PassInRegAttr::CreateImplicit(ast->getASTContext()));
  auto instance = FunctionContext(type, true);
  ASSERT_TRUE(SetReturn(type, {0x3f, 0xa0, 0, 0}, &instance).Success());
  EXPECT_EQ(registers->Get("f1"), 0x3ff4000000000000ull);
  ExpectReturnBytes(type, {0x3f, 0xa0, 0, 0}, &instance);
}

TEST_F(Xbox360ABI, SetReturnWritesLiveVolatileRegisters) {
  auto destination = std::make_shared<TestRegisters>(*thread);
  auto frame = ReturnFrame(destination);
  auto value = ReturnValue(Type(eBasicTypeUnsignedInt), {1, 2, 3, 4});
  registers->Set("r3", RegisterValue(uint64_t(0xdeadbeef)));
  ASSERT_TRUE(abi->SetReturnValueObject(frame, value).Success());
  EXPECT_EQ(registers->Get("r3"), 0x01020304u);
  EXPECT_EQ(destination->Get("r3"), LLDB_INVALID_ADDRESS);
}

TEST_F(Xbox360ABI, ForcedReturnPreservesResultWhenRestoringCaller) {
  registers->EnableRegisterSet();
  auto caller = std::make_shared<TestRegisters>(*thread);
  caller->EnableRegisterSet();
  caller->unreadable = "r3";
  caller->unwritable = "r3";
  caller->Set("r1", RegisterValue(uint64_t(0x12000)));
  caller->Set("pc", RegisterValue(uint32_t(0x82004321)));
  auto current = std::make_shared<StackFrame>(
      thread, 0, 0, registers, 0x11000, addr_t(0x82001000), true, nullptr);
  thread->frames = {current, ReturnFrame(caller)};
  auto value = ReturnValue(Type(eBasicTypeUnsignedInt), {1, 2, 3, 4});
  Status error = thread->ReturnFromFrame(current, value, false);
  ASSERT_TRUE(error.Success()) << error.AsCString();
  EXPECT_EQ(registers->Get("r3"), 0x01020304u);
  EXPECT_EQ(registers->Get("r1"), 0x12000u);
  EXPECT_EQ(registers->Get("pc"), 0x82004321u);
  thread->frames.clear();
}

TEST_F(Xbox360ABI, SetReturnReportsFailuresAndRestoresSplitResults) {
  auto frame = ReturnFrame();
  auto value = ReturnValue(Type(eBasicTypeUnsignedInt), {1, 2, 3, 4});
  StackFrameSP missing_frame;
  ValueObjectSP missing_value;
  EXPECT_TRUE(abi->SetReturnValueObject(missing_frame, value).Fail());
  EXPECT_TRUE(abi->SetReturnValueObject(frame, missing_value).Fail());
  ASSERT_TRUE(SetReturn(Type(eBasicTypeVoid), {}).Success());
  registers->Set("r3", RegisterValue(uint64_t(0x1234)));
  auto unavailable = ValueObjectConstResult::Create(
      thread.get(), Status::FromErrorString("unavailable test result"));
  EXPECT_TRUE(abi->SetReturnValueObject(frame, unavailable).Fail());
  EXPECT_EQ(registers->Get("r3"), 0x1234u);
  registers->unwritable = "r3";
  EXPECT_TRUE(abi->SetReturnValueObject(frame, value).Fail());
  registers->unwritable = "r4";
  registers->Set("r3", RegisterValue(uint64_t(0x1234)));
  const std::array<uint8_t, 16> wide = {};
  EXPECT_TRUE(SetReturn(Type(eBasicTypeUnsignedInt128), wide).Fail());
  EXPECT_EQ(registers->Get("r3"), 0x1234u);
  registers->unwritable.clear();
  registers->unreadable = "r4";
  EXPECT_TRUE(SetReturn(Type(eBasicTypeUnsignedInt128), wide).Fail());
  EXPECT_EQ(registers->Get("r3"), 0x1234u);
  registers->unreadable.clear();
  auto *reg =
      const_cast<RegisterInfo *>(registers->GetRegisterInfoByName("r3"));
  reg->byte_size = 4;
  EXPECT_TRUE(abi->SetReturnValueObject(frame, value).Fail());
}

TEST_F(Xbox360ABI, SmallAggregateReturnsAreRawR3MemoryImages) {
  registers->Set("r3", RegisterValue(uint64_t(0x0123456789abcdef)));
  ExpectReturnBytes(Record("ByteResult", Type(eBasicTypeUnsignedChar)), {0xef});
  ExpectReturnBytes(Record("ShortResult", Type(eBasicTypeUnsignedShort)),
                    {0xcd, 0xef});
  ExpectReturnBytes(Record("IntResult", Type(eBasicTypeUnsignedInt)),
                    {0x89, 0xab, 0xcd, 0xef});
  ExpectReturnBytes(Record("LongResult", Type(eBasicTypeUnsignedLongLong)),
                    {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef});
  // An ordinary struct containing float still returns in r3, not f1.
  registers->Set("r3", RegisterValue(uint64_t(0xdeadbeef3fa00000)));
  registers->unreadable = "f1";
  ExpectReturnBytes(Record("FloatResult", Type(eBasicTypeFloat)),
                    {0x3f, 0xa0, 0, 0});
}

TEST_F(Xbox360ABI, IndirectReturnsUseTheLow32BitsOfR3AndAreFrozen) {
  auto array = ast->GetType(ast->getASTContext().getConstantArrayType(
      ClangUtil::GetQualType(Type(eBasicTypeUnsignedChar)), llvm::APInt(64, 3),
      nullptr, clang::ArraySizeModifier::Normal, 0));
  auto type = Record("ThreeByteResult", array);
  registers->Set("r3", RegisterValue(uint64_t(0xdeadbeef00010000)));
  process->memory[0] = 0x12;
  process->memory[1] = 0x34;
  process->memory[2] = 0x56;
  ExpectReturnBytes(type, {0x12, 0x34, 0x56});
  auto value = Return(type);
  ASSERT_TRUE(value);
  Status error;
  const uint8_t replacement = 0xff;
  ASSERT_EQ(process->WriteMemory(TestProcess::base, &replacement, 1, error),
            1u);
  DataExtractor data;
  ASSERT_EQ(value->GetData(data, error), 3u);
  lldb::offset_t offset = 0;
  EXPECT_EQ(data.GetU8(&offset), 0x12);
  auto array_value = value->GetChildAtIndex(0);
  ASSERT_TRUE(array_value);
  auto element = array_value->GetChildAtIndex(0);
  ASSERT_TRUE(element);
  EXPECT_EQ(element->GetValueAsUnsigned(0), 0x12u);

  type = Record("LargeResult", VectorType());
  const std::array<uint8_t, 16> bytes = {0, 1, 2,  3,  4,  5,  6,  7,
                                         8, 9, 10, 11, 12, 13, 14, 15};
  ASSERT_EQ(process->WriteMemory(TestProcess::base, bytes.data(), bytes.size(),
                                 error),
            bytes.size());
  ExpectReturnBytes(type, bytes);
}

TEST_F(Xbox360ABI, InstanceAndStaticMethodReturnsDiffer) {
  auto type = Record("CXXResult", Type(eBasicTypeUnsignedInt), /*cxx=*/true);
  auto instance = FunctionContext(type, true);
  auto statik = FunctionContext(type, false);
  registers->Set("r3", RegisterValue(uint64_t(0x1234567800010000)));
  process->memory[0] = 0x12;
  process->memory[1] = 0x34;
  process->memory[2] = 0x56;
  process->memory[3] = 0x78;
  ExpectReturnBytes(type, {0x12, 0x34, 0x56, 0x78}, &instance);
  ExpectReturnBytes(type, {0, 1, 0, 0}, &statik);
  auto vector_type = VectorType();
  instance = FunctionContext(vector_type, true);
  statik = FunctionContext(vector_type, false);
  std::array<uint8_t, 16> vector = {};
  vector.fill(0xab);
  registers->Set("v1", RegisterValue(vector, eByteOrderBig));
  ExpectReturnBytes(vector_type, vector, &statik);
  ExpectReturnBytes(vector_type, llvm::ArrayRef(process->memory).take_front(16),
                    &instance);
}

TEST_F(Xbox360ABI, NonTrivialMSVCRecordsReturnIndirectly) {
  auto type = Record("PrivateResult", Type(eBasicTypeUnsignedInt),
                     /*cxx=*/true, eAccessPrivate);
  registers->Set("r3", RegisterValue(uint64_t(0x10000)));
  process->memory[0] = 0x12;
  process->memory[1] = 0x34;
  process->memory[2] = 0x56;
  process->memory[3] = 0x78;
  ExpectReturnBytes(type, {0x12, 0x34, 0x56, 0x78});
}

TEST_F(Xbox360ABI, ExplicitPassInRegAndIntrinsicReturnOverrides) {
  auto intrinsic = Record("IntrinsicResult", VectorType(), true);
  auto &context = ast->getASTContext();
  ClangUtil::GetAsTagDecl(intrinsic)->addAttr(
      clang::IntrinTypeAttr::CreateImplicit(context, 128, 128));
  const std::array<uint8_t, 16> bytes = {0, 1, 2,  3,  4,  5,  6,  7,
                                         8, 9, 10, 11, 12, 13, 14, 15};
  registers->Set("v1", RegisterValue(bytes, eByteOrderBig));
  ExpectReturnBytes(intrinsic, bytes);
  auto instance = FunctionContext(intrinsic, true);
  registers->Set("r3", RegisterValue(uint64_t(0x10000)));
  ExpectReturnBytes(intrinsic, llvm::ArrayRef(process->memory).take_front(16),
                    &instance);
  auto wrapper = Record("VectorWrapper", intrinsic, true);
  ClangUtil::GetAsTagDecl(wrapper)->addAttr(
      clang::PassInRegAttr::CreateImplicit(context));
  instance = FunctionContext(wrapper, true);
  ExpectReturnBytes(wrapper, bytes, &instance);

  auto floating = Record("FloatWrapper", Type(eBasicTypeFloat), true);
  ClangUtil::GetAsTagDecl(floating)->addAttr(
      clang::PassInRegAttr::CreateImplicit(context));
  registers->Set("f1", RegisterValue(1.25));
  instance = FunctionContext(floating, true);
  ExpectReturnBytes(floating, {0x3f, 0xa0, 0, 0}, &instance);
}

TEST_F(Xbox360ABI, ReturnReadFailuresDoNotFabricateValues) {
  registers->unreadable = "r3";
  EXPECT_FALSE(Return(Type(eBasicTypeInt)));
  registers->unreadable = "f1";
  EXPECT_FALSE(Return(Type(eBasicTypeFloat)));
  registers->unreadable = "v1";
  EXPECT_FALSE(Return(VectorType()));
  registers->unreadable.clear();
  registers->Set("r3", RegisterValue(uint64_t(0)));
  auto indirect = Record("UnreadableResult", VectorType());
  EXPECT_FALSE(Return(indirect));
  registers->Set("r3", RegisterValue(uint64_t(0xfffffff8)));
  EXPECT_FALSE(Return(indirect));
  registers->Set("r3", RegisterValue(uint64_t(0x10000)));
  process->fail_memory = true;
  EXPECT_FALSE(Return(indirect));
  process->fail_memory = false;
  auto small = Record("UnknownContextResult", Type(eBasicTypeInt));
  CompilerDeclContext unknown;
  EXPECT_FALSE(Return(small, &unknown));
  EXPECT_FALSE(Return(Type(eBasicTypeVoid)));
}
