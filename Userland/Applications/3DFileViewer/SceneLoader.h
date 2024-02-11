/*
 * Copyright (c) 2021, Jesse Buhagiar <jooster669@gmail.com>
 * Copyright (c) 2022, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibCore/Forward.h>

#include "Common.h"
#include "Scene.h"

class SceneLoader {
public:
    SceneLoader() = default;
    virtual ~SceneLoader() = default;

    virtual ErrorOr<NonnullRefPtr<Scene>> load(ByteString const& filename, NonnullOwnPtr<Core::File> file) = 0;
};
