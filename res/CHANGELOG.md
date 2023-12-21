### 6.05
#### Released TBD
* Scripting-setting to select whether to upload the editor content or file content.
* Autocompletion and bracket matching completion improvements.
* Support for selecting multiple files when saving and deleting files from the log browser.
* Removed firmware and package tabs and replaced them with buttons.
* Shorter fault stop time for esk8 and balance in FOC wizard.
* Sanity-check motor parameters on write.
* Log analysis graph and table alignment fix.
* Disable BMS limits on balance vehicles in wizard.
* Ctrl+R to run selected code block LBM in repl.
* Script editor search improvements.
* Check if there are unsaved scripts before closing.
* Better lisp code read import handling.
* Added filter-box to example and recent lists in script editors.
* Limit output size in vesc and lbm terminal to prevent problems when printing too much.
* Use all 3 measured currents in sampled data instead of calculating one.
* Added support for loading sampled data CSV files.

---

### 6.02
#### Released 2023-03-12
* Improved experiment plot.
* Updated PPM and ADC mapping widgets.
* Better compatibility with old firmwares.
* LispBM
	* More examples
	* Rearranged UI
	* Added experiment plot
* Mobile custom package install support.
* Old firmware download RAM usage fix.
* Config backup/restore improvements.
* Automatically reload UI after installing and uninstalling packages.
* Automatically scan mobile CAN-list if empty.
* Changelogs updated and changed to markdown.
* Pixmap caching for faster start and config reload.
* Firmware archive added in mobile VESC Tool.
* Added display tool.
* QML upload bug fixes.

---

### 6.00
#### Released 2022-12-08
* Added support for compressed firmware upload.
* Reduce CPU usage by only plotting RT data when the plot is visible.
* Lisp scripting editor and integration.
	* Autocompletion.
	* Syntax highlighting.
	* Real-time binding, CPU and memory monitors.
	* Console for printing
	* Examples
* Added custom load code support to motor comparison tool.
* Added support for bitfield parameters.
* Added VESC UDP broadcast listener.
* Added VESC package creation and loading support.
* Added VESC package store. See https://github.com/vedderb/vesc_pkg
* Added TCP bridge.
* Custom config backup and restore.
* Connect screen on welcome page.
* Attempt at BLE-support on windows.
* Save and load support of configs on mobile.
* Custom config pages on mobile.
* Wizard and multisettings improvements when connected over CAN.

---

### 3.01
#### Released 2022-01-16
* Fixed simultaneous CAN FW upload when other devices (such as BMS) are on the CAN-bus.
* Fixed configuration backup and restore over CAN-bus.
* Added test version information to about dialog.
* Added scripting page
	* Syntax highlighting.
	* Recent files.
	* Example files.
	* Auto-completion tree.
	* Run in widget, window or full screen.
	* Debug print output.
	* Toggle line comment.
	* Auto-indentation of line or block.
	* Search, highlight and replace text.
* Only capture esc key for stopping the motor when connected.
* Fixed FOC detect all no can when connected over CAN.
* Increased motor and mosfet temperature limit editor range.
* Mobile RT display update. See: https://github.com/vedderb/vesc_tool/pull/115
* Added FOC wizard, setup data and profiles from mobile UI to desktop UI.
* Added usage page to FOC wizard.
* Added servo output control slider.
* Added support for loading custom UI from firmware.
* Dark theme.
* Mobile UI refactoring.
* Configurable data polling rates.
* Better DPI scaling.
* Configurable plot line thickness.
* Mobile:
	* New CAN forwarding bar.
	* Setting to disable screen rotation.
	* New RT Data page and gauges.
	* Statistics page.
* Upload bootloader before firmware if firmware has bootloader erase support.
* Added motor comparison tool.

---

### 3.00
#### Released 2021-01-11
* Focus for this release: External hardware and BMS support.
	* Added hw type setting.
	* Hide VESC settings when connected to other hardware.
	* Added BMS commands
* Other updates
	* Only allow uploading firmware to all VESCs when they have the same hardware version.
	* Added UDP server and client.
	* Added NRF52 UICR read and write support to SWDPROG page.
	* Realtime IMU and balance data. See: https://github.com/vedderb/vesc_tool/pull/91
	* Made CAN list resizeable.
	* Made FOC detection result dialog scrollable.
	* Added gamepad control support.
	* Use QStandardPaths::AppDataLocation for OSM tile cache.
	* Added more custom firmare paths to SWDPROG to easily switch between firmware files.

