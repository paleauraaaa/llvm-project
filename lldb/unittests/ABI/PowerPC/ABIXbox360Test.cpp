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
#include "Plugins/Platform/Linux/PlatformLinux.h"
#include "Plugins/TypeSystem/Clang/TypeSystemClang.h"
#include "TestingSupport/SubsystemRAII.h"
#include "TestingSupport/Symbol/ClangTestUtils.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Value.h"
#include "lldb/Host/FileSystem.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/Xbox360RegisterInfo.h"
#include "lldb/Utility/Listener.h"
#include "lldb/Utility/RegisterValue.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/TargetSelect.h"
#include "gtest/gtest.h"
#include <array>

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
    if (fail_memory || address < base || address - base > memory.size() ||
        size > memory.size() - (address - base)) {
      error = Status::FromErrorString("unwritable test memory");
      return 0;
    }
    memcpy(memory.data() + address - base, buffer, size);
    return size;
  }

  static constexpr addr_t base = 0x10000;
  std::array<uint8_t, 0x4000> memory = {};
  bool fail_memory = false;
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
  size_t GetRegisterSetCount() override { return 0; }
  const RegisterSet *GetRegisterSet(size_t) override { return nullptr; }
  ByteOrder GetByteOrder() override { return eByteOrderBig; }
  bool ReadRegister(const RegisterInfo *reg, RegisterValue &value) override {
    if (!reg || unreadable == reg->name)
      return false;
    value = values[reg->kinds[eRegisterKindLLDB]];
    return true;
  }
  bool WriteRegister(const RegisterInfo *reg,
                     const RegisterValue &value) override {
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
  bool CalculateStopInfo() override { return false; }
  std::shared_ptr<TestRegisters> registers;
};

class Xbox360ABI : public testing::Test {
  SubsystemRAII<FileSystem, HostInfo, platform_linux::PlatformLinux> subsystems;

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
