#include "obj_loader.hpp"

#include "mesh_builder.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace sushi {

auto load_obj_file(const std::string &fname) -> std::optional<mesh_group> {
    std::ifstream file(fname);

    if (!file) {
        return std::nullopt;
    }

    std::string line;
    std::string word;
    int line_number = 0;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> normals;

    mesh_group_builder mb;
    mb.enable(attrib_location::POSITION);
    mb.enable(attrib_location::TEXCOORD);
    mb.enable(attrib_location::NORMAL);

    auto vb = std::optional<mesh_group_builder::vertex_builder>{};

    while (getline(file, line)) {
        ++line_number;
        std::istringstream iss(line);
        iss >> word;

        if (word == "o") {
            std::string s;
            std::getline(iss, s);
            mb.mesh(s);
        } else if (word == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (word == "vt") {
            glm::vec2 v;
            iss >> v.x >> v.y;
            v.y = 1.0 - v.y;
            texcoords.push_back(v);
        } else if (word == "vn") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            normals.push_back(v);
        } else if (word == "f") {
            GLuint verts[3];

            for (auto& vert : verts) {
                int pos_idx = 1;
                int texcoord_idx = 1;
                int normal_idx = 1;

                iss >> word;
                std::replace(begin(word), end(word), '/', ' ');
                std::istringstream iss2(word);
                iss2 >> pos_idx >> texcoord_idx >> normal_idx;

                vert = mb.vertex()
                    .position(vertices[pos_idx - 1])
                    .texcoord(texcoords[texcoord_idx - 1])
                    .normal(normals[normal_idx - 1])
                    .get();
            }

            mb.tri(verts[0], verts[1], verts[2]);
        } else if (word[0] == '#') {
            // pass
        } else {
            std::clog << "sushi::load_obj_file(): Warning: Unknown OBJ directive at " << fname << "[" <<
            line_number
            << "]: \"" << word << "\"." << std::endl;
        }
    }

    return mb.get();
}

} // namespace sushi
