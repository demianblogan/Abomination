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

    // ---------- HUD icons ----------
    // Icons of the HUD in the look of the world: drawn at 32 x 32 pixels like the textures (no smoothing, colors
    // posterized), from battered materials - scratched iron, rust, chipped paint, tarnished brass, grime in the corners -
    // with a light top-left edge and a dark bottom-right one, and a black outline. Saved 4 times larger with every pixel
    // turned into a 4 x 4 block, so the smooth filtering of the interface keeps them crisp.
    const int IconPixels = 32;
    const int IconScale = 4;

    // Which pixels of a 32 x 32 icon a shape covers, drawn without smoothing.
    static bool[,] Mask(Action<Graphics> draw)
    {
        bool[,] mask = new bool[IconPixels, IconPixels];
        using (Bitmap bmp = new Bitmap(IconPixels, IconPixels, PixelFormat.Format32bppArgb))
        using (Graphics g = Graphics.FromImage(bmp))
        {
            g.SmoothingMode = SmoothingMode.None;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            g.Clear(Color.Transparent);
            draw(g);
            for (int y = 0; y < IconPixels; y++)
                for (int x = 0; x < IconPixels; x++)
                    mask[x, y] = bmp.GetPixel(x, y).A > 128;
        }
        return mask;
    }

    static bool In(bool[,] mask, int x, int y)
    {
        return x >= 0 && y >= 0 && x < IconPixels && y < IconPixels && mask[x, y];
    }

    // A worn material: base colors from light to dark, how much rust and how many scratches, a seed.
    sealed class Material
    {
        public double[] Light, Dark, Rust;
        public double RustAmount, Scratches, Grime;
        public Material(string light, string dark, string rust, double rustAmount, double scratches, double grime)
        {
            Light = Hex(light); Dark = Hex(dark); Rust = Hex(rust);
            RustAmount = rustAmount; Scratches = scratches; Grime = grime;
        }
    }

    static readonly Material IconIron = new Material("#8A877C", "#2E2C28", "#6E3B22", 0.35, 0.10, 0.35);
    static readonly Material IconRedPaint = new Material("#A8281C", "#3E0C08", "#3A2418", 0.25, 0.12, 0.30);
    static readonly Material IconBrass = new Material("#C8A04A", "#4A3414", "#3E4A2A", 0.20, 0.08, 0.30);
    static readonly Material IconBronze = new Material("#A8703A", "#3A220E", "#3E5A40", 0.35, 0.08, 0.30);
    static readonly Material IconSilver = new Material("#C4C4BC", "#4A4A46", "#2E2C28", 0.15, 0.10, 0.30);
    static readonly Material IconGold = new Material("#E0B848", "#5A3C0C", "#4A3010", 0.10, 0.06, 0.25);
    static readonly Material IconCellBlue = new Material("#4A78A0", "#101E30", "#2A3A2A", 0.20, 0.10, 0.35);
    static readonly Material IconArmorGreen = new Material("#7A9446", "#1A2410", "#5A3A1E", 0.25, 0.10, 0.30);
    static readonly Material IconBone = new Material("#D8CCA8", "#5A5038", "#4A3A22", 0.10, 0.06, 0.30);

    // Paints the shape of mask into the icon with the material: noise for a rough surface, a bevel (lighter where the
    // shape has an edge above or to the left, darker below or to the right), rust patches, scratches and grime near the
    // edges.
    static void Paint(Color[,] icon, bool[,] mask, Material m, int seed)
    {
        // The bounds of the shape, for the light falling across it.
        int minX = IconPixels, minY = IconPixels, maxX = 0, maxY = 0;
        for (int y = 0; y < IconPixels; y++)
            for (int x = 0; x < IconPixels; x++)
                if (mask[x, y])
                {
                    minX = Math.Min(minX, x); minY = Math.Min(minY, y);
                    maxX = Math.Max(maxX, x); maxY = Math.Max(maxY, y);
                }
        double width = Math.Max(1, maxX - minX), height = Math.Max(1, maxY - minY);

        for (int y = 0; y < IconPixels; y++)
            for (int x = 0; x < IconPixels; x++)
            {
                if (!mask[x, y])
                    continue;

                // A rough surface lit from the top left: a gradient across the whole shape from bright (top left) to dark
                // (bottom right) makes it look rounded, raised out of the screen.
                double rough = Fbm(x * 2, y * 2, 8, 8, 3, seed);
                double light = 1.0 - ((x - minX) / width + (y - minY) / height) * 0.5;
                double shade = 0.0 + rough * 0.3 + light * 0.85;

                // The bevel, two pixels wide: edges facing up-left catch the light, edges facing down-right are in shadow.
                if (!In(mask, x - 1, y) || !In(mask, x, y - 1)) shade += 0.35;
                else if (!In(mask, x - 2, y) || !In(mask, x, y - 2)) shade += 0.15;
                if (!In(mask, x + 1, y) || !In(mask, x, y + 1)) shade -= 0.40;
                else if (!In(mask, x + 2, y) || !In(mask, x, y + 2)) shade -= 0.18;

                double[] c = Mix(m.Dark, m.Light, Math.Max(0, Math.Min(1, shade)));

                // Rust and wear in patches.
                double rust = Fbm(x * 2 + 17, y * 2 + 5, 4, 4, 2, seed + 7);
                if (rust > 1.0 - m.RustAmount)
                    c = Mix(c, m.Rust, Math.Min(1, (rust - (1.0 - m.RustAmount)) * 4));

                // Scratches: single bright pixels.
                if (Rand(x, y, seed + 13) < m.Scratches)
                    c = Mix(c, m.Light, 0.6);

                // Grime collects along the shaded edges and in random specks (not on the lit edges, which stay bright).
                bool shadedEdge = !In(mask, x + 1, y) || !In(mask, x, y + 1);
                if (shadedEdge || Rand(x, y, seed + 29) < m.Grime * 0.25)
                    c = Mix(c, Hex("#1A1712"), m.Grime * (shadedEdge ? 0.4 : 0.8));

                icon[x, y] = ToColor(c);
            }
    }

    // Knocks pixels out of the edge of a shape (chips), and draws the black outline around everything painted.
    static Bitmap Finish(Color[,] icon, int seed, double chips)
    {
        bool[,] filled = new bool[IconPixels, IconPixels];
        for (int y = 0; y < IconPixels; y++)
            for (int x = 0; x < IconPixels; x++)
                filled[x, y] = icon[x, y].A > 0;

        for (int y = 0; y < IconPixels; y++)
            for (int x = 0; x < IconPixels; x++)
            {
                bool edge = filled[x, y] &&
                    (!In(filled, x - 1, y) || !In(filled, x + 1, y) || !In(filled, x, y - 1) || !In(filled, x, y + 1));
                if (edge && Rand(x, y, seed + 41) < chips)
                    icon[x, y] = Color.Transparent;
            }

        Color outline = Color.FromArgb(255, 10, 9, 7);
        Bitmap bmp = new Bitmap(IconPixels * IconScale, IconPixels * IconScale, PixelFormat.Format32bppArgb);
        for (int y = 0; y < IconPixels; y++)
            for (int x = 0; x < IconPixels; x++)
            {
                Color c = icon[x, y];
                if (c.A == 0)
                {
                    bool nearShape = false;
                    for (int dy = -1; dy <= 1 && !nearShape; dy++)
                        for (int dx = -1; dx <= 1 && !nearShape; dx++)
                        {
                            int nx = x + dx, ny = y + dy;
                            if (nx >= 0 && ny >= 0 && nx < IconPixels && ny < IconPixels && icon[nx, ny].A > 0)
                                nearShape = true;
                        }
                    c = nearShape ? outline : Color.Transparent;
                }
                for (int by = 0; by < IconScale; by++)
                    for (int bx = 0; bx < IconScale; bx++)
                        bmp.SetPixel(x * IconScale + bx, y * IconScale + by, c);
            }
        return bmp;
    }

    static Color[,] NewIconPixels()
    {
        Color[,] icon = new Color[IconPixels, IconPixels];
        for (int y = 0; y < IconPixels; y++)
            for (int x = 0; x < IconPixels; x++)
                icon[x, y] = Color.Transparent;
        return icon;
    }

    static readonly Brush White = Brushes.White;

    // Health: a dented square iron box with a chipped red cross painted on it.
    static Bitmap HealthIcon()
    {
        Color[,] icon = NewIconPixels();
        Paint(icon, Mask(g => g.FillRectangle(White, 3, 3, 26, 26)), IconIron, 101);
        Paint(icon, Mask(g => { g.FillRectangle(White, 13, 7, 6, 18); g.FillRectangle(White, 7, 13, 18, 6); }),
              IconRedPaint, 103);
        return Finish(icon, 104, 0.10);
    }

    // Armor: a battered green breastplate with rivets and rust, like the green armor of Quake.
    static Bitmap ArmorIcon()
    {
        Color[,] icon = NewIconPixels();
        Paint(icon, Mask(g => g.FillPolygon(White, new Point[] {
            new Point(4, 4), new Point(11, 4), new Point(16, 8), new Point(21, 4), new Point(28, 4), new Point(28, 14),
            new Point(25, 24), new Point(16, 29), new Point(7, 24), new Point(4, 14) })), IconArmorGreen, 111);
        Paint(icon, Mask(g => { g.FillRectangle(White, 15, 10, 2, 16); }), New("#4A4740", "#1E1C18"), 112);
        Point[] rivets = { new Point(7, 7), new Point(24, 7), new Point(8, 17), new Point(23, 17) };
        Paint(icon, Mask(g => { foreach (Point p in rivets) g.FillRectangle(White, p.X, p.Y, 2, 2); }), IconBrass, 113);
        return Finish(icon, 114, 0.08);
    }

    // Shells: two scuffed red shotgun shells on tarnished brass bases.
    static Bitmap ShellsIcon()
    {
        Color[,] icon = NewIconPixels();
        foreach (int x in new int[] { 6, 17 })
        {
            Paint(icon, Mask(g => g.FillRectangle(White, x, 4, 9, 18)), IconRedPaint, 121 + x);
            Paint(icon, Mask(g => g.FillRectangle(White, x - 1, 21, 11, 7)), IconBrass, 131 + x);
        }
        return Finish(icon, 124, 0.10);
    }

    // Bullets: three dirty brass cartridges with dark copper tips.
    static Bitmap BulletsIcon()
    {
        Color[,] icon = NewIconPixels();
        foreach (int x in new int[] { 5, 13, 21 })
        {
            Point[] tip = {
                new Point(x, 12), new Point(x + 1, 7), new Point(x + 3, 3), new Point(x + 5, 7), new Point(x + 6, 12) };
            Paint(icon, Mask(g => g.FillPolygon(White, tip)), New("#A0583A", "#3A180C"), 141 + x);
            Paint(icon, Mask(g => g.FillRectangle(White, x, 12, 6, 16)), IconBrass, 151 + x);
        }
        return Finish(icon, 144, 0.10);
    }

    // Rockets: a rusty iron rocket with a chipped red nose and bent fins, standing upright.
    static Bitmap RocketsIcon()
    {
        Color[,] icon = NewIconPixels();
        Point[] leftFin = { new Point(11, 20), new Point(6, 29), new Point(11, 27) };
        Point[] rightFin = { new Point(21, 20), new Point(26, 29), new Point(21, 27) };
        Paint(icon, Mask(g => { g.FillPolygon(White, leftFin); g.FillPolygon(White, rightFin); }), IconRedPaint, 161);
        Paint(icon, Mask(g => g.FillRectangle(White, 11, 10, 10, 19)), IconIron, 162);
        Point[] nose = { new Point(11, 11), new Point(13, 5), new Point(16, 2), new Point(19, 5), new Point(21, 11) };
        Paint(icon, Mask(g => g.FillPolygon(White, nose)), IconRedPaint, 163);
        return Finish(icon, 164, 0.08);
    }

    // Cells: a grimy battery of dull blue metal with iron caps and a bone-white lightning bolt.
    static Bitmap CellsIcon()
    {
        Color[,] icon = NewIconPixels();
        Paint(icon, Mask(g => g.FillRectangle(White, 6, 7, 20, 21)), IconCellBlue, 171);
        Paint(icon, Mask(g => { g.FillRectangle(White, 9, 3, 5, 4); g.FillRectangle(White, 18, 3, 5, 4); }), IconIron, 172);
        Point[] bolt = {
            new Point(18, 9), new Point(10, 19), new Point(15, 19), new Point(13, 27), new Point(22, 15), new Point(17, 15) };
        Paint(icon, Mask(g => g.FillPolygon(White, bolt)), IconBone, 173);
        return Finish(icon, 174, 0.08);
    }

    // A key: an old skeleton key - a ring, a long shaft and a toothed bit - in the given metal.
    static Bitmap KeyIcon(Material metal, int seed)
    {
        Color[,] icon = NewIconPixels();
        Paint(icon, Mask(g => {
            g.FillEllipse(White, 3, 3, 13, 13);
            g.FillRectangle(White, 13, 8, 16, 4);
            g.FillRectangle(White, 22, 12, 3, 6);
            g.FillRectangle(White, 26, 12, 3, 8);
        }), metal, seed);
        // The hole of the ring.
        bool[,] hole = Mask(g => g.FillEllipse(White, 7, 7, 5, 5));
        for (int y = 0; y < IconPixels; y++)
            for (int x = 0; x < IconPixels; x++)
                if (hole[x, y]) icon[x, y] = Color.Transparent;
        return Finish(icon, seed + 1, 0.06);
    }

    static Material New(string light, string dark)
    {
        return new Material(light, dark, "#3A2418", 0.20, 0.08, 0.30);
    }

    // Writes the HUD icons into iconsDirectory, and a preview sheet of all of them into previewPath.
    public static void RunIcons(string iconsDirectory, string previewPath)
    {
        Directory.CreateDirectory(iconsDirectory);
        string[] names = { "Health", "Armor", "Shells", "Bullets", "Rockets", "Cells", "KeyBronze", "KeySilver", "KeyGold" };
        Bitmap[] icons = {
            HealthIcon(), ArmorIcon(), ShellsIcon(), BulletsIcon(), RocketsIcon(), CellsIcon(),
            KeyIcon(IconBronze, 181), KeyIcon(IconSilver, 191), KeyIcon(IconGold, 201),
        };
        for (int i = 0; i < names.Length; i++)
            icons[i].Save(Path.Combine(iconsDirectory, names[i] + ".png"), ImageFormat.Png);

        // The preview: every icon twice, on a dark and on a light wall color, to see that it reads on both.
        int cell = IconPixels * IconScale + 16;
        using (Bitmap sheet = new Bitmap(cell * icons.Length, cell * 2, PixelFormat.Format32bppArgb))
        using (Graphics g = Graphics.FromImage(sheet))
        {
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.FillRectangle(new SolidBrush(ToColor(Silt)), 0, 0, sheet.Width, cell);
            g.FillRectangle(new SolidBrush(ToColor(WetStone)), 0, cell, sheet.Width, cell);
            for (int i = 0; i < icons.Length; i++)
            {
                g.DrawImage(icons[i], i * cell + 8, 8);
                g.DrawImage(icons[i], i * cell + 8, cell + 8);
            }
            Directory.CreateDirectory(Path.GetDirectoryName(previewPath));
            sheet.Save(previewPath, ImageFormat.Png);
        }
    }

    // ---------- HUD effects ----------
    // Full-screen and overlay pictures of the HUD: the vignettes of a blow (red) and of healing (green), and the arc that
    // shows where a blow came from. Pixelated like the icons: drawn small, posterized, with ragged edges from noise, and
    // saved with every pixel a 4 x 4 block.
    const int VignetteWidth = 128, VignetteHeight = 72;

    // How far a pixel is from the middle of the screen towards its edges: 0 in the middle, 1 at the middle of an edge,
    // more in the corners. A power of 2.6 rounds the shape between an ellipse and a rectangle, like the screen itself.
    static double EdgeDistance(int x, int y, int width, int height)
    {
        double nx = Math.Abs((x + 0.5) / width * 2 - 1), ny = Math.Abs((y + 0.5) / height * 2 - 1);
        return Math.Pow(Math.Pow(nx, 2.6) + Math.Pow(ny, 2.6), 1 / 2.6);
    }

    // Saves a picture of width x height pixels, every pixel a scale x scale block.
    static Bitmap Upscale(Color[,] pixels, int width, int height, int scale)
    {
        Bitmap bmp = new Bitmap(width * scale, height * scale, PixelFormat.Format32bppArgb);
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                for (int by = 0; by < scale; by++)
                    for (int bx = 0; bx < scale; bx++)
                        bmp.SetPixel(x * scale + bx, y * scale + by, pixels[x, y]);
        return bmp;
    }

    // A vignette: the edges of the screen tinted, ragged like a smear, fading to nothing in the middle. edge is the color
    // at the very edge (dark blood for a blow, a bright green glow for healing), inner the color further in, opacity
    // how strong it is at the edge (a glow is lighter than blood).
    static Bitmap Vignette(string inner, string edge, double opacity, int seed)
    {
        double[] innerColor = Hex(inner), edgeColor = Hex(edge);
        Color[,] pixels = new Color[VignetteWidth, VignetteHeight];
        for (int y = 0; y < VignetteHeight; y++)
            for (int x = 0; x < VignetteWidth; x++)
            {
                // The noise is laid over the 64 x 64 tile twice across and once down; it makes the inner edge ragged.
                double noise = Fbm(x * 64.0 / VignetteWidth * 2, y * 64.0 / VignetteHeight, 6, 6, 3, seed);
                double distance = EdgeDistance(x, y, VignetteWidth, VignetteHeight) + (noise - 0.5) * 0.45;
                double t = Math.Max(0, Math.Min(1, (distance - 0.62) / 0.5));
                double alpha = Math.Pow(t, 1.4);
                double[] c = Mix(innerColor, edgeColor, Math.Min(1, t * 1.2 + (Rand(x, y, seed + 3) - 0.5) * 0.2));
                pixels[x, y] = alpha <= 0 ? Color.Transparent : ToColorAlpha(c, alpha * opacity);
            }
        return Upscale(pixels, VignetteWidth, VignetteHeight, 4);
    }

    const int ArcWidth = 48, ArcHeight = 16;

    // The arc of the damage direction: a ragged crescent of blood, the top of a ring, thick in the middle and thin at its
    // ends. It points up; the HUD turns it towards where the blow came from.
    static Bitmap DamageArc(int seed)
    {
        double[] red = Hex("#B0180E"), dark = Hex("#3A0604");
        Color[,] pixels = new Color[ArcWidth, ArcHeight];
        // The ring has its middle below the picture; the arc spans 35 degrees to either side of straight up.
        double centerX = ArcWidth / 2.0, centerY = 52, radius = 46, span = 35 * Math.PI / 180;
        for (int y = 0; y < ArcHeight; y++)
            for (int x = 0; x < ArcWidth; x++)
            {
                pixels[x, y] = Color.Transparent;
                double dx = x + 0.5 - centerX, dy = y + 0.5 - centerY;
                double angle = Math.Atan2(dx, -dy);
                double along = Math.Abs(angle) / span;
                if (along > 1)
                    continue;

                // Thicker in the middle of the arc, ragged at both edges.
                double thickness = 5.5 * (1 - along * along) + 1;
                double ragged = (Fbm(x * 4, y * 4, 8, 8, 2, seed) - 0.5) * 3;
                double offset = Math.Sqrt(dx * dx + dy * dy) - radius;
                if (Math.Abs(offset) > thickness / 2 + ragged)
                    continue;

                double depth = Math.Abs(offset) / (thickness / 2 + 0.01);
                double[] c = Mix(red, dark, Math.Min(1, depth * 0.8 + along * 0.4 + (Rand(x, y, seed + 5) - 0.5) * 0.3));
                pixels[x, y] = ToColorAlpha(c, 1.0 - along * along * 0.6);
            }
        return Upscale(pixels, ArcWidth, ArcHeight, 4);
    }

    // Writes the HUD effects into imagesDirectory, and a preview of them over a wall of the first episode into
    // previewPath.
    public static void RunHUDEffects(string imagesDirectory, string wallTexturePath, string previewPath)
    {
        Directory.CreateDirectory(imagesDirectory);
        Bitmap red = Vignette("#B01A0C", "#4A0603", 0.95, 301);
        Bitmap green = Vignette("#F0FFD8", "#B4F09A", 0.5, 311);
        Bitmap arc = DamageArc(321);
        red.Save(Path.Combine(imagesDirectory, "VignetteDamage.png"), ImageFormat.Png);
        green.Save(Path.Combine(imagesDirectory, "VignetteHeal.png"), ImageFormat.Png);
        arc.Save(Path.Combine(imagesDirectory, "DamageArc.png"), ImageFormat.Png);
        using (Bitmap deathVignette = DeathVignette())
            deathVignette.Save(Path.Combine(imagesDirectory, "VignetteDeath.png"), ImageFormat.Png);
        using (Bitmap eyelid = Eyelid())
            eyelid.Save(Path.Combine(imagesDirectory, "Eyelid.png"), ImageFormat.Png);
        using (Bitmap titleGlow = TitleGlow())
            titleGlow.Save(Path.Combine(imagesDirectory, "TitleGlow.png"), ImageFormat.Png);

        // The preview: three frames of a wall (tiled, pixelated like in the game): with the red vignette, with the green
        // one, and with the arc turned towards a blow from the left.
        int frameWidth = VignetteWidth * 4, frameHeight = VignetteHeight * 4;
        using (Bitmap wall = new Bitmap(wallTexturePath))
        using (Bitmap sheet = new Bitmap(frameWidth * 3 + 32, frameHeight + 16, PixelFormat.Format32bppArgb))
        using (Graphics g = Graphics.FromImage(sheet))
        {
            g.Clear(Color.FromArgb(255, 20, 20, 20));
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            for (int frame = 0; frame < 3; frame++)
            {
                int left = 8 + frame * (frameWidth + 8);
                g.SetClip(new Rectangle(left, 8, frameWidth, frameHeight));
                for (int ty = 0; ty < frameHeight; ty += wall.Height * 3)
                    for (int tx = 0; tx < frameWidth; tx += wall.Width * 3)
                        g.DrawImage(wall, left + tx, 8 + ty, wall.Width * 3, wall.Height * 3);
                if (frame < 2)
                    g.DrawImage(frame == 0 ? red : green, left, 8, frameWidth, frameHeight);
                else
                {
                    // The arc around the middle of the screen, turned 90 degrees to the left.
                    var state = g.Save();
                    g.TranslateTransform(left + frameWidth / 2, 8 + frameHeight / 2);
                    g.RotateTransform(-90);
                    g.DrawImage(arc, -arc.Width / 2, -frameHeight / 3 - arc.Height / 2, arc.Width, arc.Height);
                    g.Restore(state);
                }
                g.ResetClip();
            }
            Directory.CreateDirectory(Path.GetDirectoryName(previewPath));
            sheet.Save(previewPath, ImageFormat.Png);
        }
    }


    // ---------- death of the player ----------
    // The eyes of a dying player grow heavy and close (see UI::DeathScreen): a soft black vignette, darkest in the corners,
    // and an eyelid, black above, its lower edge a soft curve that reaches further down in the middle. Smooth, not
    // pixelated: they are not drawn things but the dark of closing eyes. The lower eyelid is the same picture turned.
    const int DeathVignetteWidth = 256, DeathVignetteHeight = 144;
    const int EyelidWidth = 256, EyelidHeight = 128;

    static Bitmap DeathVignette()
    {
        Bitmap bmp = new Bitmap(DeathVignetteWidth, DeathVignetteHeight, PixelFormat.Format32bppArgb);
        for (int y = 0; y < DeathVignetteHeight; y++)
            for (int x = 0; x < DeathVignetteWidth; x++)
            {
                // The distance from the middle in an ellipse as wide as the picture: 1 at the middle of every edge,
                // about 1.41 in the corners. Clear inside 0.55, fully dark from 1.25 on.
                double dx = (x + 0.5) / DeathVignetteWidth * 2 - 1, dy = (y + 0.5) / DeathVignetteHeight * 2 - 1;
                double distance = Math.Sqrt(dx * dx + dy * dy);
                double t = Math.Max(0, Math.Min(1, (distance - 0.55) / 0.7));
                double alpha = t * t * (3 - 2 * t);
                bmp.SetPixel(x, y, Color.FromArgb((int)Math.Round(alpha * 255), 0, 0, 0));
            }
        return bmp;
    }

    // The glow behind GAME OVER: a soft ellipse of dark blood red, brightest in the middle and fading to nothing at the
    // edges, as if the iron letters were lit from behind by embers.
    const int TitleGlowWidth = 256, TitleGlowHeight = 96;

    static Bitmap TitleGlow()
    {
        Bitmap bmp = new Bitmap(TitleGlowWidth, TitleGlowHeight, PixelFormat.Format32bppArgb);
        for (int y = 0; y < TitleGlowHeight; y++)
            for (int x = 0; x < TitleGlowWidth; x++)
            {
                double dx = (x + 0.5) / TitleGlowWidth * 2 - 1, dy = (y + 0.5) / TitleGlowHeight * 2 - 1;
                double t = Math.Max(0, 1 - Math.Sqrt(dx * dx + dy * dy));
                double alpha = t * t * (3 - 2 * t);
                bmp.SetPixel(x, y, Color.FromArgb((int)Math.Round(alpha * 255), 150, 18, 8));
            }
        return bmp;
    }

    static Bitmap Eyelid()
    {
        Bitmap bmp = new Bitmap(EyelidWidth, EyelidHeight, PixelFormat.Format32bppArgb);
        for (int y = 0; y < EyelidHeight; y++)
            for (int x = 0; x < EyelidWidth; x++)
            {
                // The edge of the lid: at 70% of the height at the sides, 95% in the middle (a curve like an eye), and
                // the dark fading out over the last 25% above it.
                double across = (x + 0.5) / EyelidWidth * 2 - 1;
                double edge = 0.95 - 0.25 * across * across;
                double v = (y + 0.5) / EyelidHeight;
                double t = Math.Max(0, Math.Min(1, (edge - v) / 0.25));
                double alpha = t * t * (3 - 2 * t);
                bmp.SetPixel(x, y, Color.FromArgb((int)Math.Round(alpha * 255), 0, 0, 0));
            }
        return bmp;
    }
    // ---------- shotgun shell ----------
    // The texture of an ejected shotgun shell, wrapped around a short cylinder built by code (see
    // Gameplay::CreateShellMesh): the horizontal axis goes around it, the vertical one along it, from its bottom (row 0)
    // to its top. A tarnished brass base, a worn red plastic tube with a darker crimp at the top. 16 x 16, pixelated like
    // the textures of the world (it is drawn in the world, with their filtering).
    const int ShellTextureSize = 16;

    static Bitmap ShellTexture(int seed)
    {
        double[] brassLight = Hex("#C8A04A"), brassDark = Hex("#5A4016");
        double[] redLight = Hex("#A8281C"), redDark = Hex("#4A0E08");
        double[] crimp = Hex("#2E0806"), rim = Hex("#3A2A10");
        Bitmap bmp = new Bitmap(ShellTextureSize, ShellTextureSize, PixelFormat.Format32bppArgb);
        for (int y = 0; y < ShellTextureSize; y++)
            for (int x = 0; x < ShellTextureSize; x++)
            {
                // Rows from the bottom of the shell: the image is stored top row first, so row 0 is the last line.
                int row = ShellTextureSize - 1 - y;

                // Light falls from one side: the columns around the middle of the texture are the lit side.
                double around = Math.Cos((x + 0.5) / ShellTextureSize * 2 * Math.PI);
                double rough = Fbm(x * 4, row * 4, 8, 8, 2, seed);
                double shade = 0.45 + around * 0.3 + (rough - 0.5) * 0.5;

                double[] c;
                if (row == 0)
                    c = rim;
                else if (row < 4)
                    c = Mix(brassDark, brassLight, Math.Max(0, Math.Min(1, shade + 0.1)));
                else if (row == 4)
                    c = Mix(rim, brassDark, 0.5);
                else if (row >= ShellTextureSize - 2)
                    c = Mix(crimp, redDark, Math.Max(0, Math.Min(1, shade)) * 0.6);
                else
                    c = Mix(redDark, redLight, Math.Max(0, Math.Min(1, shade)));

                // Scratches and grime.
                if (Rand(x, row, seed + 3) < 0.08)
                    c = Mix(c, Hex("#1A1712"), 0.5);
                else if (Rand(x, row, seed + 5) < 0.05)
                    c = Mix(c, Hex("#E0C080"), 0.3);
                bmp.SetPixel(x, y, ToColor(c));
            }
        return bmp;
    }

    // Writes the texture of the shell into weaponsDirectory, and a preview of it into previewPath: the texture enlarged,
    // and a shell drawn from the side with it.
    public static void RunShell(string weaponsDirectory, string previewPath)
    {
        Directory.CreateDirectory(weaponsDirectory);
        Bitmap texture = ShellTexture(401);
        texture.Save(Path.Combine(weaponsDirectory, "ShotgunShell.png"), ImageFormat.Png);

        using (Bitmap sheet = new Bitmap(16 * 12 + 16 * 6 + 48, 16 * 12 + 16, PixelFormat.Format32bppArgb))
        using (Graphics g = Graphics.FromImage(sheet))
        {
            g.Clear(ToColor(Silt));
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            g.DrawImage(texture, 8, 8, 16 * 12, 16 * 12);

            // The shell from the side: the front half of the texture (columns 4 to 11) is what faces the viewer.
            g.DrawImage(texture, new Rectangle(16 * 12 + 32, 8, 8 * 6, 16 * 12), new Rectangle(4, 0, 8, 16), GraphicsUnit.Pixel);
            Directory.CreateDirectory(Path.GetDirectoryName(previewPath));
            sheet.Save(previewPath, ImageFormat.Png);
        }
    }

    // ---------- the opening behind the shotgun bolt ----------
    // The bolt of the shotgun model slides back with the pump (see Renderer::ModelPartSplit) and uncovers this: a 16 x 16
    // texture of the dark inside of the receiver, almost black, with the dim brass glint of the chamber at its bottom.
    static Bitmap OpeningTexture(int seed)
    {
        double[] black = Hex("#0A0908"), soot = Hex("#1E1C18"), brass = Hex("#5A4016");
        Bitmap bmp = new Bitmap(16, 16, PixelFormat.Format32bppArgb);
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++)
            {
                double rough = Fbm(x * 4, y * 4, 8, 8, 2, seed);
                double[] c = Mix(black, soot, rough);

                // The chamber: a dim glint along the bottom rows (the image is stored top row first).
                if (y >= 12)
                    c = Mix(c, brass, 0.25 + (y - 12) * 0.08 + (rough - 0.5) * 0.2);
                bmp.SetPixel(x, y, ToColor(c));
            }
        return bmp;
    }

    // Writes the texture of the opening into weaponsDirectory, and a preview of it, enlarged, into previewPath.
    public static void RunBoltOpening(string weaponsDirectory, string previewPath)
    {
        Directory.CreateDirectory(weaponsDirectory);
        Bitmap opening = OpeningTexture(509);
        opening.Save(Path.Combine(weaponsDirectory, "ShotgunOpening.png"), ImageFormat.Png);

        using (Bitmap sheet = new Bitmap(16 * 10 + 16, 16 * 10 + 16, PixelFormat.Format32bppArgb))
        using (Graphics g = Graphics.FromImage(sheet))
        {
            g.Clear(ToColor(Silt));
            g.InterpolationMode = InterpolationMode.NearestNeighbor;
            g.PixelOffsetMode = PixelOffsetMode.Half;
            g.DrawImage(opening, 8, 8, 16 * 10, 16 * 10);
            Directory.CreateDirectory(Path.GetDirectoryName(previewPath));
            sheet.Save(previewPath, ImageFormat.Png);
        }
    }

    // ---------- fire ----------
    // The flame of torches, braziers, candles and lanterns: a flipbook, 16 frames of 32 x 32 in a 4 x 4 sheet (left to
    // right, top to bottom), played in a loop by the game. Drawn with additive blending: black adds nothing.
    const int FireFrameSize = 32;
    const int FireColumns = 4;
    const int FireFrameCount = 16;

    // One frame of the flame. The shape is a tongue, wide at the bottom and thin at the top; noise rising through it tears
    // its edges and makes tongues break off. The noise is tileable over 64 units of height and rises 4 units per frame:
    // after 16 frames it is back where it started, so the loop has no jump.
    static Bitmap FireFrame(int frame, int seed)
    {
        int size = FireFrameSize;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        double[] white = Hex("#FFF4D0"), yellow = Hex("#FFC83A"), orange = Hex("#F07A1E"), red = Hex("#A8300C");
        double rise = frame * 64.0 / FireFrameCount;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                double u = (x + 0.5) / size - 0.5;   // -0.5 left .. 0.5 right
                double v = 1.0 - (y + 0.5) / size;   // 0 bottom .. 1 top

                // The noise moves down the image while the flame stays: the fire seems to rise. Its sway grows with
                // the height, like a flame that is calm at the wick and flickers at the tip.
                double turbulence = Fbm(x * 2, y * 2 + rise, 8, 4, 3, seed);
                double sway = (Fbm(x * 2 + 32, y * 2 + rise, 2, 2, 2, seed + 7) - 0.5) * 0.4 * v;
                double width = 0.28 * Math.Pow(Math.Max(0.0, 1.0 - v), 0.8) + 0.02;
                double density = 1.0 - Math.Abs(u - sway) / width - v * 0.8 + (turbulence - 0.5) * 1.4;

                // A round bottom (the flame sits on the wick or the coals) and a tip that fades out before the top edge.
                density -= Math.Max(0.0, 0.15 - v) * 6.0 + Math.Max(0.0, v - 0.75) * 3.0;
                if (density <= 0.0) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }

                // Hotter low in the middle: white, yellow, orange, and dark red at the torn edges.
                double heat = Math.Min(1.0, density * (1.15 - v * 0.6));
                double[] c = heat > 0.75 ? Mix(yellow, white, (heat - 0.75) / 0.25)
                           : heat > 0.4 ? Mix(orange, yellow, (heat - 0.4) / 0.35)
                           : Mix(red, orange, heat / 0.4);
                bmp.SetPixel(x, y, ToColorAlpha(c, Math.Min(1.0, density * 2.5)));
            }
        return bmp;
    }

    // One frame of the flame of a candle (and of the wick of a lantern): calm, a smooth drop of light that only sways a
    // little at its tip and breathes in height. Both movements are whole waves over the 16 frames, so the loop has no jump.
    static Bitmap CandleFrame(int frame)
    {
        int size = FireFrameSize;
        Bitmap bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        double[] white = Hex("#FFF8E0"), yellow = Hex("#FFD660"), orange = Hex("#F08C2A");
        double phase = 2.0 * Math.PI * frame / FireFrameCount;
        double lean = 0.05 * Math.Sin(phase);
        double height = 0.6 + 0.05 * Math.Sin(2.0 * phase + 1.0);
        const double bottom = 0.12;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
            {
                double u = (x + 0.5) / size - 0.5;
                double v = 1.0 - (y + 0.5) / size;

                // s goes from 0 at the bottom of the drop to 1 at its tip; the drop is widest a third of the way up.
                double s = (v - bottom) / height;
                if (s <= 0.0 || s >= 1.0) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }
                double width = 0.13 * Math.Sin(Math.PI * Math.Pow(s, 0.6));
                double density = 1.0 - Math.Abs(u - lean * s * s) / Math.Max(width, 1e-3);
                if (density <= 0.0) { bmp.SetPixel(x, y, Color.FromArgb(0, 0, 0, 0)); continue; }

                // A white core low in the drop, yellow around it, an orange rim.
                double heat = density * (1.0 - s * 0.5);
                double[] c = heat > 0.55 ? Mix(yellow, white, (heat - 0.55) / 0.3) : Mix(orange, yellow, heat / 0.55);
                bmp.SetPixel(x, y, ToColorAlpha(c, Math.Min(1.0, density * 3.0)));
            }
        return bmp;
    }

    // Saves the 16 frames as one sheet at sheetPath, and the sheet enlarged 4 times on a dark background at previewPath.
    static void SaveFlipbook(Func<int, Bitmap> drawFrame, string sheetPath, string previewPath)
    {
        int size = FireFrameSize, rows = FireFrameCount / FireColumns;
        using (Bitmap sheet = new Bitmap(size * FireColumns, size * rows, PixelFormat.Format32bppArgb))
        {
            using (Graphics g = Graphics.FromImage(sheet))
            {
                g.Clear(Color.FromArgb(0, 0, 0, 0));
                g.CompositingMode = CompositingMode.SourceCopy;
                for (int frame = 0; frame < FireFrameCount; frame++)
                    using (Bitmap image = drawFrame(frame))
                        g.DrawImage(image, (frame % FireColumns) * size, (frame / FireColumns) * size, size, size);
            }
            sheet.Save(sheetPath, ImageFormat.Png);

            const int scale = 4;
            using (Bitmap preview = new Bitmap(sheet.Width * scale, sheet.Height * scale, PixelFormat.Format32bppArgb))
            using (Graphics g = Graphics.FromImage(preview))
            {
                g.Clear(Color.FromArgb(255, 18, 16, 14));
                g.InterpolationMode = InterpolationMode.NearestNeighbor;
                g.PixelOffsetMode = PixelOffsetMode.Half;
                g.DrawImage(sheet, 0, 0, preview.Width, preview.Height);
                preview.Save(previewPath, ImageFormat.Png);
            }
        }
    }

    // Writes Fire.png (torches, braziers) and CandleFire.png (candles, lanterns) into effectsDirectory, and their sheets
    // enlarged into previewDirectory, so the frames can be looked at one by one.
    public static void RunFire(string effectsDirectory, string previewDirectory)
    {
        Directory.CreateDirectory(effectsDirectory);
        Directory.CreateDirectory(previewDirectory);
        SaveFlipbook(frame => FireFrame(frame, 601), Path.Combine(effectsDirectory, "Fire.png"),
                     Path.Combine(previewDirectory, "FireSheet.png"));
        SaveFlipbook(CandleFrame, Path.Combine(effectsDirectory, "CandleFire.png"),
                     Path.Combine(previewDirectory, "CandleFireSheet.png"));
    }
}
