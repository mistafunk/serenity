#pragma once

#include <AK/RefCounted.h>
#include <AK/RefPtr.h>

#include "Mesh.h"

// represents all (viewable) content of a 3d file
class Scene : public RefCounted<Scene> {
public:
    Scene() = delete;
    Scene(NonnullRefPtr<Mesh> mesh)
    {
        m_mesh.append(mesh);
    }

    size_t vertex_count() const { return m_mesh.first()->vertex_count(); }

    size_t triangle_count() const { return m_mesh.first()->triangle_count(); }

    void draw(float uv_scale) { m_mesh.first()->draw(uv_scale); }

    bool is_textured() const { return m_mesh.first()->is_textured(); }

    bool has_normals() const { return m_mesh.first()->has_normals(); }

private:
    Vector<RefPtr<Mesh>> m_mesh;
};
