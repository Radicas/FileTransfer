/**
 * @file versionmanager.h
 * @brief 版本号管理类
 * @date 2026-02-28
 * @version 1.0.0
 */

#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

#include <regex>
#include <string>

/**
 * @class VersionManager
 * @brief 版本号管理类，实现语义化版本控制规范
 * 
 * 遵循语义化版本控制(Semantic Versioning)规范，版本号格式为a.b.c，其中：
 * - 主版本号(a)：在进行不兼容的API更改时递增
 * - 次版本号(b)：在添加向后兼容的功能时递增
 * - 修订号(c)：在进行向后兼容的问题修复时递增
 * 
 * 支持预发布版本和构建元数据
 */
class VersionManager
{
   private:
    int majorVersion;           ///< 主版本号
    int minorVersion;           ///< 次版本号
    int patchVersion;           ///< 修订号
    std::string preRelease;     ///< 预发布版本标识符
    std::string buildMetadata;  ///< 构建元数据

    /**
     * @brief 验证版本号格式是否正确
     * @param version 版本号字符串
     * @return 格式是否正确
     */
    bool validateVersionFormat(const std::string& version);

   public:
    /**
     * @brief 默认构造函数，初始化版本号为0.0.0
     */
    VersionManager();

    /**
     * @brief 构造函数，从版本号字符串初始化
     * @param version 版本号字符串，格式为a.b.c[-preRelease][+buildMetadata]
     * @throws std::invalid_argument 如果版本号格式不正确
     */
    VersionManager(const std::string& version);

    /**
     * @brief 构造函数，从主版本号、次版本号和修订号初始化
     * @param major 主版本号
     * @param minor 次版本号
     * @param patch 修订号
     * @param preRelease 预发布版本标识符
     * @param buildMetadata 构建元数据
     */
    VersionManager(int major, int minor, int patch, const std::string& preRelease = "",
                   const std::string& buildMetadata = "");

    /**
     * @brief 递增主版本号，重置次版本号和修订号为0
     * @return 当前VersionManager对象的引用
     */
    VersionManager& incrementMajor();

    /**
     * @brief 递增次版本号，重置修订号为0
     * @return 当前VersionManager对象的引用
     */
    VersionManager& incrementMinor();

    /**
     * @brief 递增修订号
     * @return 当前VersionManager对象的引用
     */
    VersionManager& incrementPatch();

    /**
     * @brief 设置预发布版本标识符
     * @param preRelease 预发布版本标识符
     * @return 当前VersionManager对象的引用
     */
    VersionManager& setPreRelease(const std::string& preRelease);

    /**
     * @brief 设置构建元数据
     * @param buildMetadata 构建元数据
     * @return 当前VersionManager对象的引用
     */
    VersionManager& setBuildMetadata(const std::string& buildMetadata);

    /**
     * @brief 获取主版本号
     * @return 主版本号
     */
    int getMajorVersion() const;

    /**
     * @brief 获取次版本号
     * @return 次版本号
     */
    int getMinorVersion() const;

    /**
     * @brief 获取修订号
     * @return 修订号
     */
    int getPatchVersion() const;

    /**
     * @brief 获取预发布版本标识符
     * @return 预发布版本标识符
     */
    std::string getPreRelease() const;

    /**
     * @brief 获取构建元数据
     * @return 构建元数据
     */
    std::string getBuildMetadata() const;

    /**
     * @brief 将版本号转换为字符串
     * @return 版本号字符串，格式为a.b.c[-preRelease][+buildMetadata]
     */
    std::string toString() const;

    /**
     * @brief 比较两个版本号的大小
     * @param other 另一个版本号
     * @return 如果当前版本号小于other，返回-1；如果相等，返回0；如果大于，返回1
     */
    int compare(const VersionManager& other) const;

    /**
     * @brief 重载小于运算符
     * @param other 另一个版本号
     * @return 当前版本号是否小于other
     */
    bool operator<(const VersionManager& other) const;

    /**
     * @brief 重载小于等于运算符
     * @param other 另一个版本号
     * @return 当前版本号是否小于等于other
     */
    bool operator<=(const VersionManager& other) const;

    /**
     * @brief 重载大于运算符
     * @param other 另一个版本号
     * @return 当前版本号是否大于other
     */
    bool operator>(const VersionManager& other) const;

    /**
     * @brief 重载大于等于运算符
     * @param other 另一个版本号
     * @return 当前版本号是否大于等于other
     */
    bool operator>=(const VersionManager& other) const;

    /**
     * @brief 重载等于运算符
     * @param other 另一个版本号
     * @return 当前版本号是否等于other
     */
    bool operator==(const VersionManager& other) const;

    /**
     * @brief 重载不等于运算符
     * @param other 另一个版本号
     * @return 当前版本号是否不等于other
     */
    bool operator!=(const VersionManager& other) const;
};

#endif  // VERSIONMANAGER_H