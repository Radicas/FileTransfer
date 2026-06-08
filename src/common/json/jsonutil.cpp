/**
 * @file jsonutil.cpp
 * @brief JSON工具类实现
 * @date 2026-03-20
 * @version 1.0.0
 */

#include "common/json/jsonutil.h"
#include "common/logger/logger.h"

#include <QFileInfo>
#include <QTextStream>

JsonUtil::JsonUtil() : m_json(json::object())
{
}

JsonUtil::JsonUtil(const json& j) : m_json(j)
{
}

JsonUtil::JsonUtil(const QString& jsonString)
{
    try
    {
        m_json = json::parse(jsonString.toStdString());
    }
    catch (const json::parse_error& e)
    {
        Logger::instance().warning(QString("JSON解析失败: %1").arg(e.what()));
        m_json = json::object();
    }
}

JsonUtil::JsonUtil(const std::string& jsonString)
{
    try
    {
        m_json = json::parse(jsonString);
    }
    catch (const json::parse_error& e)
    {
        Logger::instance().warning(QString("JSON解析失败: %1").arg(e.what()));
        m_json = json::object();
    }
}

JsonUtil::JsonUtil(const char* jsonString)
{
    try
    {
        m_json = json::parse(jsonString);
    }
    catch (const json::parse_error& e)
    {
        Logger::instance().warning(QString("JSON解析失败: %1").arg(e.what()));
        m_json = json::object();
    }
}

JsonUtil::JsonUtil(const QVariant& variant)
{
    m_json = variantToJson(variant);
}

JsonUtil::JsonUtil(const QVariantMap& map)
{
    m_json = variantToJson(map);
}

JsonUtil::JsonUtil(const QVariantList& list)
{
    m_json = variantToJson(list);
}

JsonParseResult JsonUtil::fromFile(const QString& filePath)
{
    JsonParseResult result;

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists())
    {
        result.errorMessage = QString("文件不存在: %1").arg(filePath);
        Logger::instance().error(result.errorMessage);
        return result;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        result.errorMessage = QString("无法打开文件: %1").arg(filePath);
        Logger::instance().error(result.errorMessage);
        return result;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    try
    {
        result.jsonData = std::make_unique<JsonUtil>(json::parse(content.toStdString()));
        result.success = true;
        Logger::instance().debug(QString("成功加载JSON文件: %1").arg(filePath));
    }
    catch (const json::parse_error& e)
    {
        result.errorMessage = QString("JSON解析失败: %1").arg(e.what());
        Logger::instance().error(result.errorMessage);
    }

    return result;
}

JsonParseResult JsonUtil::fromString(const QString& jsonString)
{
    JsonParseResult result;

    try
    {
        result.jsonData = std::make_unique<JsonUtil>(json::parse(jsonString.toStdString()));
        result.success = true;
    }
    catch (const json::parse_error& e)
    {
        result.errorMessage = QString("JSON解析失败: %1").arg(e.what());
        Logger::instance().error(result.errorMessage);
    }

    return result;
}

