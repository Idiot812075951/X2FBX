#include "ueXReader.h"

#include <map>
#include <list>
#include <set>
#include<iostream>

#include "D3DX9Mesh.h"
#include <algorithm>

#include <fbxsdk.h>
#include "Common.h"
#include "MyKFbxMesh.h"
#include "Common.cxx"


FbxManager* gSdkManager = NULL;
#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(gSdkManager->GetIOSettings()))
#endif


static const char* gDiffuseElementName = "DiffuseUV";

static const char* gAmbientElementName = "AmbientUV";

static const char* gEmissiveElementName = "EmissiveUV";

char* CutPostfix(char* Target)
{
	int len = strlen(Target);
	int DotPosition = 0;
	for (int i = 0; i < len; i++)
	{
		if (Target[i] == '.')
			DotPosition = i;
	}
	if (DotPosition > 0)
	{
		Target[DotPosition] = 0;
	}
	return Target;
}


bool getXInfo(const std::string& lxoFilename, HWND hwnd, std::string ExportPath)
{

	//D3DCOLOR myColor = D3DCOLOR_XRGB(0, 0, 255);//绿色
	//D3DCOLOR myColor = D3DCOLOR_XRGB(0, 0, 255);//绿色

	IDirect3D9* g_d3d;
	IDirect3DDevice9* g_device;
	ID3DXMesh* g_mesh;
	ID3DXBuffer* material_buffer;

	unsigned long g_num_materials;
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_d3d == NULL)
	{
		return false;
	}

	g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_device);
	if (FAILED(D3DXLoadMeshFromXA(lxoFilename.data(), D3DXMESH_MANAGED, g_device, NULL, &material_buffer, NULL, &g_num_materials, &g_mesh)))
	{
		return false;
	}

	ID3DXMesh* g_mesh_temp;
	unsigned long oldFVF;
	PDWORD rgdwAdjacency;
	oldFVF = g_mesh->GetFVF();
	if (oldFVF != (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1))
	{
		g_mesh->CloneMeshFVF(g_mesh->GetOptions(), D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1, g_device, &g_mesh_temp);
		g_mesh->Release();
		g_mesh = g_mesh_temp;
	}
	if ((oldFVF && D3DFVF_NORMAL) == 0)
		D3DXComputeNormals(g_mesh, NULL);
	rgdwAdjacency = (PDWORD)malloc(sizeof(unsigned long) * g_mesh->GetNumFaces() * 3);
	g_mesh->GenerateAdjacency(1E-6, rgdwAdjacency);
	g_mesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, rgdwAdjacency, NULL, NULL, NULL);
	free(rgdwAdjacency);

	auto NumFaces = g_mesh->GetNumFaces();
	auto NumVertices = g_mesh->GetNumVertices();

	IDirect3DIndexBuffer9* ppIB;
	g_mesh->GetIndexBuffer(&ppIB);

	D3DINDEXBUFFER_DESC pDesc;
	ppIB->GetDesc(&pDesc);
	int step = pDesc.Size / (NumFaces * 3);

	PDWORD AttributeBuffer = 0;
	g_mesh->LockAttributeBuffer(0, &AttributeBuffer);

	PWORD IndexBufferSize2 = nullptr;
	PDWORD IndexBufferSize4 = nullptr;
	if (step == 2)
	{
		g_mesh->LockIndexBuffer(0, (void**)&IndexBufferSize2);
	}
	else
	{
		g_mesh->LockIndexBuffer(0, (void**)&IndexBufferSize4);
	}
	TueVertex* pv = nullptr;
	g_mesh->LockVertexBuffer(0, (void**)&pv);

	std::vector<TueVertex> VerAry;
	for (int i = 0; i < NumVertices; i++)
	{
		VerAry.emplace_back(*pv);
		pv++;
	}

	std::map<int, std::pair<std::string, std::vector<long long>>> SubsetIndexMap;
	if (step == 2)
	{
		for (int i = 0; i < NumFaces; i++)
		{
			auto& SebsetIndexRef = SubsetIndexMap[*(AttributeBuffer + i)];
			for (int j = 0; j < 3; j++)
			{
				SebsetIndexRef.second.emplace_back(*(IndexBufferSize2 + (i * 3 + j)));
			}
			//材质索引
		}
	}
	else
	{
		for (int i = 0; i < NumFaces; i++)
		{
			auto& SebsetIndexRef = SubsetIndexMap[*(AttributeBuffer + i)];
			for (int j = 0; j < 3; j++)
			{
				SebsetIndexRef.second.emplace_back(*(IndexBufferSize4 + (i * 3 + j)));
			}
			//材质索引
		}
	}

	g_mesh->UnlockAttributeBuffer();
	g_mesh->UnlockIndexBuffer();
	g_mesh->UnlockVertexBuffer();

	std::vector<Material> vMaterial;
	D3DXMATERIAL* MaterialsPtr = (D3DXMATERIAL*)(material_buffer)->GetBufferPointer();
	for (unsigned int j = 0; j < g_num_materials; j++)
	{

		D3DXMATERIAL* tempMaterials = (D3DXMATERIAL*)(material_buffer)->GetBufferPointer();
		Material m;

		auto ptr = tempMaterials + j;

		m.diffuse = ptr->MatD3D.Diffuse;
		m.ambient = ptr->MatD3D.Ambient;
		m.emissive = ptr->MatD3D.Emissive;
		m.specular = ptr->MatD3D.Specular;
		vMaterial.push_back(m);

		if (MaterialsPtr[j].pTextureFilename)
		{
			SubsetIndexMap[j].first = MaterialsPtr[j].pTextureFilename;
		}

	}


	auto pManager = FbxManager::Create();
	if (!pManager)
	{
		//FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	else
	{
		//FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());
	}
	//Create an IOSettings object.This object holds all import / export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory(optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());


	FbxScene* pScene = FbxScene::Create(pManager, "My Scene");
	const char* pName = "test";
	FbxMesh* lMesh = FbxMesh::Create(pScene, pName);




	lMesh->InitControlPoints(VerAry.size());
	FbxVector4* lControlPoints = lMesh->GetControlPoints();





	for (int i = 0; i < VerAry.size(); i++)
	{
		//lControlPoints[i].mData[0] = VerAry[i].position.Z / 10;
		//lControlPoints[i].mData[1] = VerAry[i].position.Y / 10;
		//lControlPoints[i].mData[2] = VerAry[i].position.X / 10;

		lControlPoints[i].mData[0] = VerAry[i].position.Z;
		lControlPoints[i].mData[1] = VerAry[i].position.Y;
		lControlPoints[i].mData[2] = VerAry[i].position.X;

	}

	FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();
	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);



	for (int i = 0; i < VerAry.size(); i++)
	{
		//FbxVector4 aa(0, 0, 1);
		//FbxVector4 aa(VerAry[i].normal.Z, VerAry[i].normal.Y, VerAry[i].normal.X);

		//FbxVector4 aa(-VerAry[i].normal.Z / 10, -VerAry[i].normal.Y / 10, -VerAry[i].normal.X / 10);
		//FbxVector4 aa(VerAry[i].normal.Z / 10, VerAry[i].normal.Y / 10, VerAry[i].normal.X / 10);
		//if (i%1==0)
		//{
		//	FbxVector4 aa(-VerAry[i].normal.X, -VerAry[i].normal.Y, -VerAry[i].normal.Z);
		//	lGeometryElementNormal->GetDirectArray().Add(aa);
		//}
		//FbxVector4 aa(VerAry[i].normal.X, VerAry[i].normal.Y, VerAry[i].normal.Z);
		FbxVector4 aa(-VerAry[i].normal.Z, -VerAry[i].normal.Y, -VerAry[i].normal.X);
		//FbxVector4 aa(1,0,1);
		lGeometryElementNormal->GetDirectArray().Add(aa);
		

	}



	FbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
	lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	for (auto Iter : SubsetIndexMap)
	{
		for (int i = 0; i < Iter.second.second.size() / 3; i++)
		{
			lMesh->BeginPolygon(Iter.first);
			for (int j = 0; j < 3; j++)
			{
				//auto Index = Iter.second.second[i * 3 + j];
				//lMesh->AddPolygon(Index);

				if (j==0)
				{
					auto Index = Iter.second.second[i * 3 + 0];
					lMesh->AddPolygon(Index);
				}


				if (j==1)
				{
					auto Index = Iter.second.second[i * 3 + 2];
					lMesh->AddPolygon(Index);
				}

				if (j== 2)
				{
					auto Index = Iter.second.second[i * 3 + 1];
					lMesh->AddPolygon(Index);
				}

				//lGeometryElementNormal->GetIndexArray().Add(0);


				/*FbxVector4 aa(VerAry[Index].normal.X*10, VerAry[Index].normal.Y*10,VerAry[Index].normal.Z*10);
				lGeometryElementNormal->GetDirectArray().Add(aa);*/

			}
			lMesh->EndPolygon();
		}
	}



	FbxNode* lNode = FbxNode::Create(pScene, pName);
	lNode->SetNodeAttribute(lMesh);

