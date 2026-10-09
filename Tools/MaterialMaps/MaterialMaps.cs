using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;

// Makes the maps of a material from its color texture (see Renderer::LoadMaterialByFileNames for the names):
//   <Name>_Normal.png      which way every texel faces, from the slopes of a height made of the brightness
//   <Name>_MetalRough.png  roughness (green) and metalness (blue), from the settings of the texture below
//   <Name>_Height.png      the height itself (white high, black low), for parallax occlusion mapping
//
// The height is a guess from the color: in these textures what is dark (mortar, cracks, the gaps between planks) lies
// deeper than what is light (the faces of stones and boards). Every texture has its own strength of relief and its own
// roughness, because a wet flagstone and dry plaster of the same brightness are not alike. Run it with Generate.ps1.
//
// Written for the C# compiler built into Windows PowerShell (C# 5): no string interpolation, no tuples.
public static class MaterialMaps
{
    // How the maps of one color texture are made.
    public class Settings
    {
        // How steep the relief is: the slope of the height is multiplied by it (0 flat; 2-4 for stone).
        public double Relief = 2.5;

        // The radius of the blur of the height in texels: it softens the noise of the color, so a speck of dirt is not a
        // bump. 0 keeps every texel.
        public int Blur = 1;

        // Roughness of the surface (0 a mirror, 1 fully matte) and how much it changes with the brightness: positive
        // makes the dark parts rougher (dirt in the gaps), negative smoother (water standing in the gaps).
        public double Roughness = 0.85;
        public double RoughnessFromDarkness = 0.1;

        // Metal: 0 none; for IronBand the dull, less saturated texels are iron (metal) and the bright brown and orange
        // ones rust.
        public bool IsIronMetal = false;
    }

    // The settings of every texture of Episode 1, by the name of its file.
    public static Dictionary<string, Settings> CreateEpisode1Settings()
    {
        var settings = new Dictionary<string, Settings>();
        settings["Crate_Rotten"] = new Settings { Relief = 2.0, Roughness = 0.9 };
        settings["Door_Chapel"] = new Settings { Relief = 2.0, Roughness = 0.85 };
        settings["Floor_MossyFlagstone"] = new Settings { Relief = 2.5, Roughness = 0.85 };
        settings["Floor_Mud"] = new Settings { Relief = 1.5, Roughness = 0.55, RoughnessFromDarkness = -0.2 };
        settings["Floor_RottenPlanks"] = new Settings { Relief = 2.0, Roughness = 0.9 };
        settings["Floor_WetFlagstone"] = new Settings { Relief = 2.5, Roughness = 0.35, RoughnessFromDarkness = -0.2 };
        settings["Trim_Arches"] = new Settings { Relief = 3.0, Roughness = 0.85 };
        settings["Trim_IronBand"] =
            new Settings { Relief = 2.5, Roughness = 0.55, RoughnessFromDarkness = 0.0, IsIronMetal = true };
        settings["Wall_CrackedPlaster"] = new Settings { Relief = 1.5, Roughness = 0.95, RoughnessFromDarkness = 0.0 };
        settings["Wall_GothicNiches"] = new Settings { Relief = 3.5, Roughness = 0.85 };
        settings["Wall_MossyBlocks"] = new Settings { Relief = 3.0, Roughness = 0.85 };
        settings["Wall_MossyBrick"] = new Settings { Relief = 3.0, Roughness = 0.85 };
        settings["Wall_RottenPlanks"] = new Settings { Relief = 2.0, Roughness = 0.9 };
        settings["Window_Chapel"] = new Settings { Relief = 1.0, Roughness = 0.4, RoughnessFromDarkness = 0.3 };
        return settings;
    }

    static int Wrap(int value, int period) { int r = value % period; return r < 0 ? r + period : r; }
    static double Clamp01(double value) { return Math.Max(0.0, Math.Min(1.0, value)); }

    // The pixels of an image as ARGB numbers, read in one go (GetPixel per texel would be slow).
    static int[] ReadPixels(Bitmap bitmap)
    {
        var area = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(area, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
        var pixels = new int[bitmap.Width * bitmap.Height];
        System.Runtime.InteropServices.Marshal.Copy(data.Scan0, pixels, 0, pixels.Length);
        bitmap.UnlockBits(data);
        return pixels;
    }

    static void SavePixels(int[] pixels, int width, int height, string path)
    {
        using (var bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb))
        {
            BitmapData data = bitmap.LockBits(new Rectangle(0, 0, width, height), ImageLockMode.WriteOnly,
                                              PixelFormat.Format32bppArgb);
            System.Runtime.InteropServices.Marshal.Copy(pixels, 0, data.Scan0, pixels.Length);
            bitmap.UnlockBits(data);
            bitmap.Save(path, ImageFormat.Png);
        }
    }

    static int Pack(int alpha, int red, int green, int blue) { return (alpha << 24) | (red << 16) | (green << 8) | blue; }
    static int ToByte(double value) { return (int)Math.Round(Clamp01(value) * 255.0); }

