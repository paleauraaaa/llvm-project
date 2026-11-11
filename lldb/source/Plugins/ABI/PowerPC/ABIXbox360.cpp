//===-- ABIXbox360.cpp -----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIXbox360.h"

#include "Plugins/ExpressionParser/Clang/ClangUtil.h"
#include "Plugins/TypeSystem/Clang/TypeSystemClang.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Section.h"
#include "lldb/Core/Value.h"
#include "lldb/Symbol/CompilerDeclContext.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/Xbox360RegisterInfo.h"
#include "lldb/Utility/ArchSpec.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"
#include "lldb/ValueObject/ValueObjectConstResult.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MathExtras.h"

using namespace lldb;
using namespace lldb_private;

LLDB_PLUGIN_DEFINE(ABIXbox360)

namespace {
namespace Stack {
constexpr unsigned kAlignment = 16;
constexpr unsigned kLinkageSize = 16;
constexpr unsigned kHomeSlots = 8;
constexpr unsigned kSlotSize = 8;
} // namespace Stack

bool Fail(llvm::StringRef message) {
  LLDB_LOG(GetLog(LLDBLog::Expressions), "Xbox 360 ABI: {0}", message);
  return false;
}

bool IsNumberedRegister(llvm::StringRef name, llvm::StringRef prefix,
                        unsigned first, unsigned last) {
  if (!name.consume_front(prefix))
    return false;
  unsigned number;
  return !name.empty() && !name.getAsInteger(10, number) && number >= first &&
         number <= last;
}

bool IsCodeAddressValid(addr_t pc) {
  return pc && pc <= UINT32_MAX && (pc & 3) == 0;
}

addr_t NormalizeAddress(addr_t address) {
  return address == LLDB_INVALID_ADDRESS ? address : address & UINT32_MAX;
}

enum class ReturnKind { GPR, FPR, Vector, Memory };

struct ReturnInfo {
  ReturnKind kind;
  uint32_t element_size;
  uint32_t count = 1;
};

// Keep this consistent with isTrivialForMSVC in MicrosoftCXXABI.cpp.
bool IsDirectCXXRecord(const clang::CXXRecordDecl &record) {
  if (!record.canPassInRegisters() || record.hasProtectedFields() ||
      record.hasPrivateFields() || record.getNumBases() ||
      record.isPolymorphic() || record.hasNonTrivialCopyAssignment() ||
      (record.needsImplicitCopyAssignment() &&
       !record.hasSimpleCopyAssignment()) ||
      record.hasNonTrivialDestructor())
    return false;
  for (const clang::Decl *decl : record.decls()) {
    if (const auto *ctor = llvm::dyn_cast<clang::CXXConstructorDecl>(decl)) {
      if (ctor->isUserProvided())
        return false;
    } else if (const auto *templ =
                   llvm::dyn_cast<clang::FunctionTemplateDecl>(decl)) {
      if (llvm::isa<clang::CXXConstructorDecl>(templ->getTemplatedDecl()))
        return false;
    } else if (const auto *method =
                   llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {
      if (method->isCopyAssignmentOperator() && method->isDeleted())
        return false;
    }
  }
  return true;
}

llvm::Expected<ReturnInfo>
ClassifyReturn(CompilerType &type, uint64_t size,
               const CompilerDeclContext *function_decl_context) {
  auto unsupported = []() {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "unsupported Xbox 360 return type");
  };
  bool is_signed = false;
  if (type.IsIntegerOrEnumerationType(is_signed) ||
      type.IsPointerOrReferenceType()) {
    if (size == 1 || size == 2 || size == 4 || size == 8)
      return ReturnInfo{ReturnKind::GPR, static_cast<uint32_t>(size)};
    if (size == 16 && type.IsIntegerOrEnumerationType(is_signed))
      return ReturnInfo{ReturnKind::GPR, 8, 2};
    return unsupported();
  }

  uint32_t count = 0;
  bool is_complex = false;
  if (!type.IsVectorType() && type.IsFloatingPointType(count, is_complex)) {
    uint64_t element_size = is_complex ? size / 2 : size;
    if ((element_size != 4 && element_size != 8) ||
        size != element_size * (is_complex ? 2 : 1))
      return unsupported();
    return ReturnInfo{ReturnKind::FPR, static_cast<uint32_t>(element_size),
                      is_complex ? 2u : 1u};
  }
  if (ClangUtil::IsClangType(type)) {
    if (const auto *complex = ClangUtil::GetCanonicalQualType(type)
                                  ->getAs<clang::ComplexType>()) {
      if (complex->getElementType()->isIntegerType() && size % 2 == 0 &&
          (size == 2 || size == 4 || size == 8 || size == 16))
        return ReturnInfo{ReturnKind::GPR, static_cast<uint32_t>(size / 2), 2};
      return unsupported();
    }
  }

  if (!type.IsAggregateType() && !type.IsVectorType())
    return unsupported();

  bool instance_method = false;
  if (function_decl_context) {
    if (!*function_decl_context ||
        !llvm::isa<TypeSystemClang>(function_decl_context->GetTypeSystem())) 
    {
      return llvm::createStringError(
          llvm::inconvertibleErrorCode(),
          "Xbox 360 aggregate/vector return requires a function declaration");
    }

    auto *decl_context = static_cast<clang::DeclContext *>(
        function_decl_context->GetOpaqueDeclContext());
    const auto *function = llvm::dyn_cast<clang::FunctionDecl>(decl_context);

    if (!function) {
      return llvm::createStringError(
          llvm::inconvertibleErrorCode(),
          "Xbox 360 return context is not a function declaration");
    }

    if (const auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(function)) {
      instance_method = method->isInstance();
    } else {
      auto context = *function_decl_context;
      instance_method = context.IsClassMethod();
    }
  }

  if (type.IsVectorType()) {
    if (size != 16)
      return unsupported();

    return ReturnInfo{instance_method ? ReturnKind::Memory : ReturnKind::Vector,
                      16};
  }

  const auto *record =
      llvm::dyn_cast_or_null<clang::RecordDecl>(ClangUtil::GetAsTagDecl(type));

  if (record) {
    if (record->hasAttr<clang::PassInRegAttr>() && !record->field_empty()) {
      clang::QualType field = record->field_begin()->getType();
      if (const auto *field_record = field->getAsRecordDecl()) {
        if (field_record->hasAttr<clang::IntrinTypeAttr>() && size == 16)
          return ReturnInfo{ReturnKind::Vector, 16};
      }

      if ((field->isSpecificBuiltinType(clang::BuiltinType::Float) &&
           size == 4) ||
          (field->isSpecificBuiltinType(clang::BuiltinType::Double) &&
           size == 8)) 
      {
        return ReturnInfo{ReturnKind::FPR, static_cast<uint32_t>(size)};
      }

      return unsupported();
    }

    if (record->hasAttr<clang::IntrinTypeAttr>()) {
      if (size != 16)
        return unsupported();

      return ReturnInfo{
          instance_method ? ReturnKind::Memory : ReturnKind::Vector, 16};
    }
    if (const auto *cxx_record = llvm::dyn_cast<clang::CXXRecordDecl>(record)) {
      if (instance_method || !IsDirectCXXRecord(*cxx_record))
        return ReturnInfo{ReturnKind::Memory, 0};
    }
  }

  if (size == 1 || size == 2 || size == 4 || size == 8)
    return ReturnInfo{ReturnKind::GPR, static_cast<uint32_t>(size)};

  return ReturnInfo{ReturnKind::Memory, 0};
}

Status ReadReturnRegister(RegisterContext &reg_ctx, llvm::StringRef name,
                          uint32_t register_size, void *bytes, uint32_t size) {
  const RegisterInfo *reg = reg_ctx.GetRegisterInfoByName(name);

  if (!reg && name == "v1")
    reg = reg_ctx.GetRegisterInfoByName("vr1");

  RegisterValue value;
  if (!reg || reg->byte_size != register_size ||
      !reg_ctx.ReadRegister(reg, value)) {
    return Status::FromErrorStringWithFormat(
        "cannot read Xbox 360 return register %s", name.str().c_str());
  }

  Status error;
  if (value.GetAsMemoryData(*reg, bytes, size, eByteOrderBig, error) != size ||
      error.Fail()) {
    return Status::FromErrorStringWithFormat(
        "cannot decode Xbox 360 return register %s", name.str().c_str());
  }

  return {};
}
} // namespace

