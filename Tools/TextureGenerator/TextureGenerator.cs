using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

// Draws the textures of Abomination with code: tileable value noise, brick and slab layouts, cracks as random walks,
// all in the colors of an episode palette (see Documentation/ART_DIRECTION.md). Run it with Generate.ps1.
//
// Written for the C# compiler built into Windows PowerShell (C# 5): no string interpolation, no tuples.
public static class TextureGen
{
    const int S = 64;

    // ---------- noise ----------
    static uint Hash(int x, int y, int seed)
    {
        unchecked
        {
            uint h = (uint)(x * 374761393 + y * 668265263 + seed * 1442695041);
            h = (h ^ (h >> 13)) * 1274126177u;
            return h ^ (h >> 16);
        }
    }

    static double Rand(int x, int y, int seed) { return (Hash(x, y, seed) & 0xFFFFFF) / 16777216.0; }
    static int Wrap(int v, int p) { int r = v % p; return r < 0 ? r + p : r; }

    // Tileable value noise: cellsX x cellsY lattice over the 64x64 texture.
    static double Value(double x, double y, int cellsX, int cellsY, int seed)
    {
        double fx = x * cellsX / S, fy = y * cellsY / S;
        int x0 = (int)Math.Floor(fx), y0 = (int)Math.Floor(fy);
        double tx = fx - x0, ty = fy - y0;
        tx = tx * tx * (3 - 2 * tx);
        ty = ty * ty * (3 - 2 * ty);
        double a = Rand(Wrap(x0, cellsX), Wrap(y0, cellsY), seed);
        double b = Rand(Wrap(x0 + 1, cellsX), Wrap(y0, cellsY), seed);
        double c = Rand(Wrap(x0, cellsX), Wrap(y0 + 1, cellsY), seed);
        double d = Rand(Wrap(x0 + 1, cellsX), Wrap(y0 + 1, cellsY), seed);
        double top = a + (b - a) * tx, bottom = c + (d - c) * tx;
        return top + (bottom - top) * ty;
    }

    static double Fbm(double x, double y, int cellsX, int cellsY, int octaves, int seed)
    {
        double sum = 0, amp = 1, total = 0;
        for (int i = 0; i < octaves; i++)
        {
            sum += Value(x, y, cellsX, cellsY, seed + i * 101) * amp;
            total += amp;
            amp *= 0.5;
            cellsX = Math.Min(cellsX * 2, S);
            cellsY = Math.Min(cellsY * 2, S);
        }
        return sum / total;
    }

    // Thin cracks as random walks: a start point and a direction that turns a little at every step.
    // Coordinates wrap around, so the texture stays tileable.
    static bool[,] CrackMask(int count, int length, int seed)
    {
        bool[,] mask = new bool[S, S];
        for (int i = 0; i < count; i++)
        {
            double x = Rand(i, 0, seed) * S, y = Rand(i, 1, seed) * S;
            double angle = Rand(i, 2, seed) * Math.PI * 2;
            int steps = length / 2 + (int)(Rand(i, 3, seed) * length);
            for (int s = 0; s < steps; s++)
            {
                mask[Wrap((int)Math.Round(x), S), Wrap((int)Math.Round(y), S)] = true;
                angle += (Rand(i * 131 + s, 4, seed) - 0.5) * 1.1;
                x += Math.Cos(angle);
                y += Math.Sin(angle);
            }
        }
        return mask;
    }

    // ---------- colors ----------
    static double[] Hex(string h)
    {
        return new double[] {
            Convert.ToInt32(h.Substring(1, 2), 16) / 255.0,
            Convert.ToInt32(h.Substring(3, 2), 16) / 255.0,
            Convert.ToInt32(h.Substring(5, 2), 16) / 255.0 };
    }

    static double[] Mix(double[] a, double[] b, double t)
    {
        t = Math.Max(0, Math.Min(1, t));
        return new double[] { a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t };
    }

    static double[] Mul(double[] a, double k) { return new double[] { a[0] * k, a[1] * k, a[2] * k }; }

