#include "bigpicturetv.h"
#include "ui_bigpicturetv.h"
#include <QDesktopServices>
#include <QJsonParseError>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <iostream>

const QString BigPictureTV::settingsFile = QStandardPaths::writableLocation(
                                               QStandardPaths::AppDataLocation)
                                           + "/BigPictureTV/settings.json";

BigPictureTV::BigPictureTV(QWidget *parent)
    : QMainWindow(parent)
    , gamemodeActive(false)
    , nightLightSwitcher(new NightLightSwitcher())
    , audioManager(new AudioManager())
    , steamWindowManager(new SteamWindowManager())
    , shortcutManager(new ShortcutManager())
    , utils(new Utils())
    , firstRun(false)
    , activePowerPlan("")
    , nightLightState(false)
    , discordState(false)
    , ui(new Ui::BigPictureTV)
    , windowCheckTimer(new QTimer(this))
{
    ui->setupUi(this);
    setWindowIcon(utils->getIconForTheme());
    setupInfoTab();
    populateComboboxes();
    loadSettings();
    setupConnections();
    getAudioCapabilities();
    windowCheckTimer->setInterval(ui->checkrateSpinBox->value());
    connect(windowCheckTimer, &QTimer::timeout, this, &BigPictureTV::checkWindowTitle);
    windowCheckTimer->start();
    createTrayIcon();
    if (firstRun) {
        this->show();
    }
}

BigPictureTV::~BigPictureTV()
{
    saveSettings();
    delete steamWindowManager;
    delete nightLightSwitcher;
    delete ui;
}

void BigPictureTV::setupConnections()
{
    connect(ui->startupCheckBox, &QCheckBox::stateChanged, this, &BigPictureTV::onStartupCheckboxStateChanged);
    connect(ui->desktopAudioLineEdit, &QLineEdit::textChanged, this, &BigPictureTV::saveSettings);
    connect(ui->gamemodeAudioLineEdit, &QLineEdit::textChanged, this, &BigPictureTV::saveSettings);
    connect(ui->disableAudioCheckBox, &QCheckBox::checkStateChanged, this, &BigPictureTV::onDisableAudioCheckboxStateChanged);
    connect(ui->disableMonitorCheckBox, &QCheckBox::stateChanged, this, &BigPictureTV::onDisableMonitorCheckboxStateChanged);
    connect(ui->checkrateSpinBox, &QSpinBox::valueChanged, this, &BigPictureTV::onCheckrateSpinBoxValueChanged);
    connect(ui->closeDiscordCheckBox, &QCheckBox::stateChanged, this, &BigPictureTV::saveSettings);
    connect(ui->pauseMediaAction, &QCheckBox::stateChanged, this, &BigPictureTV::saveSettings);
    connect(ui->enablePerformancePowerPlan,  &QCheckBox::stateChanged, this, &BigPictureTV::saveSettings);
    connect(ui->desktopMonitorComboBox, &QComboBox::currentIndexChanged, this, &BigPictureTV::saveSettings);
    connect(ui->gamemodeMonitorComboBox, &QComboBox::currentIndexChanged, this, &BigPictureTV::saveSettings);
    connect(ui->installAudioButton,  &QPushButton::clicked, this, &BigPictureTV::onAudioButtonClicked);
    connect(ui->targetWindowComboBox, &QComboBox::currentIndexChanged, this, &BigPictureTV::onTargetWindowComboBoxIndexChanged);
    connect(ui->resetSettingsButton, &QPushButton::clicked, this, &BigPictureTV::createDefaultSettings);
    connect(ui->openSettingsButton, &QPushButton::clicked, this, []() {
        QString settingsFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDesktopServices::openUrl(QUrl::fromLocalFile(settingsFolder));
    });

    ui->startupCheckBox->setChecked(shortcutManager->isShortcutPresent());
    initDiscordAction();
}

void BigPictureTV::initDiscordAction()
{
    if (!utils->isDiscordInstalled()) {
        ui->closeDiscordCheckBox->setChecked(false);
        ui->closeDiscordCheckBox->setEnabled(false);
        ui->CloseDiscordLabel->setEnabled(false);
        ui->closeDiscordCheckBox->setToolTip(tr("Discord does not appear to be installed"));
        ui->CloseDiscordLabel->setToolTip(tr("Discord does not appear to be installed"));
    }
}
void BigPictureTV::getAudioCapabilities()
{
    if (!utils->isAudioDeviceCmdletsInstalled()) {
        ui->disableAudioCheckBox->setChecked(true);
        ui->disableAudioCheckBox->setEnabled(false);
        toggleAudioSettings(false);
    } else {
        ui->disableAudioCheckBox->setEnabled(true);
        ui->installAudioButton->setEnabled(false);
        ui->installAudioButton->setText(tr("Audio module installed"));
        if (!ui->disableAudioCheckBox->isChecked()) {
            toggleAudioSettings(true);
        }
    }
}

