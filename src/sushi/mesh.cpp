//
// Created by Jeramy on 8/22/2015.
//

#include "mesh.hpp"

#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>

namespace sushi {

namespace {

static_mesh upload_static_mesh(const std::vector<GLfloat>& data, int num_tris) {
    static_mesh rv;

    rv.vao = make_unique_vertex_array();
    rv.vertex_buffer = make_unique_buffer();
    rv.num_triangles = num_tris;

    glBindBuffer(GL_ARRAY_BUFFER, rv.vertex_buffer.get());
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), &data[0], GL_STATIC_DRAW);

    glBindVertexArray(rv.vao.get());

    auto stride = sizeof(GLfloat) * (3 + 2 + 3);
    glEnableVertexAttribArray(attrib_location::POSITION);
    glEnableVertexAttribArray(attrib_location::TEXCOORD);
    glEnableVertexAttribArray(attrib_location::NORMAL);
    glVertexAttribPointer(
        attrib_location::POSITION, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(0));
    glVertexAttribPointer(
        attrib_location::TEXCOORD, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(
        attrib_location::NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const GLvoid *>(sizeof(GLfloat) * (3 + 2)));

    return rv;
}

} // static

static_mesh load_static_mesh_file(const std::string &fname) {
    std::ifstream file(fname);
    std::string line;
    std::string word;
    int line_number = 0;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::vector<GLfloat> data;
    int num_tris = 0;

    while (getline(file, line)) {
        ++line_number;
        std::istringstream iss(line);
        iss >> word;

        if (word == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (word == "vn") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            normals.push_back(v);
        } else if (word == "vt") {
            glm::vec2 v;
            iss >> v.x >> v.y;
            v.y = 1.0 - v.y;
            texcoords.push_back(v);
        } else if (word == "f") {
            int index;
            for (int i = 0; i < 3; ++i) {
                iss >> word;
                std::replace(begin(word), end(word), '/', ' ');
                std::istringstream iss2(word);
                iss2 >> index;
                --index;
                data.push_back(vertices[index].x);
                data.push_back(vertices[index].y);
                data.push_back(vertices[index].z);
                iss2 >> index;
                --index;
                data.push_back(texcoords[index].x);
                data.push_back(texcoords[index].y);
                iss2 >> index;
                --index;
                data.push_back(normals[index].x);
                data.push_back(normals[index].y);
                data.push_back(normals[index].z);
            }
            ++num_tris;
        } else if (word[0] == '#') {
            // pass
        } else {
            std::clog << "sushi::load_static_mesh_file(): Warning: Unknown OBJ directive at " << fname << "[" <<
            line_number
            << "]: \"" << word << "\"." << std::endl;
        }
    }

    return upload_static_mesh(data, num_tris);
}

static_mesh load_static_mesh_data(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texcoords, const std::vector<Tri>& tris) {
    std::vector<GLfloat> data;
    int num_tris = tris.size();

    for (const auto& tri : tris) {
        for (const auto& vert : tri.verts) {
            data.push_back(positions[vert.pos].x);
            data.push_back(positions[vert.pos].y);
            data.push_back(positions[vert.pos].z);
            data.push_back(texcoords[vert.tex].x);
            data.push_back(texcoords[vert.tex].y);
            data.push_back(normals[vert.norm].x);
            data.push_back(normals[vert.norm].y);
            data.push_back(normals[vert.norm].z);
        }
    }

    return upload_static_mesh(data, num_tris);
}

} // namespace sushi
