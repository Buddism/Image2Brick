# Image2Brick
good luck compiling this - compiled using visualstudio 2022

Torquescript code is at:
https://github.com/Buddism/Image2Brick-TorqueScript<br>

# TS-SETUP
`makeColorset();`<br>
sets the clipboard data to colorset info, paste into `colorset.txt` in the folder Image2Brick is contained<br>
`makeBrickList();`<br>
sets the clipboard data to brick datablock list info, paste into `bricklist.txt` in the folder Image2Brick is contained<br>

# TS-BUILD
takes the clipboard input from the Image2Brick.exe and builds it<br>
lazy_build(getClipboard());