void ABIXbox360::Initialize() {
  PluginManager::RegisterPlugin(GetPluginNameStatic(), "Xbox 360 PowerPC ABI",
                                CreateInstance);
}

void ABIXbox360::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}

ABISP ABIXbox360::CreateInstance(ProcessSP process_sp, const ArchSpec &arch) {
  const auto &triple = arch.GetTriple();
  if (!triple.isOSXbox360() ||
      (triple.getArch() != llvm::Triple::ppc &&
       triple.getArch() != llvm::Triple::ppc64)) 
  {
    return {};
  }

  return ABISP(new ABIXbox360(std::move(process_sp), MakeMCRegisterInfo(arch)));
}

void ABIXbox360::AugmentRegisterInfo(
    std::vector<DynamicRegisterInfo::Register> &regs) {
  AugmentXbox360RegisterInfo(ArchSpec("powerpc-ibm-xbox360"), regs);
}

bool ABIXbox360::RegisterIsVolatile(const RegisterInfo *reg_info) {
  if (!reg_info)
    return true;

  auto is_preserved = [](llvm::StringRef name) {
    // r2 and r13 are reserved state, not allocatable callee-saved registers.
    // The complete CR is volatile: only its individual fields 2-5 survive.
    return name == "sp" || name == "r1" || name == "r2" || name == "r13" ||
           IsNumberedRegister(name, "r", 14, 31) ||
           IsNumberedRegister(name, "f", 14, 31) ||
           IsNumberedRegister(name, "v", 64, 127) ||
           IsNumberedRegister(name, "vr", 64, 127) ||
           IsNumberedRegister(name, "cr", 2, 5);
  };

  return !is_preserved(llvm::StringRef(reg_info->name ? reg_info->name : "")) &&
         !is_preserved(
             llvm::StringRef(reg_info->alt_name ? reg_info->alt_name : ""));
}