    static Color ToColor(double[] c)
    {
        // Slight posterization for the retro look: steps of 6 per channel.
        int[] v = new int[3];
        for (int i = 0; i < 3; i++)
        {
            int x = (int)Math.Round(Math.Max(0, Math.Min(1, c[i])) * 255 / 6.0) * 6;
            v[i] = Math.Min(255, x);
        }
        return Color.FromArgb(255, v[0], v[1], v[2]);
    }

    // Episode 1 palette
    static readonly double[] Silt = Hex("#2B2A1F");
    static readonly double[] Moss = Hex("#4A4A2E");
    static readonly double[] Olive = Hex("#6B6A3C");
    static readonly double[] RotWood = Hex("#5A4632");
    static readonly double[] WetStone = Hex("#7D7458");
    static readonly double[] Lichen = Hex("#9FA35A");
    static readonly double[] Rust = Hex("#6E3B22");
    static readonly double[] RustLight = Hex("#9A4E24");
    static readonly double[] Iron = Hex("#3A3936");
    static readonly double[] WoodDark = Hex("#2E231B");
    static readonly double[] WoodMid = Hex("#4E3B2A");

    // ---------- walls ----------
    static Bitmap Bricks(int brickW, int brickH, double mossAmount, int seed, double[] stoneA, double[] stoneB)
    {
        Bitmap bmp = new Bitmap(S, S, PixelFormat.Format32bppArgb);
        bool[,] cracks = CrackMask(5, 10, seed + 23);
        for (int y = 0; y < S; y++)
            for (int x = 0; x < S; x++)
            {
                int row = y / brickH;
                int offset = (row % 2) * brickW / 2;
                int sx = Wrap(x + offset, S);
                int col = sx / brickW;
                int lx = sx % brickW, ly = y % brickH;
                bool mortar = lx == 0 || ly == 0;

                double detail = Fbm(x, y, 8, 8, 3, seed);
                double[] c;
                if (mortar)
                {
                    c = Mix(Silt, Moss, detail * 0.8);
                }
                else
                {
                    double tone = Rand(col, row, seed + 5);
                    c = Mix(stoneA, stoneB, tone);
                    c = Mul(c, 0.74 + detail * 0.36);
                    // crevice darkening next to the mortar (no light direction)
                    if (lx == 1 || ly == 1 || lx == brickW - 1 || ly == brickH - 1) c = Mul(c, 0.82);
                    // chipped edges: blobs of low-frequency noise near the mortar
                    double chip = Fbm(x, y, 8, 8, 2, seed + 11);
                    bool nearEdge = lx <= 2 || ly <= 2 || lx >= brickW - 2 || ly >= brickH - 2;
                    if (nearEdge && chip > 0.7) c = Mix(Silt, c, 0.4);
                    if (cracks[x, y]) c = Mul(c, 0.62);
                }

                // moss grows in the mortar and on the lower part of each brick
                double m = Fbm(x, y, 4, 4, 4, seed + 7) + (double)ly / brickH * 0.18 + (mortar ? 0.12 : 0);
                double threshold = 1.0 - mossAmount;
                if (m > threshold)
                {
                    double shade = Fbm(x, y, 16, 16, 2, seed + 9);
                    double[] mossColor = Mix(Moss, Olive, shade);
                    if (shade > 0.72) mossColor = Mix(mossColor, Lichen, 0.4);
                    c = Mix(c, mossColor, Math.Min(1, (m - threshold) * 6));
                }

                // damp streaks: bands 4 pixels wide, smooth inside (one noise cell per band across, a few down)
                if (Rand(x / 4, 0, seed + 33) < 0.25)
                    c = Mul(c, 0.94 - 0.12 * Fbm(x, y, 16, 4, 2, seed + 34));

                bmp.SetPixel(x, y, ToColor(c));
            }
        return bmp;
    }

