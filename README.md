# VESC® Tool

This is the source code of VESC Tool. A pre-compiled binary of both the stable release as well as the development release packaged with all the matching firmware for all supported hardware can be downloaded at http://vesc-project.com/

The stable binary is available for **Linux**, **Windows**, **MacOS**, **Android** and **iOS**. The development binary is available for **Linux**, **Windows** and **Android** and is updated every few days.

All binaries can also be downloaded free-of-charge for all platforms except for iOS, which only is available via the Apple App Store as they do not allow any other distribution channel.

## Code Contribution, Distribution and Trademark Usage

VESC is a registered trademark of Benjamin Vedder. Read the [trademark policies](https://vesc-project.com/trademark_policies) for more information.

The "official" binary release of VESC Tool is done via VESC Project only, as that gives users a way to verify that releases, that use the registered VESC trademark, originate from the VESC Project. It is not ok to host a binary release on a different channel and use the VESC trademark for that release.

It is ok to use the github fork function to make contributions to the code. That is because 1) it is the most convenient way to make contributions and 2) the forked repository states clearly that it is a fork and points back to the main repository where the original code can be found. Further, it is easy to see what the code changes are from the forked repository compared to the main repository via github, but that information is lost in a binary release.

Forks of VESC Tool on github are not encouraged to provide a binary release in the repository. That is because there is no way to tell the final binary apart from the official release once downloaded. Further, packaging the firmware, which has to be done as an additional step from a different repository, also cannot be verified whether it is done correctly.

**Forks without branding**  

Because the topic came up, here are some words about forking VESC Tool and removing the branding.  

If you make a fork of VESC Tool and remove all traces of the VESC trademark you are not breaking the trademark policies, but we still do not encourage that. The reason is that such forks are 1) confusing to users and 2) divert users away from the VESC Project itself and take away the opportunity to learn about it and to make donations if they choose to. For example, the majority of VESC donations today come from VESC Tool downloads.  

If you see a missing feature and you want to put in some work and make that feature available, we would appreciate if you contribute that back to the main VESC repositories. That way there is only one consistent and compatible release for everyone that is managed by the main authors of the VESC code who make the vast majority of the development. It also gives the main authors, who are the most familiar with the code, a chance to review features to make sure that they are as safe as possible and don't break other parts of the functionality.

## Forking, porting or rewriting VESC® Tool — good practices

