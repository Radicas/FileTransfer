/**
 * @file versionmanager.cpp
 * @brief 版本号管理类实现
 * @date 2026-02-28
 * @version 1.0.0
 */

#include "common/utils/versionmanager.h"
#include <sstream>
#include <stdexcept>

/**
 * @brief 验证版本号格式是否正确
 * @param version 版本号字符串
 * @return 格式是否正确
 */
bool VersionManager::validateVersionFormat(const std::string& version)
{
    // 语义化版本号正则表达式
    std::regex versionRegex(R"(^\d+\.\d+\.\d+(-[a-zA-Z0-9]+(\.[a-zA-Z0-9]+)*)?(\+[a-zA-Z0-9]+(\.[a-zA-Z0-9]+)*)?$)");
    return std::regex_match(version, versionRegex);
}

/**
 * @brief 默认构造函数，初始化版本号为0.0.0
 */
VersionManager::VersionManager() : majorVersion(0), minorVersion(0), patchVersion(0), preRelease(""), buildMetadata("")
{
}

/**
 * @brief 构造函数，从版本号字符串初始化
 * @param version 版本号字符串，格式为a.b.c[-preRelease][+buildMetadata]
 * @throws std::invalid_argument 如果版本号格式不正确
 */
VersionManager::VersionManager(const std::string& version)
{
    if (!validateVersionFormat(version))
    {
        throw std::invalid_argument("版本号格式不正确");
    }

    size_t preReleasePos = version.find('-');
    size_t buildMetadataPos = version.find('+');

    // 提取核心版本号部分
    std::string coreVersion = version;
    if (preReleasePos != std::string::npos)
    {
        coreVersion = version.substr(0, preReleasePos);
        if (buildMetadataPos != std::string::npos)
        {
            preRelease = version.substr(preReleasePos + 1, buildMetadataPos - preReleasePos - 1);
            buildMetadata = version.substr(buildMetadataPos + 1);
        }
        else
        {
            preRelease = version.substr(preReleasePos + 1);
        }
    }
    else if (buildMetadataPos != std::string::npos)
    {
        coreVersion = version.substr(0, buildMetadataPos);
        buildMetadata = version.substr(buildMetadataPos + 1);
    }

    // 解析核心版本号
    std::istringstream iss(coreVersion);
    char dot;
    iss >> majorVersion >> dot >> minorVersion >> dot >> patchVersion;
}

/**
 * @brief 构造函数，从主版本号、次版本号和修订号初始化
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 修订号
 * @param preRelease 预发布版本标识符
 * @param buildMetadata 构建元数据
 */
VersionManager::VersionManager(int major, int minor, int patch, const std::string& preRelease,
                               const std::string& buildMetadata)
    : majorVersion(major),
      minorVersion(minor),
      patchVersion(patch),
      preRelease(preRelease),
      buildMetadata(buildMetadata)
{
}

/**
 * @brief 递增主版本号，重置次版本号和修订号为0
 * @return 当前VersionManager对象的引用
 */
VersionManager& VersionManager::incrementMajor()
{
    majorVersion++;
    minorVersion = 0;
    patchVersion = 0;
    preRelease = "";
    return *this;
}

/**
 * @brief 递增次版本号，重置修订号为0
 * @return 当前VersionManager对象的引用
 */
VersionManager& VersionManager::incrementMinor()
{
    minorVersion++;
    patchVersion = 0;
    preRelease = "";
    return *this;
}

/**
 * @brief 递增修订号
 * @return 当前VersionManager对象的引用
 */
VersionManager& VersionManager::incrementPatch()
{
    patchVersion++;
    preRelease = "";
    return *this;
}

/**
 * @brief 设置预发布版本标识符
 * @param preRelease 预发布版本标识符
 * @return 当前VersionManager对象的引用
 */
VersionManager& VersionManager::setPreRelease(const std::string& preRelease)
{
    this->preRelease = preRelease;
    return *this;
}

/**
 * @brief 设置构建元数据
 * @param buildMetadata 构建元数据
 * @return 当前VersionManager对象的引用
 */
