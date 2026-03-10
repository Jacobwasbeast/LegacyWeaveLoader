using System;

namespace WeaveLoader.API.Mixins;

[AttributeUsage(AttributeTargets.Method, Inherited = false)]
public sealed class InjectAttribute : Attribute
{
    public string Method { get; }
    public At At { get; }
    public int Require { get; set; } = 0;
    public bool Cancellable { get; set; } = false;

    public InjectAttribute(string method, At at = At.Head)
    {
        Method = method;
        At = at;
    }
}
