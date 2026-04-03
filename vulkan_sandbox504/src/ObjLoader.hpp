#pragma once

#define NOMINMAX

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


#include "defines.hpp"

#include <glm/glm.hpp>
#include <unordered_set>

struct Vertex {
    Vec4f position = VEC4F_ZERO;
    Vec4f color = VEC4F_ONE;
    Vec4f normal = VEC4F_ZERO;
    Vec4f tangent = VEC4F_ZERO;
    Vec4f bitangent = VEC4F_ZERO;
    Vec4f texCoord = VEC4F_ZERO;
};

struct NGonFace {
    Vec4f normal;        // face normal
    Vec4f center;        // face center
    uint32 offset;       // offset into the index indices array
    uint32 count;        // number of vertices in the face
    float faceArea;      // face area
    int32 padding2 = 69; // to align the struct to 16 bytes
};

struct NgonData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<NGonFace> faces;

    size_t getDataSize() const {
        return (vertices.size() * sizeof(Vertex)) + (indices.size() * sizeof(uint)) + (faces.size() * sizeof(NGonFace));
    }
};

struct NGonDataWBones {
    NgonData data;
    std::vector<Vec4f> jointIndices;
    std::vector<Vec4f> jointWeights;
};

struct TriangleData {
    // std::vector<Vertex> vertices;
    std::vector<Vec4f> position;
    // std::vector<Vec4f> color;
    std::vector<Vec4f> normal;
    // std::vector<Vec4f> tangent;
    // std::vector<Vec4f> bitangent;
    std::vector<Vec4f> texCoord;
    std::vector<uint> indices;

    size_t getDataSize() const {
        return position.size() * 3 * sizeof(Vec4f) + indices.size() * sizeof(uint);
    }
};

static void extractIndices(const std::string &token, size_t &posIndex, size_t &texIndex, size_t &nIndex) {
    posIndex = texIndex = nIndex = 0; // Default values in case no indices are found

    size_t firstSlash = token.find('/');
    size_t secondSlash = token.find('/', firstSlash + 1);

    // Extract the position index (before the first slash)
    if (firstSlash != std::string::npos) {
        if (firstSlash > 0) {
            posIndex = std::stoi(token.substr(0, firstSlash));
        }
    } else {
        // If no slashes are found, treat the whole token as the position index
        posIndex = std::stoi(token);
        return; // No texture or normal index to extract
    }

    // Extract the texture index (between the first and second slashes)
    if (secondSlash != std::string::npos) {
        if (secondSlash > firstSlash + 1) {
            texIndex = std::stoi(token.substr(firstSlash + 1, secondSlash - firstSlash - 1));
        }
    } else if (firstSlash + 1 < token.size()) {
        // If only one slash is found and there are characters after it, it may contain a texture index
        texIndex = std::stoi(token.substr(firstSlash + 1));
        return; // No normal index to extract
    }

    // Extract the normal index (after the second slash)
    if (secondSlash + 1 < token.size()) {
        nIndex = std::stoi(token.substr(secondSlash + 1));
    }
}

class NgonLoader {
public:
    static NgonData loadNgonData(const std::string &filename) {
        // Logger::info("Loading ngon data from file: ", filename);
        NgonData ngonData;
        std::ifstream file(filename);
        std::string line;
        // temp data
        std::vector<Vec4f> temp_positions;
        std::vector<Vec4f> temp_normals;
        std::vector<Vec4f> temp_texCoords;
        // open file
        if (!file.is_open()) {
            std::cerr << "Error opening the file: " << filename << '\n';
            return ngonData;
        }
        // Logger::info("Parsing ngon data from file: ", filename);
        // read file
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string type;
            ss >> type;
            if (type == "v") {
                Vec4f position = Vec4f(0, 0, 0, 1);
                ss >> position.x >> position.y >> position.z;
                temp_positions.push_back(position);
            }

            else if (type == "vn") {
                Vec4f normal = Vec4f(0);
                ss >> normal.x >> normal.y >> normal.z;
                temp_normals.push_back(normal);
            }

            else if (type == "vt") {
                Vec4f texCoord = Vec4f(0);
                ss >> texCoord.x >> texCoord.y;
                temp_texCoords.push_back(texCoord);
            }

            else if (type == "f") {
                NGonFace face{};
                std::string token;
                face.count = 0;
                face.offset = unsigned(ngonData.indices.size());
                face.normal = VEC4F_ONE;
                while (ss >> token) {
                    size_t posIndex = 0, texIndex = 0, nIndex = 0;
                    extractIndices(token, posIndex, texIndex, nIndex);
                    Vertex vertex{};
                    // OBJ file uses 1-based indexing
                    // set vertex data (position, normal, texCoord
                    if (posIndex > 0 && temp_positions.size() >= posIndex) {
                        Vec3f pos = temp_positions[posIndex - 1];
                        vertex.position = Vec4f(pos.x, pos.y, pos.z, 1.0f);
                    }
                    if (nIndex > 0 && temp_normals.size() >= nIndex) {
                        vertex.normal = temp_normals[nIndex - 1];
                    }
                    if (texIndex > 0 && temp_texCoords.size() >= texIndex) {
                        vertex.texCoord = temp_texCoords[texIndex - 1];
                    }
                    // set vertex at pos (posIndex - 1) in _vertices
                    // but first allocate space for it up to posIndex
                    if (posIndex > ngonData.vertices.size()) {
                        ngonData.vertices.resize(posIndex);
                    }
                    ngonData.vertices.at(posIndex - 1) = vertex;
                    ngonData.indices.push_back(uint(posIndex - 1));
                    face.count++;
                }
                ngonData.faces.push_back(face);
            }
        }

