#pragma once
#include "defines.hpp"
#include "ObjLoader.hpp"

#include <vector>
#include <unordered_map>

// Helper function to calculate centroid of a triangle
Vec3f calculateCentroid(const Vec3f& v1, const Vec3f& v2, const Vec3f& v3) {
    return (v1 + v2 + v3) / 3.0f;
}

// Helper function to normalize a vector
Vec3f normalize(const Vec3f& v) {
    return glm::normalize(v);
}

unsigned int addVertex(NgonData& data, const Vec3f& position) {
    Vertex vertex;
    vertex.position = Vec4f(position, 1.0f);
    vertex.normal = Vec4f(glm::normalize(position), 0.0f); // Normal is the normalized position
    data.vertices.push_back(vertex);
    return data.vertices.size() - 1;
}

// Helper function to calculate middle point index (using cache)
unsigned int getMiddlePoint(NgonData& data, unsigned int v1, unsigned int v2, std::unordered_map<uint64_t, unsigned int>& middlePointCache) {
    // Sort the two indices
    if (v1 > v2) std::swap(v1, v2);

    // Create a unique key for the edge
    uint64_t key = ((uint64_t)v1 << 32) | v2;

    // Check if this edge has been processed before
    #pragma omp critical
    {
        if (middlePointCache.find(key) != middlePointCache.end()) {
            return middlePointCache[key];
        }
    }

    // If not, compute the middle point and add a new vertex
    Vec3f pos1 = glm::vec3(data.vertices[v1].position);
    Vec3f pos2 = glm::vec3(data.vertices[v2].position);
    Vec3f middle = normalize((pos1 + pos2) / 2.0f);

    unsigned int index = addVertex(data, middle);

    #pragma omp critical
    {
        middlePointCache[key] = index;
    }

    return index;
}

// Helper function to compute area of a triangle using three points
float computeTriangleArea(const Vec3f& p1, const Vec3f& p2, const Vec3f& p3) {
    return 0.5f * glm::length(glm::cross(p2 - p1, p3 - p1));
}

// Helper function to compute the normal of a triangle using three points
Vec3f computeTriangleNormal(const Vec3f& p1, const Vec3f& p2, const Vec3f& p3) {
    return normalize(glm::cross(p2 - p1, p3 - p1));
}

// Helper function to compute the center of a triangle
Vec3f computeTriangleCenter(const Vec3f& p1, const Vec3f& p2, const Vec3f& p3) {
    return (p1 + p2 + p3) / 3.0f;
}

// Generate an icosahedron
void createIcosahedron(NgonData& data) {
    const float t = (1.0f + sqrt(5.0f)) / 2.0f;

    std::vector<Vec3f> initialVertices = {
        normalize(Vec3f(-1,  t,  0)),
        normalize(Vec3f( 1,  t,  0)),
        normalize(Vec3f(-1, -t,  0)),
        normalize(Vec3f( 1, -t,  0)),
        normalize(Vec3f( 0, -1,  t)),
        normalize(Vec3f( 0,  1,  t)),
        normalize(Vec3f( 0, -1, -t)),
        normalize(Vec3f( 0,  1, -t)),
        normalize(Vec3f( t,  0, -1)),
        normalize(Vec3f( t,  0,  1)),
        normalize(Vec3f(-t,  0, -1)),
        normalize(Vec3f(-t,  0,  1))
    };

    for (const Vec3f& pos : initialVertices) {
        addVertex(data, pos);
    }

    // Initial icosahedron faces
    std::vector<Vec3u> faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };

    // Add indices and faces
    for (const Vec3u& face : faces) {
        data.indices.push_back(face.x);
        data.indices.push_back(face.y);
        data.indices.push_back(face.z);

        Vec3f v1 = glm::vec3(data.vertices[face.x].position);
        Vec3f v2 = glm::vec3(data.vertices[face.y].position);
        Vec3f v3 = glm::vec3(data.vertices[face.z].position);

        Vec3f normal = computeTriangleNormal(v1, v2, v3);
        Vec3f center = computeTriangleCenter(v1, v2, v3);
        float area = computeTriangleArea(v1, v2, v3);

        NGonFace ngonFace;
        ngonFace.normal = Vec4f(normal, 0.0f);
        ngonFace.center = Vec4f(center, 1.0f);
        ngonFace.offset = data.indices.size() - 3;
        ngonFace.count = 3;
        ngonFace.faceArea = area;

        data.faces.push_back(ngonFace);
    }
}