#pragma region Material
	int c = 0;
	for (auto i : vMaterial)
	{
		FbxString lMaterialName = "material";
		FbxDouble3 dif(i.diffuse.r, i.diffuse.g, i.diffuse.b);
		//FbxDouble3 amb(i.ambient.r, i.ambient.g, i.ambient.b);
		//FbxDouble3 emi(i.emissive.r, i.emissive.g, i.emissive.b);
		//FbxDouble3 spe(i.specular.r, i.specular.g, i.specular.b);
		lMaterialName += c;
		c += 1;

		FbxDouble3 red(1.0, 0.0, 0.0);

		auto lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

		/*lMaterial->Diffuse.Set(dif);*/
		lMaterial->Diffuse.Set(dif);
		//lMaterial->Ambient.Set(amb);
		//lMaterial->Emissive.Set(emi);
		//lMaterial->Specular.Set(spe);

		FbxNode* lNode = lMesh->GetNode();
		if (lNode)
			lNode->AddMaterial(lMaterial);
	}

#pragma endregion



	
	pScene->GetRootNode()->AddChild(lNode);

	SaveScene(pManager, pScene, ExportPath.data());

	return true;
}

TueXFile::TueXFile()
{
}

TueXFile::~TueXFile()
{

}

bool TueXFile::LoadFromFile(
	const std::string& FileName,
	HWND hWnd, std::string ExportPath
)
{
	if (!getXInfo(FileName.data(), hWnd, ExportPath))
	{
		return false;
	}

	return true;
}