VersionManager& VersionManager::setBuildMetadata(const std::string& buildMetadata)
{
    this->buildMetadata = buildMetadata;
    return *this;
}

/**
 * @brief 获取主版本号
 * @return 主版本号
 */
int VersionManager::getMajorVersion() const
{
    return majorVersion;
}

/**
 * @brief 获取次版本号
 * @return 次版本号
 */
int VersionManager::getMinorVersion() const
{
    return minorVersion;
}

/**
 * @brief 获取修订号
 * @return 修订号
 */
int VersionManager::getPatchVersion() const
{
    return patchVersion;
}

/**
 * @brief 获取预发布版本标识符
 * @return 预发布版本标识符
 */
std::string VersionManager::getPreRelease() const
{
    return preRelease;
}

/**
 * @brief 获取构建元数据
 * @return 构建元数据
 */
std::string VersionManager::getBuildMetadata() const
{
    return buildMetadata;
}

/**
 * @brief 将版本号转换为字符串
 * @return 版本号字符串，格式为a.b.c[-preRelease][+buildMetadata]
 */
std::string VersionManager::toString() const
{
    std::ostringstream oss;
    oss << majorVersion << "." << minorVersion << "." << patchVersion;
    if (!preRelease.empty())
    {
        oss << "-" << preRelease;
    }
    if (!buildMetadata.empty())
    {
        oss << "+" << buildMetadata;
    }
    return oss.str();
}

/**
 * @brief 比较两个版本号的大小
 * @param other 另一个版本号
 * @return 如果当前版本号小于other，返回-1；如果相等，返回0；如果大于，返回1
 */
int VersionManager::compare(const VersionManager& other) const
{
    // 比较主版本号
    if (majorVersion < other.majorVersion)
        return -1;
    if (majorVersion > other.majorVersion)
        return 1;

    // 比较次版本号
    if (minorVersion < other.minorVersion)
        return -1;
    if (minorVersion > other.minorVersion)
        return 1;

    // 比较修订号
    if (patchVersion < other.patchVersion)
        return -1;
    if (patchVersion > other.patchVersion)
        return 1;

    // 比较预发布版本（正式版本大于预发布版本）
    if (preRelease.empty() && !other.preRelease.empty())
        return 1;
    if (!preRelease.empty() && other.preRelease.empty())
        return -1;

    // 比较预发布版本标识符
    if (!preRelease.empty() && !other.preRelease.empty())
    {
        if (preRelease < other.preRelease)
            return -1;
        if (preRelease > other.preRelease)
            return 1;
    }

    // 构建元数据不参与版本比较
    return 0;
}

/**
 * @brief 重载小于运算符
 * @param other 另一个版本号
 * @return 当前版本号是否小于other
 */
bool VersionManager::operator<(const VersionManager& other) const
{
    return compare(other) < 0;
}

/**
 * @brief 重载小于等于运算符
 * @param other 另一个版本号
 * @return 当前版本号是否小于等于other
 */
bool VersionManager::operator<=(const VersionManager& other) const
{
    return compare(other) <= 0;
}

/**
 * @brief 重载大于运算符
 * @param other 另一个版本号
 * @return 当前版本号是否大于other
 */
bool VersionManager::operator>(const VersionManager& other) const
{
    return compare(other) > 0;
}

/**
 * @brief 重载大于等于运算符
 * @param other 另一个版本号
 * @return 当前版本号是否大于等于other
 */
bool VersionManager::operator>=(const VersionManager& other) const
{
    return compare(other) >= 0;
}

/**
 * @brief 重载等于运算符
 * @param other 另一个版本号
 * @return 当前版本号是否等于other
 */
bool VersionManager::operator==(const VersionManager& other) const
{
    return compare(other) == 0;
}

/**
 * @brief 重载不等于运算符
 * @param other 另一个版本号
 * @return 当前版本号是否不等于other
 */
bool VersionManager::operator!=(const VersionManager& other) const
{
    return compare(other) != 0;
}