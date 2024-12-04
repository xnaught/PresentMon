// Parse a C++ files
using CppAst;

Console.WriteLine(Directory.GetCurrentDirectory());

var compilation = CppParser.ParseFile("Reflector\\Test1.h");
if (compilation.HasErrors) {
    foreach (var error in compilation.Diagnostics.Messages) {
        Console.WriteLine(error);
    }
}
// Print diagnostic messages
foreach (var message in compilation.Diagnostics.Messages)
    Console.WriteLine(message);

foreach (var e in compilation.Enums) {
    Console.WriteLine(e);
    foreach (var v in e.Items) {
        Console.WriteLine($"  {v}");
    }
}

// Print All functions
foreach (var cppFunction in compilation.Functions)
    Console.WriteLine(cppFunction);

// Print All classes, structs
foreach (var cppClass in compilation.Classes) {
    Console.WriteLine(cppClass);
    foreach (var f in cppClass.Fields) {
        Console.WriteLine($"  {f}");
        if (f.Type is CppArrayType arrayType) {
            Console.WriteLine($"    Array is length [{arrayType.Size}]");
        }
    }
}