using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
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
                new HeaderStruct("Reflector/Test1.h", new HashSet<string> { "AAA" }),
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
            var processedStructs = new HashSet<string>();

            foreach (var headerStruct in structDefinitions) {
                foreach (var targetStructName in headerStruct.StructNames) {
                    ProcessStruct(targetStructName, headerStruct.HeaderFile, compilation, structs, includes, processedStructs);
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

        // Recursive method to process structs
        static void ProcessStruct(string structName, string headerFile, CppCompilation compilation, List<StructInfo> structs, HashSet<string> includes, HashSet<string> processedStructs)
        {
            if (processedStructs.Contains(structName))
                return;

            processedStructs.Add(structName);

            CppClass? cppClass = compilation.Classes.FirstOrDefault(c => c.Name == structName);

            // If not found, attempt to resolve typedef
            if (cppClass == null) {
                var resolved = compilation.Typedefs.FirstOrDefault(t => t.Name == structName)?.ElementType;
                if (resolved is CppClass resolvedCpp) {
                    cppClass = resolvedCpp;
                }
                else {
                    Console.WriteLine($"Struct {structName} not found in {headerFile}");
                    return;
                }
            }

            var structInfo = new StructInfo {
                Name = structName,
                Members = new List<MemberInfo>()
            };

            foreach (var field in cppClass.Fields) {
                var memberInfo = new MemberInfo {
                    Name = field.Name,
                    Type = field.Type.GetDisplayName(),
                    DumpExpression = GetDumpExpression(field.Type, $"s.{field.Name}")
                };
                structInfo.Members.Add(memberInfo);

                // Check if the field's type is a struct, and process it recursively
                var fieldType = UnwrapType(field.Type);
                if (fieldType is CppClass fieldCppClass) {
                    ProcessStruct(field.Type.GetDisplayName(), headerFile, compilation, structs, includes, processedStructs);
                }
            }

            structs.Add(structInfo);
            includes.Add(headerFile);
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
                if (UnwrapType(arrayType.ElementType).FullName == "char") {
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
