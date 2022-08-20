#ifndef NODEMANAGER_H
#define NODEMANAGER_H

#include "Node.h"

#include <QObject>

class CameraManager;
class LightManager;
class NodeManager : public QObject
{
    Q_OBJECT

private:
    explicit NodeManager(QObject *parent = nullptr);

public:
    Node *create(Node::NodeType type, const QString &name = QString());
    void removeNode(Node *node);

    const QList<Node *> &nodes() const;
    static NodeManager *instance();

private:
    QList<Node *> mNodes;
    int mNumberOfNodes;
    CameraManager *mCameraManager;
    LightManager *mLightManager;
};

#endif // NODEMANAGER_H
