//----------------------------------------------------------------------------
//! @file   skinned_mesh.cpp
//! @brief  SkinnedMesh 実装
//----------------------------------------------------------------------------
#include "skinned_mesh.h"
#include "common/logging/logging.h"

std::shared_ptr<SkinnedMesh> SkinnedMesh::Create(const SkinnedMeshDesc& desc)
{
    if (desc.vertices.empty() || desc.indices.empty()) {
        LOG_ERROR("[SkinnedMesh] Empty vertex or index data");
        return nullptr;
    }

    auto mesh = std::shared_ptr<SkinnedMesh>(new SkinnedMesh());

    // 頂点バッファ作成
    mesh->vertexBuffer_ = Buffer::CreateVertex(
        static_cast<uint32_t>(desc.vertices.size() * sizeof(SkinnedMeshVertex)),
        sizeof(SkinnedMeshVertex),
        false,  // dynamic = false（静的メッシュ）
        desc.vertices.data()
    );

    if (!mesh->vertexBuffer_) {
        LOG_ERROR("[SkinnedMesh] Failed to create vertex buffer");
        return nullptr;
    }

    // インデックスバッファ作成
    mesh->indexBuffer_ = Buffer::CreateIndex(
        static_cast<uint32_t>(desc.indices.size() * sizeof(uint32_t)),
        false,  // dynamic = false
        desc.indices.data()
    );

    if (!mesh->indexBuffer_) {
        LOG_ERROR("[SkinnedMesh] Failed to create index buffer");
        return nullptr;
    }

    mesh->vertexCount_ = static_cast<uint32_t>(desc.vertices.size());
    mesh->indexCount_ = static_cast<uint32_t>(desc.indices.size());
    mesh->subMeshes_ = desc.subMeshes;
    mesh->bounds_ = desc.bounds;
    mesh->name_ = desc.name;
    mesh->skeleton_ = desc.skeleton;
    mesh->animations_ = desc.animations;

    LOG_INFO("[SkinnedMesh] Created '" + desc.name + "' with " +
             std::to_string(mesh->vertexCount_) + " vertices, " +
             std::to_string(mesh->GetBoneCount()) + " bones, " +
             std::to_string(mesh->animations_.size()) + " animations");

    return mesh;
}