---

### 2.06
#### Released ?
* Prevent repeated test version notification dialogs.
* Made IMU calibration more compact to decrease the minimum horizontal size.

---

### 2.05
#### Released 2020-04-27
* FW 5.01 support:
	* Fixed PPM bug in previous release.

---

### 2.04
#### Released 2020-04-27
* FW 5.0 support:
	* Dual motor support, for e.g. unity.
* Added NRF pair to VESC Remote page.
* Added detect all without CAN button.
* Updated observer gain calculation.
* Added support for FW test build flag.
* Added unity firmwares.
* Added stormcore firmwares.
* Removed some uncommon firmwares.
* Disable LZO FW compression if 3 consecutive chunks fail.
* Added CAN-list with connected VESCs.
* Filter pages based on selected motor and app.
* Log setup values in desktop VESC Tool too.
* Added IMU orientation calibration widget.

---

### 2.03
#### Released ?
* App balance updates. See https://github.com/vedderb/bldc/pull/138.
* Changed back FOC time constant.
* Added test version warning.
* Remind user to switch on the location service on android 10 if BLE scan fails.
* Updated to Qt 5.12.7 for the android build.
* Start linux version without openssl on version mismatch.

---

### 2.02
#### Released ?
* Better autoconnect.
* Increased maximum file size for SWD prog.

---

### 2.01
#### Released ?
* Upadted NRF52 firmware to support NRF remotes at the same time as VESC Tool is connected.
* Fixed swdprog bootloader paths.
* Added option to remove experiment plot line.
* Disconnect port after firmware upload.
* Support for FW 4.00, which has HFI support.
* Try regular FW chunk write if LZO write fails.
* Use 4 ms current controller time constant by default.
* Added TCP server to desktop VESC Tool.
* Added CAN analyzer page.

---

### 2.00
#### Released ?
* Focus for this release: increase backwards compatibility with old firmwares.
	* Include XML files for old firmwares. Currently 3.60	* 3.66, but more will be added.
	* Store grouping information in XML files to allow changing the GUI dynamically based on firmware version.
	* Support for more commands in limited mode.
	* Support for reading and writing motor and app configuration in limited mode.
	* Updated _all_ configuration pages to use the grouping information, and to dynamically reload when the configuration changes.
	* Keep a record of known issues with old firmwares, and warn if a problematic firmware is in use.
	* Support for making/restoring backups of the motor and app configurations of all VESCs on the CAN Bus.
* Other updates
	* Store XML configurations with predefined parameter order instead of randomly.
	* Use OSM tile server by default as the higher resolution one is quite slow.
	* Added support for VESC 100/250.
	* Clear arrays in settings before writing to them.
	* Sync settings at end of write.
	* Fixed temperature plot visibility boxes.
	* Made yAxis2 visible on FOC RT data.
	* Better tooltip on keyboard control button.
	* Show message when configuration verification fails.

---

### 1.29
#### Released ?
* Replaced miniLZO with lzokay to resolve crash on Android, and to get a bit better compression ratio.

---

### 1.28
#### Released 2019-12-22
* FW 3.65:
	* Added support for PTC motor thermistors (such as the KTY84).
	* APP_PPM sleep fix. Should solve CAN issues.

---

### 1.27
#### Released ?
* Added option to truncate history in experiment plot.
* Disable optimization for LZO on android to prevent crash during FW update on 32-bit devices.
* Added HW60_MK3 FW.
* New wand firmware.

---

### 1.26
#### Released ?
* Log analysis:
	* Added ability to filter out outliers from map plot.
	* Added file browser tab.
* Renamed 75/300 bootloader so that it also matches with the R2.
* Added FW upload compression support for faster firmware uploads.
* Added clear data button to experiment plot.
* Added support for TS5700N8501 encoder.
* Better observer gain calculation.
* Disconnect BLE in destructor.

---

### 1.25
#### Released ?
* Renamed Nunchuk to VESC Remote.
* Log analysis:
	* Improved tile caching.
	* Option to not autozoom map when zooming data.
	* Only update ENU reference when loading data.
* Use statically linked openssl on linux build.

---

