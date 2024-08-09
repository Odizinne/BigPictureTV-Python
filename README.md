# BigPictureTV
[![Github All Releases](https://img.shields.io/github/downloads/odizinne/bigpicturetv/total.svg)]()
[![license](https://img.shields.io/github/license/odizinne/bigpicturetv)]()

This Python project automates switching between a monitor and a TV when launching Steam Big Picture on Windows.  
*Looking for the [linux version](https://github.com/Odizinne/BigpictureTV-Linux)?*

**If you appreciate my work and would like to support me:**

<a href="https://ko-fi.com/odizinne">
  <img src="assets/kofi_button_red.png" alt="Ko-fi" width="200" height="auto">
</a>

## Overview

![image](assets/screenshot.png)

## Requirements

**Runtime:**
- [AudioDeviceCmdlets](https://github.com/frgnca/AudioDeviceCmdlets) for audio output switching.
 
## Download

Head to [release](https://github.com/Odizinne/BigPictureTV/releases/latest) section and grab the latest one.

Place the directory wherever you like (`%localappdata%\Programs` is a good one)

## Usage

### Error: The program can't start ...

Download and install [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

### Window check rate

In ms, from 100 to 1000, the interval for checking Steam Big Picture window presence.  
1000 is the default value.  
100 ms will lead to next to no delay mode switch but will consume more resources.  
Do not change if unsure.

### Audio configuration

1. Click on "Install audio module" button and wait for its completion.    
2. You can now uncheck `Disable audio switching`.  
3. Specify your audio outputs.  
You can use a short name. BigPictureTV will try to find the correct audio output from keywords. Less is more.

### Monitor configuration

For monitor switching, BigPictureTV relies on Windows built in `displayswitch.exe`.

There are 4 modes available:
- Internal: Default monitor enabled, all others disabled
- Extend: Default monitor enabled, selected monitors enabled.
- Clone: Every monitors enabled, mirrored.
- External: Default monitor disabled, selected monitors enabled.

**If you have more than two monitors, follow these steps to configure your preferred external monitor:**

1. Press Win + P and select the "External" option.
2. Go to Windows Settings > System > Displays.
3. Choose your desired monitor setup by disconnecting all monitors except the one you want to use in game mode.
4. Press Win + P again and switch back to "Internal" or "Extend".

If you'd prefer to skip the screen configuration, simply select `Clone` in BigPictureTV settings.  
**You're all set!** You can now close the settings window.

### Actions

Actions will be performed when entering gamemode and reverted when going back to desktop.

- Close discord
Close discord application, reopen minimized
- Enable performance powerplan
Revert to balanced in desktop mode

## Build

- Install Python.
- Clone this repository: `git clone https://github.com/Odizinne/BigPictureTV.git`<br/>
- CD inside the cloned directory: `cd BigPictureTV`<br/>
- Install dependencies: `pip install -r requirements.txt`
- Build the executable: `python src\setup.py build`<br/>

## I found a bug

Feel free to [open an issue](https://github.com/Odizinne/BigPictureTV/issues/new) in the repository.  
Be sure to add as many information as possible to help me reproduce it!

## I want to help

I need help for translating the app.

What is currently done:
- English
- french
