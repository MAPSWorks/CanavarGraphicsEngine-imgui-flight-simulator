#include "Window.h"
#include "qmath.h"

#include <imgui.h>
#include <QDateTime>
#include <QKeyEvent>
#include <QtImGui.h>

Window::Window(QWindow *parent)
    : QOpenGLWindow(QOpenGLWindow::UpdateBehavior::NoPartialUpdate, parent)
    , mSelectedNode(nullptr)
{
    mRendererManager = RendererManager::instance();
    mLightManager = LightManager::instance();
    mNodeManager = NodeManager::instance();
    mTerrain = Terrain::instance();
    mRandomGenerator = QRandomGenerator::securelySeeded();

    QSurfaceFormat format;
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(16);
    format.setSwapInterval(1);
    setFormat(format);

    connect(this, &QOpenGLWindow::frameSwapped, this, [=]() { update(); });
}

void Window::initializeGL()
{
    initializeOpenGLFunctions();

    QtImGui::initialize(this);

    emit init();

    mCurrentTime = QDateTime::currentMSecsSinceEpoch();
    mPreviousTime = mCurrentTime;
}

void Window::resizeGL(int w, int h)
{
    emit resized(w, h);
}

void Window::paintGL()
{
    mSun = mLightManager->directionalLight();
    mNodes = mNodeManager->nodes();

    mAileron = mAircraftController->aileron();
    mElevator = mAircraftController->elevator();
    mRudder = mAircraftController->rudder();
    mThrottle = mAircraftController->throttle();

    mRenderObjects = mRendererManager->renderObjects();
    mRenderWireframe = mRendererManager->renderWireframe();
    mRenderNormals = mRendererManager->renderNormals();

    mFog = mRendererManager->fog();
    mTerrainProperties = mTerrain->properties();
    mTerrainMaterial = mTerrain->material();

    mCurrentTime = QDateTime::currentMSecsSinceEpoch();
    float ifps = (mCurrentTime - mPreviousTime) * 0.001f;
    mPreviousTime = mCurrentTime;

    emit render(ifps);

    mImguiWantCapture = ImGui::GetIO().WantCaptureMouse;

    QtImGui::newFrame();
    ImGui::SetNextWindowSize(ImVec2(420, 820), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls");

    // Render Settings
    if (!ImGui::CollapsingHeader("Render Settings"))
    {
        if (ImGui::Checkbox("Render Objects", &mRenderObjects))
            mRendererManager->setRenderObjects(mRenderObjects);

        if (ImGui::Checkbox("Wireframe", &mRenderWireframe))
            mRendererManager->setRenderWireframe(mRenderWireframe);

        if (ImGui::Checkbox("Render Normals", &mRenderNormals))
            mRendererManager->setRenderNormals(mRenderNormals);
    }

    // Light
    if (!ImGui::CollapsingHeader("Sun"))
    {
        {
            ImGui::Text("Direction:");
            QVector3D direction = mSun->direction();
            float x = direction.x();
            float y = direction.y();
            float z = direction.z();
            float r = direction.length();
            float theta = qRadiansToDegrees(atan2(z, x));
            float phi = qRadiansToDegrees(atan2(y, sqrt(z * z + x * x)));
            if (qFuzzyCompare(abs(phi), 90.0f))
                theta = 0.0f;
            ImGui::SliderFloat("Theta", &theta, -180.0f, 180.0f, "%.1f");
            ImGui::SliderFloat("Phi", &phi, -90.0f, 90.0f, "%.1f");

            x = r * cos(qDegreesToRadians(phi)) * cos(qDegreesToRadians(theta));
            y = r * sin(qDegreesToRadians(phi));
            z = r * cos(qDegreesToRadians(phi)) * sin(qDegreesToRadians(theta));

            mSun->setDirection(QVector3D(x, y, z));
        }

        {
            ImGui::Text("Shading Parameters:");
            float ambient = mSun->ambient();
            float diffuse = mSun->diffuse();
            float specular = mSun->specular();

            if (ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f, "%.3f"))
                mSun->setAmbient(ambient);

            if (ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f, "%.3f"))
                mSun->setDiffuse(diffuse);

            if (ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f, "%.3f"))
                mSun->setSpecular(specular);

            float color[4] = {mSun->color().x(), mSun->color().y(), mSun->color().z(), mSun->color().w()};

            if (ImGui::ColorEdit4("Color##Light", (float *) &color))
                mSun->setColor(QVector4D(color[0], color[1], color[2], color[3]));
        }
    }

    ImGui::Spacing();

    // Fog
    if (!ImGui::CollapsingHeader("Fog"))
    {
        if (ImGui::SliderFloat("Density##Fog", &mFog.density, 0.0f, 4.0f, "%.3f"))
            mRendererManager->setFog(mFog);

        if (ImGui::SliderFloat("Gradient##Fog", &mFog.gradient, 0.0f, 4.0f, "%.3f"))
            mRendererManager->setFog(mFog);

        float color[3] = {mFog.color.x(), mFog.color.y(), mFog.color.z()};

        if (ImGui::ColorEdit3("Color##Fog", (float *) &color))
        {
            mFog.color = QVector3D(color[0], color[1], color[2]);
            mRendererManager->setFog(mFog);
        }

        if (ImGui::Checkbox("Enabled##Fog", &mFog.enabled))
            mRendererManager->setFog(mFog);

        if (ImGui::Button("Reset##Fog"))
            mRendererManager->resetFog();
    }

    ImGui::Spacing();

    // Fog
    if (!ImGui::CollapsingHeader("Terrain"))
    {
        if (ImGui::SliderFloat("Amplitude##Terrain", &mTerrainProperties.amplitude, 0.0f, 50.0f, "%.3f"))
            mTerrain->setProperties(mTerrainProperties);

        if (ImGui::SliderInt("Octaves##Terrain", &mTerrainProperties.octaves, 1, 20))
            mTerrain->setProperties(mTerrainProperties);

        if (ImGui::SliderFloat("Power##Terrain", &mTerrainProperties.power, 0.1f, 10.0f, "%.3f"))
            mTerrain->setProperties(mTerrainProperties);

        if (ImGui::SliderFloat("Tessellation Multiplier##Terrain", &mTerrainProperties.tessellationMultiplier, 0.1f, 10.0f, "%.3f"))
            mTerrain->setProperties(mTerrainProperties);

        if (ImGui::SliderFloat("Grass Coverage##Terrain", &mTerrainProperties.grassCoverage, 0.0f, 1.0f, "%.3f"))
            mTerrain->setProperties(mTerrainProperties);

        if (ImGui::SliderFloat("Ambient##Terrain", &mTerrainMaterial.ambient, 0.0f, 1.0f, "%.3f"))
            mTerrain->setMaterial(mTerrainMaterial);

        if (ImGui::SliderFloat("Diffuse##Terrain", &mTerrainMaterial.diffuse, 0.0f, 1.0f, "%.3f"))
            mTerrain->setMaterial(mTerrainMaterial);

        if (ImGui::SliderFloat("Specular##Terrain", &mTerrainMaterial.specular, 0.0f, 1.0f, "%.3f"))
            mTerrain->setMaterial(mTerrainMaterial);

        if (ImGui::SliderFloat("Shininess##Terrain", &mTerrainMaterial.shininess, 0.1f, 128.0f, "%.3f"))
            mTerrain->setMaterial(mTerrainMaterial);

        if (ImGui::Button("Generate Seed##Terrain"))
        {
            mTerrainProperties.seed = getRandomSeed();
            mTerrain->setProperties(mTerrainProperties);
        }

        if (ImGui::Button("Reset##Terrain"))
            mTerrain->reset();
    }

    ImGui::Spacing();

    // Nodes
    if (!ImGui::CollapsingHeader("Select a Node"))
    {
        QString preview = mSelectedNode ? mSelectedNode->name() : "-";
        if (ImGui::BeginCombo("Nodes", preview.toStdString().c_str()))
        {
            for (int i = 0; i < mNodes.size(); ++i)
                populateComboBox(mNodes[i]);

            ImGui::EndCombo();
        }

        // Metadata
        if (mSelectedNode)
        {
            ImGui::Text("Type: %s", mSelectedNode->nodeTypeString().toStdString().c_str());
            ImGui::Text("Parent: 0x%p", static_cast<void *>(mSelectedNode->parent()));
        }

        // Position
        if (mSelectedNode)
        {
            {
                ImGui::Text("Position:");
                QVector3D position = mSelectedNode->position();
                float x = position.x();
                float y = position.y();
                float z = position.z();

                if (ImGui::SliderFloat("x##NodePosition", &x, -100.0f, 100.0f, "%.3f"))
                    mSelectedNode->setPosition(QVector3D(x, y, z));
                if (ImGui::SliderFloat("y##NodePosition", &y, -100.0f, 100.0f, "%.3f"))
                    mSelectedNode->setPosition(QVector3D(x, y, z));
                if (ImGui::SliderFloat("z##NodePosition", &z, -100.0f, 100.0f, "%.3f"))
                    mSelectedNode->setPosition(QVector3D(x, y, z));
            }

            {
                ImGui::Text("World Position:");
                QVector3D worldPosition = mSelectedNode->worldPosition();
                float x = worldPosition.x();
                float y = worldPosition.y();
                float z = worldPosition.z();

                if (ImGui::SliderFloat("x##NodeWorldPosition", &x, -100.0f, 100.0f, "%.3f"))
                    mSelectedNode->setWorldPosition(QVector3D(x, y, z));
                if (ImGui::SliderFloat("y##NodeWorldPosition", &y, -100.0f, 100.0f, "%.3f"))
                    mSelectedNode->setWorldPosition(QVector3D(x, y, z));
                if (ImGui::SliderFloat("z##NodeWorldPosition", &z, -100.0f, 100.0f, "%.3f"))
                    mSelectedNode->setWorldPosition(QVector3D(x, y, z));
            }
        }

        // Rotation
        if (mSelectedNode)
        {
            {
                ImGui::Text("Rotation:");
                QQuaternion rotation = mSelectedNode->rotation();
                QVector3D eulerAngles = rotation.toEulerAngles();
                float pitch = eulerAngles.x();
                float yaw = eulerAngles.y();
                float roll = eulerAngles.z();

                if (ImGui::SliderFloat("Pitch##NodeRotation", &pitch, -90.0f, 90.0f, "%.3f"))
                    mSelectedNode->setRotation(QQuaternion::fromEulerAngles(QVector3D(pitch, yaw, roll)));
                if (ImGui::SliderFloat("Yaw##NodeRotation", &yaw, -180.0f, 180.0f, "%.3f"))
                    mSelectedNode->setRotation(QQuaternion::fromEulerAngles(QVector3D(pitch, yaw, roll)));
                if (ImGui::SliderFloat("Roll##NodeRotation", &roll, -180.0f, 180.0f, "%.3f"))
                    mSelectedNode->setRotation(QQuaternion::fromEulerAngles(QVector3D(pitch, yaw, roll)));
            }

            {
                ImGui::Text("World Rotation:");
                QQuaternion rotation = mSelectedNode->worldRotation();
                QVector3D eulerAngles = rotation.toEulerAngles();
                float pitch = eulerAngles.x();
                float yaw = eulerAngles.y();
                float roll = eulerAngles.z();

                if (ImGui::SliderFloat("Pitch##NodeWorldRotation", &pitch, -90.0f, 90.0f, "%.3f"))
                    mSelectedNode->setWorldRotation(QQuaternion::fromEulerAngles(QVector3D(pitch, yaw, roll)));
                if (ImGui::SliderFloat("Yaw##NodeWorldRotation", &yaw, -180.0f, 180.0f, "%.3f"))
                    mSelectedNode->setWorldRotation(QQuaternion::fromEulerAngles(QVector3D(pitch, yaw, roll)));
                if (ImGui::SliderFloat("Roll##NodeWorldRotation", &roll, -180.0f, 180.0f, "%.3f"))
                    mSelectedNode->setWorldRotation(QQuaternion::fromEulerAngles(QVector3D(pitch, yaw, roll)));
            }
        }

        // Shading Parameters
        //        {

        //            ImGui::Text("Shading Parameters:");
        //            float ambient = mSelectedNode->material().ambient();
        //            float diffuse = mSelectedNode->material().diffuse();
        //            float specular = mSelectedNode->material().specular();

        //            if (ImGui::SliderFloat("Ambient##Model", &ambient, 0.0f, 1.0f, "%.3f"))
        //                mSelectedModel->material().setAmbient(ambient);

        //            if (ImGui::SliderFloat("Diffuse##Model", &diffuse, 0.0f, 1.0f, "%.3f"))
        //                mSelectedModel->material().setDiffuse(diffuse);

        //            if (ImGui::SliderFloat("Specular##Model", &specular, 0.0f, 1.0f, "%.3f"))
        //                mSelectedModel->material().setSpecular(specular);

        //            float color[4] = {mSelectedModel->material().color().x(), //
        //                              mSelectedModel->material().color().y(),
        //                              mSelectedModel->material().color().z(),
        //                              mSelectedModel->material().color().w()};

        //            if (ImGui::ColorEdit4("Color##Model", (float *) &color))
        //                mSelectedModel->material().setColor(QVector4D(color[0], color[1], color[2], color[3]));
        //        }
    }

    ImGui::Spacing();

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    // Simulator
    {
        ImGui::SetNextWindowSize(ImVec2(420, 820), ImGuiCond_FirstUseEver);
        ImGui::Begin("Simulator", NULL, ImGuiWindowFlags_MenuBar);

        ImGui::BeginDisabled();

        ImGui::SliderFloat("Elevator", &mElevator, -1.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Aileron", &mAileron, -1.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Rudder", &mRudder, -1.0f, 1.0f, "%.3f");
        ImGui::SliderFloat("Throttle", &mThrottle, 0.0f, 1.0f, "%.3f");

        ImGui::EndDisabled();

        if (ImGui::Button("Init Running"))
            emit command(Aircraft::Command::InitRunning);

        if (ImGui::Button("Hold"))
            emit command(Aircraft::Command::Hold);

        if (ImGui::Button("Resume"))
            emit command(Aircraft::Command::Resume);

        ImGui::Text("Airspeed:    %.2f knots", mPfd.airspeed);
        ImGui::Text("Latitude:    %.6f °", mPfd.latitude);
        ImGui::Text("Longitude:   %.6f °", mPfd.longitude);
        ImGui::Text("Altitude:    %.2f meters", mPfd.altitude);
        ImGui::Text("Roll:        %.1f °", mPfd.roll);
        ImGui::Text("Pitch:       %.1f °", mPfd.pitch);
        ImGui::Text("Heading:     %.1f °", mPfd.heading);
    }

    glViewport(0, 0, width(), height());
    ImGui::Render();
    QtImGui::render();
}

