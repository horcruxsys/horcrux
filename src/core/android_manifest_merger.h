// Horcrux - Android Manifest Merger
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#ifndef HORCRUX_CORE_ANDROID_MANIFEST_MERGER_H_
#define HORCRUX_CORE_ANDROID_MANIFEST_MERGER_H_

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <tl/expected.hpp>
#include <tinyxml2.h>

namespace horcrux::core {

// Forward declarations
enum class AndroidResourceError;

// Manifest merge actions following Gradle's specification
enum class MergeAction {
  // Merge child elements and attributes (default)
  Merge,
  // Replace lower-priority elements with higher-priority ones
  Replace,
  // Keep only if present in higher priority manifest
  MergeOnly,
  // Remove element from final manifest
  Remove,
  // Strict merge - fail if conflicts
  Strict
};

// Node selector type for matching elements
enum class NodeKey {
  Name,        // Match by element name
  NameAndAttr, // Match by element name and specific attribute (e.g., name, class)
  NameAndId    // Match by element name and android:name attribute
};

// Manifest element priority (higher priority wins conflicts)
enum class ManifestPriority {
  Library = 0,     // Lowest priority - library manifests
  BuildType = 1,   // Build type (debug/release) overlay
  Flavor = 2,      // Product flavor overlay
  Main = 3,        // Main manifest
  Override = 4     // Highest priority - explicit overrides
};

// Merge rule for specific element types
struct MergeRule {
  std::string element_name;
  MergeAction default_action;
  NodeKey key_type;
  std::optional<std::string> key_attribute; // For NameAndAttr matching
};

// Conflict resolution strategy
enum class ConflictStrategy {
  UseHigherPriority, // Use value from higher priority manifest (default)
  UseLowerPriority,  // Use value from lower priority manifest
  Fail,              // Fail on conflict
  Concatenate        // Concatenate values (for certain attributes)
};

// Represents a parsed manifest with its priority
struct ParsedManifest {
  std::filesystem::path source_path;
  ManifestPriority priority;
  std::unique_ptr<tinyxml2::XMLDocument> document;
  tinyxml2::XMLElement* root;

  // Parse manifest from file
  static auto parse(const std::filesystem::path& path, ManifestPriority priority)
      -> tl::expected<ParsedManifest, AndroidResourceError>;
};

// Android Manifest Merger - implements Gradle's manifest merger algorithm
class AndroidManifestMerger {
public:
  AndroidManifestMerger();

  // Configuration for merge operation
  struct MergeConfig {
    std::filesystem::path main_manifest;
    std::vector<std::filesystem::path> library_manifests;
    std::vector<std::filesystem::path> flavor_manifests;
    std::vector<std::filesystem::path> build_type_manifests;
    std::filesystem::path output_manifest;
    bool verbose = false;
    bool strict = false; // Fail on any conflict
  };

  // Result of merge operation
  struct MergeResult {
    std::filesystem::path merged_manifest;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    bool success;
  };

  // Merge multiple manifests into one
  auto merge(const MergeConfig& config) -> tl::expected<MergeResult, AndroidResourceError>;

  // Add custom merge rule for specific element
  void add_merge_rule(const MergeRule& rule);

  // Set conflict resolution strategy for attribute
  void set_conflict_strategy(const std::string& attribute, ConflictStrategy strategy);

private:
  // Default merge rules for Android manifest elements
  std::map<std::string, MergeRule> merge_rules_;
  std::map<std::string, ConflictStrategy> conflict_strategies_;

  // Initialize default merge rules
  void initialize_default_rules();

  // Merge two XML elements
  auto merge_elements(tinyxml2::XMLElement* target, tinyxml2::XMLElement* source,
                      ManifestPriority source_priority, ManifestPriority target_priority)
      -> tl::expected<void, AndroidResourceError>;

  // Merge attributes from source to target
  auto merge_attributes(tinyxml2::XMLElement* target, tinyxml2::XMLElement* source,
                        ManifestPriority source_priority, ManifestPriority target_priority)
      -> std::vector<std::string>;

  // Find matching child element
  auto find_matching_element(tinyxml2::XMLElement* parent, tinyxml2::XMLElement* needle,
                             const MergeRule& rule) -> tinyxml2::XMLElement*;

  // Get merge rule for element
  auto get_merge_rule(const std::string& element_name) const -> MergeRule;

  // Get merge action from tools:node attribute
  auto get_tools_node_action(tinyxml2::XMLElement* element) const -> std::optional<MergeAction>;

  // Apply tools:remove, tools:replace, etc.
  auto apply_tools_directives(tinyxml2::XMLElement* element) -> void;

  // Sort children deterministically
  void sort_children(tinyxml2::XMLElement* parent);

  // Validate merged manifest
  auto validate_manifest(tinyxml2::XMLDocument* doc) -> std::vector<std::string>;
};

// Utility functions for manifest operations
namespace manifest_utils {

// Get element key for matching
auto get_element_key(tinyxml2::XMLElement* element, NodeKey key_type,
                     const std::optional<std::string>& key_attribute) -> std::string;

// Check if two elements are equivalent
auto elements_match(tinyxml2::XMLElement* elem1, tinyxml2::XMLElement* elem2, NodeKey key_type,
                    const std::optional<std::string>& key_attribute) -> bool;

// Get attribute value
auto get_attribute(tinyxml2::XMLElement* element, const std::string& name) -> std::optional<std::string>;

// Set attribute value
void set_attribute(tinyxml2::XMLElement* element, const std::string& name, const std::string& value);

// Deep clone element
auto clone_element(tinyxml2::XMLElement* source, tinyxml2::XMLDocument* target_doc)
    -> tinyxml2::XMLElement*;

// Convert merge action to string
auto merge_action_to_string(MergeAction action) -> std::string;

// Parse merge action from string
auto string_to_merge_action(const std::string& str) -> MergeAction;

} // namespace manifest_utils

} // namespace horcrux::core

#endif // HORCRUX_CORE_ANDROID_MANIFEST_MERGER_H_
