#pragma once

#include <AK/RefCounted.h>
#include <AK/RefPtr.h>

#include "Mesh.h"
#include "MeshLoader.h"

class GltfLoader final : public MeshLoader {
public:
    GltfLoader() = default;
    ~GltfLoader() override = default;

    ErrorOr<NonnullRefPtr<Mesh>> load(ByteString const& filename, NonnullOwnPtr<Core::File> file) override;
};
