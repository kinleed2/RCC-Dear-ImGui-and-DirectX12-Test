#include "PMDLoader.h"

void PMDLoader::Load(const std::string& filename)
{
	ifstream infile;
	infile.open(filename, ios::in | ios::binary);

	if (!infile.is_open())
	{
		return;
	}


	PMDFile m_PMDData;

	infile >> m_PMDData.m_header.m_magic;
	infile >> m_PMDData.m_header.m_version;
	infile >> m_PMDData.m_header.m_modelName;
	infile >> m_PMDData.m_header.m_comment;
	
	



}
