#pragma once

#include "managedUBO.hpp"

#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

namespace ShaderUtils {
static std::string readShaderSource(const std::string &shaderPath) {
    std::filesystem::path path(shaderPath);
    std::ifstream shaderFile(path);

    if (!shaderFile.is_open()) { return ""; }

    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();
    return shaderStream.str();
}

static shaderc_shader_kind getKindFromFileExt(const std::string &filePath) {
    const std::string extension = std::filesystem::path(filePath).extension().string();
    if (extension == ".vert") { return shaderc_glsl_vertex_shader; }
    if (extension == ".frag") { return shaderc_glsl_fragment_shader; }
    if (extension == ".mesh") { return shaderc_glsl_mesh_shader; }
    if (extension == ".task") { return shaderc_glsl_task_shader; }
    if (extension == ".geom") { return shaderc_glsl_geometry_shader; }
    if (extension == ".tesc") { return shaderc_glsl_tess_control_shader; }
    if (extension == ".tese") { return shaderc_glsl_tess_evaluation_shader; }
    if (extension == ".comp") { return shaderc_glsl_compute_shader; }
    return shaderc_glsl_infer_from_source;
}

static constexpr vk::ShaderStageFlagBits getVkShaderStage(shaderc_shader_kind kind) {
    switch (kind) {
    case shaderc_glsl_vertex_shader: return vk::ShaderStageFlagBits::eVertex;
    case shaderc_glsl_fragment_shader: return vk::ShaderStageFlagBits::eFragment;
    case shaderc_glsl_compute_shader: return vk::ShaderStageFlagBits::eCompute;
    case shaderc_glsl_geometry_shader: return vk::ShaderStageFlagBits::eGeometry;
    case shaderc_glsl_tess_control_shader: return vk::ShaderStageFlagBits::eTessellationControl;
    case shaderc_glsl_tess_evaluation_shader: return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case shaderc_glsl_mesh_shader: return vk::ShaderStageFlagBits::eMeshEXT;
    case shaderc_glsl_task_shader: return vk::ShaderStageFlagBits::eTaskEXT;
    default: throw std::runtime_error("Unsupported shader stage");
    }
}

// static void binaryDump(const std::vector<uint32_t> &binary) {
//     // dump the binary to a file
//     std::ofstream out("shader.bin", std::ios::binary);
//     out.write(reinterpret_cast<const char *>(binary.data()), binary.size() * sizeof(uint32_t));
// }

// static void textDump(const std::string source, const std::string path) {
//     std::ofstream out(path);
//     // dump the source to a file
//     out << source;
// }

// Todo: this is basicaly a copy of https://github.com/google/shaderc/blob/main/glslc/src/file_includer.cc, we can do better, but for now it's fine
class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    ShaderIncluder() = default;

    // Add an include directory to search (appended to search list).
    void addIncludePath(const std::filesystem::path &dir) {
        include_paths_.push_back(dir);
    }

	bool removeIncludePath(const std::filesystem::path &dir) {
        std::filesystem::path normalized = std::filesystem::absolute(dir).lexically_normal();
        auto it = std::find(include_paths_.begin(), include_paths_.end(), normalized);
        if (it == include_paths_.end()) return false;
        include_paths_.erase(it);
        return true;
    }


