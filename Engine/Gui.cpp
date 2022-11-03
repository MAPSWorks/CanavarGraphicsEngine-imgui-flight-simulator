#include "Gui.h"
#include "Haze.h"
#include "Helper.h"
#include "ModelDataManager.h"
#include "RendererManager.h"
#include "SelectableNodeRenderer.h"

Canavar::Engine::Gui::Gui(QObject *parent)
    : QObject(parent)
    , mSelectedNode(nullptr)
    , mSelectedModel(nullptr)
    , mSelectedMesh(nullptr)
    , mSelectedVertexIndex(-1)
    , mDrawAllBBs(false)
    , mNodeSelectionEnabled(true)
    , mMeshSelectionEnabled(false)
    , mVertexSelectionEnabled(false)
{}

void Canavar::Engine::Gui::draw()
{
    // Render Settings
    ImGui::SetNextWindowSize(ImVec2(420, 820), ImGuiCond_FirstUseEver);
    ImGui::Begin("Render Settings");

    ImGui::SliderFloat("Exposure##RenderSettings", &RendererManager::instance()->getExposure_nonConst(), 0.01f, 2.0f, "%.3f");
    ImGui::SliderFloat("Gamma##RenderSettings", &RendererManager::instance()->getGamma_nonConst(), 0.01f, 4.0f, "%.3f");
    ImGui::SliderInt("Bloom Blur Pass##RenderSettings", &RendererManager::instance()->getBlurPass_nonConst(), 0, 100);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    // Nodes
    ImGui::SetNextWindowSize(ImVec2(420, 820), ImGuiCond_FirstUseEver);
    ImGui::Begin("Nodes");

    const auto &nodes = NodeManager::instance()->nodes();

    // Draw All Bounding Boxes
    if (ImGui::Checkbox("Draw All Bounding Boxes", &mDrawAllBBs))
    {
        if (mDrawAllBBs)
            for (const auto &node : nodes)
                RendererManager::instance()->addSelectableNode(node, QVector4D(1, 1, 1, 1));
        else
            for (const auto &node : nodes)
                RendererManager::instance()->removeSelectableNode(node);

        if (mNodeSelectionEnabled)
            if (mSelectedNode)
                RendererManager::instance()->addSelectableNode(mSelectedNode, QVector4D(1, 0, 0, 1));
    }

    // Node Selection
    if (ImGui::Checkbox("Node Selection", &mNodeSelectionEnabled))
    {
        if (mNodeSelectionEnabled)
        {
            RendererManager::instance()->addSelectableNode(mSelectedNode, QVector4D(1, 0, 0, 1));

        } else
        {
            if (mDrawAllBBs)
                RendererManager::instance()->addSelectableNode(mSelectedNode, QVector4D(1, 1, 1, 1));
            else
                for (const auto &node : nodes)
                    RendererManager::instance()->removeSelectableNode(node);

            mMeshSelectionEnabled = false;
            mVertexSelectionEnabled = false;
            setSelectedMesh(nullptr);
        }
    }

    // Mesh Selection
    ImGui::BeginDisabled(!mSelectedModel || !mNodeSelectionEnabled);
    if (ImGui::Checkbox("Mesh Selection", &mMeshSelectionEnabled))
    {
        setSelectedMesh(nullptr);
        mVertexSelectionEnabled = false;
    }
    ImGui::EndDisabled();

    // Vertex Selection
    ImGui::BeginDisabled(!mSelectedMesh || !mMeshSelectionEnabled);
    if (ImGui::Checkbox("Vertex Selection", &mVertexSelectionEnabled))
    {
        RendererManager::instance()->getSelectedMeshParameters_Ref(mSelectedModel).mRenderVertices = true;
        setSelectedVertexIndex(-1);
    }
    ImGui::EndDisabled();

    // Select a node
    if (ImGui::BeginCombo("Select a node", mSelectedNode ? mSelectedNode->getName().toStdString().c_str() : "-"))
    {
        for (int i = 0; i < nodes.size(); ++i)
            if (ImGui::Selectable(nodes[i]->getName().toStdString().c_str()))
            {
                setSelectedNode(nodes[i]);
            }

        ImGui::EndCombo();
    }

    // Actions
    if (mSelectedNode)
    {
        if (!ImGui::CollapsingHeader("Actions"))
        {
            ImGui::Text("Type: %d", mSelectedNode->getType());
            ImGui::Text("ID: %d", mSelectedNode->getID());
            ImGui::Text("UUID: %s", mSelectedNode->getUUID().toStdString().c_str());

            // Assign a parent
            {
                if (ImGui::BeginCombo("Assign a parent", mSelectedNode->parent() ? mSelectedNode->parent()->getName().toStdString().c_str() : "-"))
                {
                    if (ImGui::Selectable("-"))
                        mSelectedNode->setParent(nullptr);

                    for (int i = 0; i < nodes.size(); ++i)
                    {
                        if (mSelectedNode == nodes[i])
                            continue;

                        if (ImGui::Selectable(nodes[i]->getName().toStdString().c_str()))
                            mSelectedNode->setParent(nodes[i]);
                    }

                    ImGui::EndCombo();
                }
            }

            if (ImGui::Button("Remove this node"))
            {
                NodeManager::instance()->removeNode(mSelectedNode);
                mNodeSelectionEnabled = false;
                mMeshSelectionEnabled = false;
                mVertexSelectionEnabled = false;
            }
        }
    }

    if (mSelectedNode)
    {
        switch (mSelectedNode->getType())
        {
        case Canavar::Engine::Node::NodeType::DummyNode:
            draw(mSelectedNode);
            break;
        case Canavar::Engine::Node::NodeType::PersecutorCamera:
        case Canavar::Engine::Node::NodeType::DummyCamera:
            draw(dynamic_cast<PerspectiveCamera *>(mSelectedNode));
            draw(mSelectedNode);
            break;
        case Canavar::Engine::Node::NodeType::FreeCamera:
            draw(dynamic_cast<FreeCamera *>(mSelectedNode));
            draw(dynamic_cast<PerspectiveCamera *>(mSelectedNode));
            draw(mSelectedNode);
            break;
        case Canavar::Engine::Node::NodeType::Sky:
            draw(dynamic_cast<Sky *>(mSelectedNode));
            break;
        case Canavar::Engine::Node::NodeType::Sun:
            draw(dynamic_cast<Sun *>(mSelectedNode));
            break;
        case Canavar::Engine::Node::NodeType::Model:
            draw(mSelectedNode);
            draw(dynamic_cast<Model *>(mSelectedNode));
            break;
        case Canavar::Engine::Node::NodeType::Terrain:
            draw(dynamic_cast<Terrain *>(mSelectedNode));
            break;
        case Canavar::Engine::Node::NodeType::Haze:
            draw(dynamic_cast<Haze *>(mSelectedNode));
            break;
        case Canavar::Engine::Node::NodeType::PointLight:
            draw(dynamic_cast<Light *>(mSelectedNode));
            draw(dynamic_cast<PointLight *>(mSelectedNode));
            draw(dynamic_cast<Node *>(mSelectedNode));
            break;
        case Canavar::Engine::Node::NodeType::NozzleEffect:
            draw(dynamic_cast<NozzleEffect *>(mSelectedNode));
            draw(dynamic_cast<Node *>(mSelectedNode));
            break;
        case Canavar::Engine::Node::NodeType::FirecrackerEffect:
            draw(dynamic_cast<FirecrackerEffect *>(mSelectedNode));
            draw(dynamic_cast<Node *>(mSelectedNode));
            break;
        default:
            break;
        }
    }

    ImGui::End();
}