bool ABIXbox360::PrepareTrivialCall(Thread &thread, addr_t sp,
                                   addr_t function_address,
                                   addr_t return_address,
                                   llvm::ArrayRef<addr_t> args) const {
  auto reg_ctx = thread.GetRegisterContext();
  auto process_sp = thread.GetProcess();

  if (!reg_ctx || !process_sp)
    return Fail("missing register context or process");

  if (sp > UINT32_MAX || !IsCodeAddressValid(function_address) ||
      !IsCodeAddressValid(return_address))
  {
    return Fail("invalid call address");
  }

  // Eight doubleword home slots are mandatory even for register-only calls.
  const uint64_t max_slots =
      (uint64_t(UINT32_MAX) - Stack::kLinkageSize - (Stack::kAlignment - 1)) /
      Stack::kSlotSize;
  if (args.size() > max_slots)
    return Fail("argument area exceeds the address space");

  uint64_t frame_size = llvm::alignTo(
      Stack::kLinkageSize +
          std::max<size_t>(Stack::kHomeSlots, args.size()) * Stack::kSlotSize,
      Stack::kAlignment);
  sp = llvm::alignDown(sp, Stack::kAlignment);

  addr_t original_sp = NormalizeAddress(reg_ctx->GetSP(LLDB_INVALID_ADDRESS));
  if (!original_sp || original_sp > UINT32_MAX ||
      original_sp % Stack::kAlignment)
    return Fail("cannot read an aligned original stack pointer");
  if (sp > original_sp)
    return Fail("call stack would overwrite the active stack");

  if (sp <= frame_size)
    return Fail("insufficient stack space");
  sp -= frame_size;

  const RegisterInfo *pc = reg_ctx->GetRegisterInfoByName("pc");
  const RegisterInfo *lr = reg_ctx->GetRegisterInfoByName("lr");
  const RegisterInfo *stack = reg_ctx->GetRegisterInfoByName("r1");

  if (!pc || (pc->byte_size != 4 && pc->byte_size != 8) || !lr ||
      lr->byte_size != 8 || !stack || stack->byte_size != 8)
    return Fail("missing or incorrectly sized PC, LR or SP register");

  std::vector<const RegisterInfo *> arg_regs;
  for (size_t i = 0; i < std::min<size_t>(args.size(), Stack::kHomeSlots);
       ++i) {
    const RegisterInfo *reg =
        reg_ctx->GetRegisterInfoByName("r" + std::to_string(3 + i));

    if (!reg || reg->byte_size != Stack::kSlotSize)
      return Fail("missing full-width argument register");

    arg_regs.push_back(reg);
  }

  // The backchain is a word, not a GPR-width save. Reserve the complete linkage
  // and home areas without borrowing any storage from the interrupted frame.
  std::vector<uint8_t> frame(frame_size, 0);
  llvm::support::endian::write32be(frame.data(), original_sp);
  for (size_t i = Stack::kHomeSlots; i < args.size(); ++i)
    llvm::support::endian::write64be(
        frame.data() + Stack::kLinkageSize + i * Stack::kSlotSize, args[i]);

  Status error;
  if (process_sp->WriteMemory(sp, frame.data(), frame.size(), error) !=
          frame.size() ||
      error.Fail())
    return Fail("cannot write the complete call stack frame");

  for (size_t i = 0; i < arg_regs.size(); ++i) {
    if (!reg_ctx->WriteRegisterFromUnsigned(arg_regs[i], args[i]))
      return Fail("cannot write argument register");
  }

  if (!reg_ctx->WriteRegisterFromUnsigned(lr, return_address) ||
      !reg_ctx->WriteRegisterFromUnsigned(stack, sp) ||
      !reg_ctx->WriteRegisterFromUnsigned(pc, function_address))
  {
    return Fail("cannot write call control registers");
  }

  return true;
}

