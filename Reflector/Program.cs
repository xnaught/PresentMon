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
            // List of header files to parse
            var headerFiles = new List<string>
            {
                "Reflector/Test1.h"
                // Add more headers as needed
            };

            // Map of header file to list of structs to target
            var targetStructs = new Dictionary<string, List<string>>
            {
                { "Reflector/Test1.h", new List<string> { "A", "B" } }
                // Add more structs as needed
            };

            var compilation = CppParser.ParseFiles(headerFiles);

            if (compilation.HasErrors) {
                foreach (var message in compilation.Diagnostics.Messages) {
                    Console.WriteLine(message);
                }
                return;
            }

            // Collect structs and their members
            var structs = new List<StructInfo>();
            var includes = new HashSet<string>();

            foreach (var header in headerFiles) {
                var structsInHeader = targetStructs.ContainsKey(header) ? targetStructs[header] : null;
                if (structsInHeader == null)
                    continue;

                foreach (var cppClass in compilation.Classes) {
                    if (cppClass.ClassKind != CppClassKind.Struct)
                        continue;

                    if (!structsInHeader.Contains(cppClass.Name))
                        continue;

                    var structInfo = new StructInfo {
                        Name = cppClass.Name,
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
                    includes.Add(header);
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
                return $"DumpStructGenerated(typeid({variableAccess}), &{variableAccess})";
            }
            else if (unwrappedType is CppPointerType) {
                // Handle pointers (you might want to customize this)
                return variableAccess;
            }
            else if (unwrappedType is CppArrayType arrayType) {
                // Handle arrays
                // For simplicity, we can output the array elements or indicate it's an array
                return $"/* Array type */ {variableAccess}";
            }
            else {
                // Default case
                return $"/* Unsupported type */ {variableAccess}";
            }
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
