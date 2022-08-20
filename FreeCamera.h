#ifndef FREECAMERA_H
#define FREECAMERA_H

#include "Camera.h"

#include <QObject>

class FreeCamera : public Camera
{
    Q_OBJECT

private:
    friend class NodeManager;
    explicit FreeCamera(QObject *parent = nullptr);
    virtual ~FreeCamera();

public:
    void onKeyPressed(QKeyEvent *event) override;
    void onKeyReleased(QKeyEvent *event) override;
    void onMousePressed(QMouseEvent *event) override;
    void onMouseReleased(QMouseEvent *event) override;
    void onMouseMoved(QMouseEvent *event) override;
    void update(float ifps) override;

    enum class Mode { //
        RotateWhileMouseIsPressing,
        RotateWhileMouseIsMoving
    };

    bool getMouseGrabbed() const;

signals:
    emit void mouseGrabbed(bool grabbed);
    emit void setCursorPosition(QPoint mouseGrabPosition);

private:
    QMap<Qt::Key, bool> mPressedKeys;

    float mMovementSpeed;
    float mAngularSpeed;

    bool mMousePressed;
    float mMousePreviousX;
    float mMousePreviousY;
    float mMouseDeltaX;
    float mMouseDeltaY;

    bool mUpdateRotation;
    bool mUpdatePosition;

    Mode mMode;

    bool mMouseGrabbed;
    QPoint mMouseGrabPosition;

    static const QMap<Qt::Key, QVector3D> KEY_BINDINGS;
};

#endif // FREECAMERA_H