bool ABIXbox360::GetArgumentValues(Thread &thread, ValueList &values) const {
  auto reg_ctx = thread.GetRegisterContext();
  auto process_sp = thread.GetProcess();
  if (!reg_ctx || !process_sp)
    return Fail("missing register context or process");

  addr_t sp = NormalizeAddress(reg_ctx->GetSP(LLDB_INVALID_ADDRESS));
  if (!sp || sp > UINT32_MAX || sp % Stack::kAlignment)
    return Fail("cannot read the stack pointer");

  uint64_t offset = Stack::kLinkageSize;
  uint64_t non_gpr_bytes = 0;
  unsigned fpr = 1, vr = 1;
  ValueList result;

  for (size_t i = 0; i < values.GetSize(); ++i) {
    Value value = *values.GetValueAtIndex(i);
    CompilerType type = value.GetCompilerType();
    auto bit_size = type.GetBitSize(&thread);

    if (!bit_size || !*bit_size)
      return Fail("unknown argument size");

    bool is_signed = false, is_complex = false;
    uint32_t count = 0;

    bool integer = type.IsIntegerOrEnumerationType(is_signed) ||
                   type.IsPointerOrReferenceType();
    bool floating = type.IsFloatingPointType(count, is_complex) && !is_complex &&
                    count == 1;
    bool vector = type.IsVectorType();

    if ((!integer && !floating && !vector) ||
        (integer && *bit_size > 64) ||
        (floating && *bit_size != 32 && *bit_size != 64) ||
        (vector && *bit_size != 128))
      return Fail("unsupported argument type; aggregate/C++ lowering requires "
                  "function ABI metadata");

    uint64_t aligned_offset =
        llvm::alignTo(offset, vector ? Stack::kAlignment : Stack::kSlotSize);

    if (vector)
      non_gpr_bytes += aligned_offset - offset;

    offset = aligned_offset;
    unsigned gpr = std::min<uint64_t>(
        (offset - Stack::kLinkageSize - non_gpr_bytes) / Stack::kSlotSize,
        Stack::kHomeSlots);
    uint32_t size = (*bit_size + 7) / 8;
    uint8_t bytes[16] = {};

    Status error;
    if (vector && vr <= 13) {
      const RegisterInfo *reg =
          reg_ctx->GetRegisterInfoByName("v" + std::to_string(vr));
      if (!reg)
        reg = reg_ctx->GetRegisterInfoByName("vr" + std::to_string(vr));
      ++vr;

      RegisterValue reg_value;
      if (!reg || reg->byte_size != 16 ||
          !reg_ctx->ReadRegister(reg, reg_value) ||
          reg_value.GetAsMemoryData(*reg, bytes, size, eByteOrderBig, error) !=
              size ||
          error.Fail()) 
      {
        return Fail("cannot read vector argument register");
      }

      value.SetBytes(bytes, size);
    } else if (floating && fpr <= 13) {
      const RegisterInfo *reg =
          reg_ctx->GetRegisterInfoByName("f" + std::to_string(fpr++));

      RegisterValue reg_value;
      if (!reg || reg->byte_size != 8 ||
          !reg_ctx->ReadRegister(reg, reg_value) ||
          reg_value.GetAsMemoryData(*reg, bytes, 8, eByteOrderBig, error) != 8 ||
          error.Fail()) 
      {
        return Fail("cannot read floating-point argument register");
      }

      DataExtractor data(bytes, 8, eByteOrderBig, 4);
      lldb::offset_t data_offset = 0;
      double number = data.GetDouble(&data_offset);

      if (*bit_size == 32)
        value.GetScalar() = static_cast<float>(number);
      else
        value.GetScalar() = number;

      value.SetValueType(Value::ValueType::Scalar);
    } else {
      if (!vector && gpr < Stack::kHomeSlots) {
        const RegisterInfo *reg =
            reg_ctx->GetRegisterInfoByName("r" + std::to_string(3 + gpr));

        RegisterValue reg_value;
        if (!reg || reg->byte_size != 8 ||
            !reg_ctx->ReadRegister(reg, reg_value) ||
            reg_value.GetAsMemoryData(*reg, bytes, size, eByteOrderBig, error) !=
                size ||
            error.Fail()) 
        {
            return Fail("cannot read integer argument register");
        }
      } else {
        // Scalars are right-justified in doubleword slots on big-endian Xbox.
        uint64_t address = sp + offset + (vector ? 0 : Stack::kSlotSize - size);
        if (address > UINT32_MAX || size - 1 > UINT32_MAX - address ||
            process_sp->ReadMemory(address, bytes, size, error) != size ||
            error.Fail()) 
        {
            return Fail("cannot read stack argument");
        }
      }

      if (vector) {
        value.SetBytes(bytes, size);
      } else {
        DataExtractor data(bytes, size, eByteOrderBig, 4);
        Encoding encoding = floating ? eEncodingIEEE754 : eEncodingUint;
        error = value.GetScalar().SetValueFromData(data, encoding, size);
        if (error.Fail())
          return Fail("cannot decode argument");
        if (integer)
          value.GetScalar().TruncOrExtendTo(*bit_size, is_signed);
        value.SetValueType(Value::ValueType::Scalar);
      }
    }

    offset += vector ? 16 : 8;
    if (vector)
      non_gpr_bytes += 16;
    result.PushValue(value);
  }
  values = result;
  return true;
}

