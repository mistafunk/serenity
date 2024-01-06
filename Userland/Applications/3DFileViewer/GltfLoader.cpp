#include "GltfLoader.h"

#include "AK/Error.h"
#include "AK/Format.h"
#include "AK/String.h"
#include "LibCore/File.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define JSON_NOEXCEPTION
#define JSON_HAS_CPP_14
#include "tinygltf/tiny_gltf.h"

#include <string>

namespace {

template<typename T>
struct arrayAdapter {
    /// Pointer to the bytes
    unsigned char const* dataPtr;
    /// Number of elements in the array
    size_t const elemCount;
    /// Stride in bytes between two elements
    size_t const stride;

    /// Construct an array adapter.
    /// \param ptr Pointer to the start of the data, with offset applied
    /// \param count Number of elements in the array
    /// \param byte_stride Stride betweens elements in the array
    arrayAdapter(unsigned char const* ptr, size_t count, size_t byte_stride)
        : dataPtr(ptr)
        , elemCount(count)
        , stride(byte_stride)
    {
    }

    /// Returns a *copy* of a single element. Can't be used to modify it.
    T operator[](size_t pos) const
    {
        //    if (pos >= elemCount)
        //      throw std::out_of_range(
        //          "Tried to access beyond the last element of an array adapter with "
        //          "count " +
        //          std::to_string(elemCount) + " while getting elemnet number " +
        //          std::to_string(pos));
        return *(reinterpret_cast<T const*>(dataPtr + pos * stride));
    }
};

/// Interface of any adapted array that returns ingeger data
struct intArrayBase {
    virtual ~intArrayBase() = default;
    virtual unsigned int operator[](size_t) const = 0;
    virtual size_t size() const = 0;
};

/// Interface of any adapted array that returns float data
struct floatArrayBase {
    virtual ~floatArrayBase() = default;
    virtual float operator[](size_t) const = 0;
    virtual size_t size() const = 0;
};

/// An array that loads interger types, returns them as int
template<class T>
struct intArray : public intArrayBase {
    arrayAdapter<T> adapter;

    intArray(arrayAdapter<T> const& a)
        : adapter(a)
    {
    }
    unsigned int operator[](size_t position) const override
    {
        return static_cast<unsigned int>(adapter[position]);
    }

    size_t size() const override { return adapter.elemCount; }
};

template<class T>
struct floatArray : public floatArrayBase {
    arrayAdapter<T> adapter;

    floatArray(arrayAdapter<T> const& a)
        : adapter(a)
    {
    }
    float operator[](size_t position) const override
    {
        return static_cast<float>(adapter[position]);
    }

    size_t size() const override { return adapter.elemCount; }
};

#pragma pack(push, 1)

template<typename T>
struct v2 {
    T x, y;
};
/// 3D vector of floats without padding
template<typename T>
struct v3 {
    T x, y, z;
};

/// 4D vector of floats without padding
template<typename T>
struct v4 {
    T x, y, z, w;
};

#pragma pack(pop)

using v2f = v2<float>;
using v3f = v3<float>;
using v4f = v4<float>;
using v2d = v2<double>;
using v3d = v3<double>;
using v4d = v4<double>;

struct v2fArray {
    arrayAdapter<v2f> adapter;
    v2fArray(arrayAdapter<v2f> const& a)
        : adapter(a)
    {
    }

    v2f operator[](size_t position) const { return adapter[position]; }
    size_t size() const { return adapter.elemCount; }
};

struct v3fArray {
    arrayAdapter<v3f> adapter;
    v3fArray(arrayAdapter<v3f> const& a)
        : adapter(a)
    {
    }

    v3f operator[](size_t position) const { return adapter[position]; }
    size_t size() const { return adapter.elemCount; }
};

struct v4fArray {
    arrayAdapter<v4f> adapter;
    v4fArray(arrayAdapter<v4f> const& a)
        : adapter(a)
    {
    }

    v4f operator[](size_t position) const { return adapter[position]; }
    size_t size() const { return adapter.elemCount; }
};

struct v2dArray {
    arrayAdapter<v2d> adapter;
    v2dArray(arrayAdapter<v2d> const& a)
        : adapter(a)
    {
    }

    v2d operator[](size_t position) const { return adapter[position]; }
    size_t size() const { return adapter.elemCount; }
};

struct v3dArray {
    arrayAdapter<v3d> adapter;
    v3dArray(arrayAdapter<v3d> const& a)
        : adapter(a)
    {
    }

    v3d operator[](size_t position) const { return adapter[position]; }
    size_t size() const { return adapter.elemCount; }
};

struct v4dArray {
    arrayAdapter<v4d> adapter;
    v4dArray(arrayAdapter<v4d> const& a)
        : adapter(a)
    {
    }