        file.close();

        // compute face normals
        for (auto &face : ngonData.faces) {
            Vec4f v0 = ngonData.vertices[ngonData.indices[face.offset]].position;
            Vec4f v1 = ngonData.vertices[ngonData.indices[face.offset + 1]].position;
            Vec4f v2 = ngonData.vertices[ngonData.indices[face.offset + 2]].position;
            Vec3f normal = glm::normalize(glm::cross(Vec3f(v1.x, v1.y, v1.z) - Vec3f(v0.x, v0.y, v0.z),
                                                     Vec3f(v2.x, v2.y, v2.z) - Vec3f(v0.x, v0.y, v0.z)));
            face.normal = Vec4f(normal.x, normal.y, normal.z, 1.0f);
        }

        // compute face areas
        for (auto &face : ngonData.faces) {
            Vec4f center = VEC4F_ZERO;
            for (uint i = 0; i < face.count; i++) {
                center += ngonData.vertices[ngonData.indices[face.offset + i]].position;
            }
            center /= float(face.count);
            face.faceArea = 0.0f;
            for (uint i = 0; i < face.count; i++) {
                Vec4f v0 = ngonData.vertices[ngonData.indices[face.offset + i]].position;
                Vec4f v1 = ngonData.vertices[ngonData.indices[face.offset + (i + 1) % face.count]].position;
                face.faceArea += glm::length(glm::cross(Vec3f(v1.x, v1.y, v1.z) - Vec3f(v0.x, v0.y, v0.z),
                                                        Vec3f(center.x, center.y, center.z) - Vec3f(v0.x, v0.y, v0.z)));
            }
            face.faceArea *= 0.5f;
        }

        // compute face centers
        for (auto &face : ngonData.faces) {
            face.center = VEC4F_ZERO;
            for (uint i = 0; i < face.count; i++) {
                face.center += ngonData.vertices[ngonData.indices[face.offset + i]].position;
            }
            face.center /= float(face.count);
        }

        // set ngonData
        ngonData.vertices.shrink_to_fit();
        ngonData.indices.shrink_to_fit();
        ngonData.faces.shrink_to_fit();
        return ngonData;
    }
};

struct LutData {
    std::vector<Vec4f> positions;
    unsigned int Nx = 0;
    unsigned int Ny = 0;
    Vec3f min = Vec3f(0);
    Vec3f max = Vec3f(0);
};

class LutLoader {
public:
    // create a grid from any grid mesh with UVs :
    // 1. load the ngon mesh (it is guaranteed to have uvs, and be composed only of quads)
    // 2. compute grid dimentions
    // 3. sort vertices by u
    // cluster vertices by v while keeping the u order
    // 4. flatten the grid into a 1D array

    static LutData loadLutData(const std::string &filename) {
        NgonData ngonData = NgonLoader::loadNgonData(filename);

        // Compute Nx and Ny from the UV layout
        uint Nx = 0;
        uint Ny = 0;

        // Compute grid dimensions
        // Example pseudo-code to compute Nx and Ny
        std::unordered_set<float> uniqueU;
        std::unordered_set<float> uniqueV;
        for (const auto &vertex : ngonData.vertices) {
            uniqueU.insert(vertex.texCoord.x); // assuming texCoord.x is U
            uniqueV.insert(vertex.texCoord.y); // assuming texCoord.y is V
        }
        Nx = uint(uniqueU.size());
        Ny = uint(uniqueV.size());

        // Reorder vertices based on grid dimensions
        // Example pseudo-code to sort by UVs
        std::sort(ngonData.vertices.begin(), ngonData.vertices.end(),
                  [](const Vertex &a, const Vertex &b) {
                      if (a.texCoord.y == b.texCoord.y) {
                          return a.texCoord.x < b.texCoord.x;
                      }
                      return a.texCoord.y < b.texCoord.y;
                  });

        // Populate LutData
        LutData lutData;
        lutData.positions.reserve(ngonData.vertices.size());

        for (const auto &vertex : ngonData.vertices) {
            lutData.positions.push_back(vertex.position);
        }

        // Set Nx and Ny in the LutData
        lutData.Nx = Nx;
        lutData.Ny = Ny;

        // compute min and max
        #undef min
        #undef max
        Vec3f min = Vec3f(std::numeric_limits<float>::max());
        Vec3f max = Vec3f(std::numeric_limits<float>::min());

        for (const auto &vertex : lutData.positions) {
            min = glm::min(min, Vec3f(vertex.x, vertex.y, vertex.z));
            max = glm::max(max, Vec3f(vertex.x, vertex.y, vertex.z));
        }

        lutData.min = min;
        lutData.max = max;

        return lutData;
    }
};