void Canavar::Engine::Gui::draw(Node *node)
{
    // Position
    if (!ImGui::CollapsingHeader("Position##Node"))
    {
        float x = node->position().x();
        float y = node->position().y();
        float z = node->position().z();

        if (ImGui::DragFloat("x##NodePosition", &x, 0.01f, -1000.0f, 1000.0f, "%.3f"))
            node->setPosition(QVector3D(x, y, z));
        if (ImGui::DragFloat("y##NodePosition", &y, 0.01f, -1000.0f, 1000.0f, "%.3f"))
            node->setPosition(QVector3D(x, y, z));
        if (ImGui::DragFloat("z##NodePosition", &z, 0.01f, -1000.0f, 1000.0f, "%.3f"))
            node->setPosition(QVector3D(x, y, z));
    }

    // Rotation
    if (ImGui::CollapsingHeader("Rotation##Node"))
    {
        float yaw, pitch, roll;

        Canavar::Engine::Helper::getEulerDegrees(node->rotation(), yaw, pitch, roll);

        if (ImGui::SliderFloat("Yaw##NodeRotation", &yaw, 0.0f, 359.999f, "%.3f"))
            node->setRotation(Canavar::Engine::Helper::constructFromEulerDegrees(yaw, pitch, roll));
        if (ImGui::SliderFloat("Pitch##NodeRotation", &pitch, -89.999f, 89.999f, "%.3f"))
            node->setRotation(Canavar::Engine::Helper::constructFromEulerDegrees(yaw, pitch, roll));
        if (ImGui::SliderFloat("Roll##NodeRotation", &roll, -179.999f, 179.999f, "%.3f"))
            node->setRotation(Canavar::Engine::Helper::constructFromEulerDegrees(yaw, pitch, roll));
    }

    // World Position
    if (ImGui::CollapsingHeader("World Position##Node"))
    {
        QVector3D position = node->worldPosition();
        float x = position.x();
        float y = position.y();
        float z = position.z();

        if (ImGui::DragFloat("x##NodeWorldPosition", &x, 0.01f, -1000.0f, 1000.0f, "%.3f"))
            node->setWorldPosition(QVector3D(x, y, z));
        if (ImGui::DragFloat("y##NodeWorldPosition", &y, 0.01f, -1000.0f, 1000.0f, "%.3f"))
            node->setWorldPosition(QVector3D(x, y, z));
        if (ImGui::DragFloat("z##NodeWorldPosition", &z, 0.01f, -1000.0f, 1000.0f, "%.3f"))
            node->setWorldPosition(QVector3D(x, y, z));
    }

    // World rotation
    if (ImGui::CollapsingHeader("World Rotation##Node"))
    {
        QQuaternion rotation = node->worldRotation();
        float yaw, pitch, roll;

        Helper::getEulerDegrees(rotation, yaw, pitch, roll);

        if (ImGui::SliderFloat("Yaw##NodeRotation", &yaw, 0.0f, 359.999f, "%.3f"))
            node->setWorldRotation(Helper::constructFromEulerDegrees(yaw, pitch, roll));
        if (ImGui::SliderFloat("Pitch##NodeRotation", &pitch, -89.999f, 89.999f, "%.3f"))
            node->setWorldRotation(Helper::constructFromEulerDegrees(yaw, pitch, roll));
        if (ImGui::SliderFloat("Roll##NodeRotation", &roll, -179.999f, 179.999f, "%.3f"))
            node->setWorldRotation(Helper::constructFromEulerDegrees(yaw, pitch, roll));
    }

    // Scale
    if (ImGui::CollapsingHeader("Scale##Node"))
    {
        QVector3D scale = node->scale();
        float x = scale.x();
        float y = scale.y();
        float z = scale.z();
        float all = qMin(x, qMin(y, z));

        if (ImGui::DragFloat("All##NodeScale", &all, 0.01f, 0.0001f, 100.0f, "%.3f"))
            node->setScale(QVector3D(all, all, all));
        if (ImGui::DragFloat("x##NodeScale", &x, 0.01f, 0.0001f, 100.0f, "%.3f"))
            node->setScale(QVector3D(x, y, z));
        if (ImGui::DragFloat("y##NodeScale", &y, 0.01f, 0.0001f, 100.0f, "%.3f"))
            node->setScale(QVector3D(x, y, z));
        if (ImGui::DragFloat("z##NodeScale", &z, 0.01f, 0.0001f, 100.0f, "%.3f"))
            node->setScale(QVector3D(x, y, z));
    }
}

