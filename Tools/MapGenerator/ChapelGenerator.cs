// Writes the first version of Assets/Maps/Chapel.map: the nave of the flooded chapel of episode 1, built from the plan
// (Build/Incoming/ChapelPlan.svg): a hall 768 x 1024 x 320 units with walls in bands of texture, four octagonal pillars,
// a raised altar with steps, a balcony with stairs, a closed arch down to the crypt, windows, doors, clip ramps over
// the steps, the player start and two dogs. It is run once: afterwards the map is edited in TrenchBroom, and running
// it again keeps the entities placed in TrenchBroom but makes the brushes of the world again (losing edits to them).
//
// Map units, Z up, like TrenchBroom: X east, Y north. (0, 0, 0) is the south-west inner corner of the hall, on its floor.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

public static class ChapelGenerator
{
    // A plane of a brush: its outward normal (integers, not normalized) and a point on it, with the texture of its face.
    struct Face
    {
        public int[] Normal;
        public int[] Point;
        public string Texture;
        public double Scale;
        public double OffsetU, OffsetV;
    }

    static readonly List<List<Face>> brushes = new List<List<Face>>();
    static readonly StringBuilder entities = new StringBuilder();

    // ---------- textures ----------
    const string Blocks = "Episode1/Wall_MossyBlocks";
    const string Niches = "Episode1/Wall_GothicNiches";
    const string Plaster = "Episode1/Wall_CrackedPlaster";
    const string Planks = "Episode1/Wall_RottenPlanks";
    const string Arches = "Episode1/Trim_Arches";
    const string IronBand = "Episode1/Trim_IronBand";
    const string Flagstone = "Episode1/Floor_MossyFlagstone";
    const string WetFlagstone = "Episode1/Floor_WetFlagstone";
    const string FloorPlanks = "Episode1/Floor_RottenPlanks";
    const string Door = "Episode1/Door_Chapel";
    const string Window = "Episode1/Window_Chapel";
    const string Clip = "Common/Clip";

    // ---------- brushes ----------
    // A box from (x0, y0, z0) to (x1, y1, z1) with one texture on every face.
    static List<Face> Box(int x0, int y0, int z0, int x1, int y1, int z1, string texture, double scale = 1)
    {
        var faces = new List<Face>
        {
            MakeFace(-1, 0, 0, x0, y0, z0, texture, scale), MakeFace(1, 0, 0, x1, y1, z1, texture, scale),
            MakeFace(0, -1, 0, x0, y0, z0, texture, scale), MakeFace(0, 1, 0, x1, y1, z1, texture, scale),
            MakeFace(0, 0, -1, x0, y0, z0, texture, scale), MakeFace(0, 0, 1, x1, y1, z1, texture, scale),
        };
        brushes.Add(faces);
        return faces;
    }

    static Face MakeFace(int nx, int ny, int nz, int px, int py, int pz, string texture, double scale = 1)
    {
        return new Face { Normal = new[] { nx, ny, nz }, Point = new[] { px, py, pz }, Texture = texture, Scale = scale };
    }

    // Changes the texture of the face of a box that faces the direction (one of the six axes), with its scale and
    // offsets.
    static void Retexture(List<Face> faces, int nx, int ny, int nz, string texture, double scale = 1, double offsetU = 0,
                          double offsetV = 0)
    {
        for (int i = 0; i < faces.Count; i++)
        {
            Face face = faces[i];
            if (face.Normal[0] == nx && face.Normal[1] == ny && face.Normal[2] == nz)
            {
                face.Texture = texture;
                face.Scale = scale;
                face.OffsetU = offsetU;
                face.OffsetV = offsetV;
                faces[i] = face;
            }
        }
    }

    // A wall from (x0, y0) to (x1, y1) and from z0 to z1, in the bands of the walls of the hall: an iron band at the
    // bottom (0-32), mossy blocks (32-176), a frieze of arches (176-208), cracked plaster (208-320). Only the bands
    // between z0 and z1 are made.
    static void BandedWall(int x0, int y0, int x1, int y1, int z0, int z1)
    {
        var bands = new[]
        {
            new { Bottom = 0, Top = 32, Texture = IronBand, Scale = 0.5 },
            new { Bottom = 32, Top = 176, Texture = Blocks, Scale = 1.0 },
            new { Bottom = 176, Top = 208, Texture = Arches, Scale = 0.5 },
            new { Bottom = 208, Top = 320, Texture = Plaster, Scale = 1.0 },
        };
        foreach (var band in bands)
        {
            int bottom = Math.Max(band.Bottom, z0), top = Math.Min(band.Top, z1);
            if (bottom >= top)
                continue;
            // The texture is stretched over the band: a band texture (64 texels high, at scale 0.5 32 units) starts at
            // the top of its band. A texel row v is at height (offset - v) * scale, so offset = top / scale.
            double offsetV = band.Top / band.Scale;
            List<Face> wall = Box(x0, y0, bottom, x1, y1, top, band.Texture, band.Scale);
            for (int i = 0; i < wall.Count; i++)
            {
                Face face = wall[i];
                face.OffsetV = offsetV;
                wall[i] = face;
            }
        }
    }