void Window::keyPressEvent(QKeyEvent *event)
{
    emit keyPressed(event);
}

void Window::keyReleaseEvent(QKeyEvent *event)
{
    emit keyReleased(event);
}

void Window::mousePressEvent(QMouseEvent *event)
{
    emit mousePressed(event);
}

void Window::mouseReleaseEvent(QMouseEvent *event)
{
    emit mouseReleased(event);
}

void Window::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMoved(event);
}

void Window::wheelEvent(QWheelEvent *event)
{
    emit wheelMoved(event);
}

void Window::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit mouseDoubleClicked(event);
}

void Window::populateComboBox(Node *node)
{
    switch (node->nodeType())
    {
    case Node::NodeType::FreeCamera:
    case Node::NodeType::DummyCamera:
    case Node::NodeType::DirectionalLight:
        return;
    default:
        break;
    }

    if (ImGui::Selectable(node->name().toStdString().c_str()))
        mSelectedNode = node;

    for (auto child : node->children())
        populateComboBox(child);
}

QVector3D Window::getRandomSeed()
{
    float x = mRandomGenerator.generateDouble();
    float y = mRandomGenerator.generateDouble();
    float z = mRandomGenerator.generateDouble();

    return QVector3D(x, y, z);
}

void Window::setAircraftController(AircraftController *newAircraftController)
{
    mAircraftController = newAircraftController;
}

void Window::onPfdChanged(Aircraft::PrimaryFlightData pfd)
{
    mPfd = pfd;
}

bool Window::imguiWantCapture() const
{
    return mImguiWantCapture;
}