void BigPictureTV::populateComboboxes()
{
    ui->desktopMonitorComboBox->addItem(tr("Internal"));
    ui->desktopMonitorComboBox->addItem(tr("Extend"));
    ui->gamemodeMonitorComboBox->addItem(tr("External"));
    ui->gamemodeMonitorComboBox->addItem(tr("Clone"));

    ui->targetWindowComboBox->addItem(tr("Big Picture"));
    ui->targetWindowComboBox->addItem(tr("Custom"));
}

void BigPictureTV::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(utils->getIconForTheme(), this);
    trayIcon->setToolTip("BigPictureTV");
    trayIcon->setContextMenu(createMenu());
    trayIcon->show();
}

QMenu *BigPictureTV::createMenu()
{
    QMenu *menu = new QMenu(this);

    QAction *settingsAction = new QAction(tr("Settings"), this);
    connect(settingsAction, &QAction::triggered, this, &BigPictureTV::showSettings);

    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);

    menu->addAction(settingsAction);
    menu->addAction(exitAction);

    return menu;
}

void BigPictureTV::showSettings()
{
    this->show();
}

void BigPictureTV::onCheckrateSpinBoxValueChanged()
{
    int newInterval = ui->checkrateSpinBox->value();
    windowCheckTimer->setInterval(newInterval);
    saveSettings();
}

void BigPictureTV::onStartupCheckboxStateChanged()
{
    shortcutManager->manageShortcut(ui->startupCheckBox->isChecked());
}

void BigPictureTV::onDisableAudioCheckboxStateChanged(int state)
{
    bool isChecked = (state == Qt::Checked);
    toggleAudioSettings(!isChecked);
    saveSettings();
}

void BigPictureTV::onDisableMonitorCheckboxStateChanged(int state)
{
    bool isChecked = (state == Qt::Checked);
    toggleMonitorSettings(!isChecked);
    saveSettings();
}

void BigPictureTV::onTargetWindowComboBoxIndexChanged(int index)
{
    if (index == 1) {
        toggleCustomWindowTitle(true);
    } else {
        toggleCustomWindowTitle(false);
    }
    saveSettings();
}

void BigPictureTV::onAudioButtonClicked()
{
    ui->installAudioButton->setEnabled(false);

    QProcess process;
    process.start(
        "powershell",
        QStringList() << "-NoProfile" << "-ExecutionPolicy" << "Bypass" << "-Command"
                      << "Install-PackageProvider -Name NuGet -Force -Scope CurrentUser; "
                         "Install-Module -Name AudioDeviceCmdlets -Force -Scope CurrentUser");

    if (!process.waitForFinished()) {
        QString status = tr("Error");
        QString message = tr("Failed to execute the PowerShell commands.\n"
                             "Please check if PowerShell is installed and properly configured.");
        QMessageBox::critical(this, status, message);
    } else {
        QString output = process.readAllStandardOutput();
        QString errorOutput = process.readAllStandardError();
        int exitCode = process.exitCode();

        QString status;
        QString message;

        if (exitCode == 0) {
            status = tr("Success");
            message = tr("AudioDeviceCmdlets module installed successfully.\nYou can now use audio "
                         "settings.");
            QMessageBox::information(this, status, message);
        } else {
            status = tr("Error");
            message = tr("Failed to install NuGet package provider or AudioDeviceCmdlets module.\n"
                         "Please install them manually by running these commands in PowerShell:\n"
                         "Install-PackageProvider -Name NuGet -Force -Scope CurrentUser;\n"
                         "Install-Module -Name AudioDeviceCmdlets -Force -Scope CurrentUser\n"
                         "You should then restart the application.\n"
                         "Error details:\n")
                      + errorOutput;
            QMessageBox::critical(this, status, message);
            ui->installAudioButton->setEnabled(true);
        }
    }

    getAudioCapabilities();
}