    // ---------- floors ----------
    static Bitmap Cobbles(int grid, double wetness, int seed)
    {
        Bitmap bmp = new Bitmap(S, S, PixelFormat.Format32bppArgb);
        double cell = (double)S / grid;
        for (int y = 0; y < S; y++)
            for (int x = 0; x < S; x++)
            {
                int cx = (int)(x / cell), cy = (int)(y / cell);
                double d1 = 1e9, d2 = 1e9;
                int id = 0;
                for (int oy = -1; oy <= 1; oy++)
                    for (int ox = -1; ox <= 1; ox++)
                    {
                        int gx = cx + ox, gy = cy + oy;
                        int wx = Wrap(gx, grid), wy = Wrap(gy, grid);
                        double px = (gx + 0.2 + 0.6 * Rand(wx, wy, seed)) * cell;
                        double py = (gy + 0.2 + 0.6 * Rand(wx, wy, seed + 1)) * cell;
                        double dx = x + 0.5 - px, dy = y + 0.5 - py;
                        double d = Math.Sqrt(dx * dx + dy * dy);
                        if (d < d1) { d2 = d1; d1 = d; id = wx + wy * 97; }
                        else if (d < d2) d2 = d;
                    }
                double edge = d2 - d1;
                double detail = Fbm(x, y, 8, 8, 3, seed + 3);
                double[] c;
                if (edge < 1.7)
                {
                    c = Mix(Silt, Moss, detail * 0.8);
                }
                else
                {
                    double tone = Rand(id, 0, seed + 4);
                    c = Mix(Mix(Moss, WetStone, 0.5), WetStone, tone);
                    c = Mul(c, 0.66 + detail * 0.32);
                    if (edge < 3.0) c = Mul(c, 0.84);
                }
                double puddle = Fbm(x, y, 2, 2, 3, seed + 8);
                if (puddle > 1.0 - wetness) c = Mul(Mix(c, Silt, 0.25), 0.8);
                bmp.SetPixel(x, y, ToColor(c));
            }
        return bmp;
    }

    static Bitmap Flagstones(double wetness, int seed)
    {
        Bitmap bmp = new Bitmap(S, S, PixelFormat.Format32bppArgb);
        bool[,] cracks = CrackMask(6, 12, seed + 21);
        const int rowH = 16;
        // slab widths per row, chosen so they add up to 64
        int[][] patterns = { new int[] { 32, 16, 16 }, new int[] { 24, 24, 16 }, new int[] { 16, 32, 16 }, new int[] { 40, 24 } };
        int[][] rows = new int[4][];
        for (int r = 0; r < 4; r++)
            rows[r] = patterns[(int)(Rand(r, 0, seed) * patterns.Length)];
        for (int y = 0; y < S; y++)
            for (int x = 0; x < S; x++)
            {
                int r = y / rowH, ly = y % rowH;
                int shift = (int)(Rand(r, 1, seed) * 4) * 8;
                int sx = Wrap(x + shift, S);
                int start = 0, slab = 0;
                while (sx >= start + rows[r][slab]) { start += rows[r][slab]; slab++; }
                int lx = sx - start, w = rows[r][slab];
                bool joint = lx == 0 || ly == 0;
                double detail = Fbm(x, y, 8, 8, 3, seed + 3);
                double[] c;
                if (joint) c = Mix(Silt, Moss, detail * 0.7);
                else
                {
                    double tone = Rand(slab, r, seed + 4);
                    c = Mix(Mix(Moss, WetStone, 0.4), WetStone, tone);
                    c = Mul(c, 0.66 + detail * 0.32);
                    if (lx == 1 || ly == 1 || lx == w - 1 || ly == rowH - 1) c = Mul(c, 0.84);
                    if (cracks[x, y]) c = Mul(c, 0.62);
                }
                double puddle = Fbm(x, y, 2, 2, 3, seed + 8);
                if (puddle > 1.0 - wetness) c = Mul(Mix(c, Silt, 0.25), 0.8);
                bmp.SetPixel(x, y, ToColor(c));
            }
        return bmp;
    }

