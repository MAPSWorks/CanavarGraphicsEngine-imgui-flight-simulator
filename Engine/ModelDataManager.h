#ifndef MODELDATAMANAGER_H
#define MODELDATAMANAGER_H

#include "Manager.h"
#include "ModelData.h"

#include <QMap>

namespace Canavar {
namespace Engine {

class ModelDataManager : public Manager
{
private:
    explicit ModelDataManager(QObject *parent = nullptr);

public:
    static ModelDataManager *instance();

    bool init();

    ModelData *getModelData(const QString &modelName);

private:
    void loadModels(const QString &path, const QStringList &formats);

private:
    QMap<QString, ModelData *> mModelsData;
};

} // namespace Engine
} // namespace Canavar

#endif // MODELDATAMANAGER_H