void Canavar::Engine::Gui::draw(Model *model)
{
    if (!ImGui::CollapsingHeader("Shading Parameters##Model"))
    {
        ImGui::SliderFloat("Ambient##Model", &model->getAmbient_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Diffuse##Model", &model->getDiffuse_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Specular##Model", &model->getSpecular_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Shininess##Model", &model->getShininess_nonConst(), 1.0f, 128.0f, "%.3f");
        ImGui::ColorEdit4("Color##Model", (float *) &model->getColor_nonConst());
        ImGui::SliderFloat("Overlay Color Factor##Model", &model->getOverlayColorFactor_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::ColorEdit4("Overlay Color##Model", (float *) &model->getOverlayColor_nonConst());
    }

    if (!ImGui::CollapsingHeader("Meshes##Model"))
    {
        if (auto data = ModelDataManager::instance()->getModelData(model->getModelName()))
        {
            const auto &meshes = data->meshes();

            if (ImGui::BeginCombo("Select a mesh", mSelectedMesh ? mSelectedMesh->getName().toStdString().c_str() : "-"))
            {
                for (int i = 0; i < meshes.size(); ++i)
                    if (ImGui::Selectable(meshes[i]->getName().toStdString().c_str()))
                        setSelectedMesh(meshes[i]);

                ImGui::EndCombo();
            }

            if (mSelectedMesh)
            {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Mesh Info");
                ImGui::Text("ID: %d", mSelectedMesh->getID());
                ImGui::Text("Number of Vertices: %d", mSelectedMesh->getNumberOfVertices());

                auto transformation = model->getMeshTransformation(mSelectedMesh->getName());
                auto position = transformation.column(3);
                auto rotation = QQuaternion::fromRotationMatrix(transformation.normalMatrix());

                // Position
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Position");
                ImGui::DragFloat("x##MeshPosition", &position[0], 0.01f, -1000.0f, 1000.0f, "%.3f");
                ImGui::DragFloat("y##MeshPosition", &position[1], 0.01f, -1000.0f, 1000.0f, "%.3f");
                ImGui::DragFloat("z##MeshPosition", &position[2], 0.01f, -1000.0f, 1000.0f, "%.3f");

                // Rotation
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Rotation");
                float yaw, pitch, roll;

                Canavar::Engine::Helper::getEulerDegrees(rotation, yaw, pitch, roll);

                if (ImGui::SliderFloat("Yaw##MeshRotation", &yaw, 0.0f, 359.999f, "%.3f"))
                    rotation = Canavar::Engine::Helper::constructFromEulerDegrees(yaw, pitch, roll);
                if (ImGui::SliderFloat("Pitch##MeshRotation", &pitch, -89.999f, 89.999f, "%.3f"))
                    rotation = Canavar::Engine::Helper::constructFromEulerDegrees(yaw, pitch, roll);
                if (ImGui::SliderFloat("Roll##MeshRotation", &roll, -179.999f, 179.999f, "%.3f"))
                    rotation = Canavar::Engine::Helper::constructFromEulerDegrees(yaw, pitch, roll);

                transformation.setToIdentity();
                transformation.setColumn(3, position);
                transformation.rotate(rotation);
                model->setMeshTransformation(mSelectedMesh->getName(), transformation);

                if (mSelectedVertexIndex != -1)
                {
                    Mesh::Vertex vertex = mSelectedMesh->getVertex(mSelectedVertexIndex);
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Vertex Info");
                    ImGui::Text("Selected vertex index: %d", mSelectedVertexIndex);
                    ImGui::Text("Selected vertex position: (%.4f, %.4f, %.4f)", vertex.position[0], vertex.position[1], vertex.position[2]);
                }
            }
        }
    }
}

void Canavar::Engine::Gui::draw(Sky *node)
{
    if (!ImGui::CollapsingHeader("Sky"))
    {
        ImGui::SliderFloat("Albedo##Sky", &node->getAlbedo_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Turbidity##Sky", &node->getTurbidity_nonConst(), 0.0f, 10.0f, "%.3f");
        ImGui::SliderFloat("Normalized Sun Y##Sun", &node->getNormalizedSunY_nonConst(), 0.0f, 10.0f, "%.3f");
    }
}

void Canavar::Engine::Gui::draw(Sun *sun)
{
    if (!ImGui::CollapsingHeader("Sun"))
    {
        ImGui::Text("Direction:");
        float x = sun->getDirection().x();
        float y = sun->getDirection().y();
        float z = sun->getDirection().z();
        float r = sun->getDirection().length();
        float theta = qRadiansToDegrees(atan2(z, x));
        float phi = qRadiansToDegrees(atan2(y, sqrt(z * z + x * x)));

        if (qFuzzyCompare(abs(phi), 90.0f))
            theta = 0.0f;

        bool updated = false;

        if (ImGui::SliderFloat("Theta##DirectionalLight", &theta, -180.0f, 180.0f, "%.1f"))
            updated = true;

        if (ImGui::SliderFloat("Phi##DirectionalLight", &phi, -90.0f, 90.0f, "%.1f"))
            updated = true;

        if (updated)
        {
            x = r * cos(qDegreesToRadians(phi)) * cos(qDegreesToRadians(theta));
            y = r * sin(qDegreesToRadians(phi));
            z = r * cos(qDegreesToRadians(phi)) * sin(qDegreesToRadians(theta));

            sun->setDirection(QVector3D(x, y, z));
        }

        ImGui::Text("Shading Parameters:");
        ImGui::SliderFloat("Ambient##Sun", &sun->getAmbient_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Diffuse##Sun", &sun->getDiffuse_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Specular##Sun", &sun->getSpecular_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::ColorEdit4("Color##Sun", (float *) &sun->getColor_nonConst());
    }
}

void Canavar::Engine::Gui::draw(PerspectiveCamera *camera)
{
    if (!ImGui::CollapsingHeader("Parameters##PerspectiveCamera"))
    {
        ImGui::SliderFloat("FOV##PerspectiveCamera", &camera->getVerticalFov_nonConst(), 1.0f, 120.0);
        ImGui::SliderFloat("Z-Near##PerspectiveCamera", &camera->getZNear_nonConst(), 0.1f, 100.0f);
        ImGui::SliderFloat("Z-Far##PerspectiveCamera", &camera->getZFar_nonConst(), 1000.0f, 1000000.0f);
    }
}

void Canavar::Engine::Gui::draw(FreeCamera *node)
{
    if (!ImGui::CollapsingHeader("Speed##Free Camera"))
    {
        ImGui::SliderFloat("Linear Speed##FreeCamera", &node->getSpeed_nonConst().mLinear, 0.1f, 10000.0f);
        ImGui::SliderFloat("Linear Speed Multiplier##FreeCamera", &node->getSpeed_nonConst().mLinearMultiplier, 0.1f, 10.0f);
        ImGui::SliderFloat("Angular Speed##FreeCamera", &node->getSpeed_nonConst().mAngular, 0.1f, 100.0f);
        ImGui::SliderFloat("Angular Speed Multiplier##FreeCamera", &node->getSpeed_nonConst().mAngularMultiplier, 0.1f, 10.0f);
    }
}

void Canavar::Engine::Gui::draw(Terrain *node)
{
    if (!ImGui::CollapsingHeader("Terrain"))
    {
        ImGui::SliderFloat("Amplitude##Terrain", &node->getAmplitude_nonConst(), 0.0f, 50.0f, "%.3f");
        ImGui::SliderInt("Octaves##Terrain", &node->getOctaves_nonConst(), 1, 20);
        ImGui::SliderFloat("Power##Terrain", &node->getPower_nonConst(), 0.1f, 10.0f, "%.3f");
        ImGui::SliderFloat("Tessellation Multiplier##Terrain", &node->getTessellationMultiplier_nonConst(), 0.1f, 10.0f, "%.3f");
        ImGui::SliderFloat("Grass Coverage##Terrain", &node->getGrassCoverage_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Ambient##Terrain", &node->getAmbient_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Diffuse##Terrain", &node->getDiffuse_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Specular##Terrain", &node->getSpecular_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Shininess##Terrain", &node->getShininess_nonConst(), 0.1f, 128.0f, "%.3f");

        if (ImGui::Button("Generate Seed##Terrain"))
            node->setSeed(Canavar::Engine::Helper::generateVec3(1, 1, 1));

        if (ImGui::Button("Reset##Terrain"))
            node->reset();
    }
}

void Canavar::Engine::Gui::draw(Light *node)
{
    if (!ImGui::CollapsingHeader("Shading Parameters##Light"))
    {
        ImGui::SliderFloat("Ambient##Light", &node->getAmbient_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Diffuse##Light", &node->getDiffuse_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Specular##Light", &node->getSpecular_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::ColorEdit4("Color##Light", (float *) &node->getColor_nonConst());
    }
}

void Canavar::Engine::Gui::draw(PointLight *node)
{
    if (!ImGui::CollapsingHeader("Attenuation##PointLight"))
    {
        ImGui::SliderFloat("Constant##PointLight", &node->getConstant_nonConst(), 0.0f, 10.0f, "%.3f");
        ImGui::SliderFloat("Linear##PointLight", &node->getLinear_nonConst(), 0.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Quadratic##PointLight", &node->getQuadratic_nonConst(), 0.00001f, 0.1f, "%.6f");
    }
}

void Canavar::Engine::Gui::draw(Haze *node)
{
    if (!ImGui::CollapsingHeader("Haze##Haze"))
    {
        ImGui::SliderFloat("Density##Haze", &node->getDensity_nonConst(), 0.0f, 4.0f, "%.3f");
        ImGui::SliderFloat("Gradient##Haze", &node->getGradient_nonConst(), 0.0f, 4.0f, "%.3f");
        ImGui::ColorEdit4("Color##Haze", (float *) &node->getColor_nonConst());
    }
}

void Canavar::Engine::Gui::draw(NozzleEffect *node)
{
    if (!ImGui::CollapsingHeader("Nozzle Effect Parameters##NozzleEffect"))
    {
        ImGui::SliderFloat("Max Radius##NozzleEffect", &node->getMaxRadius_nonConst(), 0.001f, 4.0f, "%.4f");
        ImGui::SliderFloat("Max Life##NozzleEffect", &node->getMaxLife_nonConst(), 0.0000f, 0.1f, "%.5f");
        ImGui::SliderFloat("Max Distance##NozzleEffect", &node->getMaxDistance_nonConst(), 1.0f, 30.0f, "%.3f");
        ImGui::SliderFloat("Min Distance##NozzleEffect", &node->getMinDistance_nonConst(), 1.0f, 30.0f, "%.3f");
        ImGui::SliderFloat("Speed##NozzleEffect", &node->getSpeed_nonConst(), 0.0f, 10.0f, "%.5f");
        ImGui::SliderFloat("Scale##NozzleEffect", &node->getScale_nonConst(), 0.001f, 0.1f, "%.4f");
    }
}

void Canavar::Engine::Gui::draw(FirecrackerEffect *node)
{
    if (!ImGui::CollapsingHeader("Firecracker Effect Parameters##Firecracker"))
    {
        ImGui::SliderFloat("Span Angle##Firecracker", &node->getSpanAngle_nonConst(), 0.0f, 180.0f, "%.1f");
        ImGui::SliderFloat("Gravity##Firecracker", &node->getGravity_nonConst(), 0.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Max Life##Firecracker", &node->getMaxLife_nonConst(), 0.0f, 20.0f, "%.2f");
        ImGui::SliderFloat("Min Life##Firecracker", &node->getMinLife_nonConst(), 0.0f, 20.0f, "%.2f");
        ImGui::SliderFloat("Initial Speed##Firecracker", &node->getInitialSpeed_nonConst(), 0.0f, 2000.0f, "%.1f");
        ImGui::SliderFloat("Damping##Firecracker", &node->getDamping_nonConst(), 0.000f, 10.0f, "%.2f");
        ImGui::SliderFloat("Scale##Firecracker", &node->getScale_nonConst(), 0.001f, 10.0f, "%.4f");
        ImGui::Checkbox("Loop##Firecracker", &node->getLoop_nonConst());
    }
}

Canavar::Engine::Node *Canavar::Engine::Gui::getSelectedNode() const
{
    return mSelectedNode;
}

void Canavar::Engine::Gui::setSelectedNode(Canavar::Engine::Node *newSelectedNode)
{
    if (mSelectedNode)
    {
        mSelectedNode->disconnect(this);

        if (mDrawAllBBs)
            RendererManager::instance()->addSelectableNode(mSelectedNode, QVector4D(1, 1, 1, 1));
        else
            RendererManager::instance()->removeSelectableNode(mSelectedNode);
    }

    mSelectedNode = newSelectedNode;

    if (mSelectedNode)
    {
        RendererManager::instance()->addSelectableNode(mSelectedNode, QVector4D(1, 0, 0, 1));
        connect(mSelectedNode, &QObject::destroyed, this, [=]() { setSelectedNode(nullptr); });
    }

    mSelectedModel = dynamic_cast<Model *>(mSelectedNode);
    setSelectedMesh(nullptr);
}

Canavar::Engine::Mesh *Canavar::Engine::Gui::getSelectedMesh() const
{
    return mSelectedMesh;
}

void Canavar::Engine::Gui::setSelectedMesh(Canavar::Engine::Mesh *newSelectedMesh)
{
    if (mSelectedMesh)
        RendererManager::instance()->removeSelectedMesh(mSelectedModel);

    mSelectedMesh = newSelectedMesh;

    if (mSelectedMesh)
    {
        SelectedMeshParameters parameters;
        parameters.mMesh = mSelectedMesh;

        RendererManager::instance()->addSelectedMesh(mSelectedModel, parameters);
    }
}

int Canavar::Engine::Gui::getSelectedVertexIndex() const
{
    return mSelectedVertexIndex;
}

void Canavar::Engine::Gui::setSelectedVertexIndex(int newSelectedVertexIndex)
{
    mSelectedVertexIndex = newSelectedVertexIndex;

    if (mSelectedModel)
        RendererManager::instance()->getSelectedMeshParameters_Ref(mSelectedModel).mSelectedVertexID = mSelectedVertexIndex;
}

void Canavar::Engine::Gui::mousePressed(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    if (mVertexSelectionEnabled)
    {
        auto info = SelectableNodeRenderer::instance()->getVertexInfoByScreenPosition(event->position().x(), event->position().y());

        if (info.success)
            if (mSelectedModel == NodeManager::instance()->getNodeByID(info.nodeID))
                if (auto data = ModelDataManager::instance()->getModelData(mSelectedModel->getModelName()))
                    if (data->getMeshByID(info.meshID) == mSelectedMesh)
                        setSelectedVertexIndex(info.vertexID);

    } else if (mMeshSelectionEnabled)
    {
        auto info = SelectableNodeRenderer::instance()->getNodeInfoByScreenPosition(event->position().x(), event->position().y());

        if (info.success)
            if (mSelectedModel == NodeManager::instance()->getNodeByID(info.nodeID))
                if (auto data = ModelDataManager::instance()->getModelData(mSelectedModel->getModelName()))
                    setSelectedMesh(data->getMeshByID(info.meshID));

    } else if (mNodeSelectionEnabled)
    {
        auto info = SelectableNodeRenderer::instance()->getNodeInfoByScreenPosition(event->position().x(), event->position().y());

        if (info.success)
            setSelectedNode(NodeManager::instance()->getNodeByID(info.nodeID));
        else
            setSelectedNode(nullptr);
    }
}
