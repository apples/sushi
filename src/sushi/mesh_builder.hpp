#ifndef SUSHI_MESH_BUILDER_HPP
#define SUSHI_MESH_BUILDER_HPP

#include "mesh_group.hpp"
#include "attrib_location.hpp"

#include <bitset>
#include <string>
#include <vector>

/// Sushi
namespace sushi {

class mesh_group_builder {
public:
    class vertex_builder {
    public:
        vertex_builder(mesh_group_builder& mgb, GLuint index);

        auto position(const glm::vec3& vec) -> vertex_builder&;
        auto texcoord(const glm::vec2& vec) -> vertex_builder&;
        auto normal(const glm::vec3& vec) -> vertex_builder&;
        auto tangent(const glm::vec3& vec) -> vertex_builder&;
        auto blendindices(const glm::ivec4& vec) -> vertex_builder&;
        auto blendweights(const glm::ivec4& vec) -> vertex_builder&;
        auto color(const glm::vec4& vec) -> vertex_builder&;

        auto get() -> GLuint { return index; }

    private:
        mesh_group_builder* mgb;
        GLuint index;
    };

    mesh_group_builder();

    void enable(attrib_location loc);

    void mesh(std::string name);

    auto vertex() -> vertex_builder;

    void tri(GLuint a, GLuint b, GLuint c);

    auto get() const -> mesh_group;

private:
    struct mesh_data {
        std::string name;
        std::vector<GLuint> elements;
    };

    struct attr_bitset : std::bitset<7> {
        using bitset::bitset;
        using bitset::operator[];
        auto operator[](attrib_location loc) { return (*this)[static_cast<GLuint>(loc)]; }
    };

    std::size_t num_vertices;
    std::vector<GLfloat> position_arr;
    std::vector<GLfloat> texcoord_arr;
    std::vector<GLfloat> normal_arr;
    std::vector<GLfloat> tangent_arr;
    std::vector<GLubyte> blendindices_arr;
    std::vector<GLubyte> blendweights_arr;
    std::vector<GLfloat> color_arr;
    std::vector<mesh_data> meshes;
    attr_bitset enabled_arrs;
    attr_bitset completed_arrs;
};

} // namespace sushi


#endif // SUSHI_MESH_BUILDER_HPP