*Community write-up, not legal advice. The official [Trademark Policies](https://vesc-project.com/trademark_policies) and the [VESC Tool README](https://github.com/vedderb/vesc_tool) always prevail. When in doubt, ask: trademarks@vedder.se*

Rewriting VESC Tool in another language (Rust, Flutter, web…) or shipping a heavily modified fork touches two separate things:

- **The code** → copyright, governed by the GPL
- **The name and logos** → trademark, governed by Benjamin Vedder's trademark policy

The GPL gives you a lot of freedom with the code. It gives you **no** right to the VESC® name or logos. Keep both in mind.

### 0. First, consider contributing upstream

The README asks people to contribute missing features back to the main repos rather than splitting the user base. Custom hardware can be added to the official release by forking `vedderb/bldc` and opening a PR. If your goal can be reached that way, it's the best option for everyone.

### 1. The code: GPL-3.0-or-later

VESC Tool is licensed GPL-3.0-or-later. In most jurisdictions, translating it into another language or porting its logic file by file makes your project a derivative work. So:

- **License your project GPL-3.0-or-later** and ship the full LICENSE text.
- **Keep every existing copyright notice** (Benjamin Vedder and contributors) in the files you port, and add your own on top.
- **State clearly that it's modified**: what you changed, when, and from which upstream commit.
- **Publish the complete source** for every binary you distribute, build scripts included. A public repo works.
- **Show the legal notices in the UI**: an About screen with copyright, license, no-warranty notice and a link to the source.
- **Only use GPLv3-compatible dependencies** (MIT, Apache-2.0, BSD, MPL-2.0… are fine).
- **Bundled firmware** (`bldc`) is GPL too: same source obligations if you ship it.
- **Third-party code inside VESC Tool** (heatshrink, minimp3, QCustomPlot, maddy…) keeps its own license. Credit it if you port it.

A true clean-room reimplementation (written only from a protocol spec, without translating existing code) is a different situation, but it's hard to claim once you've worked from the source.

### 2. Naming

**VESC®** is a registered word mark of Benjamin Vedder. The policy says a mark can't be used as part of a product name, a domain name, a trade name or a company logo, and names confusingly similar to it are prohibited too.

- ❌ VESC Tool RS · RustVESC · VescTool NG · V-Tool · VSC Tool
- ✅ A name of your own that doesn't echo "VESC"
- Same for app store titles, listing titles, repo / package / crate names and domains. Even "MyApp for VESC" shouldn't be the title: compatibility goes in the description.
- A plain GitHub fork used to contribute upstream is fine; the README explicitly allows it.

### 3. Logos, icons, look & feel

- **No VESC logos**, and don't reuse the VESC Tool app icon (`VESC Favicon.iconset`).
- **Don't copy the UI icons from `res/`** without checking where each one comes from. Easiest: draw your own or use a clearly licensed set (Material Symbols, Lucide…).
- **Use your own colour palette and visual identity.** An app that looks like the official one creates exactly the confusion trademarks exist to prevent.

### 4. Technical identifiers & UI text

- **Change every app identifier**: Android package `com.vedder.vesc`, organisation name, domain, application name, settings paths.
- **Remove or replace the update checker** that queries vesc-project.com and tells users to download VESC Tool. Don't silently rely on VESC Project servers (package archive, etc.); ask first.
- **In UI text, say "the controller", not "the VESC".** The policy says a mark should only be used as an adjective followed by a generic noun, never as a noun, verb, plural or possessive.
- **Interoperability identifiers** (protocol command names, file extensions…): keep them in the code, out of your branding.

### 5. Describing compatibility

The policy explicitly allows factual compatibility statements:

- ✅ `MyApp — compatible with VESC® Software`
- ✅ `MyApp, for use with VESC® Software`
- ❌ `VESC® MyApp` · `MyApp VESC®`

Also:

- Put ® right after the first mention.
- Never make it more prominent than your own name.
- Use it only as often as needed to state compatibility.
- Never call your app "official", "certified" or "endorsed".

### 6. Credits / About screen (template)

```
MyApp 1.0
© 2026 <you> — port and modifications

Based on VESC® Tool
© 2016-2026 Benjamin Vedder and VESC Tool contributors
https://github.com/vedderb/vesc_tool (commit xxxxxxx)

Free software under the GNU GPL v3 or later.
Provided WITHOUT ANY WARRANTY. [License] [Source code]

VESC is a registered trademark of Benjamin Vedder.
This project is not affiliated with, endorsed or
supported by Benjamin Vedder or the VESC Project.
```

A factual "Based on VESC® Tool" line is fair use, and the GPL requires the copyright notices anyway. If you want zero ambiguity, have your wording checked at trademarks@vedder.se.

### 7. Distribution

- **Official VESC Tool binaries come only from VESC Project.** Don't host or publish binaries that use the VESC name.
- **Make it obvious that your app is a separate project.** Point users to *your* repo for support, not to the VESC forums for bugs in your app.
- **Consider giving back.** The README is honest about why unbranded forks are discouraged: they confuse users and divert donations, most of which come from VESC Tool downloads. A visible link to the VESC Project donations page in your About screen is a fair way to give back.

### TL;DR checklist

- [ ] GPL-3.0-or-later, LICENSE file, full source published
- [ ] All upstream copyright notices kept + "modified / based on commit" notice
- [ ] Name, icon, logo, colours, app ID: nothing VESC-like
- [ ] "Compatible with VESC® Software" only as a factual statement
- [ ] Not-affiliated disclaimer in the About screen and store listing
- [ ] No binaries redistributed under the VESC name
- [ ] Considered contributing upstream first

## Add Your Hardware to the Binary Release

If you have custom hardware and you want to add support for it in the official release of VESC Tool, you can use the following steps:

1) Go to https://github.com/vedderb/bldc and use the github fork function.  
2) Make your changes, test them and make a pull request to the main repository.  
3) If the pull request gets accepted your hardware will become part of the next official release. It will show up in the binary beta typically after a few days and in the stable version the next time a stable release is made.

## Development

**Note:** These instructions build VESC Tool without the BLDC firmwares bundled.

### Linux

Make sure that the required dependencies are installed. There is some advice in the [build_lin](./build_lin) file's comments. If you have Nix installed see below.

```shell
qmake -config release "CONFIG += release_lin build_original exclude_fw"
make -j8
./build/lin/vesc_tool_6.06
```

### Nix

The most easy way to build and run VESC Tool is to just run the provided program:

```shell
nix run
```

This will rebuild the program from scratch on each invokation. To enter a build environment with the dependencies installed for building it manually with QMake, run

```shell
nix develop
```

Then follow the normal build instructions for Linux.

### Starting QT Creator in Nix

QT Creator allows you to easily build and run the project. It also allows you to edit the page UIs with it's graphical editor. To run it using Nix simply start QT Creator from a shell with the build dependencies:

```shell
nix develop
nix run nixpkgs#qtcreator
```

This makes sure that QT Creator has access to the required dependencies.
