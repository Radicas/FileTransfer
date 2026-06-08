/**
 * @file jsonutil.h
 * @brief JSON工具类，提供JSON序列化/反序列化功能
 * @date 2026-03-20
 * @version 1.0.0
 * 
 * @details 该模块基于nlohmann/json库封装，提供：
 * - 文件读写功能
 * - QString与JSON的互转
 * - 统一的错误处理和日志记录
 * - 支持直接访问底层json对象以获得完整功能
 */

#ifndef JSONUTIL_H
#define JSONUTIL_H

#include <QFile>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/json/thirdparty/json.hpp"

using nlohmann::json;

class JsonUtil;

/**
 * @struct JsonParseResult
 * @brief JSON解析结果结构
 */
struct JsonParseResult
{
    bool success;                     ///< 操作是否成功
    QString errorMessage;             ///< 错误信息（中文）
    std::unique_ptr<JsonUtil> jsonData; ///< 解析结果（成功时有效）

    JsonParseResult() : success(false) {}

    JsonUtil& json() { return *jsonData; }
    const JsonUtil& json() const { return *jsonData; }
    operator bool() const { return success; }
};

/**
 * @class JsonUtil
 * @brief JSON工具类，提供JSON序列化/反序列化的便捷接口
 * 
 * 该类封装了nlohmann/json库，提供：
 * - 文件读写：从文件加载JSON、保存JSON到文件
 * - 类型转换：QString、QVariant与JSON互转
 * - 错误处理：统一的异常捕获和日志记录
 * - 直接访问：可通过raw()方法访问底层json对象
 * 
 * @example
 * @code
 * // 从文件加载
 * auto result = JsonUtil::fromFile("config.json");
 * if (result.success) {
 *     QString name = result.json().getString("name");
 * }
 * 
 * // 保存到文件
 * JsonUtil json;
 * json.set("name", "测试");
 * json.set("version", 1);
 * json.toFile("output.json");
 * 
 * // 直接访问底层对象
 * json& raw = jsonUtil.raw();
 * raw["nested"]["deep"]["value"] = 123;
 * @endcode
 */
class JsonUtil
{
public:
    /**
     * @brief 默认构造函数，创建空JSON对象
     */
    JsonUtil();

    /**
     * @brief 从json对象构造
     * @param j nlohmann::json对象
     */
    JsonUtil(const json& j);

    /**
     * @brief 从字符串解析JSON
     * @param jsonString JSON格式字符串
     */
    JsonUtil(const QString& jsonString);

    /**
     * @brief 从字符串解析JSON
     * @param jsonString JSON格式字符串
     */
    JsonUtil(const std::string& jsonString);

    /**
     * @brief 从C字符串解析JSON
     * @param jsonString JSON格式字符串
     */
    JsonUtil(const char* jsonString);

    /**
     * @brief 从QVariant构造JSON
     * @param variant QVariant对象
     */
    JsonUtil(const QVariant& variant);

    /**
     * @brief 从QVariantMap构造JSON对象
     * @param map QVariantMap对象
     */
    JsonUtil(const QVariantMap& map);

    /**
     * @brief 从QVariantList构造JSON数组
     * @param list QVariantList对象
     */
    JsonUtil(const QVariantList& list);

    /**
     * @brief 从文件加载JSON
     * @param filePath JSON文件路径
     * @return 解析结果
     */
    static JsonParseResult fromFile(const QString& filePath);

    /**
     * @brief 从字符串解析JSON
     * @param jsonString JSON格式字符串
     * @return 解析结果
     */
    static JsonParseResult fromString(const QString& jsonString);

    /**
     * @brief 保存JSON到文件
     * @param filePath 目标文件路径
     * @param indent 缩进空格数，-1表示紧凑格式
     * @return 是否成功
     */
    bool toFile(const QString& filePath, int indent = 4) const;

    /**
     * @brief 转换为JSON字符串
     * @param indent 缩进空格数，-1表示紧凑格式
     * @return JSON格式字符串
     */
    QString toString(int indent = -1) const;

    /**
     * @brief 转换为QVariant
     * @return QVariant对象
     */
    QVariant toVariant() const;

    /**
     * @brief 转换为QVariantMap（仅当JSON为对象时有效）
     * @return QVariantMap对象
     */
    QVariantMap toVariantMap() const;

    /**
     * @brief 转换为QVariantList（仅当JSON为数组时有效）
     * @return QVariantList对象
     */
    QVariantList toVariantList() const;

    /**
     * @brief 获取底层json对象的引用
     * @return json对象引用
     * 
     * @note 使用此方法可以直接操作底层json对象，获得完整功能
     */
    json& raw();

    /**
     * @brief 获取底层json对象的常量引用
     * @return json对象常量引用
     */
    const json& raw() const;

    /**
     * @brief 检查是否包含指定键
     * @param key 键名
     * @return 是否包含
     */
    bool contains(const QString& key) const;

    /**
     * @brief 检查是否包含指定索引（数组）
     * @param index 索引
     * @return 是否包含
     */
    bool contains(size_t index) const;

