#pragma once
#include "../Common/d3dUtil.h"

#include "../Include/assimp/Importer.hpp"
#include "../Include/assimp/scene.h"
#include "../Include/assimp/postprocess.h"


struct ModelMaterialData
{
	string materialname;
	string meshname;
	wstring diffuse;
	wstring normal;
};

class ModelLoader
{
public:
	ModelLoader() {};
	~ModelLoader() {};
	void Load(const std::string& filename);
	std::vector<std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::vector<ModelMaterialData> mMaterialData;

	string mDirectory;
private:
	void processNode(aiNode* node, const aiScene* scene);
	std::unique_ptr<MeshGeometry> processMesh(aiMesh* mesh, const aiScene* scene);

	wstring loadMaterialTextures(aiMaterial* mat, aiTextureType type, const aiScene* scene);

};