    static Bitmap Planks(int seed)
    {
        Bitmap bmp = new Bitmap(S, S, PixelFormat.Format32bppArgb);
        const int plankH = 8;
        double[] light = Hex("#6E5840");
        for (int y = 0; y < S; y++)
            for (int x = 0; x < S; x++)
            {
                int p = y / plankH, ly = y % plankH;
                int seam = (int)(Rand(p, 0, seed) * S);
                int lx = Wrap(x - seam, S);
                bool gap = ly == 0 || lx == 0;
                double grain = Fbm(x, y, 2, 16, 3, seed + p);
                double[] c;
                if (gap) c = Mix(Silt, WoodDark, 0.3);
                else
                {
                    c = Mix(WoodDark, RotWood, 0.2 + grain * 0.9);
                    if (Rand(p, 3, seed) > 0.6) c = Mix(c, light, 0.15);
                    if (ly == 1 || ly == plankH - 1 || lx == 1 || lx == S - 1) c = Mul(c, 0.82);
                    // rot: dark damp patches, different on every plank
                    double rot = Fbm(x, y, 4, 8, 3, seed + 40 + p * 7);
                    if (rot > 0.62) c = Mix(c, Mix(Silt, Moss, 0.4), (rot - 0.62) * 5);
                    // nails near the seam
                    if ((lx == 2 || lx == S - 3) && (ly == 2 || ly == 6)) c = Mix(Iron, Rust, 0.4);
                }
                bmp.SetPixel(x, y, ToColor(c));
            }
        return bmp;
    }

    // ---------- crates ----------
    static double[] Wood(double grain, double darkness)
    {
        return Mul(Mix(WoodDark, WoodMid, 0.25 + grain * 0.95), darkness);
    }

    static double[] IronAt(int x, int y, int seed)
    {
        double n = Fbm(x, y, 16, 16, 3, seed);
        double[] c = Mul(Iron, 0.85 + n * 0.35);
        double rust = Fbm(x, y, 8, 8, 3, seed + 3);
        if (rust > 0.52) c = Mix(c, Mix(rust > 0.64 ? RustLight : Rust, Iron, 0.35), Math.Min(1, (rust - 0.52) * 5));
        return c;
    }

    static Bitmap Crate(bool brace, int seed)
    {
        Bitmap bmp = new Bitmap(S, S, PixelFormat.Format32bppArgb);
        bool[,] cracks = CrackMask(7, 9, seed + 100);
        double[] rivetColor = Hex("#6E6A62");
        const int frame = 7;
        for (int y = 0; y < S; y++)
            for (int x = 0; x < S; x++)
            {
                bool inFrame = x < frame || x >= S - frame || y < frame || y >= S - frame;
                int ex = Math.Min(x, S - 1 - x), ey = Math.Min(y, S - 1 - y);
                double[] c;
                if (inFrame)
                {
                    bool horizontalPart = (y < frame || y >= S - frame) && ex >= frame;
                    double grain = horizontalPart ? Fbm(x, y, 2, 16, 3, seed + 50) : Fbm(x, y, 16, 2, 3, seed + 51);
                    c = Wood(grain, 0.85);
                    if (ex == 0 || ey == 0) c = Mul(c, 0.7);
                    if ((ex == frame - 1 && ey >= frame - 1) || (ey == frame - 1 && ex >= frame - 1)) c = Mul(c, 0.72);
                }
                else
                {
                    // inner planks: vertical, 13 pixels wide, with dark gaps between them
                    int plank = (x - frame) / 13, lx = (x - frame) % 13;
                    c = Wood(Fbm(x, y, 16, 2, 3, seed + 60 + plank), 0.95);
                    if (lx == 0) c = Mix(Silt, c, 0.25);
                    if (brace)
                    {
                        double dist = Math.Abs((x + 0.5) + (y + 0.5) - S) / Math.Sqrt(2);
                        if (dist < 4.5)
                        {
                            // grain along the brace: noise stretched across the diagonal direction
                            c = Wood(Fbm(x + y, x - y + 64, 2, 16, 3, seed + 70), 0.88);
                            if (dist > 3.5) c = Mul(c, 0.7);
                        }
                    }
                    else if (y >= 29 && y <= 34)
                    {
                        // an iron band across the middle, riveted to the frame
                        c = IronAt(x, y, seed + 80);
                        if (y == 29 || y == 34) c = Mul(c, 0.7);
                        if ((y == 31 || y == 32) && (x == 12 || x == 13 || x == 50 || x == 51)) c = Mix(rivetColor, Rust, 0.3);
                    }
                }

                // iron corner brackets (L shapes) with rivets
                bool bracket = (ex < 13 && ey < frame) || (ex < frame && ey < 13);
                if (bracket)
                {
                    c = IronAt(x, y, seed + 90);
                    bool outline = ex == 0 || ey == 0 || ex == 12 || ey == 12 ||
                                   (ey == frame - 1 && ex >= frame - 1) || (ex == frame - 1 && ey >= frame - 1);
                    if (outline) c = Mul(c, 0.65);
                    bool rivet = (ex == 3 && ey == 3) || (ex == 9 && ey == 3) || (ex == 3 && ey == 9);
                    if (rivet) c = Mix(rivetColor, Rust, 0.35);
                }

                // cracks and grime
                if (!bracket && cracks[x, y]) c = Mul(c, 0.55);
                double grime = Fbm(x, y, 4, 4, 3, seed + 110);
                c = Mul(c, 0.95 - 0.18 * grime);
                if (grime > 0.66) c = Mix(c, Silt, (grime - 0.66) * 2.5);

                // rust stains running down from the upper brackets
                if (!bracket && y >= frame && y < S / 2 && ex >= 2 && ex <= 10 && Rand(x, 7, seed + 120) > 0.55)
                {
                    double fade = 1.0 - (double)(y - frame) / 22.0;
                    if (fade > 0) c = Mix(c, Rust, 0.4 * fade);
                }

                bmp.SetPixel(x, y, ToColor(c));
            }
        return bmp;
    }

