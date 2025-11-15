// Horcrux - Android Manifest Merger Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_manifest_merger.h"

#include <algorithm>
#include <iostream>
#include <set>

#include "android_resources.h"

namespace horcrux::core {

// ParsedManifest implementation
auto ParsedManifest::parse(const std::filesystem::path& path, ManifestPriority priority)
    -> tl::expected<ParsedManifest, AndroidResourceError> {
  if (!std::filesystem::exists(path)) {
    return tl::unexpected(AndroidResourceError::InvalidManifest);
  }

  ParsedManifest result;
  result.source_path = path;
  result.priority = priority;
  result.document = std::make_unique<tinyxml2::XMLDocument>();

  auto load_result = result.document->LoadFile(path.string().c_str());
  if (load_result != tinyxml2::XML_SUCCESS) {
    return tl::unexpected(AndroidResourceError::InvalidManifest);
  }

  result.root = result.document->RootElement();
  if (!result.root || std::string(result.root->Name()) != "manifest") {
    return tl::unexpected(AndroidResourceError::InvalidManifest);
  }

  return result;
}

// AndroidManifestMerger implementation
AndroidManifestMerger::AndroidManifestMerger() {
  initialize_default_rules();
}

void AndroidManifestMerger::initialize_default_rules() {
  // Application element - merge children
  merge_rules_["application"] = MergeRule{
      .element_name = "application",
      .default_action = MergeAction::Merge,
      .key_type = NodeKey::Name,
      .key_attribute = std::nullopt};

  // Activity, Service, Receiver, Provider - match by android:name
  for (const auto& component : {"activity", "service", "receiver", "provider"}) {
    merge_rules_[component] = MergeRule{
        .element_name = component,
        .default_action = MergeAction::Merge,
        .key_type = NodeKey::NameAndId,
        .key_attribute = "android:name"};
  }

  // Uses-permission, uses-feature - match by android:name
  for (const auto& uses : {"uses-permission", "uses-feature", "uses-library"}) {
    merge_rules_[uses] = MergeRule{
        .element_name = uses,
        .default_action = MergeAction::Merge,
        .key_type = NodeKey::NameAndId,
        .key_attribute = "android:name"};
  }

  // Intent-filter - merge all (no unique key)
  merge_rules_["intent-filter"] = MergeRule{
      .element_name = "intent-filter",
      .default_action = MergeAction::Merge,
      .key_type = NodeKey::Name,
      .key_attribute = std::nullopt};

  // Meta-data - match by android:name
  merge_rules_["meta-data"] = MergeRule{
      .element_name = "meta-data",
      .default_action = MergeAction::Merge,
      .key_type = NodeKey::NameAndId,
      .key_attribute = "android:name"};

  // Uses-sdk - merge attributes
  merge_rules_["uses-sdk"] = MergeRule{
      .element_name = "uses-sdk",
      .default_action = MergeAction::Merge,
      .key_type = NodeKey::Name,
      .key_attribute = std::nullopt};

  // Default conflict strategies
  conflict_strategies_["android:minSdkVersion"] = ConflictStrategy::UseHigherPriority;
  conflict_strategies_["android:targetSdkVersion"] = ConflictStrategy::UseHigherPriority;
  conflict_strategies_["android:maxSdkVersion"] = ConflictStrategy::UseHigherPriority;
  conflict_strategies_["android:versionCode"] = ConflictStrategy::UseHigherPriority;
  conflict_strategies_["android:versionName"] = ConflictStrategy::UseHigherPriority;
}

auto AndroidManifestMerger::merge(const MergeConfig& config)
    -> tl::expected<MergeResult, AndroidResourceError> {
  MergeResult result;
  result.success = false;

  // Parse all manifests with their priorities
  std::vector<ParsedManifest> manifests;

  // Parse main manifest
  auto main_result = ParsedManifest::parse(config.main_manifest, ManifestPriority::Main);
  if (!main_result) {
    result.errors.push_back("Failed to parse main manifest: " + config.main_manifest.string());
    return tl::unexpected(main_result.error());
  }
  manifests.push_back(std::move(*main_result));

  // Parse library manifests
  for (const auto& lib_path : config.library_manifests) {
    auto lib_result = ParsedManifest::parse(lib_path, ManifestPriority::Library);
    if (!lib_result) {
      result.warnings.push_back("Failed to parse library manifest: " + lib_path.string());
      continue;
    }
    manifests.push_back(std::move(*lib_result));
  }

  // Parse flavor manifests
  for (const auto& flavor_path : config.flavor_manifests) {
    auto flavor_result = ParsedManifest::parse(flavor_path, ManifestPriority::Flavor);
    if (!flavor_result) {
      result.warnings.push_back("Failed to parse flavor manifest: " + flavor_path.string());
      continue;
    }
    manifests.push_back(std::move(*flavor_result));
  }

  // Parse build type manifests
  for (const auto& build_path : config.build_type_manifests) {
    auto build_result = ParsedManifest::parse(build_path, ManifestPriority::BuildType);
    if (!build_result) {
      result.warnings.push_back("Failed to parse build type manifest: " + build_path.string());
      continue;
    }
    manifests.push_back(std::move(*build_result));
  }

  // Sort manifests by priority (lowest first, so we merge lower priority into higher)
  std::sort(manifests.begin(), manifests.end(),
            [](const ParsedManifest& a, const ParsedManifest& b) { return a.priority < b.priority; });

  // Create output document starting with main manifest
  auto output_doc = std::make_unique<tinyxml2::XMLDocument>();
  
  // Clone the main manifest as base
  auto main_manifest = std::find_if(
      manifests.begin(), manifests.end(),
      [](const ParsedManifest& m) { return m.priority == ManifestPriority::Main; });
  
  if (main_manifest == manifests.end()) {
    result.errors.push_back("Main manifest not found");
    return tl::unexpected(AndroidResourceError::InvalidManifest);
  }

  // Deep clone the main manifest root
  auto output_root = manifest_utils::clone_element(main_manifest->root, output_doc.get());
  output_doc->InsertEndChild(output_root);

  // Merge each manifest into the output (skip main as it's already the base)
  for (auto& manifest : manifests) {
    if (manifest.priority == ManifestPriority::Main) {
      continue;
    }

    if (config.verbose) {
      std::cout << "Merging: " << manifest.source_path << " (priority: "
                << static_cast<int>(manifest.priority) << ")" << std::endl;
    }

    // Merge root attributes
    auto attr_warnings = merge_attributes(output_root, manifest.root, manifest.priority,
                                          ManifestPriority::Main);
    result.warnings.insert(result.warnings.end(), attr_warnings.begin(), attr_warnings.end());

    // Merge child elements
    for (auto child = manifest.root->FirstChildElement(); child != nullptr;
         child = child->NextSiblingElement()) {
      auto merge_result =
          merge_elements(output_root, child, manifest.priority, ManifestPriority::Main);
      if (!merge_result && config.strict) {
        result.errors.push_back("Merge conflict in element: " + std::string(child->Name()));
        return tl::unexpected(merge_result.error());
      }
    }
  }

  // Sort children deterministically for reproducible output
  sort_children(output_root);

  // Validate merged manifest
  auto validation_errors = validate_manifest(output_doc.get());
  result.warnings.insert(result.warnings.end(), validation_errors.begin(),
                         validation_errors.end());

  // Save output manifest
  std::filesystem::create_directories(config.output_manifest.parent_path());
  auto save_result = output_doc->SaveFile(config.output_manifest.string().c_str());
  if (save_result != tinyxml2::XML_SUCCESS) {
    result.errors.push_back("Failed to save merged manifest");
    return tl::unexpected(AndroidResourceError::IoError);
  }

  result.merged_manifest = config.output_manifest;
  result.success = true;
  return result;
}

void AndroidManifestMerger::add_merge_rule(const MergeRule& rule) {
  merge_rules_[rule.element_name] = rule;
}

void AndroidManifestMerger::set_conflict_strategy(const std::string& attribute,
                                                   ConflictStrategy strategy) {
  conflict_strategies_[attribute] = strategy;
}

auto AndroidManifestMerger::merge_elements(tinyxml2::XMLElement* target,
                                           tinyxml2::XMLElement* source,
                                           ManifestPriority source_priority,
                                           ManifestPriority target_priority)
    -> tl::expected<void, AndroidResourceError> {
  if (!target || !source) {
    return tl::unexpected(AndroidResourceError::InvalidManifest);
  }

  std::string element_name = source->Name();
  auto rule = get_merge_rule(element_name);

  // Check for tools:node directive
  auto tools_action = get_tools_node_action(source);
  if (tools_action) {
    // Handle explicit merge directives
    if (*tools_action == MergeAction::Remove) {
      // Find and remove matching element from target
      auto matching = find_matching_element(target, source, rule);
      if (matching) {
        target->DeleteChild(matching);
      }
      return {};
    } else if (*tools_action == MergeAction::Replace) {
      rule.default_action = MergeAction::Replace;
    }
  }

  // Find matching element in target
  auto matching_target = find_matching_element(target, source, rule);

  if (!matching_target) {
    // No matching element - add source element to target
    auto cloned = manifest_utils::clone_element(source, target->GetDocument());
    target->InsertEndChild(cloned);
    return {};
  }

  // Elements match - merge according to rule
  switch (rule.default_action) {
  case MergeAction::Merge: {
    // Merge attributes
    auto warnings = merge_attributes(matching_target, source, source_priority, target_priority);

    // Recursively merge children
    for (auto child = source->FirstChildElement(); child != nullptr;
         child = child->NextSiblingElement()) {
      auto merge_result = merge_elements(matching_target, child, source_priority, target_priority);
      if (!merge_result) {
        return merge_result;
      }
    }
    break;
  }

  case MergeAction::Replace: {
    // Replace target element with source
    auto cloned = manifest_utils::clone_element(source, target->GetDocument());
    target->InsertAfterChild(matching_target, cloned);
    target->DeleteChild(matching_target);
    break;
  }

  case MergeAction::MergeOnly: {
    // Only merge if element exists in higher priority
    if (source_priority >= target_priority) {
      auto warnings = merge_attributes(matching_target, source, source_priority, target_priority);
    }
    break;
  }

  case MergeAction::Remove: {
    // Remove element
    target->DeleteChild(matching_target);
    break;
  }

  case MergeAction::Strict: {
    // Fail on any conflict
    return tl::unexpected(AndroidResourceError::MergingFailed);
  }
  }

  return {};
}

auto AndroidManifestMerger::merge_attributes(tinyxml2::XMLElement* target,
                                             tinyxml2::XMLElement* source,
                                             ManifestPriority source_priority,
                                             ManifestPriority target_priority)
    -> std::vector<std::string> {
  std::vector<std::string> warnings;

  // Merge all attributes from source
  for (const auto* attr = source->FirstAttribute(); attr != nullptr; attr = attr->Next()) {
    std::string attr_name = attr->Name();
    std::string attr_value = attr->Value();

    // Skip tools: attributes
    if (attr_name.starts_with("tools:")) {
      continue;
    }

    auto existing = target->Attribute(attr_name.c_str());
    if (!existing) {
      // Attribute doesn't exist in target - add it
      target->SetAttribute(attr_name.c_str(), attr_value.c_str());
    } else if (std::string(existing) != attr_value) {
      // Attribute exists with different value - resolve conflict
      auto strategy_it = conflict_strategies_.find(attr_name);
      ConflictStrategy strategy = strategy_it != conflict_strategies_.end()
                                      ? strategy_it->second
                                      : ConflictStrategy::UseHigherPriority;

      switch (strategy) {
      case ConflictStrategy::UseHigherPriority:
        if (source_priority > target_priority) {
          target->SetAttribute(attr_name.c_str(), attr_value.c_str());
        }
        break;

      case ConflictStrategy::UseLowerPriority:
        if (source_priority < target_priority) {
          target->SetAttribute(attr_name.c_str(), attr_value.c_str());
        }
        break;

      case ConflictStrategy::Fail:
        warnings.push_back("Attribute conflict: " + attr_name + " (" + existing + " vs " +
                           attr_value + ")");
        break;

      case ConflictStrategy::Concatenate:
        // Concatenate values with comma separator
        std::string new_value = std::string(existing) + "," + attr_value;
        target->SetAttribute(attr_name.c_str(), new_value.c_str());
        break;
      }
    }
  }

  return warnings;
}

auto AndroidManifestMerger::find_matching_element(tinyxml2::XMLElement* parent,
                                                   tinyxml2::XMLElement* needle,
                                                   const MergeRule& rule) -> tinyxml2::XMLElement* {
  if (!parent || !needle) {
    return nullptr;
  }

  std::string needle_name = needle->Name();

  for (auto child = parent->FirstChildElement(needle_name.c_str()); child != nullptr;
       child = child->NextSiblingElement(needle_name.c_str())) {
    if (manifest_utils::elements_match(child, needle, rule.key_type, rule.key_attribute)) {
      return child;
    }
  }

  return nullptr;
}

auto AndroidManifestMerger::get_merge_rule(const std::string& element_name) const -> MergeRule {
  auto it = merge_rules_.find(element_name);
  if (it != merge_rules_.end()) {
    return it->second;
  }

  // Default rule for unknown elements
  return MergeRule{.element_name = element_name,
                   .default_action = MergeAction::Merge,
                   .key_type = NodeKey::Name,
                   .key_attribute = std::nullopt};
}

auto AndroidManifestMerger::get_tools_node_action(tinyxml2::XMLElement* element) const
    -> std::optional<MergeAction> {
  if (!element) {
    return std::nullopt;
  }

  auto tools_node = element->Attribute("tools:node");
  if (!tools_node) {
    return std::nullopt;
  }

  return manifest_utils::string_to_merge_action(tools_node);
}

void AndroidManifestMerger::sort_children(tinyxml2::XMLElement* parent) {
  if (!parent) {
    return;
  }

  // Collect all child elements with their clones
  std::vector<std::pair<tinyxml2::XMLElement*, tinyxml2::XMLElement*>> children_with_clones;
  for (auto child = parent->FirstChildElement(); child != nullptr;
       child = child->NextSiblingElement()) {
    // Clone the child before deletion
    auto cloned = manifest_utils::clone_element(child, parent->GetDocument());
    children_with_clones.emplace_back(child, cloned);
  }

  // Sort by element name, then by key attribute
  std::sort(children_with_clones.begin(), children_with_clones.end(),
            [](const auto& a, const auto& b) {
              std::string name_a = a.second->Name();
              std::string name_b = b.second->Name();

              if (name_a != name_b) {
                return name_a < name_b;
              }

              // If same element name, sort by android:name attribute if present
              auto attr_a = a.second->Attribute("android:name");
              auto attr_b = b.second->Attribute("android:name");

              if (attr_a && attr_b) {
                return std::string(attr_a) < std::string(attr_b);
              }

              return false;
            });

  // Remove all original children
  for (auto& [original, cloned] : children_with_clones) {
    parent->DeleteChild(original);
  }

  // Add sorted clones
  for (auto& [original, cloned] : children_with_clones) {
    parent->InsertEndChild(cloned);
  }

  // Recursively sort children
  for (auto child = parent->FirstChildElement(); child != nullptr;
       child = child->NextSiblingElement()) {
    sort_children(child);
  }
}

auto AndroidManifestMerger::validate_manifest(tinyxml2::XMLDocument* doc)
    -> std::vector<std::string> {
  std::vector<std::string> errors;

  auto root = doc->RootElement();
  if (!root || std::string(root->Name()) != "manifest") {
    errors.push_back("Root element must be <manifest>");
    return errors;
  }

  // Check required attributes
  if (!root->Attribute("package")) {
    errors.push_back("Manifest must have a 'package' attribute");
  }

  // Check for application element
  auto app = root->FirstChildElement("application");
  if (!app) {
    errors.push_back("Manifest must contain an <application> element");
  }

  return errors;
}

// Utility functions implementation
namespace manifest_utils {

auto get_element_key(tinyxml2::XMLElement* element, NodeKey key_type,
                     const std::optional<std::string>& key_attribute) -> std::string {
  if (!element) {
    return "";
  }

  std::string key = element->Name();

  switch (key_type) {
  case NodeKey::Name:
    return key;

  case NodeKey::NameAndAttr:
  case NodeKey::NameAndId:
    if (key_attribute) {
      auto attr = element->Attribute(key_attribute->c_str());
      if (attr) {
        key += ":" + std::string(attr);
      }
    }
    return key;
  }

  return key;
}

auto elements_match(tinyxml2::XMLElement* elem1, tinyxml2::XMLElement* elem2, NodeKey key_type,
                    const std::optional<std::string>& key_attribute) -> bool {
  if (!elem1 || !elem2) {
    return false;
  }

  if (std::string(elem1->Name()) != std::string(elem2->Name())) {
    return false;
  }

  switch (key_type) {
  case NodeKey::Name:
    return true;

  case NodeKey::NameAndAttr:
  case NodeKey::NameAndId:
    if (key_attribute) {
      auto attr1 = elem1->Attribute(key_attribute->c_str());
      auto attr2 = elem2->Attribute(key_attribute->c_str());

      if (!attr1 || !attr2) {
        return false;
      }

      return std::string(attr1) == std::string(attr2);
    }
    return true;
  }

  return false;
}

auto get_attribute(tinyxml2::XMLElement* element, const std::string& name)
    -> std::optional<std::string> {
  if (!element) {
    return std::nullopt;
  }

  auto attr = element->Attribute(name.c_str());
  if (!attr) {
    return std::nullopt;
  }

  return std::string(attr);
}

void set_attribute(tinyxml2::XMLElement* element, const std::string& name,
                   const std::string& value) {
  if (element) {
    element->SetAttribute(name.c_str(), value.c_str());
  }
}

auto clone_element(tinyxml2::XMLElement* source, tinyxml2::XMLDocument* target_doc)
    -> tinyxml2::XMLElement* {
  if (!source || !target_doc) {
    return nullptr;
  }

  // Create new element
  auto cloned = target_doc->NewElement(source->Name());

  // Clone attributes
  for (const auto* attr = source->FirstAttribute(); attr != nullptr; attr = attr->Next()) {
    cloned->SetAttribute(attr->Name(), attr->Value());
  }

  // Clone text content
  auto text = source->GetText();
  if (text) {
    cloned->SetText(text);
  }

  // Recursively clone children
  for (auto child = source->FirstChildElement(); child != nullptr;
       child = child->NextSiblingElement()) {
    auto cloned_child = clone_element(child, target_doc);
    if (cloned_child) {
      cloned->InsertEndChild(cloned_child);
    }
  }

  return cloned;
}

auto merge_action_to_string(MergeAction action) -> std::string {
  switch (action) {
  case MergeAction::Merge:
    return "merge";
  case MergeAction::Replace:
    return "replace";
  case MergeAction::MergeOnly:
    return "merge-only";
  case MergeAction::Remove:
    return "remove";
  case MergeAction::Strict:
    return "strict";
  }
  return "merge";
}

auto string_to_merge_action(const std::string& str) -> MergeAction {
  if (str == "merge") {
    return MergeAction::Merge;
  } else if (str == "replace") {
    return MergeAction::Replace;
  } else if (str == "merge-only" || str == "mergeOnly") {
    return MergeAction::MergeOnly;
  } else if (str == "remove") {
    return MergeAction::Remove;
  } else if (str == "strict") {
    return MergeAction::Strict;
  }
  return MergeAction::Merge;
}

} // namespace manifest_utils

} // namespace horcrux::core