    shaderc_include_result* GetInclude(const char* requested_source,
                                       shaderc_include_type type,
                                       const char* requesting_source,
                                       size_t include_depth) override {
        (void)include_depth; // unused parameter

        if (requested_source == nullptr) {
            throw std::runtime_error("GetInclude: requested_source is null");
        }
        if (requesting_source == nullptr) {
            // Some callers may pass nullptr for top-level shaders; treat as empty path
            requesting_source = "";
        }

        std::filesystem::path requestedPath(requested_source);
        std::filesystem::path resolvedPath;
        bool found = false;

        if (type == shaderc_include_type_relative) { // e.g. "myinclude.h"

            // First try relative to requesting_source's directory
            std::filesystem::path reqPath(requesting_source);
            std::filesystem::path candidate = reqPath.parent_path() / requestedPath;
            if (std::filesystem::exists(candidate)) {
                resolvedPath = std::filesystem::canonical(candidate);
                found = true;
            } else {
                // Fall back to searching the configured include paths
                for (const auto &incDir : include_paths_) {
                    candidate = incDir / requestedPath;
                    if (std::filesystem::exists(candidate)) {
                        resolvedPath = std::filesystem::canonical(candidate);
                        found = true;
                        break;
                    }
                }
            }
        } else { // shaderc_include_type_standard (angle-bracket style)  e.g. <myinclude.h>
            for (const auto &incDir : include_paths_) {
                std::filesystem::path candidate = incDir / requestedPath;
                if (std::filesystem::exists(candidate)) {
                    resolvedPath = std::filesystem::canonical(candidate);
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            // Build helpful error message
            std::string err = std::string(requesting_source) + ": error: include file not found: " + requestedPath.string();
            // Optionally append include paths tried
            if (!include_paths_.empty()) {
                err += " (searched:";
                if (type == shaderc_include_type_relative) {
                    err += " " + std::string(std::filesystem::path(requesting_source).parent_path().string());
                }
                for (const auto &p : include_paths_) {
                    err += " " + p.string();
                }
                err += ")";
            }
            throw std::runtime_error(std::move(err));
        }

        // Read the file
        auto file = new FileInfo{ resolvedPath.string(), {} };
        file->content = readShaderSource(file->path);
        if (file->content.empty()) {
            delete file;
            throw std::runtime_error(std::string(requesting_source) + ": error: failed to read include file: " + resolvedPath.string());
        }

        // Note: file->path.c_str() and file->content.c_str() will remain valid
        // until ReleaseInclude deletes the FileInfo.
        auto result = new shaderc_include_result{
            file->path.c_str(),           // char* source_name
            file->path.size(),           // size_t source_name_size
            file->content.c_str(),       // char* content
            file->content.size(),        // size_t content_size
            file                         // void* user_data
        };
        return result;
    }

    // Releases an include result.
    void ReleaseInclude(shaderc_include_result* include_result) override {
        if (include_result == nullptr) return;
        FileInfo* info = static_cast<FileInfo*>(include_result->user_data);
        delete info;
        delete include_result;
    }

    struct FileInfo {
        std::string path;
        std::string content;
    };

private:
    std::vector<std::filesystem::path> include_paths_;
};

static std::vector<SpvReflectDescriptorSet *> getDescriptorSets(const spv_reflect::ShaderModule &reflection) {
    uint32_t setCount = 0;
    reflection.EnumerateDescriptorSets(&setCount, nullptr);
    std::vector<SpvReflectDescriptorSet *> sets(setCount);
    reflection.EnumerateDescriptorSets(&setCount, sets.data());
    return sets;
}

// get push constants from reflection data
static std::vector<SpvReflectBlockVariable *> getPushConstants(const spv_reflect::ShaderModule &reflection) {
    uint32_t pushConstantCount = 0;
    reflection.EnumeratePushConstantBlocks(&pushConstantCount, nullptr);
    std::vector<SpvReflectBlockVariable *> pushConstants(pushConstantCount);
    reflection.EnumeratePushConstantBlocks(&pushConstantCount, pushConstants.data());
    return pushConstants;
}

static RHI::UBOLayout::Field deduceField(const SpvReflectTypeDescription &typeDesc) {
    RHI::UBOLayout::Field field;
    if (typeDesc.type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
        if (typeDesc.traits.numeric.scalar.signedness == 1) {
            field.type = RHI::UBOLayout::ManagedType::INT;
        } else {
            field.type = RHI::UBOLayout::ManagedType::UINT;
        }
    } else if (typeDesc.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
        if (typeDesc.traits.numeric.scalar.width == 64) {
            field.type = RHI::UBOLayout::ManagedType::DOUBLE;
        } else {
            field.type = RHI::UBOLayout::ManagedType::FLOAT;
        }
    }
    if (typeDesc.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
        field.components = typeDesc.traits.numeric.vector.component_count;
        if (typeDesc.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
            if (typeDesc.traits.numeric.matrix.column_count != typeDesc.traits.numeric.matrix.row_count) {
                throw std::runtime_error("Non-square matrices are not supported, please use an array of vectors instead.");
            }
            field.components *= typeDesc.traits.numeric.vector.component_count;
        }
    }

    if (typeDesc.type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) {
        if (typeDesc.traits.array.dims_count > 1) {
            throw std::runtime_error("Multidimensional arrays are not supported.");
        }
        field.count = typeDesc.traits.array.dims[0];
    }
    return field;
}

static RHI::UBOLayout generateUBOLayout(const SpvReflectDescriptorBinding &descriptorReflection) {
    RHI::UBOLayout layout;
    layout.uboName = descriptorReflection.name;
    layout.paddedSize = descriptorReflection.block.padded_size;
    layout.fields.resize(descriptorReflection.block.member_count);
    layout.fieldIndexToNameMap.resize(descriptorReflection.block.member_count);
    for (unsigned int i = 0; i < descriptorReflection.block.member_count; i++) {
        SpvReflectBlockVariable variable = descriptorReflection.block.members[i];
        layout.fieldIndexToNameMap[i] = variable.name;
        layout.fields[i] = deduceField(*variable.type_description);
        layout.fields[i].paddedSize = variable.padded_size;
    }
    return layout;
}

static RHI::UBOLayout generatePushConstantLayout(const SpvReflectBlockVariable &pushConstantReflection) {
    RHI::UBOLayout layout;
    layout.uboName = "PushConstants";
    layout.paddedSize = pushConstantReflection.padded_size;
    layout.fields.resize(pushConstantReflection.member_count);
    layout.fieldIndexToNameMap.resize(pushConstantReflection.member_count);
    for (unsigned int i = 0; i < pushConstantReflection.member_count; i++) {
        SpvReflectBlockVariable variable = pushConstantReflection.members[i];
        layout.fieldIndexToNameMap[i] = variable.name;
        layout.fields[i] = deduceField(*variable.type_description);
        layout.fields[i].paddedSize = variable.padded_size;
    }
    return layout;
}

} // namespace ShaderUtils