bool ABIXbox360::CreateFunctionEntryUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  unwind_plan.SetRegisterKind(eRegisterKindGeneric);
  auto row = std::make_shared<UnwindPlan::Row>();
  row->GetCFAValue().SetIsRegisterPlusOffset(LLDB_REGNUM_GENERIC_SP, 0);
  row->SetRegisterLocationToRegister(LLDB_REGNUM_GENERIC_PC,
                                     LLDB_REGNUM_GENERIC_RA, true);
  unwind_plan.AppendRow(row);
  unwind_plan.SetReturnAddressRegister(LLDB_REGNUM_GENERIC_RA);
  unwind_plan.SetSourceName("Xbox 360 function entry");
  unwind_plan.SetSourcedFromCompiler(eLazyBoolNo);
  unwind_plan.SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
  return true;
}

std::optional<addr_t> ABIXbox360::GetThreadLocalData(Thread &thread,
                                                     const ModuleSP &module,
                                                     addr_t tls_offset) {
  auto fail = [](llvm::StringRef message) -> std::optional<addr_t> {
    LLDB_LOG(GetLog(LLDBLog::Expressions), "Xbox 360 TLS: {0}", message);
    return LLDB_INVALID_ADDRESS;
  };
  auto process = thread.GetProcess();
  if (!process || !module)
    return fail("missing process or module");
  // r13 names the executable's static block. There is no per-module DTV or
  // Windows TLS index to resolve TLS in independently loaded images.
  if (process->GetTarget().GetExecutableModule() != module)
    return fail("TLS is only supported for the executable module");
  auto *sections = module->GetSectionList();
  auto section =
      sections ? sections->FindSectionByName(ConstString(".tls")) : SectionSP();
  if (!section || tls_offset >= section->GetByteSize())
    return fail("offset is outside the executable TLS section");

  auto registers = thread.GetRegisterContext();
  const RegisterInfo *r13 =
      registers ? registers->GetRegisterInfoByName("r13") : nullptr;
  if (!r13 || r13->byte_size != 8)
    return fail("missing full-width r13");
  addr_t thread_pointer = NormalizeAddress(
      registers->ReadRegisterAsUnsigned(r13, LLDB_INVALID_ADDRESS));
  if (!thread_pointer || thread_pointer > UINT32_MAX - 3 || thread_pointer % 4)
    return fail("invalid or unreadable thread pointer");

  uint8_t bytes[4];
  Status error;
  if (process->ReadMemory(thread_pointer, bytes, sizeof(bytes), error) !=
          sizeof(bytes) ||
      error.Fail())
    return fail("cannot read the static TLS block pointer");
  addr_t block = llvm::support::endian::read32be(bytes);
  if (!block || block % 4 || tls_offset > UINT32_MAX - block)
    return fail("invalid or overflowing static TLS address");
  return block + tls_offset;
}

