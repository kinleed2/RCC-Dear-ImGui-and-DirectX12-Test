#include "FBXLoader.h"
#include "../SystemTable.h"

void FBXLoader::Load(const std::string& filename)
{
   
    FbxManager* manager = FbxManager::Create();

    manager->SetIOSettings(FbxIOSettings::Create(manager, IOSROOT));

    FbxImporter* importer = FbxImporter::Create(manager, "");

    bool importStatus = false;

    importStatus = importer->Initialize(filename.c_str(), -1, manager->GetIOSettings());
    _ASSERT_EXPR(importStatus, importer->GetStatus().GetErrorString());

    FbxScene* scene = FbxScene::Create(manager, "");

    importStatus = importer->Import(scene);
    _ASSERT_EXPR(importStatus, importer->GetStatus().GetErrorString());

    fbxsdk::FbxGeometryConverter gemoetryConverter(manager);
    gemoetryConverter.Triangulate(scene, /*replace*/true);

    std::vector <FbxNode*> fetchedMeshes;
    std::vector <FbxNode*> boneNodes;
    std::function<void(FbxNode*)> traverse = [&](FbxNode* node)
    {
        if (node)
        {
            FbxNodeAttribute* fbx_node_attribute = node->GetNodeAttribute();

            if (fbx_node_attribute)
            {
                switch (fbx_node_attribute->GetAttributeType())
                {
                case FbxNodeAttribute::eMesh:
                    fetchedMeshes.push_back(node);
                    break;
                case FbxNodeAttribute::eSkeleton:
                    boneNodes.push_back(node);

                    break;
                }
            }

            for (int i = 0; i < node->GetChildCount(); i++)
            {
                traverse(node->GetChild(i));
            }
        }

    };
    traverse(scene->GetRootNode());

    if (fetchedMeshes.size() > 0)
    {
        mGeometries.resize(fetchedMeshes.size());
        for (int index_of_mesh = 0; index_of_mesh < fetchedMeshes.size(); index_of_mesh++)
        {
            FbxMesh* fbxMesh = fetchedMeshes.at(index_of_mesh)->GetMesh();
            auto mesh = make_unique<MeshGeometry>();
            mesh->Name = fbxMesh->GetName();
            const int numberOfMaterials = fbxMesh->GetNode()->GetMaterialCount();
            mMaterialData.resize(numberOfMaterials);

            //load bone_node influence per mesh
            //std::vector<bone_influences_per_control_point> bone_influences;
            //Fetch_bone_influences(fbxMesh, bone_influences);

            FbxTime::EMode time_mode = fbxMesh->GetScene()->GetGlobalSettings().GetTimeMode();
            FbxTime frame_time;
            frame_time.SetTime(0, 0, 0, 1, 0, time_mode);

            //Fetch_bone_matrices(fbxMesh);

            FbxAMatrix global_transform = fbxMesh->GetNode()->EvaluateGlobalTransform(0);

            for (int row = 0; row < 4; row++)
            {
                for (int column = 0; column < 4; column++)
                {
                    m_global_transform.m[row][column] = static_cast<float>(global_transform[row][column]);
                }
            }

            for (int index_of_material = 0; index_of_material < numberOfMaterials; index_of_material++)
            {
                const FbxSurfaceMaterial* surface_material = fbxMesh->GetNode()->GetMaterial(index_of_material);
                //load diffuseMap
                const FbxProperty diffuseMap = surface_material->FindProperty(FbxSurfaceMaterial::sDiffuse);
                const FbxProperty factor = surface_material->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
                FBXMaterialData& mat = mMaterialData.at(index_of_material);

                mat.materialname = surface_material->GetName();

                //if (diffuseMap.IsValid() && factor.IsValid())
                //{
                //    FbxDouble3 color = diffuseMap.Get<FbxDouble3>();
                //    double f = factor.Get<FbxDouble>();
                //    subset.material.DiffuseAlbedo.x = static_cast<float>(color[0] * f);
                //    subset.material.DiffuseAlbedo.y = static_cast<float>(color[1] * f);
                //    subset.material.DiffuseAlbedo.z = static_cast<float>(color[2] * f);
                //    subset.material.DiffuseAlbedo.w = 1.0f;
                //}

                //load normalMap
                if (diffuseMap.IsValid())
                {
                    const int number_of_textures = diffuseMap.GetSrcObjectCount<FbxFileTexture>();
                    if (number_of_textures)
                    {
                        const FbxFileTexture* file_texture = diffuseMap.GetSrcObject<FbxFileTexture>();
                        if (file_texture)
                        {
                            mat.diffuse = file_texture->GetRelativeFileName();
                        }
                    }
                }
                const FbxProperty normalMap = surface_material->FindProperty(FbxSurfaceMaterial::sNormalMap);
                if (normalMap.IsValid())
                {
                    const int number_of_textures = normalMap.GetSrcObjectCount<FbxFileTexture>();
                    if (number_of_textures)
                    {
                        const FbxFileTexture* file_texture = normalMap.GetSrcObject<FbxFileTexture>();
                        if (file_texture)
                        {
                            mat.normal = file_texture->GetRelativeFileName();
                        }
                    }
                }
                //const FbxProperty specularMap = surface_material->FindProperty(FbxSurfaceMaterial::sSpecular);
                //if (specularMap.IsValid())
                //{
                //    const int number_of_textures = specularMap.GetSrcObjectCount<FbxFileTexture>();
                //    if (number_of_textures)
                //    {
                //        const FbxFileTexture* file_texture = specularMap.GetSrcObject<FbxFileTexture>();
                //        if (file_texture)
                //        {
                //            subset.material.SpecularName = file_texture->GetRelativeFileName();
                //        }
                //    }
                //}
            }
            // Count the polygon count of each material
            if (numberOfMaterials > 0)
            {
                // Count the faces of each material
                const int number_of_polygons = fbxMesh->GetPolygonCount();
                for (int index_of_polygon = 0; index_of_polygon < number_of_polygons; ++index_of_polygon)
                {
                    const u_int material_index = fbxMesh->GetElementMaterial()->GetIndexArray().GetAt(index_of_polygon);
                    mesh->DrawArgs[fbxMesh->GetNode()->GetMaterial(material_index)->GetName()].IndexCount += 3;
                }
                // Record the offset (how many vertex)
                int offset = 0;
                for (auto & subset : mesh->DrawArgs)
                {
                    SubmeshGeometry &e = subset.second;
                    e.StartIndexLocation = offset;
                    offset += e.IndexCount;
                    // This will be used as counter in the following procedures, reset to zero
                    e.IndexCount = 0;
                }
            }

            std::vector<VertexPositionNormalTexture> vertices;
            std::vector<uint16_t> indices;
            u_int vertex_count = 0;

            //Tangent
            FbxGeometryElementTangent* element = fbxMesh->CreateElementTangent();
            //  保存形式の取得
            FbxGeometryElement::EMappingMode mapmode = element->GetMappingMode();
            FbxGeometryElement::EReferenceMode refmode = element->GetReferenceMode();

            const FbxVector4* array_of_control_points = fbxMesh->GetControlPoints();
            const int number_of_polygons = fbxMesh->GetPolygonCount();
            indices.resize(number_of_polygons * 3);
            for (int index_of_polygon = 0; index_of_polygon < number_of_polygons; index_of_polygon++)
            {

                // The material for current face.
                int index_of_material = 0;
                if (numberOfMaterials > 0)
                {
                    index_of_material = fbxMesh->GetElementMaterial()->GetIndexArray().GetAt(index_of_polygon);
                }
                // Where should I save the vertex attribute index, according to the material
                SubmeshGeometry& subset =
                    mesh->DrawArgs[fbxMesh->GetNode()->GetMaterial(index_of_material)->GetName()];
                const int index_offset = subset.StartIndexLocation + subset.IndexCount;

                for (int index_of_vertex = 0; index_of_vertex < 3; index_of_vertex++)
                {
                    VertexPositionNormalTexture vertex;
                    const int index_of_control_point = fbxMesh->GetPolygonVertex(index_of_polygon, index_of_vertex);
                    vertex.position.x = static_cast<float>(array_of_control_points[index_of_control_point][0]);
                    vertex.position.y = static_cast<float>(array_of_control_points[index_of_control_point][1]);
                    vertex.position.z = static_cast<float>(array_of_control_points[index_of_control_point][2]);

                    FbxVector4 normal;
                    fbxMesh->GetPolygonVertexNormal(index_of_polygon, index_of_vertex, normal);
                    vertex.normal.x = static_cast<float>(normal[0]);
                    vertex.normal.y = static_cast<float>(normal[1]);
                    vertex.normal.z = static_cast<float>(normal[2]);

                    fbxsdk::FbxStringList uv_names;
                    fbxMesh->GetUVSetNames(uv_names);

                    if (uv_names.GetCount() > 0)
                    {
                        FbxVector2 uv;
                        bool unmapped_uv;
                        fbxMesh->GetPolygonVertexUV(index_of_polygon, index_of_vertex, uv_names[0], uv, unmapped_uv);
                        vertex.textureCoordinate.x = static_cast<float>(uv[0]);
                        vertex.textureCoordinate.y = 1.0f - static_cast<float>(uv[1]);
                    }
                    ////    ポリゴン頂点に対するインデックス参照形式のみ対応
                    //if (mapmode == FbxGeometryElement::eByPolygonVertex)
                    //{
                    //    if (refmode == FbxGeometryElement::eIndexToDirect)
                    //    {
                    //        FbxLayerElementArrayTemplate<int>* index = &element->GetIndexArray();
                    //        // FbxColor取得
                    //        FbxVector4 v = element->GetDirectArray().GetAt(index->GetAt(index_of_control_point));
                    //        // DWORD型のカラー作成        
                    //        vertex.TangentU.x = (float)v[0];
                    //        vertex.TangentU.y = (float)v[1];
                    //        vertex.TangentU.z = (float)v[2];
                    //    }
                    //}
                    //else
                    //{
                    //    vertex.TangentU.x = 0;
                    //    vertex.TangentU.y = 0;
                    //    vertex.TangentU.z = 0;
                    //}

                    //bone_influences_per_control_point influences_per_control_point = bone_influences.at(index_of_control_point);

                    //for (size_t bone_index = 0; bone_index < influences_per_control_point.size(); ++bone_index)
                    //{

                    //    if (bone_index < MAX_BONE_INFLUENCES)
                    //    {
                    //        vertex.BoneIndices[bone_index] = influences_per_control_point.at(bone_index).index;
                    //        vertex.BoneWeights[bone_index] = influences_per_control_point.at(bone_index).weight;
                    //    }
                    //}

                    vertices.push_back(vertex);

                    indices.at(static_cast<std::vector<uint16_t, std::allocator<uint16_t>>::size_type>(index_offset) + index_of_vertex) = static_cast<uint16_t>(vertex_count);

                    vertex_count += 1;
                }
                subset.IndexCount += 3;
            }

            mesh->Set(g_pSys->pDeviceResources.get(),
                mesh->Name,
                vertices, indices);
            mGeometries.push_back(std::move(mesh));
        }
        
    }
    //if (boneNodes.size() > 0)
    //{
    //    scene->SetName(_filename.c_str());
    //    Fetch_bone_animations(boneNodes, extra_animations);
    //}
    manager->Destroy();
}