    // An octagonal prism around (cx, cy) from z0 to z1: halfWidth to each side, its corners cut at 45 degrees where
    // |x| + |y| > halfWidth + halfWidth / 2.
    static void Octagon(int cx, int cy, int halfWidth, int z0, int z1, string texture)
    {
        int diagonal = halfWidth + halfWidth / 2;
        var faces = new List<Face>
        {
            MakeFace(1, 0, 0, cx + halfWidth, cy, z0, texture), MakeFace(-1, 0, 0, cx - halfWidth, cy, z0, texture),
            MakeFace(0, 1, 0, cx, cy + halfWidth, z0, texture), MakeFace(0, -1, 0, cx, cy - halfWidth, z0, texture),
            MakeFace(1, 1, 0, cx + diagonal, cy, z0, texture), MakeFace(1, -1, 0, cx + diagonal, cy, z0, texture),
            MakeFace(-1, 1, 0, cx - diagonal, cy, z0, texture), MakeFace(-1, -1, 0, cx - diagonal, cy, z0, texture),
            MakeFace(0, 0, -1, cx, cy, z0, texture), MakeFace(0, 0, 1, cx, cy, z1, texture),
        };
        brushes.Add(faces);
    }

    // A clip ramp from x0 to x1: its foot on the floor at z = 0 where y = footY, rising one unit per two units north
    // (like stairs of 16-unit steps 32 deep) up to y = topY.
    static void ClipRampNorth(int x0, int x1, int footY, int topY)
    {
        brushes.Add(new List<Face>
        {
            MakeFace(-1, 0, 0, x0, footY, 0, Clip), MakeFace(1, 0, 0, x1, footY, 0, Clip),
            MakeFace(0, 1, 0, x0, topY, 0, Clip), MakeFace(0, 0, -1, x0, footY, 0, Clip),
            MakeFace(0, -1, 2, x0, footY, 0, Clip),
        });
    }

    // ---------- entities ----------
    static void PointEntity(string className, int x, int y, int z, int angle)
    {
        entities.AppendFormat(CultureInfo.InvariantCulture,
                              "{{\n\"classname\" \"{0}\"\n\"origin\" \"{1} {2} {3}\"\n\"angle\" \"{4}\"\n}}\n", className, x, y,
                              z, angle);
    }