### 1.24
#### Released ?
* Log:
	* Fixed midnight bug.
	* Improved spanslider behavior.
	* Implemented scroll-to-zoom.
	* Improved performance when analyzing large logs.
	* Remember vertical line position.

---

### 1.23
#### Released ?
* Added more data to log analysis:
	* GNSS Accuracy.
	* Values for first VESC on CAN-bus.
	* Latitude and longitude.
	* GNSS vertical speed and accuracy.
	* Number of VESCs.

---

### 1.22
#### Released ?
* Use foregroud service to keep GNSS alive during logging.
* Always use wake lock during logging.

---

### 1.21
#### Released ?
* Only hide GUI when the application enters the hidden state.
* Opengl ES2 for static linux build.

---

### 1.20
#### Released 2019-09-27
* Added TCP server to mobile version.
* Remember log files location.
* Added comperhensive log analysis page.
* Added GNSS data to RT log.
* Added setup data to RT log.
* Added IMU data to RT log.
* Use imperial units in profiles as well when selected.
* Allow running in background.
* Added wake lock when VESC is connected on Android.
* Added flash verification option to SWD programming.
* Added IMU 3D view to desktop version.
* Fixed FW upload button flickering bug.
* EUC app support.
* FW 3.62 support:
	* EUC app.
	* Fixed NRF remote reverse bug.
	* Added COMM_BM_MEM_READ.
* New Wand firmware:
	* Possible freeze fix.
	* Added joystick error detection.
	* Switch between imperial and metric by holding both buttons.

---

### 1.19
#### Released 2019-09-09
* Display prefix and suffix correctly in mobile app.
* FW 3.61 support:
	* Added PPM_CTRL_TYPE_CURRENT_SMART_REV mode.

---

### 1.18
#### Released 2019-09-08
* Native CAN-bus fixes.
* Don't build with can-bus support in static mode as it breaks the linux serial port.
* FW 3.60 support:
	* Fixed IMU9x50 bug.
	* Unrigester ICM20948 terminal callbacks when unused.
	* Added experiment plot functions.
	* Added D and Q axis voltage to RT data.
	* Added smart reverse function to nunchuk app.
* Added experiment plots to RT data page.
* D and Q axis voltage in FOC RT plot.
* Updated wand firmware.
* More debug information from input reset CAN function.

---

### 1.17
#### Released 2019-09-03
* FOC wizard motor pole setting now only allows multiples of two.
* Native CAN-bus support.
* Added support for multiple SWD interfaces.
* FW 3.59 support.
* Added wand firmware and NRF52840 firmware for VESC HD.
* Added header to RT CSV log.
* Added VESC HD and UAVC Omega firmwares.
* Added NRF quick pair button.
* Support as many commands as possible in limited mode. This will make the app quite useful even if the FW is out of date.
* Ble improvements for NRF52 firmwares.
* Do not disconnect after bootloader update and show different message.
* Added IMU page with many IMU settings and live plots to adjust them.

---

### 1.16
#### Released ?
* Added support to export sampled data as CSV.
* Added erase only button to SWD prog.

---

### 1.15
#### Released ?
* Keep screen on setting on android.
* Added support for imperial units.
* Added battery current warning.

---

### 1.14
#### Released 2019-07-01
* FW 3.58 support:
	* Set motor to FOC mode after successful FOC detection instead of the default type for the hardware.
	* APP_ADC: Do not send brake command over CAN if config.multi_esc is not set.
	* APP_PPM: Make pulses invalid if they are above 150 % instead of 120 %.
	* APP_PPM: Introduced a new control mode that allows reverse with hysteria (@ackmaniac port).
* Moved to Qt 5.12 for the mobile version.
* Added arm64_v8a and x86 builds.
* Request file permission when opening log and fw files.
* Enable RT logging without having the RT data page active.
* Added help text about RT logging.
* Try to create RT logging path if it does not exist.

---

### 1.13
#### Released 2019-05-16
* FW 3.57 support:
	* Added CAN status message 5 with input voltage and tachometer data.
	* Fix github issue #94.
	* Use default F_SW for HW after autodetect FOC.

---

### 1.12
#### Released ?
* Fixed 16k_16m_rx11_tx9 NRF51 fw as well.

---

### 1.11
#### Released ?
* Added RT data logging to CSV file (desktop and mobile).
* Fixed 16K NRF51 firmware (again).