bool JsonUtil::toFile(const QString& filePath, int indent) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        Logger::instance().error(QString("无法创建文件: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    if (indent >= 0)
    {
        out << QString::fromStdString(m_json.dump(indent));
    }
    else
    {
        out << QString::fromStdString(m_json.dump());
    }

    file.close();
    Logger::instance().debug(QString("成功保存JSON文件: %1").arg(filePath));
    return true;
}

QString JsonUtil::toString(int indent) const
{
    if (indent >= 0)
    {
        return QString::fromStdString(m_json.dump(indent));
    }
    return QString::fromStdString(m_json.dump());
}

QVariant JsonUtil::toVariant() const
{
    return jsonToVariant(m_json);
}

QVariantMap JsonUtil::toVariantMap() const
{
    if (!isObject())
    {
        return QVariantMap();
    }

    QVariantMap map;
    for (auto it = m_json.begin(); it != m_json.end(); ++it)
    {
        map[QString::fromStdString(it.key())] = jsonToVariant(it.value());
    }
    return map;
}

QVariantList JsonUtil::toVariantList() const
{
    if (!isArray())
    {
        return QVariantList();
    }

    QVariantList list;
    for (const auto& item : m_json)
    {
        list.append(jsonToVariant(item));
    }
    return list;
}

json& JsonUtil::raw()
{
    return m_json;
}

const json& JsonUtil::raw() const
{
    return m_json;
}

bool JsonUtil::contains(const QString& key) const
{
    if (!isObject())
    {
        return false;
    }
    return m_json.contains(key.toStdString());
}

bool JsonUtil::contains(size_t index) const
{
    if (!isArray())
    {
        return false;
    }
    return index < m_json.size();
}

QStringList JsonUtil::keys() const
{
    QStringList result;
    if (isObject())
    {
        for (auto it = m_json.begin(); it != m_json.end(); ++it)
        {
            result.append(QString::fromStdString(it.key()));
        }
    }
    return result;
}

size_t JsonUtil::size() const
{
    return m_json.size();
}

bool JsonUtil::isEmpty() const
{
    return m_json.empty();
}

bool JsonUtil::isNull() const
{
    return m_json.is_null();
}

bool JsonUtil::isObject() const
{
    return m_json.is_object();
}

bool JsonUtil::isArray() const
{
    return m_json.is_array();
}

bool JsonUtil::isString() const
{
    return m_json.is_string();
}

bool JsonUtil::isNumber() const
{
    return m_json.is_number();
}

bool JsonUtil::isBool() const
{
    return m_json.is_boolean();
}

JsonUtil JsonUtil::value(const QString& key, const JsonUtil& defaultValue) const
{
    if (!isObject() || !contains(key))
    {
        return defaultValue;
    }
    return JsonUtil(m_json[key.toStdString()]);
}

JsonUtil JsonUtil::at(size_t index) const
{
    if (!isArray() || index >= m_json.size())
    {
        return JsonUtil();
    }
    return JsonUtil(m_json[index]);
}

void JsonUtil::set(const QString& key, const JsonUtil& value)
{
    if (!isObject())
    {
        m_json = json::object();
    }
    m_json[key.toStdString()] = value.m_json;
}

void JsonUtil::set(const QString& key, const QString& value)
{
    if (!isObject())
    {
        m_json = json::object();
    }
    m_json[key.toStdString()] = value.toStdString();
}

void JsonUtil::set(const QString& key, int value)
{
    if (!isObject())
    {
        m_json = json::object();
    }
    m_json[key.toStdString()] = value;
}

void JsonUtil::set(const QString& key, double value)
{
    if (!isObject())
    {
        m_json = json::object();
    }
    m_json[key.toStdString()] = value;
}

void JsonUtil::set(const QString& key, bool value)
{
    if (!isObject())
    {
        m_json = json::object();
    }
    m_json[key.toStdString()] = value;
}

bool JsonUtil::remove(const QString& key)
{
    if (!isObject() || !contains(key))
    {
        return false;
    }
    m_json.erase(key.toStdString());
    return true;
}

void JsonUtil::append(const JsonUtil& value)
{
    if (!isArray())
    {
        m_json = json::array();
    }
    m_json.push_back(value.m_json);
}

void JsonUtil::append(const QString& value)
{
    if (!isArray())
    {
        m_json = json::array();
    }
    m_json.push_back(value.toStdString());
}

void JsonUtil::clear()
{
    m_json.clear();
}

QString JsonUtil::getString(const QString& key, const QString& defaultValue) const
{
    if (!contains(key))
    {
        return defaultValue;
    }
    const auto& val = m_json[key.toStdString()];
    if (val.is_string())
    {
        return QString::fromStdString(val.get<std::string>());
    }
    return defaultValue;
}

int JsonUtil::getInt(const QString& key, int defaultValue) const
{
    if (!contains(key))
    {
        return defaultValue;
    }
    const auto& val = m_json[key.toStdString()];
    if (val.is_number_integer())
    {
        return val.get<int>();
    }
    return defaultValue;
}

double JsonUtil::getDouble(const QString& key, double defaultValue) const
{
    if (!contains(key))
    {
        return defaultValue;
    }
    const auto& val = m_json[key.toStdString()];
    if (val.is_number())
    {
        return val.get<double>();
    }
    return defaultValue;
}

bool JsonUtil::getBool(const QString& key, bool defaultValue) const
{
    if (!contains(key))
    {
        return defaultValue;
    }
    const auto& val = m_json[key.toStdString()];
    if (val.is_boolean())
    {
        return val.get<bool>();
    }
    return defaultValue;
}

JsonUtil JsonUtil::getArray(const QString& key) const
{
    if (!contains(key))
    {
        return JsonUtil(json::array());
    }
    const auto& val = m_json[key.toStdString()];
    if (val.is_array())
    {
        return JsonUtil(val);
    }
    return JsonUtil(json::array());
}

JsonUtil JsonUtil::getObject(const QString& key) const
{
    if (!contains(key))
    {
        return JsonUtil(json::object());
    }
    const auto& val = m_json[key.toStdString()];
    if (val.is_object())
    {
        return JsonUtil(val);
    }
    return JsonUtil(json::object());
}

void JsonUtil::forEach(std::function<void(const QString&, const JsonUtil&)> callback) const
{
    if (!isObject())
    {
        return;
    }
    for (auto it = m_json.begin(); it != m_json.end(); ++it)
    {
        callback(QString::fromStdString(it.key()), JsonUtil(it.value()));
    }
}

void JsonUtil::forEachInArray(std::function<void(size_t, const JsonUtil&)> callback) const
{
    if (!isArray())
    {
        return;
    }
    for (size_t i = 0; i < m_json.size(); ++i)
    {
        callback(i, JsonUtil(m_json[i]));
    }
}

JsonUtil JsonUtil::getByPath(const QString& path, const JsonUtil& defaultValue) const
{
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty())
    {
        return defaultValue;
    }

    json current = m_json;
    for (const QString& part : parts)
    {
        if (current.is_object())
        {
            std::string key = part.toStdString();
            if (!current.contains(key))
            {
                return defaultValue;
            }
            current = current[key];
        }
        else if (current.is_array())
        {
            bool ok = false;
            int index = part.toInt(&ok);
            if (!ok || index < 0 || static_cast<size_t>(index) >= current.size())
            {
                return defaultValue;
            }
            current = current[index];
        }
        else
        {
            return defaultValue;
        }
    }

    return JsonUtil(current);
}