void BigPictureTV::checkWindowTitle()
{
    if (ui->targetWindowComboBox->currentIndex() == 1) {
        if (ui->customWindowLineEdit->text().isEmpty() || ui->customWindowLineEdit->hasFocus()) {
            return;
        }
    }

    if (utils->isSunshineStreaming()) {
        return;
    }

    bool disableVideo = ui->disableMonitorCheckBox->isChecked();
    bool disableAudio = ui->disableAudioCheckBox->isChecked();
    bool isRunning;
    if (ui->targetWindowComboBox->currentIndex() == 0) {
        isRunning = steamWindowManager->isBigPictureRunning();
    } else if (ui->targetWindowComboBox->currentIndex() == 1) {
        isRunning = steamWindowManager->isCustomWindowRunning(ui->customWindowLineEdit->text());
    }

    if (isRunning && !gamemodeActive) {
        gamemodeActive = true;
        handleActions(false);
        handleMonitorChanges(false, disableVideo);
        handleAudioChanges(false, disableAudio);
    } else if (!isRunning && gamemodeActive) {
        gamemodeActive = false;
        handleActions(true);
        handleMonitorChanges(true, disableVideo);
        handleAudioChanges(true, disableAudio);
    }
}

void BigPictureTV::handleMonitorChanges(bool isDesktopMode, bool disableVideo)
{
    if (disableVideo)
        return;

    int index = isDesktopMode ? ui->desktopMonitorComboBox->currentIndex()
                              : ui->gamemodeMonitorComboBox->currentIndex();

    const char *command = nullptr;

    if (index == 0) {
        command = isDesktopMode ? "/internal" : "/external";
    } else if (index == 1) {
        command = isDesktopMode ? "/extend" : "/clone";
    }

    if (command) {
        utils->runEnhancedDisplayswitch(command);
    }
}

void BigPictureTV::handleAudioChanges(bool isDesktopMode, bool disableAudio)
{
    if (disableAudio)
        return;

    std::string audioDevice = isDesktopMode ? ui->desktopAudioLineEdit->text().toStdString()
                                            : ui->gamemodeAudioLineEdit->text().toStdString();

    try {
        audioManager->setAudioDevice(audioDevice);
    } catch (const std::runtime_error &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void BigPictureTV::handleActions(bool isDesktopMode)
{
    if (ui->closeDiscordCheckBox->isChecked()) {
        handleDiscordAction(isDesktopMode);
    }
    if (ui->enablePerformancePowerPlan->isChecked()) {
        handleNightLightAction(isDesktopMode);
    }
    if (ui->disableNightLightCheckBox->isChecked()) {
        handlePowerPlanAction(isDesktopMode);
    }
    if (ui->pauseMediaAction->isChecked()) {
        handleMediaAction(isDesktopMode);
    }
}

void BigPictureTV::handleDiscordAction(bool isDesktopMode)
{
    if (isDesktopMode) {
        if (discordState) {
            utils->startDiscord();
        }
    } else {
        discordState = utils->isDiscordRunning();
        utils->closeDiscord();
    }
}

void BigPictureTV::handleNightLightAction(bool isDesktopMode)
{
    if (isDesktopMode) {
        if (nightLightState) {
            nightLightSwitcher->enable();
        }
    } else {
        nightLightState = nightLightSwitcher->enabled();
        nightLightSwitcher->disable();
    }
}

void BigPictureTV::handleMediaAction(bool isDesktopMode)
{
    if (!isDesktopMode) {
        utils->sendMediaKey(VK_MEDIA_STOP);
    }
}

void BigPictureTV::handlePowerPlanAction(bool isDesktopMode)
{
    if (isDesktopMode) {
        if (!activePowerPlan.isEmpty()) {
            utils->setPowerPlan(activePowerPlan);
        } else {
            utils->setPowerPlan("381b4222-f694-41f0-9685-ff5bb260df2e");
        }
    } else {
        activePowerPlan = utils->getActivePowerPlan();
        utils->setPowerPlan("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c");
    }
}

void BigPictureTV::createDefaultSettings()
{
    firstRun = true;
    ui->desktopAudioLineEdit->setText("Headset");
    ui->gamemodeAudioLineEdit->setText("TV");
    ui->desktopMonitorComboBox->setCurrentIndex(0);
    ui->gamemodeMonitorComboBox->setCurrentIndex(0);
    ui->closeDiscordCheckBox->setChecked(false);
    ui->enablePerformancePowerPlan->setChecked(false);
    ui->startupCheckBox->setChecked(false);
    ui->disableAudioCheckBox->setChecked(false);
    ui->disableMonitorCheckBox->setChecked(false);
    ui->checkrateSpinBox->setValue(1000);
    ui->disableNightLightCheckBox->setChecked(false);
    saveSettings();
}

void BigPictureTV::loadSettings()
{
    QDir settingsDir(QFileInfo(settingsFile).absolutePath());
    if (!settingsDir.exists()) {
        settingsDir.mkpath(settingsDir.absolutePath());
    }

    QFile file(settingsFile);
    if (!file.exists()) {
        createDefaultSettings();

    } else {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                settings = doc.object();
            }
            file.close();
        }
    }
    applySettings();
}

