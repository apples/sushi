#include "mesh_builder.hpp"

#include "mesh_utils.hpp"

#include <algorithm>
#include <iostream>

namespace sushi {

mesh_group_builder::vertex_builder::vertex_builder(mesh_group_builder& mgb, GLuint index)
    : mgb(&mgb), index(index) {}

auto mesh_group_builder::vertex_builder::position(const glm::vec3& vec) -> vertex_builder& {
    auto n = 3;
    std::copy_n(glm::value_ptr(vec), n, begin(mgb->position_arr) + index * n);
    return *this;
}

auto mesh_group_builder::vertex_builder::texcoord(const glm::vec2& vec) -> vertex_builder& {
    auto n = 2;
    std::copy_n(glm::value_ptr(vec), n, begin(mgb->texcoord_arr) + index * n);
    return *this;
}

auto mesh_group_builder::vertex_builder::normal(const glm::vec3& vec) -> vertex_builder& {
    auto n = 3;
    std::copy_n(glm::value_ptr(vec), n, begin(mgb->normal_arr) + index * n);
    return *this;
}

auto mesh_group_builder::vertex_builder::tangent(const glm::vec3& vec) -> vertex_builder& {
    auto n = 3;
    std::copy_n(glm::value_ptr(vec), n, begin(mgb->tangent_arr) + index * n);
    return *this;
}

auto mesh_group_builder::vertex_builder::blendindices(const glm::ivec4& vec) -> vertex_builder& {
    auto n = 4;
    std::copy_n(glm::value_ptr(vec), n, begin(mgb->blendindices_arr) + index * n);
    return *this;
}

auto mesh_group_builder::vertex_builder::blendweights(const glm::ivec4& vec) -> vertex_builder& {
    auto n = 4;
    std::copy_n(glm::value_ptr(vec), n, begin(mgb->blendweights_arr) + index * n);
    return *this;
}

auto mesh_group_builder::vertex_builder::color(const glm::vec4& vec) -> vertex_builder& {
    auto n = 4;
    std::copy_n(glm::value_ptr(vec), n, begin(mgb->color_arr) + index * n);
    return *this;
}

mesh_group_builder::mesh_group_builder() : num_vertices(0), enabled_arrs(0) {}

void mesh_group_builder::enable(attrib_location loc) {
    if (!meshes.empty()) {
        std::cerr << "mesh_group_builder: Enabling attributes after creating a mesh can result in wasted memory.\n";

        switch (loc) {
            case attrib_location::POSITION: position_arr.resize(num_vertices * 3); break;
            case attrib_location::TEXCOORD: texcoord_arr.resize(num_vertices * 2); break;
            case attrib_location::NORMAL: normal_arr.resize(num_vertices * 3); break;
            case attrib_location::TANGENT: tangent_arr.resize(num_vertices * 3); break;
            case attrib_location::BLENDINDICES: blendindices_arr.resize(num_vertices * 4); break;
            case attrib_location::BLENDWEIGHTS: blendweights_arr.resize(num_vertices * 4); break;
            case attrib_location::COLOR: color_arr.resize(num_vertices * 4); break;
        }
    }

    enabled_arrs[loc] = 1;
}

void mesh_group_builder::mesh(std::string name) {
    if (enabled_arrs.none()) {
        std::cerr << "mesh_group_builder: Attempting to build a mesh with no enabled attributes.\n";
    }

    meshes.push_back({ std::move(name), {} });
}

auto mesh_group_builder::vertex() -> vertex_builder {
    for (int i = 0; i < 7; ++i) {
        auto loc = static_cast<attrib_location>(i);
        if (enabled_arrs[loc]) {
            switch (loc) {
                case attrib_location::POSITION: position_arr.insert(end(position_arr), { 0, 0, 0 }); break;
                case attrib_location::TEXCOORD: texcoord_arr.insert(end(texcoord_arr), { 0, 0 }); break;
                case attrib_location::NORMAL: normal_arr.insert(end(normal_arr), { 0, 0, 0 }); break;
                case attrib_location::TANGENT: tangent_arr.insert(end(tangent_arr), { 0, 0, 0 }); break;
                case attrib_location::BLENDINDICES: blendindices_arr.insert(end(blendindices_arr), { 0, 0, 0, 0 }); break;
                case attrib_location::BLENDWEIGHTS: blendweights_arr.insert(end(blendweights_arr), { 0, 0, 0, 0 }); break;
                case attrib_location::COLOR: color_arr.insert(end(color_arr), { 1, 1, 1, 1 }); break;
            }
        }
    }

    auto vb = vertex_builder(*this, num_vertices);

    ++num_vertices;

    return vb;
}

void mesh_group_builder::tri(GLuint a, GLuint b, GLuint c) {
    if (meshes.empty()) {
        meshes.push_back({});
    }

    auto& elements = meshes.back().elements;

    elements.insert(end(elements), { a, b, c });
}

auto mesh_group_builder::get() const -> mesh_group {
    using _detail::load_buffer;
    using _detail::bind_attrib_float;
    using _detail::bind_attrib_ubyte;

    mesh_group group;

    group.position_buffer = load_buffer(position_arr);
    group.texcoord_buffer = load_buffer(texcoord_arr);
    group.normal_buffer = load_buffer(normal_arr);
    group.tangent_buffer = load_buffer(tangent_arr);
    group.blendindices_buffer = load_buffer(blendindices_arr);
    group.blendweights_buffer = load_buffer(blendweights_arr);
    group.color_buffer = load_buffer(color_arr);

    for (auto& my_mesh : meshes) {
        auto mesh = mesh_group::mesh{};
        mesh.name = my_mesh.name;
        mesh.num_tris = my_mesh.elements.size() / 3;
        mesh.tris = make_unique_buffer();
        mesh.vao = make_unique_vertex_array();

        glBindVertexArray(mesh.vao.get());
        SUSHI_DEFER { glBindVertexArray(0); };

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.tris.get());
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            my_mesh.elements.size() * sizeof(GLuint),
            my_mesh.elements.data(),
            GL_STATIC_DRAW);

        bind_attrib_float(attrib_location::POSITION, group.position_buffer, 3, 0, {});
        bind_attrib_float(attrib_location::TEXCOORD, group.texcoord_buffer, 2, 0, {});
        bind_attrib_float(attrib_location::NORMAL, group.normal_buffer, 3, 0, {});
        bind_attrib_float(attrib_location::TANGENT, group.tangent_buffer, 3, 0, {});
        bind_attrib_ubyte(attrib_location::BLENDINDICES, group.blendindices_buffer, 4, GL_FALSE, 0, {});
        bind_attrib_ubyte(attrib_location::BLENDWEIGHTS, group.blendweights_buffer, 4, GL_TRUE, 0, {});
        bind_attrib_float(attrib_location::COLOR, group.color_buffer, 4, 0, {1.f, 1.f, 1.f, 1.f});

        group.meshes.push_back(std::move(mesh));
    }

    return group;
}

} // namespace sushi
