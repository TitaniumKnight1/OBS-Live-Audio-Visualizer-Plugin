# Installation Guide - OBS Live Audio Visualizer Plugin

## For Non-Technical Users

Follow these simple steps to install the plugin on Windows:

### Step 1: Download the Plugin Files

1. Go to [Releases](https://github.com/TitaniumKnight1/OBS-Live-Audio-Visualizer-Plugin/releases)
2. Click on the latest release
3. Download both files:
   - `audio-visualizer.dll`
   - `en-US.ini`

### Step 2: Navigate to Your OBS Plugin Folder

**Windows users:**

1. Press `Win + R` on your keyboard
2. Copy and paste this path:
   ```
   C:\Program Files\obs-studio\obs-plugins\64bit
   ```
3. Press Enter

This will open your OBS plugins folder. Keep this window open.

### Step 3: Place the DLL File

1. In the folder that opened, look for other `.dll` files
2. Drag and drop `audio-visualizer.dll` into this folder

### Step 4: Create the Locale Folder and Place the INI File

1. In the same plugins folder (`C:\Program Files\obs-studio\obs-plugins\64bit`), **right-click** in empty space
2. Select **New** → **Folder**
3. Name the folder exactly: `audio-visualizer`
4. **Double-click** the new `audio-visualizer` folder to open it
5. **Right-click** in empty space again
6. Select **New** → **Folder**
7. Name this folder exactly: `locale`
8. **Double-click** the `locale` folder to open it
9. Drag and drop `en-US.ini` into this `locale` folder

**Your folder structure should look like this:**
```
C:\Program Files\obs-studio\obs-plugins\64bit\
├── audio-visualizer.dll
└── audio-visualizer\         (folder you created)
    └── locale\              (folder you created)
        └── en-US.ini        (file you placed here)
```

### Step 5: Restart OBS

1. **Close OBS completely** (if it's open)
2. **Open OBS Studio again**
3. The plugin is now installed!

## How to Use the Plugin

1. Open OBS Studio
2. In your Scene, click the **`+`** button under **Sources**
3. Select **"Audio Visualizer"** from the list
4. Choose your audio source (usually "Desktop Audio" or your microphone)
5. Customize the visualization with the settings panel

## Troubleshooting

### Plugin doesn't appear in OBS

**Solution:**
- Make sure you have OBS Studio **31.1.1 or newer**
- Check that the DLL is in: `C:\Program Files\obs-studio\obs-plugins\64bit\`
- Verify the folder structure is correct (audio-visualizer → locale → en-US.ini)
- Try restarting your computer, then restart OBS

### Audio isn't showing

**Solution:**
- Make sure you selected an audio source in the plugin settings
- Check that audio is actually playing
- Try increasing the "Sensitivity" slider in the plugin settings

### Text labels are missing in settings

**Solution:**
- Make sure `en-US.ini` is in the correct folder path:
  ```
  C:\Program Files\obs-studio\obs-plugins\64bit\audio-visualizer\locale\en-US.ini
  ```

## Need Help?

If you're still having issues:
1. Check that you're using the **latest version** of OBS Studio
2. Verify all files are in the correct locations
3. Open an issue on [GitHub](https://github.com/TitaniumKnight1/OBS-Live-Audio-Visualizer-Plugin/issues)

## For Advanced Users / Developers

If you want to build the plugin from source, see the [README.md](README.md) for build instructions.