    // ---------- preview sheets ----------
    static void Sheet(string path, string[] names, Bitmap[] textures)
    {
        const int single = 256, tile = 384, pad = 16, label = 26;
        int w = pad + single + pad + tile + pad;
        int h = textures.Length * (tile + label + pad) + pad;
        using (Bitmap sheet = new Bitmap(w, h))
        using (Graphics g = Graphics.FromImage(sheet))
        using (Font font = new Font("Segoe UI", 12, FontStyle.Bold))
        {
            g.Clear(Color.FromArgb(30, 30, 32));
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            int y = pad;
            for (int i = 0; i < textures.Length; i++)
            {
                g.DrawString(names[i], font, Brushes.Gainsboro, pad, y);
                int top = y + label;
                g.DrawImage(textures[i], new Rectangle(pad, top, single, single));
                for (int ty = 0; ty < 3; ty++)
                    for (int tx = 0; tx < 3; tx++)
                        g.DrawImage(textures[i], new Rectangle(pad + single + pad + tx * 128, top + ty * 128, 128, 128));
                y += tile + label + pad;
            }
            sheet.Save(path, ImageFormat.Png);
        }
    }

    // The palettes of all episodes (Documentation/ART_DIRECTION.md), drawn as rows of swatches for the documentation.
    static readonly string[] EpisodeNames = { "E1  The Drowned Parish", "E2  The Iron Crypts", "E3  The Ossuary", "E4  The Flesh Abyss" };
    static readonly string[][] EpisodePalettes = {
        new string[] { "#2B2A1F", "#4A4A2E", "#6B6A3C", "#5A4632", "#7D7458", "#9FA35A" },
        new string[] { "#1E1A18", "#3B2F2A", "#6E3B22", "#9A4E24", "#5E5E5A", "#D9792B" },
        new string[] { "#1C1A24", "#3A3448", "#5E5470", "#A89F8A", "#D6CEB8", "#8C7A3E" },
        new string[] { "#120B0D", "#3A1216", "#6E1A22", "#A8323A", "#7A3A5E", "#E0662E" },
    };

