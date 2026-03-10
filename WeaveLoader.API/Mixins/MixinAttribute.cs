using System;

namespace WeaveLoader.API.Mixins;

[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public sealed class MixinAttribute : Attribute
{
    public string Target { get; }

    public MixinAttribute(string target)
    {
        Target = target;
    }
}