llvm::Expected<ABI::RuntimeTLS> ABIXbox360::GetRuntimeTLS(Thread &thread) {
  constexpr uint32_t kCurrentThreadOffset = 0x100;
  constexpr uint32_t kThreadTLSOffset = 0x68;
  constexpr uint32_t kSlotCount = 256;
  constexpr uint32_t kSlotSize = 4;
  auto process = thread.GetProcess();
  auto registers = thread.GetRegisterContext();

  const RegisterInfo *r13 =
      registers ? registers->GetRegisterInfoByName("r13") : nullptr;

  if (!process || !r13 || r13->byte_size != 8)
    return llvm::createStringError(
        "Xbox 360 runtime TLS requires a process and full-width r13");

  RegisterValue value;
  if (!registers->ReadRegister(r13, value))
    return llvm::createStringError("cannot read Xbox 360 thread pointer");

  addr_t pcr = NormalizeAddress(value.GetAsUInt64());
  auto read_pointer = [&](addr_t base,
                          uint32_t offset) -> llvm::Expected<addr_t> {
    if (!base || base % 4 || base > UINT32_MAX - offset - 3)
      return llvm::createStringError("invalid Xbox 360 runtime TLS address");

    uint8_t bytes[4];
    Status error;
    if (process->ReadMemory(base + offset, bytes, sizeof(bytes), error) !=
            sizeof(bytes) ||
        error.Fail())
    {
      return llvm::createStringError(
          "cannot read Xbox 360 runtime TLS pointer at 0x%" PRIx64,
          base + offset);
    }

    addr_t pointer = llvm::support::endian::read32be(bytes);
    if (!pointer || pointer % 4)
      return llvm::createStringError("invalid Xbox 360 runtime TLS pointer");

    return pointer;
  };

  // KPCR.PrcbData.CurrentThread -> KTHREAD.TlsData, not KPCR.TlsData.
  auto current_thread = read_pointer(pcr, kCurrentThreadOffset);
  if (!current_thread)
    return current_thread.takeError();

  auto slots = read_pointer(*current_thread, kThreadTLSOffset);
  if (!slots)
    return slots.takeError();

  if (*slots > UINT32_MAX - (kSlotCount * kSlotSize - 1))
    return llvm::createStringError("Xbox 360 runtime TLS slots overflow");

  return RuntimeTLS{*slots, kSlotCount, kSlotSize};
}

bool ABIXbox360::CreateDefaultUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  return false;
}

bool ABIXbox360::CallFrameAddressIsValid(addr_t cfa) {
  return cfa && cfa <= UINT32_MAX && (cfa & 15) == 0;
}

bool ABIXbox360::CodeAddressIsValid(addr_t pc) {
  return IsCodeAddressValid(pc);
}

addr_t ABIXbox360::FixCodeAddress(addr_t pc) { return NormalizeAddress(pc); }

addr_t ABIXbox360::FixDataAddress(addr_t address) {
  return FixCodeAddress(address);
}

Status ABIXbox360::SetReturnValueObject(StackFrameSP &frame_sp,
                                       ValueObjectSP &new_value) {
  return SetReturnValueObject(frame_sp, new_value, nullptr);
}

