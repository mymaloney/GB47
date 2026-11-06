# Super Game Boy Border Example
This example shows how to convert and display an SGB border when the game is running on a Super Game Boy.

png2asset is used for converting the border.


### Image Properties
* The entire image should be 256 x 224 pixels
* The game area in the center should be:
  * 160 x 144 pixels, from (48,40) to (207,183)
  * The pixels in the game area at the center of the image should be set to 100% Alpha Transparency (as in the example image). If this is not done then the game area may share a palette color with other parts of the border leading to tile pattern flashing while the border loads.
  * The palette will be sorted from lightest to darkest. There may be up to 4 SGB palettes. For Each SGB 16 color palette the first color (zero) is reserved as transparent.
  * If the image is indexed and you want the palette order preserved, use `-keep_palette_order` with png2asset (but make sure color zero is used for transparent areas)
* See the pandocs for additional details
  * https://gbdev.io/pandocs/SGB_Functions.html
  * https://gbdev.io/pandocs/SGB_Color_Palettes.html

When using the SGB with a PAL SNES, a delay should be added just after program startup such as:

```
    // Wait 4 frames
    // For PAL SNES this delay is required on startup
    for (uint8_t i = 4; i != 0; i--) wait_vbl_done();
```



