///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  CreateSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  textures to bind to texture slots for rendering  
 ***********************************************************/
void SceneManager::CreateSceneTextures()
{
	CreateGLTexture("textures/wood.jpg", "WoodFloor");
	CreateGLTexture("textures/aluminum.jpg", "Aluminum");
	CreateGLTexture("textures/egyptian-bricks.jpg", "Pyramid");
	CreateGLTexture("textures/orange.jpg", "Orange");
	CreateGLTexture("textures/dirt.jpg", "Dirt");

	BindGLTextures();

}
/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// back left light (doesnt show up for some reason)
	m_pShaderManager->setVec3Value("lightSources[0].position", -50.0f, 20.0, -60.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.07f, 0.07f, 0.07f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 100.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// back right light
	m_pShaderManager->setVec3Value("lightSources[1].position", 50.0f, 20.0f, -60.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.07f, 0.07f, 0.07f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 100.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// front left light (middle left yellow light)
	m_pShaderManager->setVec3Value("lightSources[2].position", -50.0f, 20.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.07f, 0.07f, 0.07f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 100.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);

	// front right light
	m_pShaderManager->setVec3Value("lightSources[3].position", 40.0f, 20.0f, -10.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.07f, 0.07f, 0.07f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 100.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.18f);

	m_pShaderManager->setBoolValue(g_UseLightingName, true);
}

/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{

	OBJECT_MATERIAL floorMaterial;
	floorMaterial.ambientColor = glm::vec3(0.10f, 0.10f, 0.10f);
	floorMaterial.ambientStrength = 0.01f;
	floorMaterial.diffuseColor = glm::vec3(0.10f, 0.10f, 0.10f);
	floorMaterial.specularColor = glm::vec3(0.10f, 0.10f, 0.10f);
	floorMaterial.shininess = 35;
	floorMaterial.tag = "floor";
	m_objectMaterials.push_back(floorMaterial);

	OBJECT_MATERIAL orangeMaterial;
	orangeMaterial.ambientColor = glm::vec3(1.0f, 0.90f, 0.0f);
	orangeMaterial.ambientStrength = 0.03f;
	orangeMaterial.diffuseColor = glm::vec3(1.0f, 0.4f, 0.0f);
	orangeMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	orangeMaterial.shininess = 75.0;
	orangeMaterial.tag = "orange";
	m_objectMaterials.push_back(orangeMaterial);

	OBJECT_MATERIAL boxMaterial;
	boxMaterial.ambientColor = glm::vec3(0.39f, 0.39f, 0.35f);
	boxMaterial.ambientStrength = 0.15f;
	boxMaterial.diffuseColor = glm::vec3(0.19f, 0.19f, 0.185f);
	boxMaterial.specularColor = glm::vec3(0.41f, 0.41f, 0.41f);
	boxMaterial.shininess = 30;
	boxMaterial.tag = "box";
	m_objectMaterials.push_back(boxMaterial);

	OBJECT_MATERIAL mugMaterial;
	mugMaterial.ambientColor = glm::vec3(0.001f, 0.01f, 0.48f);
	mugMaterial.ambientStrength = 0.05f;
	mugMaterial.diffuseColor = glm::vec3(0.001f, 0.001f, 0.48f);
	mugMaterial.specularColor = glm::vec3(0.01f, 0.1f, 0.1f);
	mugMaterial.shininess = 50;
	mugMaterial.tag = "mug";
	m_objectMaterials.push_back(mugMaterial);

	OBJECT_MATERIAL coneMaterial;
	coneMaterial.ambientColor = glm::vec3(0.75f, 0.75f, 0.75f);
	coneMaterial.ambientStrength = 0.025f;
	coneMaterial.diffuseColor = glm::vec3(0.25f, 0.25f, 0.25f);
	coneMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	coneMaterial.shininess = 80;
	coneMaterial.tag = "cone";
	m_objectMaterials.push_back(coneMaterial);

	OBJECT_MATERIAL coffeeMaterial;
	coffeeMaterial.ambientColor = glm::vec3(0.44f, 0.31f, 0.21f);
	coffeeMaterial.ambientStrength = 0.05f;
	coffeeMaterial.diffuseColor = glm::vec3(0.44f, 0.31f, 0.21f);
	coffeeMaterial.specularColor = glm::vec3(0.44f, 0.31, 0.21f);
	coffeeMaterial.shininess = 2;
	coffeeMaterial.tag = "coffee";
	m_objectMaterials.push_back(coffeeMaterial);

	OBJECT_MATERIAL cylinderMaterial;
	cylinderMaterial.ambientColor = glm::vec3(0.70f, 0.70f, 0.70f);
	cylinderMaterial.ambientStrength = 0.05f;
	cylinderMaterial.diffuseColor = glm::vec3(0.10f, 0.10f, 0.1f);
	cylinderMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	cylinderMaterial.shininess = 75;
	cylinderMaterial.tag = "cylinder";
	m_objectMaterials.push_back(cylinderMaterial);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// making the textures for the scene
	CreateSceneTextures();
	// loading in the meshes
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPyramid4Mesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderFloor();
	RenderCoffeeMaker();
	RenderOranges();
	RenderCoffeeMug();
	RenderMilkCarton();
}

/***********************************************************
 *  RenderFloor()
 *
 *  This method is for rendering the 
 *  floor plane and its texture. 
 *  The texture is tiled wood btw...
 ***********************************************************/
void SceneManager::RenderFloor()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// since were rotating just the floor, lets make a personal variable
	float YrotationDegreesFloor = 90.0f;
	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// draw the mesh with transformation values
	// Floor plane mesh

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegreesFloor,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	// Floor plane mesh

	// setting the texture of the floor plane to wood
	SetShaderTexture("WoodFloor");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("floor");
	m_basicMeshes->DrawPlaneMesh();
}