// Subdivide the icosahedron faces to form an icosphere
void subdivideIcosphere(NgonData& data, int subdivisions) {
    // Use a local cache to avoid reuse of stale middle points across icospheres
    std::unordered_map<uint64_t, unsigned int> middlePointCache;

    for (int i = 0; i < subdivisions; ++i) {
        std::vector<unsigned int> newIndices;
        std::vector<NGonFace> newFaces;

        // Parallelize the loop using OpenMP
        #pragma omp parallel for
        for (int j = 0; j < static_cast<int>(data.indices.size()); j += 3) {
            unsigned int v1 = data.indices[j];
            unsigned int v2 = data.indices[j + 1];
            unsigned int v3 = data.indices[j + 2];

            unsigned int a = getMiddlePoint(data, v1, v2, middlePointCache);
            unsigned int b = getMiddlePoint(data, v2, v3, middlePointCache);
            unsigned int c = getMiddlePoint(data, v3, v1, middlePointCache);

            Vec3f p1 = glm::vec3(data.vertices[v1].position);
            Vec3f p2 = glm::vec3(data.vertices[v2].position);
            Vec3f p3 = glm::vec3(data.vertices[v3].position);
            Vec3f pa = glm::vec3(data.vertices[a].position);
            Vec3f pb = glm::vec3(data.vertices[b].position);
            Vec3f pc = glm::vec3(data.vertices[c].position);

            // Create 4 new triangles for each face
            std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> newTris = {
                {v1, a, c}, {v2, b, a}, {v3, c, b}, {a, b, c}
            };

            #pragma omp critical
            {
                for (auto [i1, i2, i3] : newTris) {
                    newIndices.push_back(i1);
                    newIndices.push_back(i2);
                    newIndices.push_back(i3);

                    Vec3f pos1 = glm::vec3(data.vertices[i1].position);
                    Vec3f pos2 = glm::vec3(data.vertices[i2].position);
                    Vec3f pos3 = glm::vec3(data.vertices[i3].position);

                    Vec3f normal = computeTriangleNormal(pos1, pos2, pos3);
                    Vec3f center = computeTriangleCenter(pos1, pos2, pos3);
                    float area = computeTriangleArea(pos1, pos2, pos3);

                    NGonFace ngonFace;
                    ngonFace.normal = Vec4f(normal, 0.0f);
                    ngonFace.center = Vec4f(center, 1.0f);
                    ngonFace.offset = newIndices.size() - 3;
                    ngonFace.count = 3;
                    ngonFace.faceArea = area;

                    newFaces.push_back(ngonFace);
                }
            }
        }
        data.indices = newIndices;
        data.faces = newFaces;
    }
}

// Create the final icosphere with a given number of subdivisions
NgonData createIcosphere(int subdivisions) {
    NgonData data;
    createIcosahedron(data);
    subdivideIcosphere(data, subdivisions);
    return data;
}

// Helper function to get the shared edge between two faces
std::pair<int, int> findSharedEdge(const std::vector<unsigned int>& indices, const NGonFace& face1, const NGonFace& face2) {
    std::set<uint> edge1 = { indices[face1.offset], indices[face1.offset + 1], indices[face1.offset + 2] };
    std::set<uint> edge2 = { indices[face2.offset], indices[face2.offset + 1], indices[face2.offset + 2] };

    std::vector<int> shared;
    for (int v : edge1) {
        if (edge2.count(v)) {
            shared.push_back(v);
        }
    }

    if (shared.size() == 2) {
        return { shared[0], shared[1] };
    }
    return { -1, -1 }; // No shared edge
}

