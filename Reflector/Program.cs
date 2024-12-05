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
            // List of structs and enums with their header files
            var structDefinitions = new List<HeaderStruct>
            {
                new HeaderStruct("Reflector/Test1.h",
                    new HashSet<string> { "AAA"
                    }),
                //new HeaderStruct("IntelPresentMon/ControlLib/igcl_api.h",
                //    new HashSet<string> { "ctl_init_args_t",
                //        "ctl_device_adapter_properties_t",
                //        "ctl_power_telemetry_t",
                //    }),
            };

            var compilation = CppParser.ParseFiles(structDefinitions.ConvertAll(hs => hs.HeaderFile));

            if (compilation.HasErrors) {
                foreach (var message in compilation.Diagnostics.Messages) {
                    Console.WriteLine(message);
                }
                return;
            }

            // Collect structs, enums, and their members
            var structs = new List<StructInfo>();
            var enums = new List<EnumInfo>();
            var includes = new HashSet<string>();
            var processedStructs = new HashSet<string>();
            var processedEnums = new HashSet<string>();

            foreach (var headerStruct in structDefinitions) {
                foreach (var targetName in headerStruct.StructNames) {
                    if (IsEnum(compilation, targetName)) {
                        ProcessEnum(targetName, headerStruct.HeaderFile, compilation, enums, includes, processedEnums);
                    }
                    else {
                        ProcessStruct(targetName, headerStruct.HeaderFile, compilation, structs, enums, includes, processedStructs, processedEnums);
                    }
                }
            }

            // Load the Scriban template
            var templateText = File.ReadAllText("Reflector\\StructDumpers.h.scriban");
            var template = Template.Parse(templateText);

            // Prepare the model for the template
            var model = new {
                includes = includes,
                structs = structs,
                enums = enums
            };

            // Render the template
            var result = template.Render(model, member => member.Name);

            // Output the generated code to a file
            File.WriteAllText("IntelPresentMon\\CommonUtilities\\ref\\gen\\GeneratedReflection.h", result);

            Console.WriteLine("Code generation complete.");
        }

        // Recursive method to process structs
        static void ProcessStruct(string structName, string headerFile, CppCompilation compilation, List<StructInfo> structs, List<EnumInfo> enums, HashSet<string> includes, HashSet<string> processedStructs, HashSet<string> processedEnums)
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
                Members = new List<MemberInfo>(),
                Type =  cppClass.ClassKind == CppClassKind.Class ? "class" :
                        cppClass.ClassKind == CppClassKind.Union ? "union" :
                        "struct"
            };

            foreach (var field in cppClass.Fields) {
                var fieldType = UnwrapType(field.Type);
                var memberInfo = new MemberInfo {
                    Name = field.Name,
                    Type = field.Type.GetDisplayName(),
                    DumpExpression = GetDumpExpression(field.Type, $"s.{field.Name}")
                };
                structInfo.Members.Add(memberInfo);

                // Check if the field's type is a struct or enum, and process it recursively, including arrays
                CppType? inclusiveFieldType = fieldType;
                if (fieldType is CppArrayType arrayType) {
                    inclusiveFieldType = arrayType.ElementType;
                }
                if (inclusiveFieldType is CppClass fieldCppClass) {
                    ProcessStruct(fieldCppClass.Name, headerFile, compilation, structs, enums, includes, processedStructs, processedEnums);
                }
                else if (inclusiveFieldType is CppEnum fieldEnum) {
                    ProcessEnum(fieldEnum.Name, headerFile, compilation, enums, includes, processedEnums);
                }
            }

            structs.Add(structInfo);
            includes.Add(headerFile);
        }

        // Method to process enums
        static void ProcessEnum(string enumName, string headerFile, CppCompilation compilation, List<EnumInfo> enums, HashSet<string> includes, HashSet<string> processedEnums)
        {
            if (processedEnums.Contains(enumName))
                return;

            processedEnums.Add(enumName);

            var cppEnum = compilation.Enums.FirstOrDefault(e => e.Name == enumName);
            if (cppEnum == null) {
                Console.WriteLine($"Enum {enumName} not found in {headerFile}");
                return;
            }

            var enumInfo = new EnumInfo {
                Name = cppEnum.Name,
                Values = cppEnum.Items.Select(item => new EnumValue {
                    Name = item.Name
                }).ToList()
            };

            enums.Add(enumInfo);
            includes.Add(headerFile);
        }

        // Helper method to determine if a name corresponds to an enum
        static bool IsEnum(CppCompilation compilation, string name)
        {
            return compilation.Enums.Any(e => e.Name == name);
        }

        // Helper method to generate the dump expression for a field
        static string GetDumpExpression(CppType type, string variableAccess)
        {
            var unwrappedType = UnwrapType(type);

            if (unwrappedType is CppPrimitiveType) {
                return variableAccess;
            }
            else if (unwrappedType is CppClass) {
                return $"DumpGenerated({variableAccess})";
            }
            else if (unwrappedType is CppEnum) {
                return $"DumpGenerated({variableAccess})";
            }
            else if (unwrappedType is CppArrayType arrayType) {
                // special case for char arrays, dump as c-string
                if (UnwrapType(arrayType.ElementType).FullName == "char") {
                    return variableAccess;
                }
                // sized arrays can be displayed
                else {
                    var isPrimitiveString = arrayType.ElementType is CppPrimitiveType ? "true" : "false";
                    return $"DumpArray_<{arrayType.ElementType.FullName}, {arrayType.Size}, {isPrimitiveString}>({variableAccess})";
                }
            }
            else if (unwrappedType is CppPointerType pointerType) {
                return $"({variableAccess} ? std::format(\"0x{{:016X}}\", reinterpret_cast<std::uintptr_t>({variableAccess})) : \"null\"s)";
            }
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

    // Class to represent a header file and its target structs/enums
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

    // Class to hold struct information
    class StructInfo
    {
        public string Name { get; set; }
        public string Type {  get; set; }
        public List<MemberInfo> Members { get; set; }
    }

    // Class to hold member information
    class MemberInfo
    {
        public string Name { get; set; }
        public string Type { get; set; }
        public string DumpExpression { get; set; }
    }

    // Class to hold enum information
    class EnumInfo
    {
        public string Name { get; set; }
        public List<EnumValue> Values { get; set; }
    }

    // Class to hold enum value information
    class EnumValue
    {
        public string Name { get; set; }
    }
}
