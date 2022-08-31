#include "NodeManager.h"
#include "CameraManager.h"
#include "DummyCamera.h"
#include "FreeCamera.h"
#include "LightManager.h"
#include "Model.h"
#include "NozzleParticles.h"
#include "RendererManager.h"

NodeManager::NodeManager(QObject *parent)
    : Manager(parent)
    , mNumberOfNodes(0)
    , mSelectedNode(nullptr)
    , mSelectedModel(nullptr)
    , mSelectedMesh(nullptr)
    , mSelectedVertexIndex(-1)
    , mMeshSelectionEnabled(false)
    , mVertexSelectionEnabled(false)
    , mMeshLockEnabled(false)

{}

bool NodeManager::init()
{
    mCameraManager = CameraManager::instance();
    mLightManager = LightManager::instance();
    mRendererManager = RendererManager::instance();

    return true;
}

Node *NodeManager::create(Node::NodeType type, const QString &name)
{
    Node *node = nullptr;
    switch (type)
    {
    case Node::NodeType::DummyNode: {
        node = new Node;
        break;
    }
    case Node::NodeType::Model: {
        node = new Model(name);
        break;
    }
    case Node::NodeType::FreeCamera: {
        node = new FreeCamera;
        mCameraManager->addCamera(dynamic_cast<FreeCamera *>(node));
        break;
    }
    case Node::NodeType::DummyCamera: {
        node = new DummyCamera;
        mCameraManager->addCamera(dynamic_cast<DummyCamera *>(node));
        break;
    }
    case Node::NodeType::DirectionalLight: {
        node = new DirectionalLight;
        mLightManager->setDirectionalLight(dynamic_cast<DirectionalLight *>(node));
        break;
    }
    case Node::NodeType::PointLight: {
        node = new PointLight;
        mLightManager->addLight(dynamic_cast<PointLight *>(node));
        break;
    }
    case Node::NodeType::SpotLight: {
        node = new SpotLight;
        mLightManager->addLight(dynamic_cast<SpotLight *>(node));
        break;
    }
    case Node::NodeType::NozzleParticles: {
        node = new NozzleParticles;
        dynamic_cast<NozzleParticles *>(node)->create();
        break;
    }
    default: {
        qWarning() << Q_FUNC_INFO << "Implement creation algorithm for this NodeType:" << (int) type;
        return nullptr;
    }
    }

    if (node)
    {
        mNumberOfNodes++;

        if (name.isEmpty())
            node->setName(QString("%1 #%2").arg(node->nodeTypeString()).arg(mNumberOfNodes));
        else
            node->setName(name);

        node->setIndex(mNumberOfNodes);
        mNodes << node;
    }

    return node;
}

void NodeManager::removeNode(Node *node)
{
    // FIXME: If a node is removed, remove also its all children
    Node *parent = dynamic_cast<Node *>(node->parent());

    switch (node->nodeType())
    {
    case Node::NodeType::Model:
    case Node::NodeType::DummyNode:
    case Node::NodeType::NozzleParticles: {
        if (parent)
            parent->removeChild(node);
        mNodes.removeAll(node);
        node->deleteLater();
        break;
    }
    case Node::NodeType::DummyCamera:
    case Node::NodeType::FreeCamera: {
        if (parent)
            parent->removeChild(node);

        mCameraManager->removeCamera(dynamic_cast<Camera *>(node));
        mNodes.removeAll(node);
        node->deleteLater();
        break;
    }
    case Node::NodeType::DirectionalLight: {
        if (parent)
            parent->removeChild(node);
        DirectionalLight *light = dynamic_cast<DirectionalLight *>(node);
        mLightManager->setDirectionalLight(nullptr);
        mNodes.removeAll(node);
        light->deleteLater();
    }
    case Node::NodeType::PointLight:
    case Node::NodeType::SpotLight: {
        if (parent)
            parent->removeChild(node);
        Light *light = dynamic_cast<Light *>(node);
        mLightManager->removeLight(light);
        mNodes.removeAll(node);
        light->deleteLater();
        break;
    }
    default: {
        qWarning() << Q_FUNC_INFO << "Unkown Node. Implement creation algorithm for this Node";
    }
    }
}

void NodeManager::setSelectedNode(Node *node)
{
    if (mSelectedNode == node)
        return;

    if (node)
        node->setSelected(true);

    if (mSelectedNode)
        mSelectedNode->setSelected(false);

    mSelectedNode = node;

    mSelectedModel = dynamic_cast<Model *>(mSelectedNode);

    if (mSelectedModel)
        mSelectedModelMeshes = mRendererManager->getModelData(mSelectedModel->modelName())->meshes();

    setSelectedMesh(nullptr);
    mVertexSelectionEnabled = false;
    mMeshSelectionEnabled = false;
    mMeshLockEnabled = false;
}