// Function to order adjacent faces in a circular manner around a vertex
std::vector<uint32_t> orderAdjacentFaces(const std::vector<NGonFace>& faces, const std::vector<unsigned int>& indices, uint32_t originalVertex, const std::vector<uint32_t>& adjacentFaces) {
    std::vector<uint32_t> orderedFaces;
    std::set<uint32_t> visited;

    if (adjacentFaces.empty()) return orderedFaces;

    // Start with the first face
    orderedFaces.push_back(adjacentFaces[0]);
    visited.insert(adjacentFaces[0]);

    while (orderedFaces.size() < adjacentFaces.size()) {
        uint32_t currentFace = orderedFaces.back();
        bool foundNext = false;

        // Find the next adjacent face that shares an edge with the current face
        for (uint32_t nextFace : adjacentFaces) {
            if (visited.count(nextFace)) continue;
            auto [sharedV1, sharedV2] = findSharedEdge(indices, faces[currentFace], faces[nextFace]);
            if (sharedV1 != -1 && sharedV2 != -1) {
                orderedFaces.push_back(nextFace);
                visited.insert(nextFace);
                foundNext = true;
                break;
            }
        }
        if (!foundNext) break;
    }

    return orderedFaces;
}

// Function to create the dual of an icosphere
NgonData createDual(const NgonData& original) {
    NgonData dual;

    // Step 1: Create vertices for the dual (centroids of the original faces)
    std::map<uint32_t, std::vector<uint32_t>> vertexToFaces; // Maps each vertex to the faces that share it
    for (uint32_t faceIndex = 0; faceIndex < original.faces.size(); ++faceIndex) {
        const NGonFace& face = original.faces[faceIndex];

        // Get the three vertices of the original face
        unsigned int v1 = original.indices[face.offset];
        unsigned int v2 = original.indices[face.offset + 1];
        unsigned int v3 = original.indices[face.offset + 2];

        Vec3f p1 = glm::vec3(original.vertices[v1].position);
        Vec3f p2 = glm::vec3(original.vertices[v2].position);
        Vec3f p3 = glm::vec3(original.vertices[v3].position);

        // Compute the centroid of the triangle (new vertex in the dual)
        Vec3f centroid = computeTriangleCenter(p1, p2, p3);
        Vertex newVertex;
        newVertex.position = Vec4f(centroid, 1.0f);
        newVertex.normal = Vec4f(normalize(centroid), 0.0f);
        dual.vertices.push_back(newVertex);

        // Update the map to track which faces share this vertex in the original
        vertexToFaces[v1].push_back(faceIndex);
        vertexToFaces[v2].push_back(faceIndex);
        vertexToFaces[v3].push_back(faceIndex);
    }

    // Step 2: Create faces for the dual
    for (const auto& [originalVertex, adjacentFaces] : vertexToFaces) {
        // Order the adjacent faces to ensure we form a valid polygon
        std::vector<uint32_t> orderedFaces = orderAdjacentFaces(original.faces, original.indices, originalVertex, adjacentFaces);

        // Create a new face in the dual by connecting the centroids of ordered adjacent faces
        if (orderedFaces.size() < 3) continue; // Ensure we have at least 3 adjacent faces (triangles)

        NGonFace newFace;
        newFace.offset = dual.indices.size(); // Offset into the index array
        newFace.count = orderedFaces.size();  // Number of vertices in the face

        // Add the new face's vertices (which are centroids of adjacent faces)
        for (uint32_t faceIndex : orderedFaces) {
            dual.indices.push_back(faceIndex);
        }

        // Calculate the face center by averaging the centroids of the adjacent faces
        Vec3f faceCenter = Vec3f(0.0f);
        for (uint32_t faceIndex : orderedFaces) {
            faceCenter += glm::vec3(dual.vertices[faceIndex].position);
        }
        faceCenter /= static_cast<float>(orderedFaces.size());

         // Calculate the face area by triangulating the face
        float faceArea = 0.0f;
        Vec3f root = glm::vec3(dual.vertices[orderedFaces[0]].position);
        for (size_t i = 1; i < orderedFaces.size() - 1; ++i) {
            Vec3f v1 = glm::vec3(dual.vertices[orderedFaces[i]].position);
            Vec3f v2 = glm::vec3(dual.vertices[orderedFaces[i + 1]].position);
            faceArea += computeTriangleArea(root, v1, v2);
        }
        Vec3f faceNormal = normalize(faceCenter);

        // Store the new face information
        newFace.center = Vec4f(faceCenter, 1.0f);
        newFace.faceArea = faceArea;
        newFace.normal = Vec4f(faceNormal, 0.0f);
        dual.faces.push_back(newFace);
    }

    return dual;
}