void BigPictureTV::applySettings()
{
    ui->gamemodeAudioLineEdit->setText(settings.value("gamemode_audio").toString());
    ui->desktopAudioLineEdit->setText(settings.value("desktop_audio").toString());
    ui->disableAudioCheckBox->setChecked(settings.value("disable_audio_switch").toBool());
    ui->checkrateSpinBox->setValue(settings.value("checkrate").toInt(1000));
    ui->closeDiscordCheckBox->setChecked(settings.value("close_discord_action").toBool(false));
    ui->enablePerformancePowerPlan->setChecked(settings.value("gamemode_powerplan").toBool(false));
    ui->pauseMediaAction->setChecked(settings.value("gamemode_pause_media").toBool(false));
    ui->gamemodeMonitorComboBox->setCurrentIndex(settings.value("gamemode_monitor").toInt(0));
    ui->desktopMonitorComboBox->setCurrentIndex(settings.value("desktop_monitor").toInt(0));
    ui->disableMonitorCheckBox->setChecked(settings.value("disable_monitor_switch").toBool());
    ui->disableNightLightCheckBox->setChecked(settings.value("disable_nightlight").toBool());
    ui->targetWindowComboBox->setCurrentIndex(settings.value("target_window").toInt(0));
    ui->customWindowLineEdit->setText(settings.value("custom_window").toString());
    toggleAudioSettings(!ui->disableAudioCheckBox->isChecked());
    toggleMonitorSettings(!ui->disableMonitorCheckBox->isChecked());
    toggleCustomWindowTitle(ui->targetWindowComboBox->currentIndex() == 1);
}

void BigPictureTV::saveSettings()
{
    settings["gamemode_audio"] = ui->gamemodeAudioLineEdit->text();
    settings["desktop_audio"] = ui->desktopAudioLineEdit->text();
    settings["disable_audio_switch"] = ui->disableAudioCheckBox->isChecked();
    settings["checkrate"] = ui->checkrateSpinBox->value();
    settings["close_discord_action"] = ui->closeDiscordCheckBox->isChecked();
    settings["gamemode_powerplan"] = ui->enablePerformancePowerPlan->isChecked();
    settings["gamemode_pause_media"] = ui->pauseMediaAction->isChecked();
    settings["gamemode_monitor"] = ui->gamemodeMonitorComboBox->currentIndex();
    settings["desktop_monitor"] = ui->desktopMonitorComboBox->currentIndex();
    settings["disable_monitor_switch"] = ui->disableMonitorCheckBox->isChecked();
    settings["disable_nightlight"] = ui->disableNightLightCheckBox->isChecked();
    settings["target_window"] = ui->targetWindowComboBox->currentIndex();
    settings["custom_window"] = ui->customWindowLineEdit->text();

    QFile file(settingsFile);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(settings);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void BigPictureTV::toggleAudioSettings(bool state)
{
    ui->desktopAudioLineEdit->setEnabled(state);
    ui->desktopAudioLabel->setEnabled(state);
    ui->gamemodeAudioLineEdit->setEnabled(state);
    ui->gamemodeAudioLabel->setEnabled(state);
}

void BigPictureTV::toggleMonitorSettings(bool state)
{
    ui->desktopMonitorComboBox->setEnabled(state);
    ui->desktopMonitorLabel->setEnabled(state);
    ui->gamemodeMonitorComboBox->setEnabled(state);
    ui->gamemodeMonitorLabel->setEnabled(state);
}

void BigPictureTV::toggleCustomWindowTitle(bool state)
{
    ui->customWindowLineEdit->setEnabled(state);
    ui->customWindowLabel->setEnabled(state);
}

void BigPictureTV::setupInfoTab()
{
    ui->detectedSteamLanguage->setText(steamWindowManager->getSteamLanguage());

    ui->targetWindowTitle->setText(steamWindowManager->getBigPictureWindowTitle());
    ui->repository->setText("<a href=\"https://github.com/odizinne/bigpicturetv/\">github.com/Odizinne/BigPictureTV</a>");
    ui->repository->setTextFormat(Qt::RichText);
    ui->repository->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->repository->setOpenExternalLinks(true);
    ui->commitID->setText(GIT_COMMIT_ID);
    ui->commitDate->setText(GIT_COMMIT_DATE);
}
