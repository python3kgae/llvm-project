//===--- HLSL.cpp - HLSL ToolChain Implementations --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HLSL.h"
#include "CommonArgs.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"

using namespace clang::driver;
using namespace clang::driver::tools;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;
using namespace llvm;

namespace {

const unsigned OfflineLibMinor = 0xF;
const unsigned MaxDXILMajor = 1;
const unsigned MaxDXILMinor = 7;
const unsigned MaxShaderModel6Minor = MaxDXILMinor;
// TODO:get default validator version from validator.
const StringRef DefaultValidatorVer = "1.7";

bool isLegalVersion(VersionTuple Version, unsigned Major, unsigned MinMinor,
                    unsigned MaxMinor) {
  VersionTuple Min(Major, MinMinor);
  VersionTuple Max(Major, MaxMinor);
  return Min <= Version && Version <= Max;
}

bool isLegalShaderModel(Triple &T) {
  if (T.getOS() != Triple::OSType::ShaderModel)
    return false;

  auto Version = T.getOSVersion();
  if (Version.getBuild())
    return false;
  if (Version.getSubminor())
    return false;
  if (!Version.getMinor())
    return false;

  auto Kind = T.getEnvironment();

  switch (Kind) {
  default:
    return false;
  case Triple::EnvironmentType::Vertex:
  case Triple::EnvironmentType::Hull:
  case Triple::EnvironmentType::Domain:
  case Triple::EnvironmentType::Geometry:
  case Triple::EnvironmentType::Pixel:
  case Triple::EnvironmentType::Compute: {
    if (isLegalVersion(Version, 4, 0, 1))
      return true;
    if (isLegalVersion(Version, 5, 0, 1))
      return true;

    if (isLegalVersion(Version, 6, 0, MaxShaderModel6Minor))
      return true;
  } break;
  case Triple::EnvironmentType::Library: {
    VersionTuple SM6x(6, OfflineLibMinor);
    if (Version == SM6x)
      return true;
    if (isLegalVersion(Version, 6, 3, MaxShaderModel6Minor))
      return true;
  } break;
  case Triple::EnvironmentType::Amplification:
  case Triple::EnvironmentType::Mesh: {
    if (isLegalVersion(Version, 6, 5, MaxShaderModel6Minor))
      return true;
  } break;
  }
  return false;
}

std::string tryParseProfile(StringRef Profile) {
  // [ps|vs|gs|hs|ds|cs|ms|as]_[major]_[minor]
  SmallVector<StringRef, 3> Parts;
  Profile.split(Parts, "_");
  if (Parts.size() != 3)
    return "";

  Triple::EnvironmentType Kind =
      StringSwitch<Triple::EnvironmentType>(Parts[0])
          .Case("ps", Triple::EnvironmentType::Pixel)
          .Case("vs", Triple::EnvironmentType::Vertex)
          .Case("gs", Triple::EnvironmentType::Geometry)
          .Case("hs", Triple::EnvironmentType::Hull)
          .Case("ds", Triple::EnvironmentType::Domain)
          .Case("cs", Triple::EnvironmentType::Compute)
          .Case("lib", Triple::EnvironmentType::Library)
          .Case("ms", Triple::EnvironmentType::Mesh)
          .Case("as", Triple::EnvironmentType::Amplification)
          .Default(Triple::EnvironmentType::UnknownEnvironment);
  if (Kind == Triple::EnvironmentType::UnknownEnvironment)
    return "";

  unsigned long long Major = 0;
  if (llvm::getAsUnsignedInteger(Parts[1], 0, Major))
    return "";

  unsigned long long Minor = 0;
  if (Parts[2] == "x")
    Minor = OfflineLibMinor;
  else if (llvm::getAsUnsignedInteger(Parts[2], 0, Minor))
    return "";

  // dxil-unknown-shadermodel-hull
  llvm::Triple T;
  T.setArch(Triple::ArchType::dxil);
  T.setOSName(Triple::getOSTypeName(Triple::OSType::ShaderModel).str() +
              VersionTuple(Major, Minor).getAsString());
  T.setEnvironment(Kind);
  if (isLegalShaderModel(T))
    return T.getTriple();
  else
    return "";
}

bool isLegalValidatorVersion(StringRef ValVersionStr, std::string &ErrorMsg) {
  auto VerPair = ValVersionStr.split(".");
  llvm::APInt APMajor, APMinor;

  if (VerPair.first.getAsInteger(0, APMajor) ||
      VerPair.second.getAsInteger(0, APMinor)) {
    ErrorMsg =
        "Format of validator version is \"<major>.<minor>\" (ex:\"1.4\").";
    return false;
  }
  uint64_t Major = APMajor.getLimitedValue();
  uint64_t Minor = APMinor.getLimitedValue();
  if (Major > MaxDXILMajor || (Major == MaxDXILMajor && Minor > MaxDXILMinor)) {
    ErrorMsg = "Validator version must be less than or equal to current "
               "internal version.";
    return false;
  }
  if (Major == 0 && Minor != 0) {
    ErrorMsg = "If validator major version is 0, minor version must also be 0.";
    return false;
  }
  return true;
}

} // namespace

/// DirectX Toolchain
HLSLToolChain::HLSLToolChain(const Driver &D, const llvm::Triple &Triple,
                             const ArgList &Args)
    : ToolChain(D, Triple, Args) {}

std::string
HLSLToolChain::ComputeEffectiveClangTriple(const ArgList &Args,
                                           types::ID InputType) const {
  if (Arg *A = Args.getLastArg(options::OPT_target_profile)) {
    StringRef Profile = A->getValue();
    std::string Triple = tryParseProfile(Profile);
    if (Triple == "") {
      getDriver().Diag(diag::err_drv_invalid_directx_shader_module) << Profile;
      Triple = ToolChain::ComputeEffectiveClangTriple(Args, InputType);
    }
    A->claim();
    return Triple;
  } else {
    return ToolChain::ComputeEffectiveClangTriple(Args, InputType);
  }
}

DerivedArgList *
HLSLToolChain::TranslateArgs(const DerivedArgList &Args, StringRef BoundArch,
                             Action::OffloadKind DeviceOffloadKind) const {
  DerivedArgList *DAL = new DerivedArgList(Args.getBaseArgs());

  const OptTable &Opts = getDriver().getOpts();

  for (Arg *A : Args) {
    if (A->getOption().getID() == options::OPT_dxil_validator_version) {
      StringRef ValVerStr = A->getValue();
      std::string ErrorMsg;
      if (!isLegalValidatorVersion(ValVerStr, ErrorMsg)) {
        getDriver().Diag(diag::err_drv_invalid_dxil_validator_version)
            << ValVerStr << ErrorMsg;

        continue;
      }
    }
    DAL->append(A);
  }
  // Add default validator version if not set.
  // TODO: remove this once read validator version from validator.
  if (!DAL->hasArg(options::OPT_dxil_validator_version)) {
    DAL->AddSeparateArg(nullptr,
                        Opts.getOption(options::OPT_dxil_validator_version),
                        DefaultValidatorVer);
  }
  return DAL;
}
