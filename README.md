# OBS Live Audio Visualizer Plugin

A real-time audio-reactive visualizer plugin for OBS Studio that displays dynamic bars synchronized with any audio input. Perfect for music streams, podcasts, or any content with audio.

## Features

- **Audio-Reactive Bars**: Visualize audio in real-time with responsive bar animations
- **Multiple Visualization Modes**:
  - **Normal**: Classic vertical bars from bottom
  - **Mirrored**: Bars emanate from center outward (left/right)
  - **Radial**: Circular visualization with bars radiating from center
- **Multiple Shapes**: Square, Rounded, Capsule, and Dot styles
- **Frequency Detail Control**: Choose focus level (Bass, Standard, Wide, Full spectrum)
- **Customizable Colors**: Full RGBA color support
- **Audio Sensitivity**: Adjust input sensitivity (0.1x to 10x multiplier)
- **Smoothing & Decay**: Control animation smoothing and peak hold decay
- **Flexible Bar Configuration**: Adjust bar width, gaps, and bar count automatically
- **Height Scaling**: Boost or reduce bar heights independently

## Requirements

- OBS Studio 31.1.1 or later
- Windows: Visual Studio 17 2022 or Visual Studio Build Tools
- macOS: XCode 16.0 or later
- Linux: CMake 3.28.3, Ninja, build-essential, pkg-config

## Quick Start

### Windows Installation

**For Non-Technical Users:** See [INSTALL.md](INSTALL.md) for detailed step-by-step instructions!

**Quick Installation:**
1. Download both files from [Releases](https://github.com/TitaniumKnight1/OBS-Live-Audio-Visualizer-Plugin/releases):
   - `audio-visualizer.dll`
   - `en-US.ini`

2. Copy `audio-visualizer.dll` to:
   ```
   C:\Program Files\obs-studio\obs-plugins\64bit\
   ```

3. Create folder structure and copy `en-US.ini` to:
   ```
   C:\Program Files\obs-studio\obs-plugins\64bit\audio-visualizer\locale\en-US.ini
   ```
   (Create the `audio-visualizer` and `locale` folders if they don't exist)

4. Restart OBS Studio

5. Add a new Source → Select "Audio Visualizer"

### Building from Source

#### Windows

```powershell
# Clone the repository
git clone https://github.com/TitaniumKnight1/OBS-Live-Audio-Visualizer-Plugin.git
cd OBS-Live-Audio-Visualizer-Plugin

# Create build directory
mkdir build_x64
cd build_x64

# Configure with CMake
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Program Files\obs-studio" ..

# Build
cmake --build . --config RelWithDebInfo

# Install
.\install-plugin.ps1
```

#### macOS

```bash
git clone https://github.com/TitaniumKnight1/OBS-Live-Audio-Visualizer-Plugin.git
cd OBS-Live-Audio-Visualizer-Plugin

mkdir build_macos
cd build_macos
cmake -G Xcode -DCMAKE_PREFIX_PATH="/usr/local/opt/libobs" ..
cmake --build . --config RelWithDebInfo
```

#### Linux

```bash
git clone https://github.com/TitaniumKnight1/OBS-Live-Audio-Visualizer-Plugin.git
cd OBS-Live-Audio-Visualizer-Plugin

mkdir build_x86_64
cd build_x86_64
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
ninja
```

## Usage

1. Open OBS Studio
2. Go to **Sources** → **+** button → Select **Audio Visualizer**
3. Configure the following settings:
   - **Audio Source**: Select which audio source to visualize
   - **Mode**: Choose visualization mode:
     - **Normal**: Bars grow upward from bottom
     - **Mirrored**: Bars emanate from center
     - **Radial**: Circular visualization
   - **Bar Shape**: Square, Rounded, Capsule, or Dots
   - **Frequency Detail**: Bass (low focus), Standard, Wide, Full (all frequencies)
   - **Colors**: Set primary color and optional gradient
   - **Sensitivity**: Boost audio sensitivity (higher = more responsive)
   - **Smoothing**: Smooth transitions between values
   - **Decay**: Speed at which peak indicators fall
   - **Bar Width & Gap**: Customize bar dimensions
   - **Height Scale**: Multiply bar heights by this value

## Settings Reference

| Setting | Range | Default | Description |
|---------|-------|---------|-------------|
| Sensitivity | 0.1 - 10.0 | 1.25 | Audio input multiplier |
| Smoothing | 0.0 - 0.95 | 0.55 | Animation smoothing factor |
| Decay | 0.0 - 1.0 | 0.12 | Peak hold decay rate |
| Bar Width | 1 - 64 | 10 | Width of each bar in pixels |
| Gap | 0 - 64 | 3 | Space between bars in pixels |
| Height Scale | 0.1 - 3.0 | 1.0 | Multiplier for bar heights |
| Frequency Detail | Bass/Standard/Wide/Full | Standard | Focus on different frequency ranges |

## Tips

- **Radial Mode**: Use the Scale option in OBS to resize the visualizer for your scene layout
- **Frequency Detail**: Lower values (Bass) emphasize kick drums; higher values show full spectrum
- **Smoothing**: Increase for jazz/ambient; decrease for electronic/EDM music
- **Decay**: Lower values create snappier movement; higher values for smoother decay

## Troubleshooting

**Plugin doesn't load:**
- Ensure OBS Studio version is 31.1.1 or later
- Check that the DLL is in the correct plugins directory
- Verify the folder structure is correct: `audio-visualizer\locale\en-US.ini`
- Restart OBS after installation

**Audio not showing:**
- Verify the audio source is selected in the plugin settings
- Ensure the audio source has active audio input
- Check that audio sensitivity is not set too low

**Visualizer looks choppy:**
- Reduce smoothing value or increase decay
- Lower the bar count by increasing gap/width

**Text labels are missing in settings:**
- Ensure `en-US.ini` is in: `C:\Program Files\obs-studio\obs-plugins\64bit\audio-visualizer\locale\en-US.ini`

## Contributing

Contributions are welcome! Please feel free to:
- Report bugs and issues
- Suggest new features
- Submit pull requests

## License

GNU General Public License v2.0 or later - See [LICENSE](LICENSE) file for details.

## Credits

Developed by TitaniumKnight

Built with:
- [OBS Plugin SDK](https://github.com/obsproject/obs-studio)
- [CMake](https://cmake.org/)

## Support

If you find this plugin useful, consider:
- Starring the repository
- Reporting issues and suggestions
- Contributing improvements

---

**Project Repository**: https://github.com/TitaniumKnight1/OBS-Live-Audio-Visualizer-Plugin
