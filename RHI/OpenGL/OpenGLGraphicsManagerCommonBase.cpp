// This is a common code snippet
// should be included in other source
// other than compile it independently
#include <sstream>
#include <functional>
using namespace std;

void OpenGLGraphicsManagerCommonBase::Clear()
{
    GraphicsManager::Clear();

    // Set the color to clear the screen to.
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    // Clear the screen and depth buffer.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLGraphicsManagerCommonBase::Finalize()
{
    GraphicsManager::Finalize();
}

void OpenGLGraphicsManagerCommonBase::Draw()
{
    GraphicsManager::Draw();

    glFlush();
}

bool OpenGLGraphicsManagerCommonBase::SetPerFrameShaderParameters(const DrawFrameContext& context)
{
    GLuint blockIndex = glGetUniformBlockIndex(m_CurrentShader, "DrawFrameConstants");

    if (blockIndex == GL_INVALID_INDEX)
    {
        // the shader does not use "DrawFrameConstants"
        // simply returns true here
        return true;
    }

    GLint blockSize;

    if (!m_UboBuffer)
    {
        glGenBuffers(1, &m_UboBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_UboBuffer);

        glGetActiveUniformBlockiv(m_CurrentShader, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

        glBufferData(GL_UNIFORM_BUFFER, blockSize, nullptr, GL_DYNAMIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_UboBuffer);
        glGetActiveUniformBlockiv(m_CurrentShader, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
    }

    GLubyte* blockBuffer = static_cast<GLubyte*>(glMapBufferRange(GL_UNIFORM_BUFFER, 0, blockSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT));

    {
        // Query for the offsets of each block variable
        const GLchar *names[] = { "viewMatrix",
                                "projectionMatrix", "ambientColor",
                                "camPos", "numLights" };

        GLuint indices[5];
        glGetUniformIndices(m_CurrentShader, 5, names, indices);

        GLint offset[5];
        glGetActiveUniformsiv(m_CurrentShader, 5, indices, GL_UNIFORM_OFFSET, offset);

        // Set the view matrix in the vertex shader.
        memcpy(blockBuffer + offset[0], &context.m_viewMatrix, sizeof(Matrix4X4f));

        // Set the projection matrix in the vertex shader.
        memcpy(blockBuffer + offset[1], &context.m_projectionMatrix, sizeof(Matrix4X4f));

        // Set the ambient color
        memcpy(blockBuffer + offset[2], &context.m_ambientColor, sizeof(Vector3f));

        // Set the camPos
        memcpy(blockBuffer + offset[3], &context.m_camPos, sizeof(Vector3f));

        // Set number of lights
        GLint numLights = (GLint) context.m_lights.size();
        memcpy(blockBuffer + offset[4], &numLights, sizeof(GLint));
    }

    // Set lighting parameters for PS shader
    for (size_t i = 0; i < context.m_lights.size(); i++)
    {
        const int32_t num_of_properties = 0xA;
        char uniformNames[num_of_properties][256];

        sprintf(uniformNames[0x0], "allLights[%zd].lightPosition", i);
        sprintf(uniformNames[0x1], "allLights[%zd].lightColor", i);
        sprintf(uniformNames[0x2], "allLights[%zd].lightIntensity", i);
        sprintf(uniformNames[0x3], "allLights[%zd].lightDirection", i);
        sprintf(uniformNames[0x4], "allLights[%zd].lightSize", i);
        sprintf(uniformNames[0x5], "allLights[%zd].lightDistAttenCurveParams", i);
        sprintf(uniformNames[0x6], "allLights[%zd].lightAngleAttenCurveParams", i);
        sprintf(uniformNames[0x7], "allLights[%zd].lightShadowMapIndex", i);
        sprintf(uniformNames[0x8], "allLights[%zd].lightVP", i);
        sprintf(uniformNames[0x9], "allLights[%zd].lightType", i);

        const char* names[num_of_properties] = {
            uniformNames[0x0], uniformNames[0x1], uniformNames[0x2], uniformNames[0x3],
            uniformNames[0x4], uniformNames[0x5], uniformNames[0x6], uniformNames[0x7],
            uniformNames[0x8], uniformNames[0x9]
        };

        GLuint indices[num_of_properties];
        glGetUniformIndices(m_CurrentShader, num_of_properties, names, indices);

        GLint offset[num_of_properties];
        glGetActiveUniformsiv(m_CurrentShader, num_of_properties, indices, GL_UNIFORM_OFFSET, offset);

        memcpy(blockBuffer + offset[0x0], &context.m_lights[i].m_lightPosition, sizeof(Vector4f));
        memcpy(blockBuffer + offset[0x1], &context.m_lights[i].m_lightColor, sizeof(Vector4f));
        memcpy(blockBuffer + offset[0x2], &context.m_lights[i].m_lightIntensity, sizeof(float));
        memcpy(blockBuffer + offset[0x3], &context.m_lights[i].m_lightDirection, sizeof(Vector4f));
        memcpy(blockBuffer + offset[0x4], &context.m_lights[i].m_lightSize, sizeof(Vector2f));
        memcpy(blockBuffer + offset[0x5], &context.m_lights[i].m_lightDistAttenCurveType, sizeof(int32_t));
        memcpy(blockBuffer + offset[0x5] + sizeof(int32_t), &context.m_lights[i].m_lightDistAttenCurveParams[0], sizeof(float) * 5);
        memcpy(blockBuffer + offset[0x6], &context.m_lights[i].m_lightAngleAttenCurveType, sizeof(int32_t));
        memcpy(blockBuffer + offset[0x6] + sizeof(int32_t), &context.m_lights[i].m_lightAngleAttenCurveParams[0], sizeof(float)* 5);
        memcpy(blockBuffer + offset[0x7], &context.m_lights[i].m_lightShadowMapIndex, sizeof(int32_t));
        memcpy(blockBuffer + offset[0x8], &context.m_lights[i].m_lightVP, sizeof(Matrix4X4f));
        memcpy(blockBuffer + offset[0x9], &context.m_lights[i].m_lightType, sizeof(LightType));
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);

    glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, m_UboBuffer);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const Matrix4X4f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniformMatrix4fv(location, 1, false, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const Matrix4X4f* param, const int32_t count)
{
    bool result = true;
    char uniformName[256];

    for (int32_t i = 0; i < count; i++)
    {
        sprintf(uniformName, "%s[%d]", paramName, i);
        result &= SetShaderParameter(uniformName, *(param + i));
    }

    return result;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const Vector2f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform2fv(location, 1, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const Vector3f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform3fv(location, 1, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const Vector4f& param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform4fv(location, 1, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const float param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1f(location, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const int32_t param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1i(location, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const uint32_t param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1ui(location, param);

    return true;
}

bool OpenGLGraphicsManagerCommonBase::SetShaderParameter(const char* paramName, const bool param)
{
    unsigned int location;

    location = glGetUniformLocation(m_CurrentShader, paramName);
    if(location == -1)
    {
            return false;
    }
    glUniform1f(location, param);

    return true;
}

static void getOpenGLTextureFormat(const Image& img, GLenum& format, GLenum& internal_format, GLenum& type)
{
    if(img.bitcount == 8)
    {
        format = GL_RED;
        internal_format = GL_R8;
        type = GL_UNSIGNED_BYTE;
    }
    else if(img.bitcount == 16)
    {
        format = GL_RED;
#ifndef OPENGL_ES
        internal_format = GL_R16;
#else
        internal_format = GL_RED;
#endif
        type = GL_UNSIGNED_SHORT;
    }
    else if(img.bitcount == 24)
    {
        format = GL_RGB;
        internal_format = GL_RGB8;
        type = GL_UNSIGNED_BYTE;
    }
    else
    {
        format = GL_RGBA;
        internal_format = GL_RGBA8;
        type = GL_UNSIGNED_BYTE;
    }
}

void OpenGLGraphicsManagerCommonBase::InitializeBuffers(const Scene& scene)
{
    GraphicsManager::InitializeBuffers(scene);

    // Geometries
    for (auto _it : scene.GeometryNodes)
    {
        auto pGeometryNode = _it.second.lock();
        if (pGeometryNode && pGeometryNode->Visible()) 
        {
            auto pGeometry = scene.GetGeometry(pGeometryNode->GetSceneObjectRef());
            assert(pGeometry);
            auto pMesh = pGeometry->GetMesh().lock();
            if (!pMesh) continue;

            // Set the number of vertex properties.
            auto vertexPropertiesCount = pMesh->GetVertexPropertiesCount();

            // Allocate an OpenGL vertex array object.
            GLuint vao;
            glGenVertexArrays(1, &vao);

            // Bind the vertex array object to store all the buffers and vertex attributes we create here.
            glBindVertexArray(vao);

            GLuint buffer_id;

            for (uint32_t i = 0; i < vertexPropertiesCount; i++)
            {
                const SceneObjectVertexArray& v_property_array = pMesh->GetVertexPropertyArray(i);
                auto v_property_array_data_size = v_property_array.GetDataSize();
                auto v_property_array_data = v_property_array.GetData();

                // Generate an ID for the vertex buffer.
                glGenBuffers(1, &buffer_id);

                // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
                glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
                glBufferData(GL_ARRAY_BUFFER, v_property_array_data_size, v_property_array_data, GL_STATIC_DRAW);

                glEnableVertexAttribArray(i);

                switch (v_property_array.GetDataType()) {
                    case VertexDataType::kVertexDataTypeFloat1:
                        glVertexAttribPointer(i, 1, GL_FLOAT, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeFloat2:
                        glVertexAttribPointer(i, 2, GL_FLOAT, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeFloat3:
                        glVertexAttribPointer(i, 3, GL_FLOAT, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeFloat4:
                        glVertexAttribPointer(i, 4, GL_FLOAT, false, 0, 0);
                        break;
#ifndef OPENGL_ES
                    case VertexDataType::kVertexDataTypeDouble1:
                        glVertexAttribPointer(i, 1, GL_DOUBLE, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeDouble2:
                        glVertexAttribPointer(i, 2, GL_DOUBLE, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeDouble3:
                        glVertexAttribPointer(i, 3, GL_DOUBLE, false, 0, 0);
                        break;
                    case VertexDataType::kVertexDataTypeDouble4:
                        glVertexAttribPointer(i, 4, GL_DOUBLE, false, 0, 0);
                        break;
#endif
                    default:
                        assert(0);
                }

                m_Buffers.push_back(buffer_id);
            }

            auto indexGroupCount = pMesh->GetIndexGroupCount();

            GLenum  mode;
            switch(pMesh->GetPrimitiveType())
            {
                case PrimitiveType::kPrimitiveTypePointList:
                    mode = GL_POINTS;
                    break;
                case PrimitiveType::kPrimitiveTypeLineList:
                    mode = GL_LINES;
                    break;
                case PrimitiveType::kPrimitiveTypeLineStrip:
                    mode = GL_LINE_STRIP;
                    break;
                case PrimitiveType::kPrimitiveTypeTriList:
                    mode = GL_TRIANGLES;
                    break;
                case PrimitiveType::kPrimitiveTypeTriStrip:
                    mode = GL_TRIANGLE_STRIP;
                    break;
                case PrimitiveType::kPrimitiveTypeTriFan:
                    mode = GL_TRIANGLE_FAN;
                    break;
                default:
                    // ignore
                    continue;
            }

            for (decltype(indexGroupCount) i = 0; i < indexGroupCount; i++)
            {
                // Generate an ID for the index buffer.
                glGenBuffers(1, &buffer_id);

                const SceneObjectIndexArray& index_array      = pMesh->GetIndexArray(i);
                auto index_array_size = index_array.GetDataSize();
                auto index_array_data = index_array.GetData();

                // Bind the index buffer and load the index data into it.
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_id);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_array_size, index_array_data, GL_STATIC_DRAW);

                // Set the number of indices in the index array.
                GLsizei indexCount = static_cast<GLsizei>(index_array.GetIndexCount());
                GLenum type;
                switch(index_array.GetIndexType())
                {
                    case IndexDataType::kIndexDataTypeInt8:
                        type = GL_UNSIGNED_BYTE;
                        break;
                    case IndexDataType::kIndexDataTypeInt16:
                        type = GL_UNSIGNED_SHORT;
                        break;
                    case IndexDataType::kIndexDataTypeInt32:
                        type = GL_UNSIGNED_INT;
                        break;
                    default:
                        // not supported by OpenGL
                        cerr << "Error: Unsupported Index Type " << index_array << endl;
                        cerr << "Mesh: " << *pMesh << endl;
                        cerr << "Geometry: " << *pGeometry << endl;
                        continue;
                }

                m_Buffers.push_back(buffer_id);

                size_t material_index = index_array.GetMaterialIndex();
                std::string material_key = pGeometryNode->GetMaterialRef(material_index);
                auto material = scene.GetMaterial(material_key);
                if (material) {
                    function<void(const string, const Image&)> upload_texture = [this](const string texture_key, const Image& texture) {
                        GLuint texture_id;
                        glGenTextures(1, &texture_id);
                        glBindTexture(GL_TEXTURE_2D, texture_id);
                        GLenum format, internal_format, type;
                        getOpenGLTextureFormat(texture, format, internal_format, type);
                        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture.Width, texture.Height, 
                            0, format, type, texture.data);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

                        glBindTexture(GL_TEXTURE_2D, 0);

                        m_TextureIndex[texture_key] = texture_id;
                        m_Textures.push_back(texture_id);
                    };

                    Image texture;
                    string texture_key;

                    // base color / albedo
                    auto color = material->GetBaseColor();
                    if (color.ValueMap) {
                        texture_key = color.ValueMap->GetName();
                        auto it = m_TextureIndex.find(texture_key);
                        if (it == m_TextureIndex.end()) {
                            texture = color.ValueMap->GetTextureImage();
                            upload_texture(texture_key, texture);
                        }
                    }

                    // normal
                    auto normal = material->GetNormal();
                    if (normal.ValueMap) {
                        texture_key = normal.ValueMap->GetName();
                        auto it = m_TextureIndex.find(texture_key);
                        if (it == m_TextureIndex.end()) {
                            texture = color.ValueMap->GetTextureImage();
                            upload_texture(texture_key, texture);
                        }
                    }

                    // metallic 
                    auto metallic = material->GetMetallic();
                    if (metallic.ValueMap) {
                        texture_key = metallic.ValueMap->GetName();
                        auto it = m_TextureIndex.find(texture_key);
                        if (it == m_TextureIndex.end()) {
                            texture = metallic.ValueMap->GetTextureImage();
                            upload_texture(texture_key, texture);
                        }
                    }

                    // roughness 
                    auto roughness = material->GetRoughness();
                    if (roughness.ValueMap) {
                        texture_key = roughness.ValueMap->GetName();
                        auto it = m_TextureIndex.find(texture_key);
                        if (it == m_TextureIndex.end()) {
                            texture = roughness.ValueMap->GetTextureImage();
                            upload_texture(texture_key, texture);
                        }
                    }

                    // ao
                    auto ao = material->GetAO();
                    if (ao.ValueMap) {
                        texture_key = ao.ValueMap->GetName();
                        auto it = m_TextureIndex.find(texture_key);
                        if (it == m_TextureIndex.end()) {
                            texture = ao.ValueMap->GetTextureImage();
                            upload_texture(texture_key, texture);
                        }
                    }
                }

                glBindVertexArray(0);

                auto dbc = make_shared<OpenGLDrawBatchContext>();
                dbc->vao     = vao;
                dbc->mode    = mode;
                dbc->type    = type;
                dbc->count   = indexCount;
                dbc->node    = pGeometryNode;
                dbc->material = material;
                m_Frames[m_nFrameIndex].batchContexts.push_back(dbc);
            }
        }
    }

    // SkyBox
    float skyboxVertices[] = {
         1.0f,  1.0f,  1.0f,  // 0
        -1.0f,  1.0f,  1.0f,  // 1
         1.0f, -1.0f,  1.0f,  // 2
         1.0f,  1.0f, -1.0f,  // 3
        -1.0f,  1.0f, -1.0f,  // 4
         1.0f, -1.0f, -1.0f,  // 5
        -1.0f, -1.0f,  1.0f,  // 6
        -1.0f, -1.0f, -1.0f   // 7
    };

    uint8_t skyboxIndices[] = {
        4, 7, 5,
        5, 3, 4,

        6, 7, 4,
        4, 1, 6,

        5, 2, 0,
        0, 3, 5,

        6, 1, 0,
        0, 2, 6,

        4, 3, 0,
        0, 1, 4,

        7, 6, 5,
        5, 6, 2
    };

    // load skybox, irradiance map and radiance map
    GLuint cubemapTexture;
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubemapTexture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAX_LEVEL, 8);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    assert(scene.SkyBox);

    // skybox, irradiance map
    for (uint32_t i = 0; i < 12; i++)
    {
        auto& texture = scene.SkyBox->GetTexture(i);
        auto& image = texture.GetTextureImage();
        GLenum format, internal_format, type;
        getOpenGLTextureFormat(image, format, internal_format, type);

        if (i == 0) // do this only once
        {
            const uint32_t faces = 6;
            const uint32_t indexies = 2;
            constexpr GLsizei depth = faces * indexies;
            glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 9, internal_format, image.Width, image.Height, depth);
        }

        GLint level = i / 6;
        GLint zoffset = i % 6;
        glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, level, 0, 0, zoffset, image.Width, image.Height, 1,
            format, type, image.data);
    }

    // radiance map
    for (uint32_t i = 12; i < 66; i++)
    {
        auto& texture = scene.SkyBox->GetTexture(i);
        auto& image = texture.GetTextureImage();
        GLenum format, internal_format, type;
        getOpenGLTextureFormat(image, format, internal_format, type);

        GLint level = (i - 12) / 6;
        GLint zoffset = (i % 6) + 6;
        glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, level, 0, 0, zoffset, image.Width, image.Height, 1,
            format, type, image.data);
    }

    m_Textures.push_back(cubemapTexture);
    m_Frames[m_nFrameIndex].frameContext.skybox = cubemapTexture;

    // skybox VAO
    GLuint skyboxVAO, skyboxVBO[2];
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(2, skyboxVBO);
    glBindVertexArray(skyboxVAO);
    // vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxVBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    
    m_Buffers.push_back(skyboxVBO[0]);
    m_Buffers.push_back(skyboxVBO[1]);

    m_SkyBoxDrawBatchContext.vao     = skyboxVAO;
    m_SkyBoxDrawBatchContext.mode    = GL_TRIANGLES;
    m_SkyBoxDrawBatchContext.type    = GL_UNSIGNED_BYTE;
    m_SkyBoxDrawBatchContext.count   = sizeof(skyboxIndices) / sizeof(skyboxIndices[0]);

    return;
}

void OpenGLGraphicsManagerCommonBase::ClearBuffers()
{
    for (int i = 0; i < kFrameCount; i++)
    {
        auto& batchContexts = m_Frames[i].batchContexts;

        for (auto dbc : batchContexts) {
            glDeleteVertexArrays(1, &dynamic_pointer_cast<OpenGLDrawBatchContext>(dbc)->vao);
        }

        batchContexts.clear();
    }

    if (m_SkyBoxDrawBatchContext.vao)
    {
        glDeleteVertexArrays(1, &m_SkyBoxDrawBatchContext.vao);
    }

    for (auto buf : m_Buffers) {
        glDeleteBuffers(1, &buf);
    }

    if (m_UboBuffer)
    {
        glDeleteBuffers(1, &m_UboBuffer);
    }
    
    for (auto texture : m_Textures) {
        glDeleteTextures(1, &texture);
    }

    m_Buffers.clear();
    m_Textures.clear();

}

void OpenGLGraphicsManagerCommonBase::UseShaderProgram(const intptr_t shaderProgram)
{
    m_CurrentShader = static_cast<GLuint>(shaderProgram);

    // Set the color shader as the current shader program and set the matrices that it will use for rendering.
    glUseProgram(m_CurrentShader);
}

void OpenGLGraphicsManagerCommonBase::SetPerFrameConstants(const DrawFrameContext& context)
{
    bool result = SetPerFrameShaderParameters(context);
    assert(result);
}

void OpenGLGraphicsManagerCommonBase::DrawBatch(const DrawBatchContext& context)
{
    const OpenGLDrawBatchContext& dbc = dynamic_cast<const OpenGLDrawBatchContext&>(context);

    bool result = SetShaderParameter("modelMatrix", dbc.trans);
    assert(result);

    result = SetShaderParameter("usingDiffuseMap", false);

    if (dbc.material) {
        Color color = dbc.material->GetBaseColor();
        if (color.ValueMap) 
        {
            result = SetShaderParameter("diffuseMap", 0);

            GLuint texture_id = m_TextureIndex[color.ValueMap->GetName()];
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            // set this to tell shader to use texture
            result = SetShaderParameter("usingDiffuseMap", true);
        }
        else
        {
            result = SetShaderParameter("diffuseColor", Vector3f({color.Value[0], color.Value[1], color.Value[2]}));
        }

        Normal normal = dbc.material->GetNormal();
        if (normal.ValueMap)
        {
            //result = SetShaderParameter("normalMap", 5);

            GLuint texture_id = m_TextureIndex[normal.ValueMap->GetName()];
            SetShaderParameter("normalMap", 5);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            // set this to tell shader to use texture
            result = SetShaderParameter("usingNormalMap", true);
        }

        color = dbc.material->GetSpecularColor();
        result = SetShaderParameter("specularColor", Vector3f({color.Value[0], color.Value[1], color.Value[2]}));

        Parameter param = dbc.material->GetSpecularPower();
        result = SetShaderParameter("specularPower", param.Value);

        // PBR
        param = dbc.material->GetMetallic();
        if (param.ValueMap)
        {
            GLuint texture_id = m_TextureIndex[param.ValueMap->GetName()];
            SetShaderParameter("metallicMap", 6);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            // set this to tell shader to use texture
            result = SetShaderParameter("usingMetallicMap", true);
        }
        else
        {
            result = SetShaderParameter("metallic", param.Value);
            result = SetShaderParameter("usingMetallicMap", false);
        }

        param = dbc.material->GetRoughness();
        if (param.ValueMap)
        {
            GLuint texture_id = m_TextureIndex[param.ValueMap->GetName()];
            SetShaderParameter("roughnessMap", 7);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            // set this to tell shader to use texture
            result = SetShaderParameter("usingRoughnessMap", true);
        }
        else
        {
            result = SetShaderParameter("roughness", param.Value);
            result = SetShaderParameter("usingRoughnessMap", false);
        }

        param = dbc.material->GetAO();
        if (param.ValueMap)
        {
            GLuint texture_id = m_TextureIndex[param.ValueMap->GetName()];
            SetShaderParameter("aoMap", 8);
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            // set this to tell shader to use texture
            result = SetShaderParameter("usingAoMap", true);
        }
        else
        {
            result = SetShaderParameter("ao", param.Value);
            result = SetShaderParameter("usingAoMap", false);
        }

        {
            GLuint texture_id = m_TextureIndex["BRDF_LUT"];
            SetShaderParameter("brdfLUT", 9);
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_2D, texture_id);
        }
    }

    glEnable(GL_CULL_FACE);

    glBindVertexArray(dbc.vao);

    glDrawElements(dbc.mode, dbc.count, dbc.type, 0x00);

    glBindVertexArray(0);
}

void OpenGLGraphicsManagerCommonBase::DrawBatchDepthOnly(const DrawBatchContext& context)
{
    const OpenGLDrawBatchContext& dbc = dynamic_cast<const OpenGLDrawBatchContext&>(context);

    bool result = SetShaderParameter("modelMatrix", dbc.trans);
    assert(result);

    glBindVertexArray(dbc.vao);

    glDrawElements(dbc.mode, dbc.count, dbc.type, 0x00);

    glBindVertexArray(0);
}

intptr_t OpenGLGraphicsManagerCommonBase::GenerateCubeShadowMapArray(const uint32_t width, const uint32_t height, const uint32_t count)
{
    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    GLuint shadowMap;

    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, shadowMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, count * 6);

    // register the shadow map
    return static_cast<intptr_t>(shadowMap);
}

intptr_t OpenGLGraphicsManagerCommonBase::GenerateShadowMapArray(const uint32_t width, const uint32_t height, const uint32_t count)
{
    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    GLuint shadowMap;

    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMap);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, width, height, count);

    // register the shadow map
    return static_cast<intptr_t>(shadowMap);
}

void OpenGLGraphicsManagerCommonBase::BeginShadowMap(const Light& light, const intptr_t shadowmap, const uint32_t width, const uint32_t height, const uint32_t layer_index)
{
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    glGenFramebuffers(1, &m_ShadowMapFramebufferName);

    glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowMapFramebufferName);

    if (light.m_lightType == LightType::Omni)
    {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint) shadowmap, 0);
    }
    else
    {
        // we only bind the single layer to FBO
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint) shadowmap, 0, layer_index);
    }

    // Always check that our framebuffer is ok
    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        assert(0);
    }

    glDrawBuffers(0, nullptr); // No color buffer is drawn to.
    glDepthMask(GL_TRUE);
    // make sure omni light shadowmap arrays get cleared only
    // once, because glClear will clear all cubemaps in the array
    if (light.m_lightType != LightType::Omni || layer_index == 0)
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    glViewport(0, 0, width, height);

    switch (light.m_lightType)
    {
        case LightType::Omni:
        {
            Matrix4X4f shadowMatrices[6];
            const Vector3f direction[6] = {
                { 1.0f, 0.0f, 0.0f },
                {-1.0f, 0.0f, 0.0f },
                { 0.0f, 1.0f, 0.0f },
                { 0.0f,-1.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f,-1.0f }
            };
            const Vector3f up[6] = {
                { 0.0f,-1.0f, 0.0f },
                { 0.0f,-1.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f,-1.0f },
                { 0.0f,-1.0f, 0.0f },
                { 0.0f,-1.0f, 0.0f }
            };

            float nearClipDistance = 0.1f;
            float farClipDistance = 10.0f;
            float fieldOfView = PI / 2.0f; // 90 degree for each cube map face
            float screenAspect = (float)width / (float)height;
            Matrix4X4f projection;

            // Build the perspective projection matrix.
            BuildPerspectiveFovRHMatrix(projection, fieldOfView, screenAspect, nearClipDistance, farClipDistance);

            Vector3f pos = {light.m_lightPosition[0], light.m_lightPosition[1], light.m_lightPosition[2]};
            for (int32_t i = 0; i < 6; i++)
            {
                BuildViewRHMatrix(shadowMatrices[i], pos, pos + direction[i], up[i]);
                shadowMatrices[i] = shadowMatrices[i] * projection;
            }

            SetShaderParameter("shadowMatrices", shadowMatrices, 6);
            SetShaderParameter("lightPos", pos);
            SetShaderParameter("layer_index", static_cast<int>(layer_index));
            SetShaderParameter("far_plane", farClipDistance);

            break;
        }
        default:
        {
            SetShaderParameter("depthVP", light.m_lightVP);
        }
    }

    glCullFace(GL_FRONT);
}

