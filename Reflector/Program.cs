using System;
using System.Collections.Generic;
using System.IO;
using CppAst;
using Scriban;
using Scriban.Runtime;

namespace StructDumperGenerator
{
    class Program
    {
        static void Main(string[] args)
        {
            // List of structs with their header files
            var structDefinitions = new List<HeaderStruct>
            {
                new HeaderStruct("Reflector/Test1.h", new HashSet<string> { "AAA", "B" }),
                new HeaderStruct("IntelPresentMon/ControlLib/igcl_api.h", new HashSet<string> { "ctl_init_args_t", "ctl_device_adapter_properties_t" })
            };

            var compilation = CppParser.ParseFiles(structDefinitions.ConvertAll(hs => hs.HeaderFile));

            if (compilation.HasErrors) {
                foreach (var message in compilation.Diagnostics.Messages) {
                    Console.WriteLine(message);
                }
                return;
            }

            // Collect structs and their members
            var structs = new List<StructInfo>();
            var includes = new HashSet<string>();

            foreach (var headerStruct in structDefinitions) {
                foreach (var targetStructName in headerStruct.StructNames) {
                    CppClass? cppClass = null;

                    // search for struct in classes
                    cppClass = compilation.Classes.Where(c => c.Name == targetStructName).FirstOrDefault();

                    // if not in classes, attempt to resolve typedef
                    if (cppClass == null) {
                        var resolved = compilation.Typedefs.Where(t => t.Name == targetStructName).FirstOrDefault()?.ElementType;
                        if (resolved is CppClass resolvedCpp) {
                            cppClass = resolvedCpp;
                        }
                        else {
                            Console.WriteLine($"Struct {targetStructName} not found in {headerStruct.HeaderFile}");
                            continue;
                        }
                    }

                    var structInfo = new StructInfo {
                        Name = targetStructName,
                        Members = new List<MemberInfo>()
                    };

                    foreach (var field in cppClass.Fields) {
                        var memberInfo = new MemberInfo {
                            Name = field.Name,
                            Type = field.Type.GetDisplayName(),
                            DumpExpression = GetDumpExpression(field.Type, $"s.{field.Name}")
                        };
                        structInfo.Members.Add(memberInfo);
                    }

                    structs.Add(structInfo);

                    // Collect includes (assuming the struct's header)
                    includes.Add(headerStruct.HeaderFile);
                }
            }

            // Load the Scriban template
            var templateText = File.ReadAllText("Reflector\\StructDumpers.h.scriban");
            var template = Template.Parse(templateText);

            // Prepare the model for the template
            var model = new {
                includes = includes,
                structs = structs
            };

            // Render the template
            var result = template.Render(model, member => member.Name);

            // Output the generated code to a file
            File.WriteAllText("IntelPresentMon\\CommonUtilities\\ref\\gen\\GeneratedReflection.h", result);

            Console.WriteLine("Code generation complete.");
        }

        // Helper method to generate the dump expression for a field
        static string GetDumpExpression(CppType type, string variableAccess)
        {
            // Unwrap the type (remove qualifiers and typedefs)
            var unwrappedType = UnwrapType(type);

            if (unwrappedType is CppPrimitiveType) {
                // For primitive types, output the variable directly
                return variableAccess;
            }
            else if (unwrappedType is CppClass cppClass) {
                // For user-defined types, call DumpStructGenerated recursively
                return $"DumpStructGenerated({variableAccess})";
            }
            else if (unwrappedType is CppArrayType arrayType) {
                if (arrayType.ElementType.FullName == "char") {
                    // char arrays are output directly as string (null-terminated)
                    return variableAccess;
                }
            }
            // Default case
            return "\"{ unsupported }\"";
        }

        static CppType UnwrapType(CppType type)
        {
            while (true) {
                if (type is CppQualifiedType qualifiedType) {
                    type = qualifiedType.ElementType;
                }
                else if (type is CppTypedef typedefType) {
                    type = typedefType.ElementType;
                }
                else {
                    break;
                }
            }
            return type;
        }
    }

    // Class to represent a header file and its target structs
    class HeaderStruct
    {
        public string HeaderFile { get; }
        public HashSet<string> StructNames { get; }

        public HeaderStruct(string headerFile, HashSet<string> structNames)
        {
            HeaderFile = headerFile;
            StructNames = structNames;
        }
    }

    // Classes to hold struct and member information
    class StructInfo
    {
        public string Name { get; set; }
        public List<MemberInfo> Members { get; set; }
    }

    class MemberInfo
    {
        public string Name { get; set; }
        public string Type { get; set; }
        public string DumpExpression { get; set; }
    }
}
