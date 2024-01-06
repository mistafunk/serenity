#pragma once

#include <AK/RefCounted.h>
#include <AK/RefPtr.h>

#include "Mesh.h"
#include "SceneLoader.h"

class GltfLoader final : public SceneLoader {
public:
    GltfLoader() = default;
    ~GltfLoader() override = default;

    ErrorOr<NonnullRefPtr<Scene>> load(ByteString const& filename, NonnullOwnPtr<Core::File> file) override;
};
