**Download:**
- [Listeningway-v{{VERSION}}.zip](https://github.com/gposingway/listeningway/releases/download/v{{VERSION}}/Listeningway-v{{VERSION}}.zip)

## Installation
1. Download the ZIP file above.
2. Extract the contents of the ZIP. You will find these files:

    | File Name                  | Where to Place It                                                                | Notes / Purpose                                  |
    | :------------------------- | :------------------------------------------------------------------------------- | :----------------------------------------------- |
    | `Listeningway.addon`       | Game Directory (same folder as your game EXE & ReShade DLL)                      | ReShade loads `.addon` files from this directory. |
    | `Listeningway.fx`          | Main ReShade Shaders Folder (e.g., `.../reshade-shaders/Shaders/`)               | The example shader effect file.                  |
    | `ListeningwayUniforms.fxh` | Main ReShade Shaders Folder (e.g., `.../reshade-shaders/Shaders/`)               | Include file needed by shaders using `#include`.   |

   *(Reminder: The `.addon` file goes directly into your main game folder with the ReShade DLL, not inside `reshade-shaders`!)*
3. Restart your game or application.

## Notes
- Requires ReShade 5.2 or newer.
- For more information and troubleshooting, see the [GitHub repository](https://github.com/gposingway/listeningway).
