#ifndef MU_PROJECT_AUDIOINPUTPARAMSJSON_H
#define MU_PROJECT_AUDIOINPUTPARAMSJSON_H

#include <QJsonObject>

#include "audio/common/audioutils.h"

namespace mu::project::audioSettingsJson {
using namespace muse::audio;

inline AudioResourceType resourceTypeFromString(const QString& string)
{
    for (const auto& pair : RESOURCE_TYPE_MAP) {
        if (pair.second == string) {
            return pair.first;
        }
    }

    return AudioResourceType::Undefined;
}

inline AudioResourceAttributes attributesFromJson(const QJsonObject& object)
{
    AudioResourceAttributes result;

    for (const QString& key : object.keys()) {
        result.emplace(muse::String::fromQString(key), muse::String::fromQString(object.value(key).toString()));
    }

    return result;
}

inline AudioUnitConfig unitConfigFromJson(const QJsonObject& object)
{
    AudioUnitConfig result;

    for (const QString& key : object.keys()) {
        QByteArray base = QByteArray::fromBase64(object.value(key).toString().toUtf8());
        result.emplace(key.toStdString(), base.toStdString());
    }

    return result;
}

inline AudioResourceMeta resourceMetaFromJson(const QJsonObject& object)
{
    AudioResourceMeta result;
    result.id = object.value("id").toString().toStdString();
    result.hasNativeEditorSupport = object.value("hasNativeEditorSupport").toBool();
    result.vendor = object.value("vendor").toString().toStdString();
    result.type = resourceTypeFromString(object.value("type").toString());
    result.attributes = attributesFromJson(object.value("attributes").toObject());

    return result;
}

inline AudioInputParams inputParamsFromJson(const QJsonObject& object)
{
    AudioInputParams result;
    result.resourceMeta = resourceMetaFromJson(object.value("resourceMeta").toObject());
    result.configuration = unitConfigFromJson(object.value("unitConfiguration").toObject());

    return result;
}

inline QJsonObject attributesToJson(const AudioResourceAttributes& attributes)
{
    QJsonObject result;

    for (const auto& pair : attributes) {
        result.insert(pair.first.toQString(), pair.second.toQString());
    }

    return result;
}

inline QJsonObject unitConfigToJson(const AudioUnitConfig& config)
{
    QJsonObject result;

    for (const auto& pair : config) {
        QByteArray byteArray = QByteArray::fromRawData(pair.second.c_str(), static_cast<int>(pair.second.size()));
        result.insert(QString::fromStdString(pair.first), QString(byteArray.toBase64()));
    }

    return result;
}

inline QJsonObject resourceMetaToJson(const AudioResourceMeta& meta)
{
    QJsonObject result;
    result.insert("id", QString::fromStdString(meta.id));
    result.insert("hasNativeEditorSupport", meta.hasNativeEditorSupport);
    result.insert("vendor", QString::fromStdString(meta.vendor));
    result.insert("type", audioResourceTypeToString(meta.type).toQString());
    result.insert("attributes", attributesToJson(meta.attributes));

    return result;
}

inline QJsonObject inputParamsToJson(const AudioInputParams& params)
{
    QJsonObject result;
    result.insert("resourceMeta", resourceMetaToJson(params.resourceMeta));
    result.insert("unitConfiguration", unitConfigToJson(params.configuration));

    return result;
}
}

#endif