void OpenGLGraphicsManagerCommonBase::EndShadowMap(const intptr_t shadowmap, uint32_t layer_index)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &m_ShadowMapFramebufferName);

    const GfxConfiguration& conf = g_pApp->GetConfiguration();
    glViewport(0, 0, conf.screenWidth, conf.screenHeight);

    glCullFace(GL_BACK);
}

void OpenGLGraphicsManagerCommonBase::SetShadowMaps(const Frame& frame)
{
    GLuint texture_id = (GLuint) frame.frameContext.shadowMap;
    SetShaderParameter("shadowMap", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, color);	

    texture_id = (GLuint) frame.frameContext.globalShadowMap;
    SetShaderParameter("globalShadowMap", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, color);	

    texture_id = (GLuint) frame.frameContext.cubeShadowMap;
    SetShaderParameter("cubeShadowMap", 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void OpenGLGraphicsManagerCommonBase::DestroyShadowMap(intptr_t& shadowmap)
{
    GLuint id = (GLuint) shadowmap;
    glDeleteTextures(1, &id);
    shadowmap = -1;
}

void OpenGLGraphicsManagerCommonBase::SetSkyBox(const DrawFrameContext& context)
{
    // skybox
    GLuint cubemapTexture = (GLuint) context.skybox;
    SetShaderParameter("skybox", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubemapTexture);
}

void OpenGLGraphicsManagerCommonBase::DrawSkyBox()
{
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    glBindVertexArray(m_SkyBoxDrawBatchContext.vao);

    glDrawElements(m_SkyBoxDrawBatchContext.mode, m_SkyBoxDrawBatchContext.count, m_SkyBoxDrawBatchContext.type, 0x00);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}

intptr_t OpenGLGraphicsManagerCommonBase::GenerateAndBindTexture(const char* id, const uint32_t width, const uint32_t height)
{
    GLuint tex_output;
    glGenTextures(1, &tex_output);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_FLOAT, NULL);
    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);
    m_TextureIndex[id] = tex_output;
    m_Textures.push_back(tex_output);
    return static_cast<intptr_t>(tex_output);
}