Status ABIXbox360::SetReturnValueObject(
    StackFrameSP &frame_sp, ValueObjectSP &new_value,
    const CompilerDeclContext *function_decl_context) {
  if (!frame_sp || !new_value)
    return Status::FromErrorString("missing Xbox 360 return frame or value");

  if (new_value->GetError().Fail())
    return new_value->GetError().Clone();

  CompilerType type = new_value->GetCompilerType();
  if (!type || !type.GetCompleteType()) {
    return Status::FromErrorString(
        "invalid or incomplete Xbox 360 return type");
  }

  if (type.IsVoidType())
    return {};

  auto size = type.GetByteSize(frame_sp.get());
  if (!size)
    return Status::FromErrorString("unknown Xbox 360 return type size");

  auto info = ClassifyReturn(type, *size, function_decl_context);
  if (!info)
    return Status::FromError(info.takeError());

  // r3 is a return pointer only after a normal return, not at an arbitrary
  // stop in the callee. Do not treat its current contents as hidden storage.
  if (info->kind == ReturnKind::Memory) {
    return Status::FromErrorString(
        "cannot force an indirect Xbox 360 return without the original "
        "hidden return-storage pointer");
  }

  auto thread = frame_sp->GetThread();

  // Return registers are volatile and may be undefined in the caller's unwind
  // context. ReturnFromFrame preserves these from the live register context.
  auto reg_ctx = thread ? thread->GetRegisterContext() : nullptr;
  if (!reg_ctx)
    return Status::FromErrorString("missing Xbox 360 return register context");

  DataExtractor data;
  Status error;
  size_t data_size = new_value->GetData(data, error);
  if (error.Fail())
    return error;

  if (data_size != *size || data.GetByteSize() < *size ||
      (data.GetByteOrder() != eByteOrderBig &&
       data.GetByteOrder() != eByteOrderLittle)) 
  {
    return Status::FromErrorString("invalid Xbox 360 return value data");
  }

  if ((type.IsAggregateType() || type.IsVectorType()) &&
      data.GetByteOrder() != eByteOrderBig) 
  {
    return Status::FromErrorString(
        "Xbox 360 aggregate/vector return data must be big-endian");
  }

  struct RegisterWrite {
    const RegisterInfo *reg;
    RegisterValue value;
    RegisterValue original;
  };

  llvm::SmallVector<RegisterWrite, 2> writes;
  for (uint32_t i = 0; i < info->count; i++) {
    std::string name;
    uint32_t register_size = 8;
    RegisterValue value;
    lldb::offset_t offset = i * info->element_size;

    switch (info->kind) {
    case ReturnKind::GPR: {
      name = "r" + std::to_string(3 + i);
      bool is_signed = false;
      bool is_integer = type.IsIntegerOrEnumerationType(is_signed);

      if (is_integer && info->count == 2 &&
          data.GetByteOrder() == eByteOrderLittle)
      {
        offset = (info->count - 1 - i) * info->element_size;
      }

      uint64_t bits = data.GetMaxU64(&offset, info->element_size);
      if (info->element_size < 8 && is_integer && is_signed)
        bits = llvm::SignExtend64(bits, info->element_size * 8);

      value.SetUInt64(bits);
      break;
    }
    case ReturnKind::FPR: {
      name = "f" + std::to_string(1 + i);
      llvm::APFloat number(
          info->element_size == 4 ? llvm::APFloat::IEEEsingle()
                                  : llvm::APFloat::IEEEdouble(),
          llvm::APInt(info->element_size * 8,
                      data.GetMaxU64(&offset, info->element_size)));

      if (info->element_size == 4) {
        bool loses_info;
        number.convert(llvm::APFloat::IEEEdouble(),
                       llvm::APFloat::rmNearestTiesToEven, &loses_info);
      }

      value.SetUInt64(number.bitcastToAPInt().getZExtValue());
      break;
    }
    case ReturnKind::Vector:
      name = "v1";
      register_size = 16;
      break;
    case ReturnKind::Memory:
      llvm_unreachable("indirect returns rejected above");
    }

    const RegisterInfo *reg = reg_ctx->GetRegisterInfoByName(name);
    if (!reg && name == "v1")
      reg = reg_ctx->GetRegisterInfoByName("vr1");

    if (!reg || reg->byte_size != register_size) {
      return Status::FromErrorStringWithFormat(
          "invalid Xbox 360 return register %s", name.c_str());
    }

    if (info->kind == ReturnKind::Vector &&
        (value.SetFromMemoryData(*reg, data.GetDataStart(), 16, eByteOrderBig,
                                 error) != 16 ||
         error.Fail())) 
    {
      return Status::FromErrorString("cannot encode Xbox 360 vector return");
    }

    RegisterValue original;
    if (info->count > 1 && !reg_ctx->ReadRegister(reg, original)) {
      return Status::FromErrorStringWithFormat(
          "cannot preserve Xbox 360 return register %s", name.c_str());
    }

    writes.push_back({reg, value, original});
  }
  for (size_t i = 0; i < writes.size(); i++) {
    if (reg_ctx->WriteRegister(writes[i].reg, writes[i].value))
      continue;

    bool restored = true;
    for (size_t j = 0; j < i; ++j)
      restored &= reg_ctx->WriteRegister(writes[j].reg, writes[j].original);

    return Status::FromErrorStringWithFormat(
        "cannot write Xbox 360 return register %s%s", writes[i].reg->name,
        restored ? "" : "; failed to restore earlier return registers");
  }

  return {};
}

