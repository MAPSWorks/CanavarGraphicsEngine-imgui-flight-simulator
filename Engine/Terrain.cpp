#include "Terrain.h"
#include "CameraManager.h"
#include "ShaderManager.h"

#include "Helper.h"

#include <QMatrix4x4>

Canavar::Engine::Terrain::Terrain()
    : Node()
    , mEnabled(true)
{
    mType = Node::NodeType::Terrain;
    mName = "Terrain";
    mSelectable = false;

    initializeOpenGLFunctions();

    mShaderManager = ShaderManager::instance();
    mCameraManager = CameraManager::instance();

    reset();

    mTileGenerator = new TileGenerator(3, 128, 1024.0f);

    mTextures.insert("Sand", Helper::createTexture("Resources/Terrain/sand.jpg"));
    mTextures.insert("Grass", Helper::createTexture("Resources/Terrain/grass0.jpg"));
    mTextures.insert("Snow", Helper::createTexture("Resources/Terrain/snow0.jpg"));
    mTextures.insert("RockDiffuse", Helper::createTexture("Resources/Terrain/rock0.jpg"));
    mTextures.insert("RockNormal", Helper::createTexture("Resources/Terrain/rnormal.jpg"));
    mTextures.insert("Terrain", Helper::createTexture("Resources/Terrain/terrain.jpg"));

    setScale(QVector3D(1, 0, 1));
}

void Canavar::Engine::Terrain::render()
{
    if (!mEnabled)
        return;

    QVector2D currentTilePosition = mTileGenerator->whichTile(mCameraManager->activeCamera()->worldPosition());

    if (currentTilePosition != mPreviousTilePosition)
    {
        mTileGenerator->translateTiles((currentTilePosition - mPreviousTilePosition));
        mPreviousTilePosition = currentTilePosition;
    }

    mShaderManager->bind(ShaderType::TerrainShader);
    mShaderManager->setUniformValue("M", worldTransformation());
    mShaderManager->setUniformValue("terrain.amplitude", mAmplitude);
    mShaderManager->setUniformValue("terrain.seed", mSeed);
    mShaderManager->setUniformValue("terrain.octaves", mOctaves);
    mShaderManager->setUniformValue("terrain.frequency", mFrequency);
    mShaderManager->setUniformValue("terrain.tessellationMultiplier", mTessellationMultiplier);
    mShaderManager->setUniformValue("terrain.power", mPower);
    mShaderManager->setUniformValue("terrain.grassCoverage", mGrassCoverage);
    mShaderManager->setUniformValue("terrain.ambient", mAmbient);
    mShaderManager->setUniformValue("terrain.diffuse", mDiffuse);
    mShaderManager->setUniformValue("terrain.shininess", mShininess);
    mShaderManager->setUniformValue("terrain.specular", mSpecular);
    mShaderManager->setUniformValue("waterHeight", -1000.0f);
    mShaderManager->setSampler("sand", 1, mTextures.value("Sand")->textureId());
    mShaderManager->setSampler("grass", 2, mTextures.value("Grass")->textureId());
    mShaderManager->setSampler("terrainTexture", 3, mTextures.value("Terrain")->textureId());
    mShaderManager->setSampler("snow", 4, mTextures.value("Snow")->textureId());
    mShaderManager->setSampler("rock", 5, mTextures.value("RockDiffuse")->textureId());
    mShaderManager->setSampler("rockNormal", 6, mTextures.value("RockNormal")->textureId());

    mTileGenerator->render(GL_PATCHES);
    mShaderManager->release();
}

void Canavar::Engine::Terrain::reset()
{
    mOctaves = 13;
    mFrequency = 0.01f;
    mTessellationMultiplier = 1.0f;
    mAmplitude = 20.0f;
    mPower = 3.0f;
    mGrassCoverage = 0.45f;
    mSeed = QVector3D(1, 1, 1);

    mAmbient = 0.5f;
    mDiffuse = 0.6f;
    mShininess = 8.0f;
    mSpecular = 0.05f;
}

void Canavar::Engine::Terrain::toJson(QJsonObject &object)
{
    Node::toJson(object);

    object.insert("amplitude", mAmplitude);
    object.insert("frequency", mFrequency);
    object.insert("octaves", mOctaves);
    object.insert("power", mPower);
    object.insert("tessellation_multiplier", mTessellationMultiplier);
    object.insert("grass_coverage", mGrassCoverage);
    object.insert("ambient", mAmbient);
    object.insert("diffuse", mDiffuse);
    object.insert("specular", mSpecular);
    object.insert("shininess", mShininess);
    object.insert("enabled", mEnabled);

    QJsonObject seed;
    seed.insert("x", mSeed.x());
    seed.insert("y", mSeed.y());
    seed.insert("z", mSeed.z());

    object.insert("seed", seed);
}

void Canavar::Engine::Terrain::fromJson(const QJsonObject &object)
{
    Node::fromJson(object);

    mAmplitude = object["amplitude"].toDouble();
    mFrequency = object["frequency"].toDouble();
    mOctaves = object["octaves"].toInt();
    mPower = object["power"].toDouble();
    mTessellationMultiplier = object["tessellation_multiplier"].toDouble();
    mGrassCoverage = object["grass_coverage"].toDouble();
    mAmbient = object["ambient"].toDouble();
    mDiffuse = object["diffuse"].toDouble();
    mSpecular = object["specular"].toDouble();
    mShininess = object["shininess"].toDouble();
    mEnabled = object["enabled"].toBool(mEnabled);

    // Seed
    float x = object["seed"]["x"].toDouble();
    float y = object["seed"]["y"].toDouble();
    float z = object["seed"]["z"].toDouble();

    mSeed = QVector3D(x, y, z);
}

Canavar::Engine::Terrain *Canavar::Engine::Terrain::instance()
{
    static Terrain instance;
    return &instance;
}