---

### 1.10
#### Released 2019-05-03
* FW 3.56 support.
	* Fixed current offset fault in BLDC mode.
	* Support for multiple IMUs and ICM-20948.
* Include working firmwares for 16K NRF51 modules.

---

### 1.09
#### Released 2019-04-26
* FW 3.55 support.
	* New ADC control mode.
	* Initial support for sin/cos encoders.
	* Nunchuk CC fixes.
	* PPM multi duty mode fix.
* Added more fault codes.

---

### 1.08
#### Released 2019-03-31
* FW 3.54 support.
* SWD programming support, using the VESC as a programmer.
	* Can be used to flash bricked VESCs from a working one.
	* Can be used to make a custom NRF5x module.
* Remember last custom firmware file path.

---

### 1.07
#### Released 2019-03-20
* FW 3.53 support.
* Updated parameters in default FOC detection.
* Added missing l_max_erpm_fbrake parameter.

---

### 1.06
#### Released 2019-03-10
* FW 3.52 support.
* Added setting to disable permanent UART.
* Added IMU page.
* Added 3D orientation visualization to mobile interface.

---

### 1.05
#### Released 2019-03-04
* Fixed bug with encoder detection command.
* FW 3.51 support.

---

### 1.04
#### Released ?
* Fixed crash when turning off BLE module while connected.

---

### 1.03
#### Released ?
* FW compatibility check on all devices on CAN-bus when performing simultaneous configurations.

---

### 1.02
#### Released 2019-03-01
* FW 3.50 support
* Signature in app and motor configuration.
* Generate parameter parser for firmware.
* Signature checks on configurations.

---

### 1.01
#### Released ?
* Save last connected port.
* Show UUID after pairing, and ask user to write it down.
* Clear version text on FW page when disconnected.

---

### 1.00
#### Released ?
* First build for VESC Tool on google play.
* Added direction setup to desktop version.
* Added direction setup to QML start tools.

---

### 0.99
#### Released 2019-02-18
* FW 3.48 support.
* Added (non-secure) BLE pairing support.
* Color adjustments.
* Highlight active profile.
* Several qml style improvements.

---

### 0.98
#### Released ?
* FW 3.47 support.
* Profiles based on current percentages.
* Removed input current from profiles.
* Made watt limit optional for profiles.

---

### 0.97
#### Released ?
* FW 3.46 support.
* CAN-bus scan support.
* New motor setup wizard.
* New QML connection page, with scroll and horizontal layout support.
* Horizontal screen support for motor cfg and app cfg.
* Invert direction setup tool.

---

### 0.96
#### Released ?
* Experiment page.
* Shorter FW Upload timeout.
* FW 3.44 support.
* FW 3.43 support.
* General Purpose Drive support.
* Setup info and profile support.
* Dark QML theme.
* Setup realtime data support.
* New style for gauges.
* Fixed bug in VByteArray.
* Some horizontal screen orientation support.
* Many QML changes and improvements in general.
* General purpose drive support.
* Added calc apply old settings button to FOC detection.

---

### 0.95
#### Released 2018-07-06
* FW 3.39: auxiliary output support.

---

### 0.94
#### Released ?
* Map detected BLE devices by address instead of by name to show multiple devices with the same name.

---

### 0.93
#### Released ?
* BLE device naming support.

---

### 0.92
#### Released 2018-04-22
* Added generic bootloader for all hardwares.
* FW 3.38 support.

---

### 0.91
#### Released 2018-03-24
* Mobile: Added controls dialog, which can be opened from drawer menu.
* FW 3.37 support.
* Current value ranges to 1000 A.

---

### 0.90
#### Released ?
* Mobile:
	* Added NRF pairing to mobile UI.
	* Added terminal page.
* FW 3.36 support: D term filter for position and speed controllers.

---

### 0.89
#### Released ?
* Added history to terminal text edit that can be accessed with the up and down arrows.
* Major mobile UI update:
	* MC configuration.
	* App configuration.
	* FOC parameter detection.
	* BLDC parameter detection.
	* FOC encoder detection.
	* FOC hall sensor detection.
	* PPM pulselength mapping.
	* ADC voltage mapping.
	* At this point the mobile UI should be able to do a full configuration.

---