ValueObjectSP
ABIXbox360::GetReturnValueObjectImpl(Thread &thread, CompilerType &type) const {
  return GetReturnValueObjectImpl(thread, type, nullptr);
}

ValueObjectSP ABIXbox360::GetReturnValueObjectImpl(
    Thread &thread, CompilerType &type,
    const CompilerDeclContext *function_decl_context) const {
  if (type.IsVoidType())
    return {};

  auto fail = [&](llvm::StringRef message) -> ValueObjectSP {
    Fail(message);
    return {};
  };

  if (!type.GetCompleteType())
    return fail("incomplete return type");

  auto size = type.GetByteSize(&thread);
  if (!size)
    return fail("unknown return type size");

  auto info = ClassifyReturn(type, *size, function_decl_context);
  if (!info)
    return fail(llvm::toString(info.takeError()));

  auto reg_ctx = thread.GetRegisterContext();
  if (!reg_ctx)
    return fail("missing return register context");

  if (info->kind == ReturnKind::Memory) {
    uint8_t bytes[4];
    Status error = ReadReturnRegister(*reg_ctx, "r3", 8, bytes, 4);
    if (error.Fail())
      return fail(error.AsCString());

    DataExtractor pointer(bytes, 4, eByteOrderBig, 4);
    lldb::offset_t offset = 0;
    addr_t address = pointer.GetU32(&offset);

    if (!address || *size > UINT32_MAX - address + 1ull)
      return fail("invalid indirect return storage");

    auto process_sp = thread.GetProcess();
    if (!process_sp)
      return fail("missing process for indirect return");

    auto buffer = std::make_shared<DataBufferHeap>(*size, 0);
    if (*size && (process_sp->ReadMemory(address, buffer->GetBytes(), *size,
                                         error) != *size ||
                  error.Fail()))
    {
      return fail("cannot read indirect return storage");
    }

    return ValueObjectConstResult::Create(&thread, type, ConstString(), buffer,
                                          eByteOrderBig, 4, address);
  }

  auto buffer = std::make_shared<DataBufferHeap>(*size, 0);
  for (uint32_t i = 0; i < info->count; i++) {
    uint8_t *destination = buffer->GetBytes() + i * info->element_size;
    Status error;

    switch (info->kind) {
    case ReturnKind::GPR:
      error = ReadReturnRegister(*reg_ctx, "r" + std::to_string(3 + i), 8,
                                 destination, info->element_size);
      break;
    case ReturnKind::FPR: {
      uint8_t bytes[8];
      error = ReadReturnRegister(*reg_ctx, "f" + std::to_string(1 + i), 8,
                                 bytes, 8);
      if (error.Fail())
        break;
      DataExtractor data(bytes, 8, eByteOrderBig, 4);
      lldb::offset_t offset = 0;
      llvm::APFloat number(llvm::APFloat::IEEEdouble(),
                           llvm::APInt(64, data.GetU64(&offset)));
      if (info->element_size == 4) {
        bool loses_info;
        number.convert(llvm::APFloat::IEEEsingle(),
                       llvm::APFloat::rmNearestTiesToEven, &loses_info);
      }
      Scalar bits(number.bitcastToAPInt());
      if (bits.GetAsMemoryData(destination, info->element_size, eByteOrderBig,
                               error) != info->element_size)
        return fail("cannot decode floating-point return");
      break;
    }
    case ReturnKind::Vector:
      error = ReadReturnRegister(*reg_ctx, "v1", 16, destination, 16);
      break;
    case ReturnKind::Memory:
      llvm_unreachable("indirect returns handled above");
    }

    if (error.Fail())
      return fail(error.AsCString());
  }
  return ValueObjectConstResult::Create(&thread, type, ConstString(), buffer,
                                        eByteOrderBig, 4);
}  
