#ifndef CAMERA_H
#define CAMERA_H

#include "Node.h"

#include <QMatrix4x4>
#include <QMouseEvent>
#include <QTimer>

class Camera : public Node
{
    Q_OBJECT
protected:
    explicit Camera(QObject *parent = nullptr);

signals:
    void activeChanged(bool active);

public:
    virtual float verticalFov() const;
    virtual void setVerticalFov(float newVerticalFov);

    virtual float horizontalFov() const;
    virtual void setHorizontalFov(float newHorizontalFov);

    virtual float aspectRatio() const;
    virtual void setAspectRatio(float newAspectRatio);

    virtual float zNear() const;
    virtual void setZNear(float newZNear);

    virtual float zFar() const;
    virtual void setZFar(float newZFar);

    virtual QMatrix4x4 transformation() const;
    virtual QMatrix4x4 worldTransformation() const;
    virtual QMatrix4x4 projection() const;
    virtual QVector3D getViewDirection();

    virtual void onKeyPressed(QKeyEvent *) = 0;
    virtual void onKeyReleased(QKeyEvent *) = 0;
    virtual void onMousePressed(QMouseEvent *event) = 0;
    virtual void onMouseReleased(QMouseEvent *event) = 0;
    virtual void onMouseMoved(QMouseEvent *event) = 0;
    virtual void update(float) = 0;

    bool active() const;
    void setActive(bool newActive);

protected:
    float mVerticalFov;
    float mHorizontalFov;
    float mAspectRatio;
    float mZNear;
    float mZFar;

    bool mActive;
};

#endif // CAMERA_H
