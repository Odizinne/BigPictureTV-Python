import os
import sys
import json
from PyQt6.QtWidgets import QApplication, QSystemTrayIcon, QMenu, QMainWindow, QMessageBox
from PyQt6.QtGui import QIcon, QAction
from PyQt6.QtCore import Qt, QTimer, QSharedMemory, QTranslator, QLocale
from settings_window import Ui_SettingsWindow
from tooltiped_slider import TooltipedSlider
import audio_manager as AudioManager
import mode_manager as ModeManager
import shortcut_manager as ShortcutManager
import color_utils as ColorUtils
import utils

SETTINGS_FILE = os.path.join(os.environ["APPDATA"], "BigPictureTV", "settings.json")
ICONS_FOLDER = "icons" if getattr(sys, "frozen", False) else os.path.join(os.path.dirname(__file__), "icons")


class BigPictureTV(QMainWindow):
    def __init__(self):
        super().__init__()
        self.ui = Ui_SettingsWindow()
        self.ui.setupUi(self)
        self.setWindowIcon(QIcon(os.path.join(ICONS_FOLDER, f"icon_desktop_{utils.get_theme()}.png")))
        self.current_mode = ModeManager.read_current_mode()
        self.settings = {}
        self.first_run = False
        self.paused = False
        self.set_fusion_frames()
        self.init_checkrate_tooltipedslider()
        self.populate_comboboxes()
        self.load_settings()
        self.setup_ui_connections()
        self.get_audio_capabilities()
        self.tray_icon = self.create_tray_icon()
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_mode)
        self.update_mode_timer_interval(self.ui.checkrate_slider.value())
        if self.first_run:
            self.show()
            self.first_run = False

    def set_fusion_frames(self):
        if app.style().objectName() == "fusion":
            ColorUtils.set_frame_color_based_on_window(self, self.ui.actions_frame)
            ColorUtils.set_frame_color_based_on_window(self, self.ui.monitor_frame)
            ColorUtils.set_frame_color_based_on_window(self, self.ui.audio_frame)
            ColorUtils.set_frame_color_based_on_window(self, self.ui.settings_frame)

    def setup_ui_connections(self):
        self.ui.disable_audio_checkbox.stateChanged.connect(self.handle_disableaudio_checkbox_state_changed)
        self.ui.gamemode_audio_lineedit.textChanged.connect(self.save_settings)
        self.ui.desktop_audio_lineedit.textChanged.connect(self.save_settings)
        self.ui.startup_checkbox.setChecked(ShortcutManager.check_startup_shortcut())
        self.ui.close_discord_checkbox.stateChanged.connect(self.save_settings)
        self.ui.close_discord_checkbox.setEnabled(utils.is_discord_installed())
        self.ui.close_discord_label.setEnabled(utils.is_discord_installed())
        self.ui.startup_checkbox.stateChanged.connect(self.handle_startup_checkbox_state_changed)
        self.ui.gamemode_monitor_combobox.currentIndexChanged.connect(self.save_settings)
        self.ui.desktop_monitor_combobox.currentIndexChanged.connect(self.save_settings)
        self.ui.checkrate_slider.valueChanged.connect(self.handle_checkrate_slider_value_changed)
        self.ui.checkrate_slider.sliderReleased.connect(self.save_settings)
        self.ui.install_audio_button.clicked.connect(self.handle_audio_button_clicked)
        self.ui.disable_monitor_checkbox.stateChanged.connect(self.handle_disablemonitor_checkbox_state_changed)

    def populate_comboboxes(self):
        self.ui.gamemode_monitor_combobox.addItem(self.tr("External"))
        self.ui.gamemode_monitor_combobox.addItem(self.tr("Clone"))
        self.ui.desktop_monitor_combobox.addItem(self.tr("Internal"))
        self.ui.desktop_monitor_combobox.addItem(self.tr("Extend"))

    def init_checkrate_tooltipedslider(self):
        self.checkrate_tooltipedslider = TooltipedSlider(Qt.Orientation.Horizontal, self.ui.centralwidget)
        layout = self.ui.settings_layout
        index = layout.indexOf(self.ui.checkrate_label)
        position = layout.getItemPosition(index)
        row = position[0]
        column = position[1]
        layout.addWidget(self.checkrate_tooltipedslider, row, column + 1)
        self.ui.checkrate_slider.deleteLater()
        self.ui.checkrate_slider = self.checkrate_tooltipedslider
        self.ui.checkrate_slider.setTickInterval(100)
        self.ui.checkrate_slider.setRange(100, 1000)

    def handle_startup_checkbox_state_changed(self, state):
        ShortcutManager.manage_startup_shortcut(state)

    def handle_audio_button_clicked(self):
        self.ui.install_audio_button.setEnabled(False)
        status, message = utils.install_audio_module()
        if status == "Success":
            QMessageBox.information(self, status, message)
        else:
            QMessageBox.critical(self, status, message)
            self.ui.install_audio_button.setEnabled(True)

        self.get_audio_capabilities()

    def handle_checkrate_slider_value_changed(self, value):
        remainder = value % 100
        if remainder != 0:
            if remainder < 50:
                value -= remainder
            else:
                value += 100 - remainder
            self.ui.checkrate_slider.setValue(value)

        self.update_mode_timer_interval(value)

    def handle_disableaudio_checkbox_state_changed(self, state):
        self.toggle_audio_settings(not state)
        self.save_settings()

    def handle_disablemonitor_checkbox_state_changed(self, state):
        self.toggle_monitor_settings(not state)
        self.save_settings()

    def toggle_audio_settings(self, enabled):
        self.ui.desktop_audio_lineedit.setEnabled(enabled)
        self.ui.desktop_audio_label.setEnabled(enabled)
        self.ui.gamemode_audio_lineedit.setEnabled(enabled)
        self.ui.gamemode_audio_label.setEnabled(enabled)
        self.ui.audio_output_label.setEnabled(enabled)

    def toggle_monitor_settings(self, enabled):
        self.ui.gamemode_monitor_combobox.setEnabled(enabled)
        self.ui.desktop_monitor_combobox.setEnabled(enabled)
        self.ui.gamemode_monitor_label.setEnabled(enabled)
        self.ui.desktop_monitor_label.setEnabled(enabled)
        self.ui.monitor_configuration_label.setEnabled(enabled)

    def create_default_settings(self):
        self.settings = {
            "gamemode_audio": "TV",
            "desktop_audio": "Headset",
            "disable_audio_switch": False,
            "checkrate": 1000,
        }
        self.apply_settings()
        self.save_settings()
        self.first_run = True

    def load_settings(self):
        if not os.path.exists(os.path.dirname(SETTINGS_FILE)):
            os.makedirs(os.path.dirname(SETTINGS_FILE))
        if not os.path.exists(SETTINGS_FILE):
            self.create_default_settings()
        else:
            with open(SETTINGS_FILE, "r") as f:
                self.settings = json.load(f)
        self.apply_settings()

    def apply_settings(self):
        self.ui.gamemode_audio_lineedit.setText(self.settings.get("gamemode_audio", ""))
        self.ui.desktop_audio_lineedit.setText(self.settings.get("desktop_audio", ""))
        self.ui.disable_audio_checkbox.setChecked(self.settings.get("disable_audio_switch", False))
        self.ui.checkrate_slider.setValue(self.settings.get("checkrate", 1000))
        self.ui.close_discord_checkbox.setChecked(self.settings.get("discord_action", False))
        self.ui.performance_powerplan_checkbox.setChecked(self.settings.get("powerplan_action", False))
        self.ui.gamemode_monitor_combobox.setCurrentIndex(self.settings.get("gamemode_monitor", 0))
        self.ui.desktop_monitor_combobox.setCurrentIndex(self.settings.get("desktop_monitor", 0))
        self.ui.disable_monitor_checkbox.setChecked(self.settings.get("disable_monitor_switch", False))
        self.toggle_audio_settings(not self.ui.disable_audio_checkbox.isChecked())
        self.toggle_monitor_settings(not self.ui.disable_monitor_checkbox.isChecked())

    def save_settings(self):
        self.settings = {
            "gamemode_audio": self.ui.gamemode_audio_lineedit.text(),
            "desktop_audio": self.ui.desktop_audio_lineedit.text(),
            "disable_audio_switch": self.ui.disable_audio_checkbox.isChecked(),
            "checkrate": self.ui.checkrate_slider.value(),
            "discord_action": self.ui.close_discord_checkbox.isChecked(),
            "powerplan_action": self.ui.performance_powerplan_checkbox.isChecked(),
            "gamemode_monitor": self.ui.gamemode_monitor_combobox.currentIndex(),
            "desktop_monitor": self.ui.desktop_monitor_combobox.currentIndex(),
            "disable_monitor_switch": self.ui.disable_monitor_checkbox.isChecked(),
        }
        os.makedirs(os.path.dirname(SETTINGS_FILE), exist_ok=True)
        with open(SETTINGS_FILE, "w") as f:
            json.dump(self.settings, f, indent=4)

    def switch_mode(self, mode):
        if mode == self.current_mode:
            return

        self.current_mode = mode
        self.handle_monitor_changes(self.current_mode)
        self.handle_actions(self.current_mode)
        self.handle_audio_changes(self.current_mode)
        ModeManager.write_current_mode(self.current_mode)

        if self.tray_icon:
            variant = "gamemode" if self.current_mode == ModeManager.Mode.GAMEMODE else "desktop"
            self.tray_icon.setIcon(QIcon(os.path.join(ICONS_FOLDER, f"icon_{variant}_{utils.get_theme()}.png")))
            self.tray_icon.setContextMenu(self.create_menu())

    def handle_monitor_changes(self, mode):
        if not self.ui.disable_monitor_checkbox.isChecked():
            if mode == ModeManager.Mode.GAMEMODE and self.ui.gamemode_monitor_combobox.currentIndex() == 0:
                monitor_mode = "/external"
            elif mode == ModeManager.Mode.GAMEMODE and self.ui.gamemode_monitor_combobox.currentIndex() == 1:
                monitor_mode = "/clone"
            elif mode == ModeManager.Mode.DESKTOP and self.ui.desktop_monitor_combobox.currentIndex() == 0:
                monitor_mode = "/internal"
            elif mode == ModeManager.Mode.DESKTOP and self.ui.desktop_monitor_combobox.currentIndex() == 1:
                monitor_mode = "/extend"
            utils.run_displayswitch(monitor_mode)

    def handle_actions(self, mode):
        if self.ui.close_discord_checkbox.isChecked():
            utils.close_discord() if mode == ModeManager.Mode.GAMEMODE else utils.start_discord()

        if self.ui.performance_powerplan_checkbox.isChecked():
            if mode == ModeManager.Mode.GAMEMODE:
                utils.switch_power_plan("8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c")  # High Performance
            else:
                utils.switch_power_plan("381b4222-f694-41f0-9685-ff5bb260df2e")  # Balanced

    def handle_audio_changes(self, mode):
        if not self.ui.disable_audio_checkbox.isChecked():
            gamemode_audio = self.ui.gamemode_audio_lineedit.text()
            desktop_audio = self.ui.desktop_audio_lineedit.text()
            AudioManager.switch_audio(gamemode_audio if mode == ModeManager.Mode.GAMEMODE else desktop_audio)

    def get_audio_capabilities(self):
        if not utils.is_audio_device_cmdlets_installed():
            self.ui.disable_audio_checkbox.setChecked(True)
            self.ui.disable_audio_checkbox.setEnabled(False)
            self.toggle_audio_settings(False)
        else:
            self.ui.disable_audio_checkbox.setEnabled(True)
            self.ui.install_audio_button.setVisible(False)
            if not self.ui.disable_audio_checkbox.isChecked():
                self.toggle_audio_settings(True)
            self.adjustSize()

    def update_mode(self):
        if (
            utils.is_bigpicture_running()
            and self.current_mode != ModeManager.Mode.GAMEMODE
            and not utils.is_sunshine_stream_active()
        ):
            self.switch_mode(ModeManager.Mode.GAMEMODE)
        elif not utils.is_bigpicture_running() and self.current_mode != ModeManager.Mode.DESKTOP:
            self.switch_mode(ModeManager.Mode.DESKTOP)

    def update_mode_timer_interval(self, check_rate):
        self.timer.setInterval(check_rate)
        self.timer.start()

    def create_tray_icon(self):
        tray_icon = QSystemTrayIcon(QIcon(os.path.join(ICONS_FOLDER, f"icon_desktop_{utils.get_theme()}.png")))
        tray_icon.setToolTip("BigPictureTV")
        tray_icon.setContextMenu(self.create_menu())
        tray_icon.show()
        return tray_icon

    def create_menu(self):
        menu = QMenu()
        pause_resume_action = QAction(self.tr("Resume detection") if self.paused else self.tr("Pause detection"), menu)
        pause_resume_action.triggered.connect(self.toggle_detection)
        settings_action = QAction(self.tr("Settings"), menu)
        settings_action.triggered.connect(self.show)
        exit_action = QAction(self.tr("Exit"), menu)
        exit_action.triggered.connect(QApplication.quit)

        menu.addAction(pause_resume_action)
        menu.addAction(settings_action)
        menu.addAction(exit_action)

        return menu

    def toggle_detection(self):
        self.paused = not self.paused
        if self.paused:
            self.timer.stop()
            self.tray_icon.setToolTip("BigPictureTV (Paused)")
        else:
            self.timer.start()
            self.tray_icon.setToolTip("BigPictureTV")
        self.tray_icon.setContextMenu(self.create_menu())

    def closeEvent(self, event):
        event.ignore()
        self.hide()


if __name__ == "__main__":
    shared_memory = QSharedMemory("BigPictureTVSharedMemory")
    if shared_memory.attach() or not shared_memory.create(1):
        sys.exit(0)

    app = QApplication(sys.argv)
    if utils.is_windows_10():
        app.setStyle("Fusion")

    translator = QTranslator()
    locale_name = QLocale.system().name()
    locale = locale_name[:2]
    if locale:
        file_name = f"tr/bigpicturetv_{locale}.qm"
    else:
        file_name = None

    if file_name and translator.load(file_name):
        app.installTranslator(translator)

    big_picture_tv = BigPictureTV()
    sys.exit(app.exec())
