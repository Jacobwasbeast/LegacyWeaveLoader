using System;

namespace WeaveLoader.API.Mixins;

[AttributeUsage(AttributeTargets.Method, Inherited = false)]
public sealed class OverwriteAttribute : Attribute
{
}
