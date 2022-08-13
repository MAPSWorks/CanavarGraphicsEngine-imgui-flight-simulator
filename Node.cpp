#include "Node.h"

Node::Node(QObject *parent)
    : QObject{parent}
    , mPosition(0, 0, 0)
    , mScale(1, 1, 1)

{}

Node::~Node() {}

const QVector3D &Node::position() const
{
    return mPosition;
}

void Node::setPosition(const QVector3D &newPosition)
{
    mPosition = newPosition;
}

QVector3D Node::worldPosition() const
{
    Node *parent = dynamic_cast<Node *>(this->parent());

    if (parent)
        return parent->worldPosition() + mPosition;
    else
        return mPosition;
}

void Node::setWorldPosition(const QVector3D &newWorldPosition)
{
    Node *parent = dynamic_cast<Node *>(this->parent());

    if (parent)
    {
        QVector3D parentWorldPosition = parent->worldPosition();
        mPosition = newWorldPosition - parentWorldPosition;
    } else
    {
        mPosition = newWorldPosition;
    }
}

const QQuaternion &Node::rotation() const
{
    return mRotation;
}

void Node::setRotation(const QQuaternion &newRotation)
{
    mRotation = newRotation;
}

QQuaternion Node::worldRotation() const
{
    Node *parent = dynamic_cast<Node *>(this->parent());

    if (parent)
        return parent->worldRotation() * mRotation;
    else
        return mRotation;
}

void Node::setWorldRotation(const QQuaternion &newWorldRotation)
{
    Node *parent = dynamic_cast<Node *>(this->parent());

    if (parent)
    {
        QQuaternion parentWorldRotation = parent->worldRotation();
        mRotation = parentWorldRotation.inverted() * newWorldRotation;
    } else
    {
        mRotation = newWorldRotation;
    }
}

const QVector3D &Node::scale() const
{
    return mScale;
}

void Node::setScale(const QVector3D &newScale)
{
    mScale = newScale;
}

QMatrix4x4 Node::transformation() const
{
    QMatrix4x4 transformation;
    transformation.scale(mScale);
    transformation.rotate(mRotation);
    transformation.setColumn(3, QVector4D(mPosition, 1.0f));

    return transformation;
}

QMatrix4x4 Node::worldTransformation() const
{
    Node *parent = dynamic_cast<Node *>(this->parent());

    if (parent)
    {
        QVector3D worldPosition = this->worldPosition();
        QQuaternion worldRotation = this->worldRotation();

        QMatrix4x4 worldTransformation;
        worldTransformation.scale(mScale);
        worldTransformation.rotate(worldRotation);
        worldTransformation.setColumn(3, QVector4D(worldPosition, 1.0f));

        return worldTransformation;
    } else
    {
        return transformation();
    }
}

const QString &Node::name() const
{
    return mName;
}

void Node::setName(const QString &newName)
{
    mName = newName;
}

void Node::addChild(Node *child)
{
    child->setParent(this);
    mChildren << child;
}

void Node::removeChild(Node *child)
{
    mChildren.removeAll(child);
    child->setParent(nullptr);
    child->deleteLater();
}

const QList<Node *> &Node::children() const
{
    return mChildren;
}
