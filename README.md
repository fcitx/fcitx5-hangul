fcitx-hangul
=====
Hangul Wrapper for Fcitx.

[![Jenkins Build Status](https://img.shields.io/jenkins/s/https/jenkins.fcitx-im.org/job/fcitx5-hangul.svg)](https://jenkins.fcitx-im.org/job/fcitx5-hangul/)

[![Coverity Scan Status](https://img.shields.io/coverity/scan/14725.svg)](https://scan.coverity.com/projects/fcitx-fcitx5-hangul)

## Using with non-qwerty English system layout
The addon handles key input based on its string value, not keycode, so pressing `t` in Colemak is interpreted as ㅅ rather than ㄹ for Dubeolsik. To fix this, set `Default` (qwerty) layout for Hangul.

Via `fcitx5-configtool`: in `Input Method` tab, select Hangul, then press `Select Layout` and choose Layout: English (US), Variant: Default. 
Or edit profile config file:
```
# group and item numbers may be different
[Groups/1/Items/0]
Name=hangul
Layout=us
```