    // Makes the three maps of one color texture into outputDirectory.
    public static void Generate(string colorPath, string outputDirectory, Settings settings)
    {
        string name = Path.GetFileNameWithoutExtension(colorPath);
        int width, height;
        int[] color;
        using (var bitmap = new Bitmap(colorPath))
        {
            width = bitmap.Width;
            height = bitmap.Height;
            color = ReadPixels(bitmap);
        }

        // 1. Brightness, the same weights the eye gives red, green and blue. A transparent texel (the outside of a
        //    cutout) counts as a middle gray, so it does not make a cliff at the edge of the shape.
        var brightness = new double[width * height];
        var isOpaque = new bool[width * height];
        for (int i = 0; i < color.Length; i++)
        {
            int argb = color[i];
            int alpha = (argb >> 24) & 255, red = (argb >> 16) & 255, green = (argb >> 8) & 255, blue = argb & 255;
            isOpaque[i] = alpha >= 128;
            brightness[i] = isOpaque[i] ? (0.299 * red + 0.587 * green + 0.114 * blue) / 255.0 : 0.5;
        }

        // 2. The height: the brightness blurred over a small square (wrapping around the edges: the textures tile) and
        //    stretched so the darkest 2% are 0 and the brightest 2% are 1, whatever the colors of the texture.
        var blurred = new double[width * height];
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
            {
                double sum = 0;
                int count = 0;
                for (int dy = -settings.Blur; dy <= settings.Blur; dy++)
                    for (int dx = -settings.Blur; dx <= settings.Blur; dx++)
                    {
                        sum += brightness[Wrap(y + dy, height) * width + Wrap(x + dx, width)];
                        count++;
                    }
                blurred[y * width + x] = sum / count;
            }
        var sorted = (double[])blurred.Clone();
        Array.Sort(sorted);
        double low = sorted[(int)(sorted.Length * 0.02)], high = sorted[(int)(sorted.Length * 0.98)];
        double range = Math.Max(high - low, 1e-6);
        var heightMap = new double[width * height];
        for (int i = 0; i < heightMap.Length; i++)
            heightMap[i] = Clamp01((blurred[i] - low) / range);

        // 3. The normals: the slope of the height along x and along y (central differences, wrapping), times the relief.
        //    A surface rising to the right faces left: the normal leans against the slope. y of the texture grows UP
        //    (OpenGL), while the rows of the image grow down, so the slope up is the row above minus the row below.
        var normals = new int[width * height];
        var heights = new int[width * height];
        var metalRough = new int[width * height];
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
            {
                int i = y * width + x;
                double right = heightMap[y * width + Wrap(x + 1, width)], left = heightMap[y * width + Wrap(x - 1, width)];
                double above = heightMap[Wrap(y - 1, height) * width + x], below = heightMap[Wrap(y + 1, height) * width + x];
                double slopeRight = (right - left) * 0.5, slopeUp = (above - below) * 0.5;
                double nx = -slopeRight * settings.Relief, ny = -slopeUp * settings.Relief, nz = 1.0;
                double length = Math.Sqrt(nx * nx + ny * ny + nz * nz);
                nx /= length; ny /= length; nz /= length;

                // A transparent texel is cut out by the shader anyway: flat.
                if (!isOpaque[i]) { nx = 0; ny = 0; nz = 1; }
                normals[i] = Pack(255, ToByte(nx * 0.5 + 0.5), ToByte(ny * 0.5 + 0.5), ToByte(nz * 0.5 + 0.5));

                int level = ToByte(heightMap[i]);
                heights[i] = Pack(255, level, level, level);

                // 4. Roughness from the settings, rougher or smoother where it is dark; metalness only for iron.
                double darkness = 1.0 - heightMap[i];
                double roughness = Clamp01(settings.Roughness + settings.RoughnessFromDarkness * (darkness - 0.5) * 2.0);
                double metalness = 0.0;
                if (settings.IsIronMetal)
                {
                    int argb = color[i];
                    int red = (argb >> 16) & 255, green = (argb >> 8) & 255, blue = argb & 255;
                    int max = Math.Max(red, Math.Max(green, blue)), min = Math.Min(red, Math.Min(green, blue));
                    double saturation = max == 0 ? 0.0 : (max - min) / (double)max;

                    // Dull dark iron (saturation below 0.45: most of the band) is metal and fairly smooth; the bright brown
                    // and orange of rust (0.5 and more) is not metal and rough.
                    bool isIron = saturation < 0.45;
                    metalness = isIron ? 1.0 : 0.0;
                    roughness = isIron ? 0.45 : 0.85;
                }

                // Red is left at 1: glTF keeps the occlusion of light there, which the game does not read.
                metalRough[i] = Pack(255, 255, ToByte(roughness), ToByte(metalness));
            }

        Directory.CreateDirectory(outputDirectory);
        SavePixels(normals, width, height, Path.Combine(outputDirectory, name + "_Normal.png"));
        SavePixels(metalRough, width, height, Path.Combine(outputDirectory, name + "_MetalRough.png"));
        SavePixels(heights, width, height, Path.Combine(outputDirectory, name + "_Height.png"));
    }

    // Makes the maps of every texture of a folder that has settings.
    public static void GenerateAll(string colorDirectory, string outputDirectory, Dictionary<string, Settings> settings)
    {
        foreach (var entry in settings)
        {
            string colorPath = Path.Combine(colorDirectory, entry.Key + ".png");
            if (!File.Exists(colorPath))
                throw new FileNotFoundException("No color texture for the settings of " + entry.Key, colorPath);
            Generate(colorPath, outputDirectory, entry.Value);
        }
    }
}
