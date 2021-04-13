#pragma once
#include "../Common/d3dUtil.h"

#include "Importer.hpp"


struct ModelMaterialData
{
	string materialname;
	string meshname;
	string diffuse;
	string normal;
};

class ModelLoader
{
public:
	ModelLoader();
	~ModelLoader();
	void Load(const std::string& filename);
	std::vector<std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::vector<ModelMaterialData> mMaterialData;

private:

};