bool CreateScene(FbxScene* pScene, char* pSampleFileName)
{
	return true;
}
//没有使用
void CreateTexture(const FMeshInfoMap MeshInfoMap, FbxScene* pScene, FbxMesh* pMesh)
{
	FbxString lMaterialName = "material";
	FbxString lShadingName = "Phong";

	FbxDouble3 lBlack(1.0, 1.0, 1.0);
	FbxDouble3 lRed(1.0, 1.0, 1.0);
	FbxDouble3 lDiffuseColor(1.0, 1.0, 1.0);

	for (auto Iter : MeshInfoMap)
	{
		//get the node of mesh, add material for it.

		auto lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

		lMaterial->Emissive.Set(lBlack);
		lMaterial->Ambient.Set(lRed);
		lMaterial->AmbientFactor.Set(1.);
		// Add texture for diffuse channel
		lMaterial->Diffuse.Set(lDiffuseColor);
		lMaterial->DiffuseFactor.Set(1.);
		lMaterial->TransparencyFactor.Set(0.4);
		lMaterial->ShadingModel.Set(lShadingName);
		lMaterial->Shininess.Set(0.5);
		lMaterial->Specular.Set(lBlack);
		lMaterial->SpecularFactor.Set(0.3);

		FbxFileTexture* lTexture = FbxFileTexture::Create(pScene, "Diffuse Texture");
		if (Iter.second.Name.size() > 0)
		{
			bool isTexture = lTexture->SetFileName(Iter.second.Name.data());
		}
		else
		{
			bool isTexture = lTexture->SetFileName("");
		}

		lTexture->SetTextureUse(FbxTexture::eStandard);
		lTexture->SetMappingType(FbxTexture::eUV);
		lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
		lTexture->SetSwapUV(false);
		lTexture->SetTranslation(0.0, 0.0);
		lTexture->SetScale(1.0, 1.0);
		lTexture->SetRotation(0.0, 0.0);
		lTexture->UVSet.Set(FbxString(gDiffuseElementName)); // Connect texture to the proper UV


		if (lMaterial)
		{
			lMaterial->Diffuse.ConnectSrcObject(lTexture);
		}
		FbxNode* lNode = pMesh->GetNode();
		if (lNode)
		{
			lNode->AddMaterial(lMaterial);
		}
	}
}