    static void PaletteSheet(string path)
    {
        const int swatch = 56, pad = 16, labelWidth = 230, rowHeight = swatch + 12;
        int w = pad + labelWidth + EpisodePalettes[0].Length * (swatch + 6) + pad;
        int h = pad + EpisodePalettes.Length * rowHeight + pad;
        using (Bitmap sheet = new Bitmap(w, h))
        using (Graphics g = Graphics.FromImage(sheet))
        using (Font font = new Font("Segoe UI", 12, FontStyle.Bold))
        {
            g.Clear(Color.FromArgb(30, 30, 32));
            for (int e = 0; e < EpisodePalettes.Length; e++)
            {
                int top = pad + e * rowHeight;
                g.DrawString(EpisodeNames[e], font, Brushes.Gainsboro, pad, top + swatch / 2 - 10);
                for (int i = 0; i < EpisodePalettes[e].Length; i++)
                    using (SolidBrush brush = new SolidBrush(ColorTranslator.FromHtml(EpisodePalettes[e][i])))
                        g.FillRectangle(brush, pad + labelWidth + i * (swatch + 6), top, swatch, swatch);
            }
            sheet.Save(path, ImageFormat.Png);
        }
    }

    // Writes every texture of the game made by this generator into texturesDirectory (Assets/Textures), a preview sheet
    // ---------- effects ----------
    // Sprites of particles and decals: not tileable, with transparency. Small (16 or 32 pixels) and drawn with the same
    // crisp nearest filtering as the walls, so they look pixelated like the rest of the game.

    // A color with transparency, posterized like ToColor; alpha is quantized to steps of 1/8, which keeps soft edges
    // pixelated instead of smooth.
    static Color ToColorAlpha(double[] c, double alpha)
    {
        Color opaque = ToColor(c);
        int a = (int)Math.Round(Math.Max(0, Math.Min(1, alpha)) * 8) * 32;
        return Color.FromArgb(Math.Min(255, a), opaque.R, opaque.G, opaque.B);
    }

    // Distance of a pixel center from the middle of a size x size sprite, 0 in the middle and 1 at the edge of the
    // inscribed circle, and the angle around the middle.
    static double Radius(int x, int y, int size) { double h = size / 2.0; return Math.Sqrt((x + 0.5 - h) * (x + 0.5 - h) + (y + 0.5 - h) * (y + 0.5 - h)) / h; }
    static double Angle(int x, int y, int size) { double h = size / 2.0; return Math.Atan2(y + 0.5 - h, x + 0.5 - h); }

    // The flash at the muzzle: a white-hot core with five uneven flame tongues, yellow to orange, fading at the tips.
    static Bitmap MuzzleFlash(int seed)
    {
        const int size = 32;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        double[] white = Hex("#FFF6D8"), yellow = Hex("#FFD23C"), orange = Hex("#E8741E");
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                double r = Radius(x, y, size), a = Angle(x, y, size);

                // The edge of the flame: a core of 0.35 plus tongues that reach out where |cos(2.5 * angle)| is large,
                // each a little longer or shorter (noise along the angle).
                double tongues = Math.Pow(Math.Abs(Math.Cos(2.5 * a)), 3.0);
                double jitter = 0.75 + 0.5 * Rand((int)((a + Math.PI) * 4), 0, seed);
                double edge = 0.35 + 0.6 * tongues * jitter;
                if (r > edge) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }

