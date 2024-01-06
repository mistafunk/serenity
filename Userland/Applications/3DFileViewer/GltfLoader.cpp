#include "GltfLoader.h"

#include "AK/Error.h"
#include "AK/ByteBuffer.h"
#include "AK/Format.h"
#include "AK/String.h"
#include "LibCore/File.h"

// #define TINYGLTF_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION
// #define TINYGLTF_USE_CPP14
// #define STB_IMAGE_IMPLEMENTATION
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #define JSON_NOEXCEPTION
// #define JSON_HAS_CPP_14
// #include "tinygltf/tiny_gltf.h"


ErrorOr<NonnullRefPtr<Scene>> GltfLoader::load(ByteString const& filename, NonnullOwnPtr<Core::File> file)
{
    dbgln("glTF: Loading '{}' ...", filename);
    auto buffered_file = TRY(Core::InputBufferedFile::create(move(file)));
    ErrorOr<ByteBuffer> fileContent = TRY(buffered_file->read_until_eof());
    if (fileContent.is_error())
        return fileContent.release_error();
    //auto& gltfData = fileContent.value();


    return fileContent.release_error();
}
