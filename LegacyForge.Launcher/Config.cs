using System.Text.Json;

namespace LegacyForge.Launcher;

public class Config
{
    public string? GameExePath { get; set; }

    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true
    };

    public static Config Load(string path)
    {
        if (!File.Exists(path))
            return new Config();

        try
        {
            var json = File.ReadAllText(path);
            return JsonSerializer.Deserialize<Config>(json, JsonOptions) ?? new Config();
        }
        catch
        {
            return new Config();
        }
    }

    public void Save(string path)
    {
        var json = JsonSerializer.Serialize(this, JsonOptions);
        File.WriteAllText(path, json);
    }
}