                double t = r / edge; // 0 in the middle, 1 at the edge of the flame
                double[] c = t < 0.35 ? Mix(white, yellow, t / 0.35) : Mix(yellow, orange, (t - 0.35) / 0.65);
                bmp.SetPixel(x, y, ToColorAlpha(c, 1.0 - t * t * 0.7));
            }
        return bmp;
    }

    // A spark: a tiny bright dot with a soft yellow glow around it.
    static Bitmap Spark()
    {
        const int size = 16;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        double[] white = Hex("#FFFBE8"), yellow = Hex("#FFC43A");
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                double r = Radius(x, y, size);
                if (r > 1.0) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }
                bmp.SetPixel(x, y, ToColorAlpha(Mix(white, yellow, r * 1.5), Math.Pow(1.0 - r, 1.5)));
            }
        return bmp;
    }

    // A puff of smoke or dust: a lumpy round cloud, denser in the middle, in the given color.
    static Bitmap Puff(double[] light, double[] dark, int seed)
    {
        const int size = 32;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                // The noise works on 64 x 64 coordinates: the sprite is spread over it twice as wide.
                double lumps = Fbm(x * 2, y * 2, 4, 4, 3, seed);
                double density = (1.0 - Radius(x, y, size)) * 1.6 - 0.35 + (lumps - 0.5) * 0.9;
                if (density <= 0) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }
                bmp.SetPixel(x, y, ToColorAlpha(Mix(dark, light, lumps), Math.Min(1.0, density) * 0.85));
            }
        return bmp;
    }

    // The mark a pellet leaves on a wall: a black hole with a ragged dark ring of chipped surface around it.
    static Bitmap PelletMark(int seed)
    {
        const int size = 16;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        double[] black = Hex("#0B0A08"), soot = Hex("#2A2620");
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                double r = Radius(x, y, size);
                double ragged = 0.75 + 0.35 * Rand((int)((Angle(x, y, size) + Math.PI) * 3), 1, seed);
                if (r > ragged) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }
                bool hole = r < 0.3;
                bmp.SetPixel(x, y, ToColorAlpha(hole ? black : soot, hole ? 1.0 : 0.85 - r * 0.4));
            }
        return bmp;
    }

    // A drop of blood: an uneven dark red blob, brighter at a highlight on one side.
    static Bitmap Blood(int seed)
    {
        const int size = 16;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        double[] dark = Hex("#4A0808"), red = Hex("#9A1410");
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                double lumps = Fbm(x * 4, y * 4, 4, 4, 2, seed);
                double inside = 1.0 - Radius(x, y, size) + (lumps - 0.5) * 0.6;
                if (inside < 0.25) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }
                double highlight = Math.Max(0, 1.0 - Math.Sqrt((x - 5) * (x - 5) + (y - 5) * (y - 5)) / 5.0);
                bmp.SetPixel(x, y, ToColorAlpha(Mix(dark, red, 0.4 + highlight * 0.6), 1.0));
            }
        return bmp;
    }

    // per episode into previewDirectory (each texture enlarged and tiled 3 x 3, to check the seams) and the image of the
    // episode palettes to paletteImagePath (for ART_DIRECTION.md).
    // The seeds are fixed, so the same code always gives exactly the same files.
    public static void Run(string texturesDirectory, string previewDirectory, string paletteImagePath)
    {
        string episode1 = Path.Combine(texturesDirectory, "Episode1");
        Directory.CreateDirectory(episode1);
        Directory.CreateDirectory(previewDirectory);

        string[] names = { "Wall_MossyBrick", "Floor_WetFlagstone", "Floor_RottenPlanks", "Crate_Rotten" };
        Bitmap[] textures = {
            Bricks(16, 8, 0.30, 1, WetStone, Olive),
            Flagstones(0.30, 5),
            Planks(6),
            Crate(true, 7),
        };

        for (int i = 0; i < names.Length; i++)
            textures[i].Save(Path.Combine(episode1, names[i] + ".png"), ImageFormat.Png);

        Sheet(Path.Combine(previewDirectory, "Episode1.png"), names, textures);
        PaletteSheet(paletteImagePath);

        string effects = Path.Combine(texturesDirectory, "Effects");
        Directory.CreateDirectory(effects);
        string[] effectNames = { "MuzzleFlash", "Spark", "Smoke", "Dust", "PelletMark", "Blood" };
        Bitmap[] effectTextures = {
            MuzzleFlash(11),
            Spark(),
            Puff(Hex("#8C8A84"), Hex("#4C4A46"), 12),
            Puff(WetStone, Silt, 13),
            PelletMark(14),
            Blood(15),
        };
        for (int i = 0; i < effectNames.Length; i++)
            effectTextures[i].Save(Path.Combine(effects, effectNames[i] + ".png"), ImageFormat.Png);
    }
}