void JsonUtil::setByPath(const QString& path, const JsonUtil& value)
{
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty())
    {
        return;
    }

    if (!isObject())
    {
        m_json = json::object();
    }

    json* current = &m_json;
    for (int i = 0; i < parts.size() - 1; ++i)
    {
        const QString& part = parts[i];
        std::string key = part.toStdString();

        if (current->is_object())
        {
            if (!current->contains(key))
            {
                (*current)[key] = json::object();
            }
            current = &(*current)[key];
        }
        else if (current->is_array())
        {
            bool ok = false;
            int index = part.toInt(&ok);
            if (!ok || index < 0 || static_cast<size_t>(index) >= current->size())
            {
                return;
            }
            current = &(*current)[index];
        }
        else
        {
            return;
        }
    }

    std::string lastKey = parts.last().toStdString();
    if (current->is_object())
    {
        (*current)[lastKey] = value.m_json;
    }
}

JsonUtil JsonUtil::operator[](const QString& key) const
{
    return value(key);
}

JsonUtil JsonUtil::operator[](size_t index) const
{
    return at(index);
}

void JsonUtil::merge(const JsonUtil& other, bool overwrite)
{
    if (!isObject() || !other.isObject())
    {
        return;
    }

    for (auto it = other.m_json.begin(); it != other.m_json.end(); ++it)
    {
        QString key = QString::fromStdString(it.key());
        if (overwrite || !contains(key))
        {
            m_json[it.key()] = it.value();
        }
    }
}

JsonUtil JsonUtil::deepCopy() const
{
    return JsonUtil(json(m_json));
}

QString JsonUtil::jsonTypeToString(json::value_t type)
{
    switch (type)
    {
        case json::value_t::null:
            return "null";
        case json::value_t::object:
            return "object";
        case json::value_t::array:
            return "array";
        case json::value_t::string:
            return "string";
        case json::value_t::boolean:
            return "boolean";
        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
        case json::value_t::number_float:
            return "number";
        default:
            return "unknown";
    }
}

QVariant JsonUtil::jsonToVariant(const json& j)
{
    if (j.is_null())
    {
        return QVariant();
    }
    else if (j.is_boolean())
    {
        return QVariant(j.get<bool>());
    }
    else if (j.is_number_integer())
    {
        return QVariant(j.get<qint64>());
    }
    else if (j.is_number_float())
    {
        return QVariant(j.get<double>());
    }
    else if (j.is_string())
    {
        return QVariant(QString::fromStdString(j.get<std::string>()));
    }
    else if (j.is_array())
    {
        QVariantList list;
        for (const auto& item : j)
        {
            list.append(jsonToVariant(item));
        }
        return QVariant(list);
    }
    else if (j.is_object())
    {
        QVariantMap map;
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            map[QString::fromStdString(it.key())] = jsonToVariant(it.value());
        }
        return QVariant(map);
    }
    return QVariant();
}

json JsonUtil::variantToJson(const QVariant& variant)
{
    if (!variant.isValid())
    {
        return json();
    }

    switch (variant.typeId())
    {
        case QMetaType::Bool:
            return json(variant.toBool());

        case QMetaType::Int:
        case QMetaType::Long:
        case QMetaType::LongLong:
            return json(variant.toLongLong());

        case QMetaType::UInt:
        case QMetaType::ULong:
        case QMetaType::ULongLong:
            return json(variant.toULongLong());

        case QMetaType::Float:
        case QMetaType::Double:
            return json(variant.toDouble());

        case QMetaType::QString:
            return json(variant.toString().toStdString());

        case QMetaType::QStringList:
        {
            json arr = json::array();
            const QStringList& list = variant.toStringList();
            for (const QString& str : list)
            {
                arr.push_back(str.toStdString());
            }
            return arr;
        }

        case QMetaType::QVariantList:
        {
            json arr = json::array();
            const QVariantList& list = variant.toList();
            for (const QVariant& item : list)
            {
                arr.push_back(variantToJson(item));
            }
            return arr;
        }

        case QMetaType::QVariantMap:
        {
            json obj = json::object();
            const QVariantMap& map = variant.toMap();
            for (auto it = map.begin(); it != map.end(); ++it)
            {
                obj[it.key().toStdString()] = variantToJson(it.value());
            }
            return obj;
        }

        case QMetaType::QVariantHash:
        {
            json obj = json::object();
            const QVariantHash& hash = variant.toHash();
            for (auto it = hash.begin(); it != hash.end(); ++it)
            {
                obj[it.key().toStdString()] = variantToJson(it.value());
            }
            return obj;
        }

        default:
            if (variant.canConvert<QString>())
            {
                return json(variant.toString().toStdString());
            }
            return json();
    }
}