class TriangleMeshLoader {
public:
    static TriangleData loadTriangleData(const std::string &filename) {
        // Logger::info("Loading triangle data from file: ", filename);
        TriangleData triangleData;
        std::ifstream file(filename);
        std::string line;

        std::vector<Vec4f> temp_positions;
        std::vector<Vec4f> temp_normals;
        std::vector<Vec4f> temp_texCoords;

        if (!file.is_open()) {
            std::cerr << "Error opening the file: " << filename << '\n';
            return triangleData;
        }

        // Logger::info("Parsing triangle data from file: ", filename);

        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "v") {
                Vec4f position = Vec4f(0, 0, 0, 1);
                ss >> position.x >> position.y >> position.z;
                temp_positions.push_back(position);
            } else if (type == "vn") {
                Vec4f normal = Vec4f(0);
                ss >> normal.x >> normal.y >> normal.z;
                temp_normals.push_back(normal);
            } else if (type == "vt") {
                Vec4f texCoord = Vec4f(0);
                ss >> texCoord.x >> texCoord.y;
                temp_texCoords.push_back(texCoord);
            } else if (type == "f") {

                std::string token;
                while (ss >> token) {
                    size_t posIndex = 0, texIndex = 0, nIndex = 0;
                    extractIndices(token, posIndex, texIndex, nIndex);

                    // Support negative indices as specified by the OBJ format.
                    // Negative values refer to elements starting from the end
                    // of the currently defined data.
                    if (posIndex < 0) {
                        posIndex = int(temp_positions.size()) + posIndex + 1;
                    }
                    if (texIndex < 0) {
                        texIndex = int(temp_texCoords.size()) + texIndex + 1;
                    }
                    if (nIndex < 0) {
                        nIndex = int(temp_normals.size()) + nIndex + 1;
                    }

                    Vertex vertex{};
                    if (posIndex > 0 && posIndex <= temp_positions.size()) {
                        Vec3f pos = temp_positions[posIndex - 1];
                        vertex.position = Vec4f(pos.x, pos.y, pos.z, 1.0f);
                    }
                    if (nIndex > 0 && nIndex <= temp_normals.size()) {
                        vertex.normal = temp_normals[nIndex - 1];
                    }
                    if (texIndex > 0 && texIndex <= temp_texCoords.size()) {
                        vertex.texCoord = temp_texCoords[texIndex - 1];
                    }

                    if (int(posIndex) > int(triangleData.position.size())) {
                        triangleData.position.resize(posIndex);
                        // triangleData.color.resize(posIndex);
                        triangleData.normal.resize(posIndex);
                        // triangleData.tangent.resize(posIndex);
                        // triangleData.bitangent.resize(posIndex);
                        triangleData.texCoord.resize(posIndex);
                    }
                    if (posIndex > 0) {
                        triangleData.position[posIndex - 1] = vertex.position;
                        //  triangleData.color[posIndex- 1] = vertex.color;
                        triangleData.normal[posIndex - 1] = vertex.normal;
                        //  triangleData.tangent[posIndex- 1] = vertex.tangent;
                        //  triangleData.bitangent[posIndex- 1] = vertex.bitangent;
                        triangleData.texCoord[posIndex - 1] = vertex.texCoord;

                        triangleData.indices.push_back(uint32_t(posIndex - 1));
                    }
                }
            }
        }

        file.close();
        triangleData.position.shrink_to_fit();
        // triangleData.color.shrink_to_fit();
        triangleData.normal.shrink_to_fit();
        // triangleData.tangent.shrink_to_fit();
        // triangleData.bitangent.shrink_to_fit();
        triangleData.texCoord.shrink_to_fit();
        triangleData.indices.shrink_to_fit();
        return triangleData;
    }

    static void replaceChar(std::string &str, const char oldChar, const char newChar) {
        for (auto &c : str) {
            if (c == oldChar) {
                c = newChar;
            }
        }
    }
};