/***********************************************************
 *  RenderCoffeeMaker()
 *
 *  This method is for rendering the complex object
 *  moka pot and its textures
 ***********************************************************/
void SceneManager::RenderCoffeeMaker()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// bottom tapered cylinder for moka pot
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);
	positionXYZ = glm::vec3(3.0f, 0.0f, 4.0f);
	SetTransformations(
		scaleXYZ,         // scale and size
		XrotationDegrees, // flipping like a coin
		180, // spinning like a top
		ZrotationDegrees, //
		positionXYZ       // positioning
	);
	SetShaderTexture("Aluminum"); // setting the texture of the bottom of moka pot to aluminum
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cone");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// middle cylinder section
	scaleXYZ = glm::vec3(0.75f, 0.35f, 0.75f);
	positionXYZ = glm::vec3(3.0f, 1.8f, 4.0f);
	SetTransformations(
		scaleXYZ,         // scale and size
		180, // flipping like a coin
		YrotationDegrees, // spinning like a top
		ZrotationDegrees, //
		positionXYZ       // positioning
	);
	SetShaderTexture("Aluminum");
	//SetShaderColor(165.0f / 255.0f, 169.0f / 255.0f, 180.0f / 255.0f, 1.0f);
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cylinder");
	m_basicMeshes->DrawCylinderMesh();

	// top tapered cylinder for moka pot (inverted)
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);
	positionXYZ = glm::vec3(3.0f, 3.5f, 4.0f);
	SetTransformations(
		scaleXYZ,         // scale and size
		180, // flipping like a coin
		YrotationDegrees, // spinning like a top
		ZrotationDegrees, //
		positionXYZ       // positioning
	);

	// setting the texture of the moka pot to aluminum
	SetShaderTexture("Aluminum");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cone");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// lid handle of the moka pot
	SetShaderColor(111.0f / 255.0f, 78.0f / 255.0f, 55.0f / 255.0f, 1.0f);
	scaleXYZ = glm::vec3(0.4f, 0.25f, 0.4f);
	positionXYZ = glm::vec3(3.0f, 3.9f, 3.9f);
	SetTransformations(
		scaleXYZ,
		90,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);
	SetShaderMaterial("coffee");
	m_basicMeshes->DrawCylinderMesh();

	// side handle for the moka pot (horizontal)
	scaleXYZ = glm::vec3(1.4f, 0.25f, 0.4f);
	positionXYZ = glm::vec3(4.0f, 3.3f, 4.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);
	SetShaderMaterial("coffee");
	m_basicMeshes->DrawBoxMesh();

	// side handle for the moka pot (horizontal)
	scaleXYZ = glm::vec3(1.25f, 0.25f, 0.4f);
	positionXYZ = glm::vec3(4.6f, 2.8f, 4.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		90,
		positionXYZ
	);
	SetShaderMaterial("coffee");
	m_basicMeshes->DrawBoxMesh();

	// Spout for moka pot
	scaleXYZ = glm::vec3(1.25f, 0.25f, 0.4f);
	positionXYZ = glm::vec3(2.1f, 3.27f, 4.0f);
	SetTransformations(
		scaleXYZ,
		90,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);
	// setting the texture of the moka pot spout to aluminum
	SetShaderTexture("Aluminum");
	SetTextureUVScale(3.0, 3.0);
	SetShaderMaterial("cone");                   // may need to change 
	m_basicMeshes->DrawPrismMesh();

	// coloring the top of the spout coffee colored
	scaleXYZ = glm::vec3(1.22f, 0.22f, 0.38f);
	positionXYZ = glm::vec3(2.1f, 3.29f, 4.0f);
	SetTransformations(
		scaleXYZ,
		90,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);
	// setting the texture of the moka pot to aluminum
	SetShaderColor(111.0f / 255.0f, 78.0f / 255.0f, 55.0f / 255.0f, 1.0f);
	SetShaderMaterial("coffee");              // need a coffee colored and matte material
	m_basicMeshes->DrawPrismMesh();

}


