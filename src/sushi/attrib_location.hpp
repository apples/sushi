#ifndef SUSHI_ATTRIB_LOCATION
#define SUSHI_ATTRIB_LOCATION

#include "gl.hpp"

#include <utility>

namespace sushi {

enum class attrib_location : GLuint {
    POSITION = 0,
    TEXCOORD = 1,
    NORMAL = 2,
    TANGENT = 3,
    BLENDINDICES = 4,
    BLENDWEIGHTS = 5,
    COLOR = 6,
};

constexpr inline std::pair<attrib_location, const char*> attrib_names[] = {
    {attrib_location::POSITION, "VertexPosition"},
    {attrib_location::TEXCOORD, "VertexTexCoord"},
    {attrib_location::NORMAL, "VertexNormal"},
    {attrib_location::TANGENT, "VertexTangent"},
    {attrib_location::BLENDINDICES, "VertexBlendIndices"},
    {attrib_location::BLENDWEIGHTS, "VertexBlendWeights"},
    {attrib_location::COLOR, "VertexColor"},
};

} // namespace sushi

#endif // SUSHI_ATTRIB_LOCATION