### 0.88
#### Released ?
* Added Option in APP > NRF > Transmit Power for powering off the nRF module.
* Mobile: reload mc_config and app_config on fw rx change to rescale meters.

---

### 0.87
#### Released 2018-01-24
* RT plot time measurement improvement.
* First implementation of BLE support.
* Added poll command timeouts.
* Less frequent FW version and conf sampling.
* Better page list size calculation.
* Refactoring for accessing most backend functionality from QML.
* First implentation of mobile GUI preview, a lot of work left.
* Firmware display synchronization fix.
* FW 3.34 support.

---

### 0.86
#### Released 2017-11-08
* FW 3.33 support.
* Synchronized CAN FWD ID between connection pages.

---

### 0.85
#### Released 2017-11-08
* FW 3.32 support.
* Configurable CAN baud rate.

---

### 0.84
#### Released 2017-10-27
* FW 3.31 support.
* Input setup wizard improvements.

---

### 0.83
#### Released 2017-10-20
* Embedded fonts for consistency and static linking without fontconfig.
* Split PPM wizard page into two pages.
* Split ADC wizard page into two pages.
* Removed devicepixelratio support from QCustomPlot as it slows it down a lot.
* FW 3.30 support.
* Added firmware upload unplug power warning.

---

### 0.82
#### Released 2017-09-21
* Only show introduction if it has changed since the previous version.
* Proper fix for RT data time scale https://github.com/vedderb/vesc_tool/pull/5
* FW 3.29 support.

---

### 0.81
#### Released 2017-09-06
* Fixed RT data time scale.
* Individual throttle curves for acc and brake.
* Support for FW 3.28.

---

### 0.80
#### Released 2017-09-04
* Fixed C header export names (PPM_HYST).
* Fixed some typos.
* Added platinum version to build system.
* Increased maximum motor resistance in detection widget.
* Support for FW 3.27.

---

### 0.79
#### Released ?
* Made help dialogs for FOC and BLDC detect non-modal.
* Support for FW 3.26.

---

### 0.78
#### Released ?
* Added experimental bootloader upload support.
* Changed tab shape and fixed spelling mistake in additional motor info page.

---

### 0.77
#### Released ?
* Fixed bug in percentage display mode for floats that would cause rounding errors.
* Changed version check to assume a floating point number so that older online versions won't be considered as newer.
* Added support for FW 3.25 with ADC ramping and a new ADC control mode.

---

### 0.76
#### Released ?
* Changed default current controller gain calculation time constant.
* Support for FW 3.24

---

### 0.75
#### Released ?
* Improved UI scaling support for high DPI monitors.
* Realtime data text box fixes.
* FOC hall table read bug fix.
* Significantly improved observer gain calculation.
* Improved current controller gain calculation.
* QCustomPlot scale factor 1 for high-DPI monitors. This gives lower resolution, but plotting is much faster.

---

### 0.7
#### Released ?
* Check motor paramters after writing them and show dialog if they were truncated.
* FW 3.22 support.
* UI scaling support.
* New configuration page for VESC Tool.
* Updated wzards with scalable high resolution images.

---

### 0.6
#### Released ?
* Added firmware changelog to help menu.
* FW 3.20 support.
* ADC center throttle support.

---

### 0.5
#### Released ?
* FW 3.19 support.
* Sampled data is now transmitted in floating point with scaling done at the VESC.
* Updated setup wizard logos.

---

### 0.4
#### Released ?
* FW 3.18 support.
* Autoconnect support.
* Autoconnect buttons in welcome and connection page.
* Autoconnect function in wizards.
* Check for the latest VESC Tool version online.
* Poll serial port in addition to waiting for the readyRead signal since readyRead is not emitted recursively. This can be a problem when e.g. showing message boxes and data comes in.

---

### 0.3
#### Released ?
* FW 3.17 support.
* Automatic build support for different versions.
* VESC Project forum link in help menu.

---

### 0.2
#### Released ?
* First complete version of input setup wizard.
* Support for DRV configuration.
* Support for throttle curves.
* Bug fix in displaybar and displaybarpercentage which made them render incorrectly in scroll areas.

---

### 0.1
#### Released ?
* Added changelog.
* Created VTextBrowser for displaying rich text with more control.
	* Hyperlink support in help texts.
	* Correct cursor display.
* Image updates.
* Text updates and corrections.
* Updated welcome page.
