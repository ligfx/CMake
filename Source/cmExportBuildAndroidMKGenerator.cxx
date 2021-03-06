/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2000-2009 Kitware, Inc., Insight Software Consortium

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "cmExportBuildAndroidMKGenerator.h"

#include "cmExportSet.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmTargetExport.h"

cmExportBuildAndroidMKGenerator::cmExportBuildAndroidMKGenerator()
{
  this->LG = CM_NULLPTR;
  this->ExportSet = CM_NULLPTR;
}

void cmExportBuildAndroidMKGenerator::GenerateImportHeaderCode(
  std::ostream& os, const std::string&)
{
  os << "LOCAL_PATH := $(call my-dir)\n\n";
}

void cmExportBuildAndroidMKGenerator::GenerateImportFooterCode(std::ostream&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateExpectedTargetsCode(
  std::ostream&, const std::string&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateImportTargetCode(
  std::ostream& os, const cmGeneratorTarget* target)
{
  std::string targetName = this->Namespace;
  targetName += target->GetExportName();
  os << "include $(CLEAR_VARS)\n";
  os << "LOCAL_MODULE := ";
  os << targetName << "\n";
  os << "LOCAL_SRC_FILES := ";
  std::string path = target->GetLocalGenerator()->ConvertToOutputFormat(
    target->GetFullPath(), cmOutputConverter::MAKERULE);
  os << path << "\n";
}

void cmExportBuildAndroidMKGenerator::GenerateImportPropertyCode(
  std::ostream&, const std::string&, cmGeneratorTarget const*,
  ImportPropertyMap const&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateMissingTargetsCheckCode(
  std::ostream&, const std::vector<std::string>&)
{
}

void cmExportBuildAndroidMKGenerator::GenerateInterfaceProperties(
  const cmGeneratorTarget* target, std::ostream& os,
  const ImportPropertyMap& properties)
{
  std::string config = "";
  if (this->Configurations.size()) {
    config = this->Configurations[0];
  }
  cmExportBuildAndroidMKGenerator::GenerateInterfaceProperties(
    target, os, properties, cmExportBuildAndroidMKGenerator::BUILD, config);
}

void cmExportBuildAndroidMKGenerator::GenerateInterfaceProperties(
  const cmGeneratorTarget* target, std::ostream& os,
  const ImportPropertyMap& properties, GenerateType type,
  std::string const& config)
{
  const bool newCMP0022Behavior =
    target->GetPolicyStatusCMP0022() != cmPolicies::WARN &&
    target->GetPolicyStatusCMP0022() != cmPolicies::OLD;
  if (!newCMP0022Behavior) {
    std::ostringstream w;
    if (type == cmExportBuildAndroidMKGenerator::BUILD) {
      w << "export(TARGETS ... ANDROID_MK) called with policy CMP0022";
    } else {
      w << "install( EXPORT_ANDROID_MK ...) called with policy CMP0022";
    }
    w << " set to OLD for target " << target->Target->GetName() << ". "
      << "The export will only work with CMP0022 set to NEW.";
    target->Makefile->IssueMessage(cmake::AUTHOR_WARNING, w.str());
  }
  if (!properties.empty()) {
    os << "LOCAL_CPP_FEATURES := rtti exceptions\n";
    for (ImportPropertyMap::const_iterator pi = properties.begin();
         pi != properties.end(); ++pi) {
      if (pi->first == "INTERFACE_COMPILE_OPTIONS") {
        os << "LOCAL_CPP_FEATURES += ";
        os << (pi->second) << "\n";
      } else if (pi->first == "INTERFACE_LINK_LIBRARIES") {
        // need to look at list in pi->second and see if static or shared
        // FindTargetToLink
        // target->GetLocalGenerator()->FindGeneratorTargetToUse()
        // then add to LOCAL_CPPFLAGS
        std::vector<std::string> libraries;
        cmSystemTools::ExpandListArgument(pi->second, libraries);
        std::string staticLibs;
        std::string sharedLibs;
        std::string ldlibs;
        for (std::vector<std::string>::iterator i = libraries.begin();
             i != libraries.end(); ++i) {
          cmGeneratorTarget* gt =
            target->GetLocalGenerator()->FindGeneratorTargetToUse(*i);
          if (gt) {

            if (gt->GetType() == cmState::SHARED_LIBRARY ||
                gt->GetType() == cmState::MODULE_LIBRARY) {
              sharedLibs += " " + *i;
            } else {
              staticLibs += " " + *i;
            }
          } else {
            // evaluate any generator expressions with the current
            // build type of the makefile
            cmGeneratorExpression ge;
            CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(*i);
            std::string evaluated =
              cge->Evaluate(target->GetLocalGenerator(), config);
            bool relpath = false;
            if (type == cmExportBuildAndroidMKGenerator::INSTALL) {
              relpath = i->substr(0, 3) == "../";
            }
            // check for full path or if it already has a -l, or
            // in the case of an install check for relative paths
            // if it is full or a link library then use string directly
            if (cmSystemTools::FileIsFullPath(evaluated) ||
                evaluated.substr(0, 2) == "-l" || relpath) {
              ldlibs += " " + evaluated;
              // if it is not a path and does not have a -l then add -l
            } else if (!evaluated.empty()) {
              ldlibs += " -l" + evaluated;
            }
          }
        }
        if (!sharedLibs.empty()) {
          os << "LOCAL_SHARED_LIBRARIES :=" << sharedLibs << "\n";
        }
        if (!staticLibs.empty()) {
          os << "LOCAL_STATIC_LIBRARIES :=" << staticLibs << "\n";
        }
        if (!ldlibs.empty()) {
          os << "LOCAL_EXPORT_LDLIBS :=" << ldlibs << "\n";
        }
      } else if (pi->first == "INTERFACE_INCLUDE_DIRECTORIES") {
        std::string includes = pi->second;
        std::vector<std::string> includeList;
        cmSystemTools::ExpandListArgument(includes, includeList);
        os << "LOCAL_EXPORT_C_INCLUDES := ";
        std::string end;
        for (std::vector<std::string>::iterator i = includeList.begin();
             i != includeList.end(); ++i) {
          os << end << *i;
          end = "\\\n";
        }
        os << "\n";
      } else {
        os << "# " << pi->first << " " << (pi->second) << "\n";
      }
    }
  }
  switch (target->GetType()) {
    case cmState::SHARED_LIBRARY:
    case cmState::MODULE_LIBRARY:
      os << "include $(PREBUILT_SHARED_LIBRARY)\n";
      break;
    case cmState::STATIC_LIBRARY:
      os << "include $(PREBUILT_STATIC_LIBRARY)\n";
      break;
    case cmState::EXECUTABLE:
    case cmState::UTILITY:
    case cmState::OBJECT_LIBRARY:
    case cmState::GLOBAL_TARGET:
    case cmState::INTERFACE_LIBRARY:
    case cmState::UNKNOWN_LIBRARY:
      break;
  }
  os << "\n";
}
