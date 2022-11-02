#ifndef LIGHT_H
#define LIGHT_H

#include "Node.h"

namespace Canavar {
namespace Engine {

class Light : public Node
{
protected:
    explicit Light(QObject *parent = nullptr);

    DEFINE_MEMBER(QVector4D, Color)
    DEFINE_MEMBER(float, Ambient)
    DEFINE_MEMBER(float, Diffuse)
    DEFINE_MEMBER(float, Specular)
};

} // namespace Engine
} // namespace Canavar
#endif // LIGHT_H
