/**
 * @file filebrowserpanel.h
 * @brief 文件浏览器面板控件定义
 * @date 2026-06-01
 * @version 1.0
 * 
 * @details 提供文件浏览功能，支持本地和远程文件系统浏览，
 * 支持拖拽操作和文件选择。
 */

#ifndef FILEBROWSERPANEL_H
#define FILEBROWSERPANEL_H

#include <QWidget>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QToolBar>
#include <QMenu>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "core/filesystem/filesystemmodel.h"

/**
 * @class FileBrowserPanel
 * @brief 文件浏览器面板控件
 * 
 * @details 该控件负责：
 * - 显示文件列表（本地或远程）
 * - 提供路径导航功能
 * - 支持拖拽文件操作
 * - 提供文件操作菜单（复制、删除等）
 */
class FileBrowserPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param type 文件系统类型
     * @param parent 父窗口指针
     */
    explicit FileBrowserPanel(FileSystemType type = FileSystemType::Local, QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~FileBrowserPanel();

    /**
     * @brief 设置文件系统类型
     * @param type 文件系统类型
     */
    void setFileSystemType(FileSystemType type);

    /**
     * @brief 获取文件系统类型
     * @return 文件系统类型
     */
    FileSystemType getFileSystemType() const;

    /**
     * @brief 设置目标设备ID
     * @param deviceId 设备ID
     */
    void setTargetDevice(const QString& deviceId);

    /**
     * @brief 获取目标设备ID
     * @return 设备ID
     */
    QString getTargetDevice() const;

    /**
     * @brief 获取当前路径
     * @return 当前路径
     */
    QString getCurrentPath() const;

    /**
     * @brief 设置当前路径
     * @param path 路径
     */
    void setCurrentPath(const QString& path);

    /**
     * @brief 获取选中的文件列表
     * @return 选中的文件列表
     */
    QList<FileItem> getSelectedFiles() const;

    /**
     * @brief 刷新文件列表
     */
    void refresh();

    /**
     * @brief 获取面板标题
     * @return 标题
     */
    QString getTitle() const;

    /**
     * @brief 设置面板标题
     * @param title 标题
     */
    void setTitle(const QString& title);

signals:
    /**
     * @brief 文件选择变化信号
     * @param files 选中的文件列表
     */
    void selectionChanged(const QList<FileItem>& files);

    /**
     * @brief 路径变化信号
     * @param path 新路径
     */
    void pathChanged(const QString& path);

    /**
     * @brief 文件拖拽开始信号
     * @param files 拖拽的文件列表
     * @param sourcePanel 源面板
     */
    void dragStarted(const QList<FileItem>& files, FileBrowserPanel* sourcePanel);

    /**
     * @brief 文件拖拽接收信号
     * @param files 接收的文件列表
     * @param targetPath 目标路径
     */
    void filesDropped(const QList<FileItem>& files, const QString& targetPath);

    /**
     * @brief 文件双击信号
     * @param file 双击的文件
     */
    void fileDoubleClicked(const FileItem& file);

    /**
     * @brief 传输请求信号
     * @param files 要传输的文件列表
     * @param targetDeviceId 目标设备ID
     * @param targetPath 目标路径
     */
    void transferRequested(const QList<FileItem>& files, const QString& targetDeviceId, const QString& targetPath);

public slots:
    /**
     * @brief 导航到上级目录
     */
    void navigateUp();

    /**
     * @brief 导航到根目录
     */
    void navigateToRoot();

    /**
     * @brief 导航到指定路径
     * @param path 目标路径
     */
    void navigateTo(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onPathEditReturnPressed();
    void onTableViewDoubleClicked(const QModelIndex& index);
    void onSelectionChanged();
    void onCustomContextMenu(const QPoint& point);
    void onCopyAction();
    void onPasteAction();
    void onDeleteAction();
    void onNewFolderAction();
    void onRefreshAction();
    void onFileListResponse(const QString& requestId, const QList<FileInfoData>& files, bool success, const QString& errorMessage);

private:
    void initUI();
    void initModel();
    void initToolBar();
    void loadLocalFiles(const QString& path);
    void loadRemoteFiles(const QString& path);
    void startDrag(const QList<FileItem>& files);
    void updatePathDisplay();
    void updateTitle();

    FileSystemType m_fileSystemType;      ///< 文件系统类型
    QString m_targetDeviceId;             ///< 目标设备ID
    QString m_currentPath;                ///< 当前路径
    QString m_pendingRequestId;           ///< 待处理的请求ID

    QLabel* m_titleLabel;                 ///< 标题标签
    QLineEdit* m_pathEdit;                ///< 路径输入框
    QTableView* m_fileTableView;          ///< 文件表格视图
    QToolBar* m_toolBar;                  ///< 工具栏
    QVBoxLayout* m_mainLayout;            ///< 主布局
    QHBoxLayout* m_pathLayout;            ///< 路径布局

    FileListModel* m_fileModel;           ///< 文件列表模型

    QPushButton* m_upButton;              ///< 上级目录按钮
    QPushButton* m_refreshButton;         ///< 刷新按钮
    QPushButton* m_homeButton;            ///< 主目录按钮

    QMenu* m_contextMenu;                 ///< 右键菜单
    QAction* m_copyAction;                ///< 复制动作
    QAction* m_pasteAction;               ///< 粘贴动作
    QAction* m_deleteAction;              ///< 删除动作
    QAction* m_newFolderAction;           ///< 新建文件夹动作
    QAction* m_refreshAction;             ///< 刷新动作

    QList<FileItem> m_clipboardFiles;     ///< 剪贴板文件
    QString m_clipboardSourcePath;        ///< 剪贴板源路径
};

#endif  // FILEBROWSERPANEL_H