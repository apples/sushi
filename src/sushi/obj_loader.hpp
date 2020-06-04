#ifndef SUSHI_OBJ_LOADER_HPP
#define SUSHI_OBJ_LOADER_HPP

#include "mesh.hpp"

#include <string>
#include <optional>

/// Sushi
namespace sushi {

/// Loads a mesh from an OBJ file.
/// The following OBJ directives are supported:
/// - `#` - Comments
/// - `v` - Vertex location.
/// - `vn` - Vertex normal.
/// - `vt` - Vertex texture coordinate.
/// - `f` - Face (triangles only).
/// \param fname File name.
/// \return The static mesh described by the file.
auto load_obj_file(const std::string &fname) -> std::optional<mesh_group>;

} // namespace sushi

#endif // SUSHI_OBJ_LOADER_HPP