/***********************************************************
 *  RenderOranges()
 *
 *  This method is for rendering the super ultra complex 
 *  object of two whole mandarin oranges.
 *  (that used to be plums but i like the oranges more) 
 *  ...WOW...
 ***********************************************************/
void SceneManager::RenderOranges()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Left mandarin orange
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);
	positionXYZ = glm::vec3(-2.0f, 0.75f, 5.0f);
	SetTransformations(
		scaleXYZ,
		90,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);
	// setting the texture of the orange
	SetShaderTexture("Dirt");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("orange");
	m_basicMeshes->DrawSphereMesh();

	// Right mandarin orange
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);
	positionXYZ = glm::vec3(-0.5f, 0.75f, 7.0f);
	SetTransformations(
		scaleXYZ,
		90,
		90, // rotating one orange to make them look a little different
		ZrotationDegrees,
		positionXYZ
	);
	SetTextureUVScale(-1.0, 1.0);
	m_basicMeshes->DrawSphereMesh();
}


/***********************************************************
 *  RenderCoffeeMug()
 *
 *  This method is for rendering the complex
 *  object, coffee mug.
 *  
 ***********************************************************/
void SceneManager::RenderCoffeeMug() 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Mug Handle
	scaleXYZ = glm::vec3(0.75f, 0.45f, 0.75f);
	positionXYZ = glm::vec3(-4.7f, 1.40f, 6.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);
	// setting the color of the mug
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderMaterial("mug");
	m_basicMeshes->DrawTorusMesh();

	// Mug Body
	scaleXYZ = glm::vec3(1.00f, 2.25f, 0.75f);
	positionXYZ = glm::vec3(-5.5f, 0.00f, 6.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees, 
		ZrotationDegrees,
		positionXYZ
	);
	m_basicMeshes->DrawCylinderMesh();
}



/***********************************************************
 *  RenderMilkCarton()
 *
 *  This method is for rendering the complex
 *  object, Milk Carton.
 *
 ***********************************************************/
void SceneManager::RenderMilkCarton()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = -20.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Milk Carton Body
	scaleXYZ = glm::vec3(3.00f, 6.00f, 3.00f);
	positionXYZ = glm::vec3(-3.0f, 3.00f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);
	// setting the texture of the milk carton
	SetShaderColor(1.0f, 0.9f, 1.0f, 1.0f);
	SetShaderMaterial("box");
	m_basicMeshes->DrawBoxMesh();

	// Carton Top (inside portion)
	scaleXYZ = glm::vec3(3.0f, 3.0f, 1.0f);
	positionXYZ = glm::vec3(-3.0f, 6.5f, 0.0f);
	SetTransformations(
		scaleXYZ,
		-90,
		0,
		-110,
		positionXYZ
	);
	m_basicMeshes->DrawPrismMesh();

	// Carton Top (tab)
	scaleXYZ = glm::vec3(2.95f, 0.50f, 0.10f);
	positionXYZ = glm::vec3(-3.0f, 7.15f, 0.0f);
	SetTransformations(
		scaleXYZ,
		0,
		-20,
		0,
		positionXYZ
	);
	m_basicMeshes->DrawBoxMesh();

	// carton lid
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);
	positionXYZ = glm::vec3(-3.3f, 6.35f, 0.90f);
	SetTransformations(
		scaleXYZ,
		30,
		0,
		15,
		positionXYZ
	);
	m_basicMeshes->DrawCylinderMesh();
}