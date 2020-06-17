#include "mesh_group.hpp"

#include "mesh_utils.hpp"
#include "attrib_location.hpp"

namespace sushi {

auto load_meshes(const iqm::iqm_data& data) -> mesh_group {
    using _detail::load_buffer;
    using _detail::bind_attrib;

    mesh_group group;

    group.position_buffer = load_buffer(data.vertexarrays.position);
    group.texcoord_buffer = load_buffer(data.vertexarrays.texcoord);
    group.normal_buffer = load_buffer(data.vertexarrays.normal);
    group.tangent_buffer = load_buffer(data.vertexarrays.tangent);
    group.blendindices_buffer = load_buffer(data.vertexarrays.blendindexes);
    group.blendweights_buffer = load_buffer(data.vertexarrays.blendweights);
    group.color_buffer = load_buffer(data.vertexarrays.color);

    for (auto& iqm_mesh : data.meshes) {
        auto mesh = mesh_group::mesh{};
        mesh.name = iqm_mesh.name;
        mesh.num_tris = iqm_mesh.num_triangles;
        mesh.tris = make_unique_buffer();
        mesh.vao = make_unique_vertex_array();
        glBindVertexArray(mesh.vao.get());
        SUSHI_DEFER { glBindVertexArray(0); };

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.tris.get());
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            iqm_mesh.num_triangles * 3 * 4,
            &data.triangles[iqm_mesh.first_triangle],
            GL_STATIC_DRAW);

        bind_attrib(attrib_location::POSITION, group.position_buffer, 3, GL_FLOAT, false, iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::TEXCOORD, group.texcoord_buffer, 2, GL_FLOAT, false,  iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::NORMAL, group.normal_buffer, 3, GL_FLOAT, false,  iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::TANGENT, group.tangent_buffer, 3, GL_FLOAT, false,  iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::BLENDINDICES, group.blendindices_buffer, 4, GL_UNSIGNED_BYTE, GL_FALSE, iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::BLENDWEIGHTS, group.blendweights_buffer, 4, GL_UNSIGNED_BYTE, GL_TRUE, iqm_mesh.first_vertex, {});
        bind_attrib(attrib_location::COLOR, group.color_buffer, 4, GL_UNSIGNED_BYTE, GL_TRUE, iqm_mesh.first_vertex, {255, 255, 255, 255});

        group.meshes.push_back(std::move(mesh));
    }

    return group;
}

void draw_mesh(const mesh_group& group) {
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    auto animated_uniform = glGetUniformLocation(program, "Animated");

    glUniform1i(animated_uniform, 0);

    SUSHI_DEFER { glBindVertexArray(0); };

    for (const auto& mesh : group.meshes) {
        glBindVertexArray(mesh.vao.get());
        glDrawElements(GL_TRIANGLES, mesh.num_tris * 3, GL_UNSIGNED_INT, nullptr);
    }
}

} // namespace sushi