    // ---------- writing ----------
    // Three points of the plane, in the order the map format wants: seen from outside the brush, the normal is
    // cross(p3 - p1, p2 - p1). With a vector a along the plane, p2 = p1 + cross(n, a) and p3 = p1 + a give exactly
    // that, because cross(a, cross(n, a)) = n |a|^2 for a perpendicular to n.
    static string FormatFace(Face face)
    {
        int[] n = face.Normal;
        int[] a = n[2] == 0 ? new[] { 0, 0, 1 } : new[] { 1, 0, 0 };
        if (n[2] != 0 && (n[0] != 0))
            a = new[] { 0, 1, 0 };
        int[] u = { n[1] * a[2] - n[2] * a[1], n[2] * a[0] - n[0] * a[2], n[0] * a[1] - n[1] * a[0] };
        int[] p1 = face.Point;
        int[] p2 = { p1[0] + u[0], p1[1] + u[1], p1[2] + u[2] };
        int[] p3 = { p1[0] + a[0], p1[1] + a[1], p1[2] + a[2] };

        // The texture axes as TrenchBroom lays them on a face by the axis its normal is closest to: on floors and
        // ceilings U east and V south, on walls U along the wall and V down.
        int ax = Math.Abs(n[0]), ay = Math.Abs(n[1]), az = Math.Abs(n[2]);
        string axes;
        if (az >= ax && az >= ay)
            axes = string.Format(CultureInfo.InvariantCulture, "[ 1 0 0 {0} ] [ 0 -1 0 {1} ]", face.OffsetU, face.OffsetV);
        else if (ax >= ay)
            axes = string.Format(CultureInfo.InvariantCulture, "[ 0 1 0 {0} ] [ 0 0 -1 {1} ]", face.OffsetU, face.OffsetV);
        else
            axes = string.Format(CultureInfo.InvariantCulture, "[ 1 0 0 {0} ] [ 0 0 -1 {1} ]", face.OffsetU, face.OffsetV);

        return string.Format(CultureInfo.InvariantCulture,
                             "( {0} {1} {2} ) ( {3} {4} {5} ) ( {6} {7} {8} ) {9} {10} 0 {11} {11}",
                             p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], p3[0], p3[1], p3[2], face.Texture, axes, face.Scale);
    }

    public static void Run(string mapPath)
    {
        brushes.Clear();
        entities.Clear();

        // The floor and the ceiling.
        Box(-32, -32, -16, 800, 1056, 0, Flagstone);
        Box(-32, -32, 320, 800, 1056, 336, Planks);

        // The walls, 32 thick, built in pieces around their openings. The doors and windows stand in niches cut into the
        // wall, not in front of it: a brush in the plane of the wall would flicker against it (two faces in one place),
        // one inside the solid wall would be hidden. So the wall is made of the pieces left, right, above and below an
        // opening, and of a thinner piece behind it (the back of the niche, which shows through the transparent corners
        // of the arched door or window).
        //
        // The south wall: the door in a niche 16 deep (x 352-416, up to 128).
        BandedWall(0, -32, 352, 0, 0, 320);
        BandedWall(416, -32, 768, 0, 0, 320);
        BandedWall(352, -32, 416, 0, 128, 320);
        BandedWall(352, -32, 416, -16, 0, 128);
        // The north wall: the window above the altar in a niche 8 deep (x 336-432, z 112-304).
        BandedWall(0, 1024, 336, 1056, 0, 320);
        BandedWall(432, 1024, 768, 1056, 0, 320);
        BandedWall(336, 1024, 432, 1056, 0, 112);
        BandedWall(336, 1024, 432, 1056, 304, 320);
        BandedWall(336, 1032, 432, 1056, 112, 304);
        // The west wall, whole.
        BandedWall(-32, -32, 0, 1056, 0, 320);
        // The east wall: two windows in niches 8 deep (y 240-336 and 464-560, z 112-304), and the arch down to the crypt
        // (y 640-768, up to 160).
        BandedWall(768, -32, 800, 240, 0, 320);
        BandedWall(768, 336, 800, 464, 0, 320);
        BandedWall(768, 560, 800, 640, 0, 320);
        foreach (int y in new[] { 240, 464 })
        {
            BandedWall(768, y, 800, y + 96, 0, 112);
            BandedWall(768, y, 800, y + 96, 304, 320);
            BandedWall(776, y, 800, y + 96, 112, 304);
        }
        BandedWall(768, 768, 800, 1056, 0, 320);
        BandedWall(768, 640, 800, 768, 160, 320);

        // Four octagonal pillars: a shaft 64 across, a base and a capital 80 across, 24 high.
        foreach (int[] center in new[] { new[] { 224, 320 }, new[] { 544, 320 }, new[] { 224, 640 }, new[] { 544, 640 } })
        {
            Octagon(center[0], center[1], 40, 0, 24, WetFlagstone);
            Octagon(center[0], center[1], 32, 24, 296, Blocks);
            Octagon(center[0], center[1], 40, 296, 320, WetFlagstone);
        }

        // The raised altar: a platform 48 high along the north wall, three steps of 16 in front of it, the altar on top,
        // and a clip ramp over the steps through their front edges.
        Box(192, 832, 0, 576, 1024, 48, WetFlagstone);
        Box(192, 736, 0, 576, 832, 16, WetFlagstone);
        Box(192, 768, 16, 576, 832, 32, WetFlagstone);
        Box(192, 800, 32, 576, 832, 48, WetFlagstone);
        Box(320, 928, 48, 448, 976, 96, Niches);
        ClipRampNorth(192, 576, 736, 832);

        // The balcony along the west wall: a floor of planks at 112-128 on three posts, a low wall of planks along its
        // open sides, and stairs of eight steps up to it from the south with a clip ramp over them.
        List<Face> balcony = Box(0, 384, 112, 160, 896, 128, Planks);
        Retexture(balcony, 0, 0, 1, FloorPlanks);
        foreach (int y in new[] { 384, 632, 880 })
            Box(144, y, 0, 160, y + 16, 112, Planks);
        Box(152, 392, 128, 160, 896, 160, Planks);
        Box(0, 888, 128, 152, 896, 160, Planks);
        Box(96, 384, 128, 152, 392, 160, Planks);
        for (int step = 1; step <= 8; step++)
        {
            List<Face> stair = Box(0, 128 + (step - 1) * 32, 0, 96, 128 + step * 32, 16 * step, Planks);
            Retexture(stair, 0, 0, 1, FloorPlanks);
        }
        ClipRampNorth(0, 96, 96, 352);

        // The arch down to the crypt, behind the east wall: four steps down of 16, a landing, and a closed door at its
        // end; its own walls, floor and ceiling.
        for (int step = 1; step <= 4; step++)
            Box(800 + (step - 1) * 32, 640, -80, 800 + step * 32, 768, -16 * step, WetFlagstone);
        Box(928, 640, -80, 960, 768, -64, WetFlagstone);
        Box(800, 608, -80, 992, 640, 176, Blocks);
        Box(800, 768, -80, 992, 800, 176, Blocks);
        // Its end wall, with the door in a niche 16 deep (y 672-736, z -64 to 64).
        Box(960, 640, -80, 992, 672, 176, Blocks);
        Box(960, 736, -80, 992, 768, 176, Blocks);
        Box(960, 672, 64, 992, 736, 176, Blocks);
        Box(960, 672, -80, 992, 736, -64, Blocks);
        Box(976, 672, -64, 992, 736, 64, Blocks);
        Box(800, 640, 160, 992, 768, 176, Blocks);

        // The doors and windows: cutouts (64 x 128 texels, transparent around the arch: the back of the niche shows
        // through) on thin brushes standing at the back of their niches: the doors 64 x 128 units (scale 1), the windows
        // 96 x 192 (scale 1.5). The offsets put the top corner of the texture at the top corner of the brush: a texel
        // column u is at (u - offset) * scale along the face, a row v at (offset - v) * scale in height.
        List<Face> cryptDoor = Box(974, 672, -64, 976, 736, 64, Blocks);
        Retexture(cryptDoor, -1, 0, 0, Door, 1, -672, 64);
        List<Face> southDoor = Box(352, -16, 0, 416, -14, 128, Blocks);
        Retexture(southDoor, 0, 1, 0, Door, 1, -352, 128);
        List<Face> northWindow = Box(336, 1030, 112, 432, 1032, 304, Blocks);
        Retexture(northWindow, 0, -1, 0, Window, 1.5, -336 / 1.5, 304 / 1.5);
        foreach (int y in new[] { 240, 464 })
        {
            List<Face> eastWindow = Box(774, y, 112, 776, y + 96, 304, Blocks);
            Retexture(eastWindow, -1, 0, 0, Window, 1.5, -y / 1.5, 304 / 1.5);
        }

        // The player at the south door looking north; a dog in front of the altar, another on the balcony.
        PointEntity("info_player_start", 384, 96, 24, 90);
        PointEntity("monster_dog", 384, 680, 0, 270);
        PointEntity("monster_dog", 80, 640, 128, 0);

        var map = new StringBuilder();
        map.Append("// Game: Abomination\n// Format: Valve\n");
        map.Append("// Generator: Tools/MapGenerator (the first version of the chapel)\n");
        map.Append("// entity 0\n{\n\"mapversion\" \"220\"\n\"classname\" \"worldspawn\"\n");
        for (int index = 0; index < brushes.Count; index++)
        {
            map.AppendFormat(CultureInfo.InvariantCulture, "// brush {0}\n{{\n", index);
            foreach (Face face in brushes[index])
                map.Append(FormatFace(face)).Append('\n');
            map.Append("}\n");
        }
        map.Append("}\n");
        // The entities of a map already edited in TrenchBroom are kept (the player start and the dogs placed there):
        // TrenchBroom writes them after the world, from "// entity 1" on. Only the brushes of the world are made again.
        string existing = File.Exists(mapPath) ? File.ReadAllText(mapPath).Replace("\r\n", "\n") : "";
        int firstEntity = existing.IndexOf("// entity 1\n", StringComparison.Ordinal);
        map.Append(firstEntity >= 0 ? existing.Substring(firstEntity) : entities.ToString());
        File.WriteAllText(mapPath, map.ToString().Replace("\n", "\r\n"), new UTF8Encoding(false));
        Console.WriteLine("Chapel written: {0} brushes, to {1}", brushes.Count, mapPath);
    }
}
