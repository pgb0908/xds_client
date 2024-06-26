#pragma once

#include <string>

// NOLINT(namespace-envoy)
namespace cc_proto_descriptor_library {
namespace internal {

struct FileDescriptorInfo final {
  // Path of the file this object represents.
  const std::string file_name;
  // The bytes of the FileDescriptorProto for this proto file.
  const std::string file_descriptor_bytes_base64;
  // File descriptors that this file is dependent on.
  // TODO(b/143243097): absl::Span requires special casing to deal with zero
  // length arrays and constexpr.
  const FileDescriptorInfo** const deps;
};

} // namespace internal
} // namespace cc_proto_descriptor_library
