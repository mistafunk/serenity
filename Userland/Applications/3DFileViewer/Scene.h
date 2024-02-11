#pragma once

#include <AK/RefCounted.h>
#include <AK/RefPtr.h>

#include "Mesh.h"

// represents all (viewable) content of a 3d file
class Scene : public RefCounted<Scene> {
public:
    Scene() = delete;
    Scene(NonnullRefPtr<Mesh> mesh)
        : m_mesh(mesh) {};

    size_t vertex_count() const { return m_mesh->vertex_count(); }

    size_t triangle_count() const { return m_mesh->triangle_count(); }

    void draw(float uv_scale) { m_mesh->draw(uv_scale); }

    bool is_textured() const { return m_mesh->is_textured(); }

    bool has_normals() const { return m_mesh->has_normals(); }

private:
    RefPtr<Mesh> m_mesh;
};
