# Horcrux Core - Build Graph Library

## Overview

The Horcrux Core library provides the fundamental data structures and algorithms for representing and manipulating build dependency graphs. It implements an immutable Directed Acyclic Graph (DAG) that ensures correctness, thread-safety, and deterministic behavior.

## Components

### BuildNode

Represents a single build target in the dependency graph.

**Key Features:**

- Immutable design for thread-safety
- Label-based identification (e.g., `//src/app:main`)
- Type classification (e.g., `cc_binary`, `cc_library`, `py_test`)
- Input/output file tracking
- Extensible attribute system
- Content-based hashing for cache keys

**Example:**

```cpp
#include "build_node.h"

using namespace horcrux::core;

// Create a library node
BuildNode lib_node(
    "//src/utils:string_utils",        // Label
    "cc_library",                       // Type
    {"string_utils.cpp", "string_utils.h"},  // Inputs
    {"libstring_utils.a"},              // Outputs
    {{"visibility", "public"}}          // Attributes
);

// Query node properties
auto label = lib_node.label();
auto inputs = lib_node.inputs();
auto hash = lib_node.compute_hash();  // For caching
```

### BuildEdge

Represents a directed dependency relationship between two nodes.

**Key Features:**

- Typed dependencies (Build, Data, Tool, Test)
- Immutable once created
- Lightweight representation

**Dependency Types:**

- `Build`: Direct build dependency (e.g., library dependency)
- `Data`: Runtime data dependency (e.g., config files)
- `Tool`: Tool dependency (e.g., code generator)
- `Test`: Test-only dependency

**Example:**

```cpp
#include "build_edge.h"

using namespace horcrux::core;

// Create a build dependency edge
BuildEdge edge(
    "//src:app",           // From (dependent)
    "//src:lib",           // To (dependency)
    BuildEdge::DependencyType::Build
);
```

### BuildGraph

Immutable DAG representing the complete build dependency structure.

**Key Features:**

- Builder pattern for construction
- Automatic cycle detection
- Topological sorting
- Dependency queries (direct and transitive)
- Reverse dependency tracking
- JSON serialization for caching
- Thread-safe once constructed

**Example:**

```cpp
#include "build_graph.h"

using namespace horcrux::core;

// Build a graph using the Builder pattern
auto builder = BuildGraph::builder();

// Add nodes
builder.add_node(BuildNode("//src:app", "cc_binary", {"main.cpp"}, {}, {}));
builder.add_node(BuildNode("//src:lib1", "cc_library", {"lib1.cpp"}, {}, {}));
builder.add_node(BuildNode("//src:lib2", "cc_library", {"lib2.cpp"}, {}, {}));

// Add edges (dependencies)
builder.add_edge(BuildEdge("//src:app", "//src:lib1"));
builder.add_edge(BuildEdge("//src:lib1", "//src:lib2"));

// Build the immutable graph
auto graph_result = builder.build();
if (!graph_result) {
    // Handle error (e.g., cycle detected)
    std::cerr << to_string(graph_result.error()) << '\n';
    return;
}

auto& graph = graph_result.value();

// Query the graph
auto* node = graph.get_node("//src:lib1");
auto deps = graph.get_dependencies("//src:app");
auto all_deps = graph.get_transitive_dependencies("//src:app");
auto order = graph.topological_sort();

// Serialize for caching
auto json = graph.serialize();
```

## Error Handling

The library uses `std::expected` for robust error handling:

```cpp
enum class GraphError {
    NodeAlreadyExists,   // Node with given label already exists
    NodeNotFound,        // Node with given label not found
    CycleDetected,       // Operation would create a cycle
    InvalidInput,        // Invalid input data
    SerializationError   // Serialization/deserialization error
};

// Example error handling
auto result = builder.add_edge(edge);
if (!result) {
    switch (result.error()) {
        case GraphError::CycleDetected:
            // Handle cycle
            break;
        case GraphError::NodeNotFound:
            // Handle missing node
            break;
        // ...
    }
}
```

## Design Principles

### Immutability

Once a `BuildGraph` is constructed via `build()`, it cannot be modified. This ensures:

- Thread-safety without locks
- No race conditions
- Reproducible behavior
- Safe sharing across threads

### Determinism

All operations are deterministic:

- Hash computation uses sorted attributes
- Serialization produces consistent output
- Topological sort is stable

### Memory Safety

- RAII for all resource management
- Smart pointers for ownership
- No manual memory management
- Move semantics for efficiency

## Performance Characteristics

| Operation | Time Complexity | Space Complexity |
|-----------|----------------|------------------|
| Add Node | O(1) | O(n) |
| Add Edge | O(V + E) | O(E) |
| Get Dependencies | O(1) | O(d) where d = out-degree |
| Transitive Dependencies | O(V + E) | O(V) |
| Topological Sort | O(V + E) | O(V) |
| Cycle Detection | O(V + E) | O(V) |

Where:

- V = number of vertices (nodes)
- E = number of edges
- d = out-degree of a node

## Thread Safety

- `BuildNode` and `BuildEdge` are immutable after construction
- `BuildGraph::Builder` is not thread-safe (single-threaded construction)
- `BuildGraph` (after `build()`) is fully thread-safe for reads
- Multiple threads can safely query the same graph concurrently

## Testing

Comprehensive test coverage (21 tests):

```bash
# Build and run tests
cd build
cmake ..
make build_graph_test
./bin/build_graph_test

# Run with CTest
ctest -R build_graph_test --output-on-failure
```

Test categories:

- Node creation and properties
- Edge creation and equality
- Builder validation (duplicates, cycles, missing nodes)
- Graph queries (dependencies, topological sort)
- Serialization
- Deterministic behavior

## Future Enhancements

- [ ] Implement full JSON deserialization (currently placeholder)
- [ ] Add graph visualization export (DOT format)
- [ ] Optimize memory usage with structural sharing
- [ ] Add graph merging capabilities
- [ ] Implement incremental graph updates
- [ ] Add graph statistics and metrics

## License

MIT License - See LICENSE file for details
