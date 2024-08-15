#ifndef BIGPICTURETV_H
#define BIGPICTURETV_H

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMainWindow>
#include <QMenu>
#include <QString>
#include <QSystemTrayIcon>
#include <QTimer>
#include <string>

namespace Ui {
class BigPictureTV;
}

class AboutWindow;

class BigPictureTV : public QMainWindow
{
    Q_OBJECT

public:
    explicit BigPictureTV(QWidget *parent = nullptr);
    ~BigPictureTV();
    void createDefaultSettings();
    void loadSettings();
    void saveSettings();
    void applySettings();

private slots:
    void onStartupCheckboxStateChanged();
    void onDisableAudioCheckboxStateChanged(int state);
    void onDisableMonitorCheckboxStateChanged(int state);
    void onAudioButtonClicked();
    void checkWindowTitle();
    void onCheckrateSpinBoxValueChanged();
    void showSettings();
    void showAbout();

private:
    QSystemTrayIcon *trayIcon;

    void createMenubar();
    void toggleMenubarVisibility();
    void createTrayIcon();
    void initDiscordAction();
    QMenu *createMenu();
    bool gamemodeActive;
    bool discordInstalled;
    bool firstRun;
    void getAudioCapabilities();
    void setCustomFusion();
    void populateComboboxes();
    void toggleAudioSettings(bool state);
    void toggleMonitorSettings(bool state);
    void setupConnections();
    void handleMonitorChanges(bool isDesktopMode, bool disableVideo);
    void handleAudioChanges(bool isDesktopMode, bool disableAudio);
    void handleActions(bool isDesktopMode);

    Ui::BigPictureTV *ui;
    QTimer *windowCheckTimer;
    std::wstring shortcutName = L"BigPictureTV.lnk";
    QString settingsFilePath;
    QJsonObject settings;
    static const QString settingsFile;
    QString status;
    QString message;
    QMenuBar *menuBar;
    bool menubarVisible;
    void setFont();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

};

#endif // BIGPICTURETV_H