    /**
     * @brief 获取键列表
     * @return 键名列表
     */
    QStringList keys() const;

    /**
     * @brief 获取数组大小
     * @return 数组大小，非数组返回0
     */
    size_t size() const;

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    bool isEmpty() const;

    /**
     * @brief 检查是否为null
     * @return 是否为null
     */
    bool isNull() const;

    /**
     * @brief 检查是否为对象
     * @return 是否为对象
     */
    bool isObject() const;

    /**
     * @brief 检查是否为数组
     * @return 是否为数组
     */
    bool isArray() const;

    /**
     * @brief 检查是否为字符串
     * @return 是否为字符串
     */
    bool isString() const;

    /**
     * @brief 检查是否为数字
     * @return 是否为数字
     */
    bool isNumber() const;

    /**
     * @brief 检查是否为布尔值
     * @return 是否为布尔值
     */
    bool isBool() const;

    /**
     * @brief 获取指定键的值
     * @param key 键名
     * @param defaultValue 默认值（键不存在时返回）
     * @return 值
     */
    JsonUtil value(const QString& key, const JsonUtil& defaultValue = JsonUtil()) const;

    /**
     * @brief 获取指定索引的值（数组）
     * @param index 索引
     * @return 值
     */
    JsonUtil at(size_t index) const;

    /**
     * @brief 设置指定键的值
     * @param key 键名
     * @param value 值
     */
    void set(const QString& key, const JsonUtil& value);

    /**
     * @brief 设置指定键的值（QString）
     * @param key 键名
     * @param value 值
     */
    void set(const QString& key, const QString& value);

    /**
     * @brief 设置指定键的值（整数）
     * @param key 键名
     * @param value 值
     */
    void set(const QString& key, int value);

    /**
     * @brief 设置指定键的值（双精度浮点）
     * @param key 键名
     * @param value 值
     */
    void set(const QString& key, double value);

    /**
     * @brief 设置指定键的值（布尔）
     * @param key 键名
     * @param value 值
     */
    void set(const QString& key, bool value);

    /**
     * @brief 移除指定键
     * @param key 键名
     * @return 是否成功移除
     */
    bool remove(const QString& key);

    /**
     * @brief 追加元素到数组
     * @param value 要追加的值
     */
    void append(const JsonUtil& value);

    /**
     * @brief 追加字符串到数组
     * @param value 要追加的字符串
     */
    void append(const QString& value);

    /**
     * @brief 清空JSON
     */
    void clear();

    /**
     * @brief 获取字符串值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 字符串值
     */
    QString getString(const QString& key, const QString& defaultValue = QString()) const;

    /**
     * @brief 获取整数值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 整数值
     */
    int getInt(const QString& key, int defaultValue = 0) const;

    /**
     * @brief 获取双精度浮点值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 双精度浮点值
     */
    double getDouble(const QString& key, double defaultValue = 0.0) const;

    /**
     * @brief 获取布尔值
     * @param key 键名
     * @param defaultValue 默认值
     * @return 布尔值
     */
    bool getBool(const QString& key, bool defaultValue = false) const;

    /**
     * @brief 获取数组值
     * @param key 键名
     * @return 数组值
     */
    JsonUtil getArray(const QString& key) const;

    /**
     * @brief 获取对象值
     * @param key 键名
     * @return 对象值
     */
    JsonUtil getObject(const QString& key) const;

    /**
     * @brief 遍历对象的所有键值对
     * @param callback 回调函数，参数为(键, 值)
     */
    void forEach(std::function<void(const QString&, const JsonUtil&)> callback) const;

    /**
     * @brief 遍历数组的所有元素
     * @param callback 回调函数，参数为(索引, 值)
     */
    void forEachInArray(std::function<void(size_t, const JsonUtil&)> callback) const;

    /**
     * @brief 路径访问，支持嵌套路径如 "user.profile.name"
     * @param path 路径，用.分隔
     * @param defaultValue 默认值
     * @return 值
     */
    JsonUtil getByPath(const QString& path, const JsonUtil& defaultValue = JsonUtil()) const;

    /**
     * @brief 路径设置，支持嵌套路径如 "user.profile.name"
     * @param path 路径，用.分隔
     * @param value 值
     */
    void setByPath(const QString& path, const JsonUtil& value);

    /**
     * @brief 操作符重载：键访问
     * @param key 键名
     * @return JsonUtil引用
     */
    JsonUtil operator[](const QString& key) const;

    /**
     * @brief 操作符重载：索引访问
     * @param index 索引
     * @return JsonUtil引用
     */
    JsonUtil operator[](size_t index) const;

    /**
     * @brief 合并另一个JSON对象
     * @param other 要合并的JSON对象
     * @param overwrite 是否覆盖已存在的键
     */
    void merge(const JsonUtil& other, bool overwrite = true);

    /**
     * @brief 深拷贝
     * @return 新的JsonUtil对象
     */
    JsonUtil deepCopy() const;

private:
    json m_json;

    static QString jsonTypeToString(json::value_t type);
    static QVariant jsonToVariant(const json& j);
    static json variantToJson(const QVariant& variant);
};

#endif
