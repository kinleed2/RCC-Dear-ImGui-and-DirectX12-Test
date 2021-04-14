#include "ModelLoader.h"
#include "../SystemTable.h"

void ModelLoader::Load(const std::string& filename)
{
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filename,
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded);
	
	this->mDirectory = filename.substr(0, filename.find_last_of('/'));

	processNode(pScene->mRootNode, pScene);
}

void ModelLoader::processNode(aiNode* node, const aiScene* scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		mGeometries.push_back(std::move(this->processMesh(mesh, scene)));
	}

	for (UINT i = 0; i < node->mNumChildren; i++) 
	{
		this->processNode(node->mChildren[i], scene);
	}
}

std::unique_ptr<MeshGeometry> ModelLoader::processMesh(aiMesh* mesh, const aiScene* scene)
{
	// Data to fill
	vector<VertexPositionNormalTexture> vertices;
	vector<uint16_t> indices;
	vector<Texture> textures;	

	
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
		ModelMaterialData matData;
		matData.materialname = mat->GetName().C_Str();
		matData.meshname = mesh->mName.C_Str();
		matData.diffuse = loadMaterialTextures(mat, aiTextureType_DIFFUSE, scene);
		matData.normal = loadMaterialTextures(mat, aiTextureType_NORMALS, scene);
		mMaterialData.push_back(matData);

	}

	// Walk through each of the mesh's vertices
	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		VertexPositionNormalTexture vertex;

		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		if (mesh->HasNormals())
		{
			vertex.normal.x = mesh->mNormals[i].x;
			vertex.normal.y = mesh->mNormals[i].y;
			vertex.normal.z = mesh->mNormals[i].z;
		}


		if (mesh->mTextureCoords[0])
		{
			vertex.textureCoordinate.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.textureCoordinate.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	auto meshGeometry = make_unique<MeshGeometry>();

	meshGeometry->Set(g_pSys->pDeviceResources.get(), mesh->mName.C_Str(), vertices, indices);

	return meshGeometry;
}
wstring ModelLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const aiScene* scene)
{
	if (mat->GetTextureCount(type) > 1)
	{
		MessageBoxW(g_pSys->pDeviceResources->GetWindow(), L"texture more than 1", L"warning", MB_OK);
	}
	else
	{
		aiString str;
		mat->GetTexture(type, 0, &str);
		string filename = string(str.C_Str());
		filename = filename.substr(filename.find_last_of("/\\") + 1);
		filename = mDirectory + '/' + filename;
		wstring filenamews = wstring(filename.begin(), filename.end());
		return filenamews;
	}
	
}