void CreateCube()
{



	FbxManager* pManager = NULL;
	FbxScene* pScene = NULL;
	InitializeSdkObjects(pManager, pScene);
	char* cc = NULL;
	bool PResult = CreateScene(pScene, cc);
	const char* pName = "test";
	FbxMesh* lMesh = FbxMesh::Create(pScene, pName);

	int PtCount = 0;

	//FbxVector4 lNormalXPos(1, 0, 0);
	//FbxVector4 lNormalXNeg(-1, 0, 0);
	//FbxVector4 lNormalYPos(0, 1, 0);
	//FbxVector4 lNormalYNeg(0, -1, 0);
	//FbxVector4 lNormalZPos(0, 0, 1);
	//FbxVector4 lNormalZNeg(0, 0, -1);

	FbxVector4 lNormalXPos(1, 0, 0);
	FbxVector4 lNormalXNeg(1, 0, 0);
	FbxVector4 lNormalYPos(1, 0, 0);
	FbxVector4 lNormalYNeg(1, 0, 0);
	FbxVector4 lNormalZPos(0, 0, 1);
	FbxVector4 lNormalZNeg(1, 0, 0);


	FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();
	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);

	lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);


	lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
	//lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);

	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
	//lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.


	FbxVector4 lControlPoint0(-50, 0, 50);
	FbxVector4 lControlPoint1(50, 0, 50);
	FbxVector4 lControlPoint2(50, 100, 50);
	FbxVector4 lControlPoint3(-50, 100, 50);
	FbxVector4 lControlPoint4(-50, 0, -50);
	FbxVector4 lControlPoint5(50, 0, -50);
	FbxVector4 lControlPoint6(50, 100, -50);
	FbxVector4 lControlPoint7(-50, 100, -50);

	lMesh->InitControlPoints(24);
	FbxVector4* lControlPoints = lMesh->GetControlPoints();
	lControlPoints[0] = lControlPoint0;
	lControlPoints[1] = lControlPoint1;
	lControlPoints[2] = lControlPoint2;
	lControlPoints[3] = lControlPoint3;
	lControlPoints[4] = lControlPoint1;
	lControlPoints[5] = lControlPoint5;
	lControlPoints[6] = lControlPoint6;
	lControlPoints[7] = lControlPoint2;
	lControlPoints[8] = lControlPoint5;
	lControlPoints[9] = lControlPoint4;
	lControlPoints[10] = lControlPoint7;
	lControlPoints[11] = lControlPoint6;
	lControlPoints[12] = lControlPoint4;
	lControlPoints[13] = lControlPoint0;
	lControlPoints[14] = lControlPoint3;
	lControlPoints[15] = lControlPoint7;
	lControlPoints[16] = lControlPoint3;
	lControlPoints[17] = lControlPoint2;
	lControlPoints[18] = lControlPoint6;
	lControlPoints[19] = lControlPoint7;
	lControlPoints[20] = lControlPoint1;
	lControlPoints[21] = lControlPoint0;
	lControlPoints[22] = lControlPoint4;
	lControlPoints[23] = lControlPoint5;

	int lPolygonVertices[] = { 0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15,
		16, 17, 18, 19,
		20, 21, 22, 23 };



	//每3个点一个面，a代表了除以几，是一个面
	int a = 3;
	for (int aa = 0; aa < 6; aa++)
	{
		lMesh->BeginPolygon(-1, -1, -1, false);
		for (int bb = 0; bb < 4; bb++)
		{
			lGeometryElementNormal->GetIndexArray().Add(0);
			lMesh->AddPolygon(lPolygonVertices[aa * 4 + bb]);

		}
		lMesh->EndPolygon();
	}

	FbxNode* lNode = FbxNode::Create(pScene, pName);
	lNode->SetNodeAttribute(lMesh);
	pScene->GetRootNode()->AddChild(lNode);
	SaveScene(pManager, pScene, "textcube.fbx");

	DestroySdkObjects(pManager, PResult);



}