void OpenGLGraphicsManagerCommonBase::Dispatch(const uint32_t width, const uint32_t height, const uint32_t depth)
{
    glDispatchCompute((GLuint)width, (GLuint)height, (GLuint)depth);
    // make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

intptr_t OpenGLGraphicsManagerCommonBase::GetTexture(const char* id)
{
    assert(id);
    auto it = m_TextureIndex.find(id);
    if (it != m_TextureIndex.end())
    {
        return static_cast<intptr_t>(it->second);
    }

    return 0;
}

#ifdef DEBUG

void OpenGLGraphicsManagerCommonBase::DrawPoint(const Point &point, const Vector3f& color)
{
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point), point, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_POINTS;
    dbc.count   = 1;
    dbc.color   = color;
    BuildIdentityMatrix(dbc.trans);

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::DrawPoints(const Point* buffer, const size_t count, const Matrix4X4f& trans, const Vector3f& color)
{
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * count, buffer, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_POINTS;
    dbc.count   = static_cast<GLsizei>(count);
    dbc.color   = color;
    dbc.trans   = trans;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::DrawPointSet(const PointSet& point_set, const Vector3f& color)
{
    Matrix4X4f trans;
    BuildIdentityMatrix(trans);

    DrawPointSet(point_set, trans, color);
}

void OpenGLGraphicsManagerCommonBase::DrawPointSet(const PointSet& point_set, const Matrix4X4f& trans, const Vector3f& color)
{
    auto count = point_set.size();
    Point* buffer = new Point[count];
    int i = 0;
    for(auto point_ptr : point_set)
    {
        buffer[i++] = *point_ptr;
    }

    DrawPoints(buffer, count, trans, color);

    delete[] buffer;
}

void OpenGLGraphicsManagerCommonBase::DrawLine(const PointList& vertices, const Matrix4X4f& trans, const Vector3f& color)
{
    auto count = vertices.size();
    GLfloat* _vertices = new GLfloat[3 * count];

    for (auto i = 0; i < count; i++)
    {
        _vertices[3 * i] = vertices[i]->data[0];
        _vertices[3 * i + 1] = vertices[i]->data[1];
        _vertices[3 * i + 2] = vertices[i]->data[2];
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * count, _vertices, GL_STATIC_DRAW);

    delete[] _vertices;

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_LINES;
    dbc.count   = static_cast<GLsizei>(count);
    dbc.color   = color;
    dbc.trans   = trans;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::DrawLine(const PointList& vertices, const Vector3f& color)
{
    Matrix4X4f trans;
    BuildIdentityMatrix(trans);

    DrawLine(vertices, trans, color);
}

void OpenGLGraphicsManagerCommonBase::DrawLine(const Point& from, const Point& to, const Vector3f& color)
{
    PointList point_list;
    point_list.push_back(make_shared<Point>(from));
    point_list.push_back(make_shared<Point>(to));

    DrawLine(point_list, color);
}

void OpenGLGraphicsManagerCommonBase::DrawTriangle(const PointList& vertices, const Vector3f& color)
{
    Matrix4X4f trans;
    BuildIdentityMatrix(trans);

    DrawTriangle(vertices, trans, color);
}

void OpenGLGraphicsManagerCommonBase::DrawTriangle(const PointList& vertices, const Matrix4X4f& trans, const Vector3f& color)
{
    auto count = vertices.size();
    assert(count >= 3);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    Vector3f* data = new Vector3f[count];
    for(auto i = 0; i < count; i++)
    {
        data[i] = *vertices[i];
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3f) * count, data, GL_STATIC_DRAW);
    delete[] data;

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_TRIANGLES;
    dbc.count   = static_cast<GLsizei>(vertices.size());
    dbc.color   = color;
    dbc.trans   = trans;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::DrawTriangleStrip(const PointList& vertices, const Vector3f& color)
{
    auto count = vertices.size();
    assert(count >= 3);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id;

    // Generate an ID for the vertex buffer.
    glGenBuffers(1, &buffer_id);

    // Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    Vector3f* data = new Vector3f[count];
    for(auto i = 0; i < count; i++)
    {
        data[i] = *vertices[i];
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3f) * count, data, GL_STATIC_DRAW);
    delete[] data;

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    m_DebugBuffers.push_back(buffer_id);

    DebugDrawBatchContext& dbc = *(new DebugDrawBatchContext);
    dbc.vao     = vao;
    dbc.mode    = GL_TRIANGLE_STRIP;
    dbc.count   = static_cast<GLsizei>(vertices.size());
    dbc.color   = color * 0.5f;

    m_DebugDrawBatchContext.push_back(std::move(dbc));
}

void OpenGLGraphicsManagerCommonBase::ClearDebugBuffers()
{
    for (auto dbc : m_DebugDrawBatchContext) {
        glDeleteVertexArrays(1, &dbc.vao);
    }

    m_DebugDrawBatchContext.clear();

    for (auto buf : m_DebugBuffers) {
        glDeleteBuffers(1, &buf);
    }

    m_DebugBuffers.clear();
}

void OpenGLGraphicsManagerCommonBase::RenderDebugBuffers()
{
    auto debugShaderProgram = g_pShaderManager->GetDefaultShaderProgram(DefaultShaderIndex::Debug);

    // Set the color shader as the current shader program and set the matrices that it will use for rendering.
    UseShaderProgram(debugShaderProgram);

    SetPerFrameShaderParameters(m_Frames[m_nFrameIndex].frameContext);

    for (auto dbc : m_DebugDrawBatchContext)
    {
        SetShaderParameter("FrontColor", dbc.color);
        SetShaderParameter("modelMatrix", dbc.trans);

        glBindVertexArray(dbc.vao);
        glDrawArrays(dbc.mode, 0x00, dbc.count);
    }
}

void OpenGLGraphicsManagerCommonBase::DrawTextureOverlay(const intptr_t texture, float vp_left, float vp_top, float vp_width, float vp_height)
{
    GLuint texture_id = (GLuint) texture;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    GLfloat vertices[] = {
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - vp_height, 0.0f,
        vp_left + vp_width, vp_top, 0.0f,
        vp_left + vp_width, vp_top - vp_height, 0.0f
    };

    GLfloat uv[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uv) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0x00, 4);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

void OpenGLGraphicsManagerCommonBase::DrawTextureArrayOverlay(const intptr_t texture, uint32_t layer_index, float vp_left, float vp_top, float vp_width, float vp_height)
{
    GLuint texture_id = (GLuint) texture;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
    bool result = SetShaderParameter("layer_index", (float) layer_index);
    assert(result);

    GLfloat vertices[] = {
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - vp_height, 0.0f,
        vp_left + vp_width, vp_top, 0.0f,
        vp_left + vp_width, vp_top - vp_height, 0.0f
    };

    GLfloat uv[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uv) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0x00, 4);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

void OpenGLGraphicsManagerCommonBase::DrawCubeMapOverlay(const intptr_t cubemap, float vp_left, float vp_top, float vp_width, float vp_height, float level)
{
    GLuint texture_id = (GLuint) cubemap;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    bool result = SetShaderParameter("level", level);
    assert(result);

    const float cell_height = vp_height * 0.5f;
    const float cell_width = vp_width * (1.0f / 3.0f);
    GLfloat vertices[] = {
        // face 1
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top, 0.0f,

        vp_left + cell_width, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        // face 2
        vp_left + cell_width, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top, 0.0f,

        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        // face 3
        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top, 0.0f,

        vp_left + cell_width * 3.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        // face 4
        vp_left, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,

        // face 5
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,

        // face 6
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height * 2.0f, 0.0f,
    };

    const GLfloat uvw[] = {
         // back
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        
        // left
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // front
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        // right
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,

        // top
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,

        // bottom
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uvw) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvw), uvw, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0x00, 36);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

void OpenGLGraphicsManagerCommonBase::DrawCubeMapArrayOverlay(const intptr_t cubemap, uint32_t layer_index, float vp_left, float vp_top, float vp_width, float vp_height, float level)
{
    GLuint texture_id = (GLuint) cubemap;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, texture_id);
    bool result = SetShaderParameter("layer_index", (float) layer_index);
    assert(result);

    result = SetShaderParameter("level", level);

    const float cell_height = vp_height * 0.5f;
    const float cell_width = vp_width * (1.0f / 3.0f);
    GLfloat vertices[] = {
        // face 1
        vp_left, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top, 0.0f,

        vp_left + cell_width, vp_top, 0.0f,
        vp_left, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        // face 2
        vp_left + cell_width, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top, 0.0f,

        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        // face 3
        vp_left + cell_width * 2.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top, 0.0f,

        vp_left + cell_width * 3.0f, vp_top, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        // face 4
        vp_left, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height, 0.0f,

        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,

        // face 5
        vp_left + cell_width, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,

        // face 6
        vp_left + cell_width * 2.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,

        vp_left + cell_width * 3.0f, vp_top - cell_height, 0.0f,
        vp_left + cell_width * 2.0f, vp_top - cell_height * 2.0f, 0.0f,
        vp_left + cell_width * 3.0f, vp_top - cell_height * 2.0f, 0.0f,
    };

    const GLfloat uvw[] = {
         // back
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        
        // left
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // front
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        // right
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,

        // top
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,

        // bottom
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Bind the vertex array object to store all the buffers and vertex attributes we create here.
    glBindVertexArray(vao);

    GLuint buffer_id[2];

    // Generate an ID for the vertex buffer.
    glGenBuffers(2, buffer_id);

    // Bind the vertex buffer and load the vertex (position) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

    // Bind the vertex buffer and load the vertex (uvw) data into the vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvw), uvw, GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0x00, 36);

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, buffer_id);
}

#endif
