# PresentMon Tests

PresentMon testing is primarily done by having a specific PresentMon build analyze a collection of ETW logs and ensuring its output matches the expected result.  The PresentMonTests application will add a test for every .etl/.csv pair it finds under a specified root directory.

To run the standard PresentMon tests, run the following two commands:
```
Tools\download_tests.cmd
Tools\run_tests.cmd
```

`Tools\download_tests.cmd` will download a set of test logs along with their expected results from the [PresentMonTestEtls](https://github.com/GameTechDev/PresentMon/releases/tag/PresentMonTestEtls) release, and place them into the Tests\Gold directory (which is the default root directory for PresentMonTests).

`Tools\run_tests.cmd` will build all configurations of PresentMon, and use PresentMonTests to validate the x86 and x64 builds using the contents of the Tests\Gold directory.


#### PresentMonTestEtls Coverage

The ETW logs provided in the PresentMonTestEtls release binary were chosen to minimally cover as many different present paths as possible.  These logs currently exercise the following PresentMon paths:

| PresentMode | TRACK_PRESENT_PATH |
| ----------- | ----------- |
| Composed_Composition_Atlas         | 000000000000000001110000000001000 |
|                                    | 000000000000000001110001000001000 |
| Composed_Copy_CPU_GDI              | 000000000000000011100001000001010 |
| Composed_Copy_GPU_GDI              | 000000000000000011110001000001000 |
|                                    | 000100000000000011100001000001000 |
|                                    | 000100000000000011110001000001000 |
|                                    | 000100000000000011110001000001000 |
|                                    | 100000000000000011110001000001000 |
|                                    | 100100000000000011110001000001000 |
| Composed_Flip                      | 000011111000000001110001000001001 |
|                                    | 000011111000000001110001000001001 |
|                                    | 010011111000000001110001000001001 |
|                                    | 010011111000000001110001000001001 |
| Hardware_Composed_Independent_Flip | 000010011000000001110001000111001 |
|                                    | 000010011000000001110001001111001 |
|                                    | 000010011000000001110101000111001 |
|                                    | 000010011000000001111001000111001 |
| Hardware_Independent_Flip          | 000010011000000001110011000001001 |
|                                    | 000010011000000001111011000001001 |
| Hardware_Legacy_Flip               | 000000000000000000000000101111001 |
|                                    | 000000000000000000000001100001001 |
|                                    | 000000000000000000000001100111000 |
|                                    | 000000000000000000000001100111001 |
|                                    | 000000000000000000000001101111001 |
|                                    | 000000000000000000001001100111000 |
|                                    | 000000000000000000001001100111001 |
|                                    | 000000000000000000011001010111000 |
|                                    | 000000000000000000011011010001000 |
|                                    | 000000000000000000011011010001001 |
| Unknown                            | 000000000000000000000000000000001 |
|                                    | 000000000000000000000000000000001 |
|                                    | 000000000000000000010000000000001 |
|                                    | 100000000000000000000000000000000 |

The following expected cases are currently missing (WIP):

- PresentMode==Hardware_Legacy_Copy_To_Front_Buffer
- [2,17-23] All Windows7 paths
- [30] Non-Win7 Microsoft_Windows_Dwm_Core::FlipChain_(Pending|Complete|Dirty) with previous Microsoft_Windows_Dwm_Core::PresentHistory[Detailed]::Start