void NodeManager::setSelectedNode(unsigned int nodeIndex)
{
    for (auto node : qAsConst(mNodes))
    {
        if (node->index() == nodeIndex)
        {
            setSelectedNode(node);
            return;
        }
    }

    setSelectedNode(nullptr);
}

void NodeManager::setSelectedMesh(unsigned int meshIndex)
{
    if (mSelectedModel)
    {
        auto data = mRendererManager->getModelData(mSelectedModel->modelName());
        auto meshes = data->meshes();

        for (auto mesh : meshes)
        {
            if (mesh->index() == meshIndex)
            {
                setSelectedMesh(mesh);
                return;
            }
        }
    }
}

void NodeManager::setSelectedMesh(Mesh *mesh)
{
    if (mSelectedMesh == mesh)
        return;

    if (mesh)
        mesh->setSelected(true);

    if (mSelectedMesh)
        mSelectedMesh->setSelected(false);

    mSelectedMesh = mesh;
}

void NodeManager::setSelectedVertexIndex(int index)
{
    mSelectedVertexIndex = index;

    if (mSelectedMesh)
        mSelectedMesh->setSelectedVertexIndex(mSelectedVertexIndex);
}

NodeManager *NodeManager::instance()
{
    static NodeManager instance;
    return &instance;
}

const QList<Node *> &NodeManager::nodes() const
{
    return mNodes;
}

void NodeManager::drawGUI()
{
    ImGui::SetNextWindowSize(ImVec2(420, 820), ImGuiCond_FirstUseEver);
    ImGui::Begin("Nodes");

    QString preview = mSelectedNode ? mSelectedNode->name() : "-";
    if (ImGui::BeginCombo("Select a node", preview.toStdString().c_str()))
    {
        for (int i = 0; i < mNodes.size(); ++i)
            populateNodesComboBox(mNodes[i]);

        ImGui::EndCombo();
    }

    if (mSelectedNode)
    {
        ImGui::Text("Type: %s", mSelectedNode->nodeTypeString().toStdString().c_str());
        ImGui::Text("Index: %d", mSelectedNode->index());
        Node *parent = dynamic_cast<Node *>(mSelectedNode->parent());
        ImGui::Text("Parent: 0x%p", parent);

        if (parent)
            ImGui::Text("Parent Name: %s", parent->name().toStdString().c_str());

        mSelectedNode->drawGUI();
    }

    if (mSelectedModel)
    {
        ImGui::Checkbox("Mesh selection", &mMeshSelectionEnabled);

        if (mMeshSelectionEnabled)
        {
            if (!ImGui::CollapsingHeader("Meshes"))
            {
                QString preview = mSelectedMesh ? mSelectedMesh->name() : "-";
                if (ImGui::BeginCombo("Select a mesh", preview.toStdString().c_str()))
                {
                    for (int i = 0; i < mSelectedModelMeshes.size(); ++i)
                        populateMeshesComboBox(mSelectedModelMeshes[i]);

                    ImGui::EndCombo();
                }

                if (mSelectedMesh)
                    mSelectedMesh->drawGUI();

                ImGui::BeginDisabled(!mSelectedMesh);
                ImGui::Checkbox("Lock mesh", &mMeshLockEnabled);
                ImGui::Checkbox("Vertex selection", &mVertexSelectionEnabled);
                ImGui::EndDisabled();

                if (mSelectedMesh)
                    mSelectedMesh->drawGUIForVertex();
            }
        }
    }

    if (!mMeshSelectionEnabled)
    {
        setSelectedMesh(nullptr);
        mVertexSelectionEnabled = false;
    }

    if (!mVertexSelectionEnabled)
        setSelectedVertexIndex(-1);

    ImGui::End();
}

void NodeManager::populateNodesComboBox(Node *node)
{
    switch (node->nodeType())
    {
    case Node::NodeType::DirectionalLight:
        return;
    default:
        break;
    }

    if (ImGui::Selectable(node->name().toStdString().c_str()))
        setSelectedNode(node);
}

void NodeManager::populateMeshesComboBox(Mesh *mesh)
{
    if (ImGui::Selectable(mesh->name().toStdString().c_str()))
    {
        mMeshSelectionEnabled = true;
        mMeshLockEnabled = false;
        setSelectedMesh(mesh);
    }
}

bool NodeManager::meshLockEnabled() const
{
    return mMeshLockEnabled;
}

bool NodeManager::meshSelectionEnabled() const
{
    return mMeshSelectionEnabled;
}

bool NodeManager::vertexSelectionEnabled() const
{
    return mVertexSelectionEnabled;
}

Model *NodeManager::selectedModel() const
{
    return mSelectedModel;
}

Mesh *NodeManager::selectedMesh() const
{
    return mSelectedMesh;
}

Node *NodeManager::selectedNode() const
{
    return mSelectedNode;
}