    v4d operator[](size_t position) const { return adapter[position]; }
    size_t size() const { return adapter.elemCount; }
};

// based on https://github.com/syoyo/tinygltf/blob/release/examples/raytrace/gltf-loader.cc
ErrorOr<NonnullRefPtr<Mesh>> assembleMesh(tinygltf::Model const& model)
{
    dbgln("Found {} meshes.", model.meshes.size());

    Vector<Vertex> vertices;
    Vector<Vertex> normals;
    Vector<TexCoord> tex_coords;
    Vector<Triangle> triangles;

    assert(model.scenes.size() > 0);
    int scene_to_display = model.defaultScene > -1 ? model.defaultScene : 0;
    tinygltf::Scene const& scene = model.scenes[scene_to_display];

    for (auto& nodeId : scene.nodes) {
        auto const meshId = model.nodes[nodeId].mesh;
        dbgln("meshId: {}", meshId);
        if (meshId < 0 || meshId >= static_cast<int>(model.meshes.size())) {
            warnln("skipping invalid meshId {}", meshId);
            continue;
        }
        for (auto const& prim : model.meshes[meshId].primitives) {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES && prim.mode != TINYGLTF_MODE_TRIANGLE_STRIP && prim.mode != TINYGLTF_MODE_TRIANGLE_FAN)
                continue;

            bool convertedToTriangleList = false;

            std::unique_ptr<intArrayBase> indicesArrayPtr = nullptr;
            {
                auto const& indicesAccessor = model.accessors[prim.indices];
                auto const& bufferView = model.bufferViews[indicesAccessor.bufferView];
                auto const& buffer = model.buffers[bufferView.buffer];
                auto const dataAddress = buffer.data.data() + bufferView.byteOffset + indicesAccessor.byteOffset;
                auto const byteStride = indicesAccessor.ByteStride(bufferView);
                auto const count = indicesAccessor.count;

                switch (indicesAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                    indicesArrayPtr = std::unique_ptr<intArray<char>>(new intArray<char>(
                        arrayAdapter<char>(dataAddress, count, byteStride)));
                    break;

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    indicesArrayPtr = std::unique_ptr<intArray<unsigned char>>(
                        new intArray<unsigned char>(arrayAdapter<unsigned char>(
                            dataAddress, count, byteStride)));
                    break;

                case TINYGLTF_COMPONENT_TYPE_SHORT:
                    indicesArrayPtr = std::unique_ptr<intArray<short>>(new intArray<short>(
                        arrayAdapter<short>(dataAddress, count, byteStride)));
                    break;

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    indicesArrayPtr = std::unique_ptr<intArray<unsigned short>>(
                        new intArray<unsigned short>(arrayAdapter<unsigned short>(
                            dataAddress, count, byteStride)));
                    break;

                case TINYGLTF_COMPONENT_TYPE_INT:
                    indicesArrayPtr = std::unique_ptr<intArray<int>>(new intArray<int>(
                        arrayAdapter<int>(dataAddress, count, byteStride)));
                    break;

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    indicesArrayPtr = std::unique_ptr<intArray<unsigned int>>(
                        new intArray<unsigned int>(arrayAdapter<unsigned int>(
                            dataAddress, count, byteStride)));
                    break;
                default:
                    break;
                }
            }
            auto const& indices = *indicesArrayPtr;

            std::vector<uint32_t> faces;
            if (indicesArrayPtr) {
                faces.reserve(indicesArrayPtr->size());
                for (size_t i(0); i < indicesArrayPtr->size(); ++i) {
                    faces.push_back(indices[i]);
                }
            }

            switch (prim.mode) {
            // We re-arrange the indices so that it describe a simple list of
            // triangles
            case TINYGLTF_MODE_TRIANGLE_FAN: {
                if (!convertedToTriangleList) {
                    // This only has to be done once per primitive
                    convertedToTriangleList = true;

                    // We steal the guts of the vector
                    auto triangleFan = std::move(faces);
                    faces.clear();

                    // Push back the indices that describe just one triangle one by one
                    for (size_t i { 2 }; i < triangleFan.size(); ++i) {
                        triangles.append({ triangleFan[0], triangleFan[i - 1], triangleFan[i], 0, 0, 0, 0, 0, 0 });
                    }
                }
                [[fallthrough]];
            }
            case TINYGLTF_MODE_TRIANGLE_STRIP: {
                if (!convertedToTriangleList) {
                    // This only has to be done once per primitive
                    convertedToTriangleList = true;

                    auto triangleStrip = std::move(faces);
                    faces.clear();

                    for (size_t i { 2 }; i < triangleStrip.size(); ++i) {
                        triangles.append({ triangleStrip[i - 2], triangleStrip[i - 1], triangleStrip[i], 0, 0, 0, 0, 0, 0 });
                    }
                }
                [[fallthrough]];
            }
            case TINYGLTF_MODE_TRIANGLES: {
                for (auto const& attribute : prim.attributes) {
                    auto const attribAccessor = model.accessors[attribute.second];
                    auto const& bufferView = model.bufferViews[attribAccessor.bufferView];
                    auto const& buffer = model.buffers[bufferView.buffer];
                    auto const dataPtr = buffer.data.data() + bufferView.byteOffset + attribAccessor.byteOffset;
                    auto const byte_stride = attribAccessor.ByteStride(bufferView);
                    auto const count = attribAccessor.count;

                    dbgln("current attribute has count {} and stride {} bytes", count, byte_stride);

                    dbgln("attribute string is: {}", AK::StringView(attribute.first.c_str(), attribute.first.length()));
                    if (attribute.first == "POSITION") {
                        dbgln("found position attribute");

                        switch (attribAccessor.type) {
                        case TINYGLTF_TYPE_VEC3: {
                            switch (attribAccessor.componentType) {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                dbgln("Type is FLOAT");

                                v3fArray positions(arrayAdapter<v3f>(dataPtr, count, byte_stride));
                                dbgln("positions.size: {}", positions.size());

                                for (size_t i { 0 }; i < positions.size(); ++i) {
                                    auto const v = positions[i];
                                    dbgln("positions[{}]: ({}, {}, {})", i, v.x, v.y, v.z);
                                    vertices.append({ v.x, v.y, v.z });
                                }
                            }
                            break;
                        case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
                            dbgln("Type is DOUBLE");
                            switch (attribAccessor.type) {
                            case TINYGLTF_TYPE_VEC3: {
                                v3dArray positions(arrayAdapter<v3d>(dataPtr, count, byte_stride));
                                for (size_t i { 0 }; i < positions.size(); ++i) {
                                    auto const v = positions[i];
                                    dbgln("positions[{}]: ({}, {}, {})", i, v.x, v.y, v.z);
                                    vertices.append({ static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z) });
                                }
                                break;
                            }
                            default:
                                // TODO Handle error
                                break;
                            }
                            break;
                        }
                        }
                        default:
                            break;
                        }
                    }

                    if (attribute.first == "NORMAL") {
                        dbgln("found normal attribute");

                        switch (attribAccessor.type) {
                        case TINYGLTF_TYPE_VEC3: {
                            dbgln("Normal is VEC3");
                            switch (attribAccessor.componentType) {
                            case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                                dbgln("Normal is FLOAT");
                                v3fArray normals_(arrayAdapter<v3f>(dataPtr, count, byte_stride));

                                // IMPORTANT: We need to reorder normals (and texture
                                // coordinates into "facevarying" order) for each face

                                // For each triangle :
                                for (size_t i { 0 }; i < indices.size() / 3; ++i) {
                                    // get the i'th triange's indexes
                                    auto f0 = indices[3 * i + 0];
                                    auto f1 = indices[3 * i + 1];
                                    auto f2 = indices[3 * i + 2];

                                    // get the 3 normal vectors for that face
                                    v3f n0, n1, n2;
                                    n0 = normals_[f0];
                                    n1 = normals_[f1];
                                    n2 = normals_[f2];

                                    // Put them in the array in the correct order
                                    normals.append({ n0.x, n0.y, n0.z });
                                    normals.append({ n1.x, n1.y, n2.z });
                                    normals.append({ n2.x, n1.y, n2.z });
                                }
                            } break;
                            case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
                                dbgln("Normal is DOUBLE");
                                v3dArray normals_(arrayAdapter<v3d>(dataPtr, count, byte_stride));

                                // IMPORTANT: We need to reorder normals (and texture
                                // coordinates into "facevarying" order) for each face

                                // For each triangle :
                                for (size_t i { 0 }; i < indices.size() / 3; ++i) {
                                    // get the i'th triange's indexes
                                    auto f0 = indices[3 * i + 0];
                                    auto f1 = indices[3 * i + 1];
                                    auto f2 = indices[3 * i + 2];

                                    // get the 3 normal vectors for that face
                                    v3d n0, n1, n2;
                                    n0 = normals_[f0];
                                    n1 = normals_[f1];
                                    n2 = normals_[f2];

                                    // Put them in the array in the correct order
                                    normals.append({ static_cast<GLfloat>(n0.x), static_cast<GLfloat>(n0.y), static_cast<GLfloat>(n0.z) });
                                    normals.append({ static_cast<GLfloat>(n1.x), static_cast<GLfloat>(n1.y), static_cast<GLfloat>(n2.z) });
                                    normals.append({ static_cast<GLfloat>(n2.x), static_cast<GLfloat>(n1.y), static_cast<GLfloat>(n2.z) });
                                }
                            } break;
                            default:
                                dbgln("Unhandled component type for normal");
                            }
                        } break;
                        default:
                            dbgln("Unhandled vector type for normal");
                        }

                        // Face varying comment on the normals is also true for the UVs
#if 0
                            if (attribute.first == "TEXCOORD_0") {
                                dbgln("Found texture coordinates");

                                switch (attribAccessor.type) {
                                case TINYGLTF_TYPE_VEC2: {
                                    dbgln("TEXTCOORD is VEC2");
                                    switch (attribAccessor.componentType) {
                                    case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                                        dbgln("TEXTCOORD is FLOAT");
                                        v2fArray uvs(
                                            arrayAdapter<v2f>(dataPtr, count, byte_stride));

                                        for (size_t i { 0 }; i < indices.size() / 3; ++i) {
                                            // get the i'th triange's indexes
                                            auto f0 = indices[3 * i + 0];
                                            auto f1 = indices[3 * i + 1];
                                            auto f2 = indices[3 * i + 2];

                                            // get the texture coordinates for each triangle's
                                            // vertices
                                            v2f uv0, uv1, uv2;
                                            uv0 = uvs[f0];
                                            uv1 = uvs[f1];
                                            uv2 = uvs[f2];

                                            // push them in order into the mesh data
                                            loadedMesh.facevarying_uvs.push_back(uv0.x);
                                            loadedMesh.facevarying_uvs.push_back(uv0.y);

                                            loadedMesh.facevarying_uvs.push_back(uv1.x);
                                            loadedMesh.facevarying_uvs.push_back(uv1.y);

                                            loadedMesh.facevarying_uvs.push_back(uv2.x);
                                            loadedMesh.facevarying_uvs.push_back(uv2.y);
                                        }

                                    } break;
                                    case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
                                        std::cout << "TEXTCOORD is DOUBLE\n";
                                        v2dArray uvs(
                                            arrayAdapter<v2d>(dataPtr, count, byte_stride));

                                        for (size_t i { 0 }; i < indices.size() / 3; ++i) {
                                            // get the i'th triange's indexes
                                            auto f0 = indices[3 * i + 0];
                                            auto f1 = indices[3 * i + 1];
                                            auto f2 = indices[3 * i + 2];

                                            v2d uv0, uv1, uv2;
                                            uv0 = uvs[f0];
                                            uv1 = uvs[f1];
                                            uv2 = uvs[f2];

                                            loadedMesh.facevarying_uvs.push_back(uv0.x);
                                            loadedMesh.facevarying_uvs.push_back(uv0.y);

                                            loadedMesh.facevarying_uvs.push_back(uv1.x);
                                            loadedMesh.facevarying_uvs.push_back(uv1.y);

                                            loadedMesh.facevarying_uvs.push_back(uv2.x);
                                            loadedMesh.facevarying_uvs.push_back(uv2.y);
                                        }
                                    } break;
                                    default:
                                        return Error::from_string_literal("unrecognized vector type for UV");
                                    }
                                } break;
                                default:
                                    return Error::from_string_literal("unreconized componant type for UV");
                                }
                            }
#endif
                    }
                }
                break;

            default:
                warnln("primitive mode not implemented");
                break;
            }
            }
        }
    }

    if (vertices.is_empty()) {
        return Error::from_string_literal("glTF: Failed to read any vertices.");
    }

    dbgln("glTF: Done.");
    return adopt_ref(*new Mesh(vertices, tex_coords, normals, triangles));
}

ErrorOr<tinygltf::Model> loadFileFromBuffer(ByteBuffer const& buffer)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, buffer.data(), buffer.size());
    if (!ret) {
        err = "Error while loading glTF." + err;
        return Error::from_string_view(StringView(warn.c_str(), warn.length()));
    }
    if (!warn.empty()) {
        warnln("Warning while loading glTF: {}", StringView(warn.c_str(), warn.length()));
    }
    return model;
}

} // namespace

ErrorOr<NonnullRefPtr<Mesh>> GltfLoader::load(ByteString const& filename, NonnullOwnPtr<Core::File> file)
{
    dbgln("glTF: Loading '{}' ...", filename);
    auto buffered_file = TRY(Core::InputBufferedFile::create(move(file)));
    ErrorOr<ByteBuffer> fileContent = TRY(buffered_file->read_until_eof());
    if (fileContent.is_error())
        return fileContent.release_error();
    auto& gltfData = fileContent.value();

    ErrorOr<tinygltf::Model> model = loadFileFromBuffer(gltfData);
    if (model.is_error())
        return model.release_error();

    return assembleMesh(model.release_value());
}
