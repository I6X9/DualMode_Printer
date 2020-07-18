ba-elf-objdump -d     BK3268_RW_Full_Func_designkit.elf    > BK3268.dmp
ba-elf-nm             BK3268_RW_Full_Func_designkit.elf    > BK3268.map

ba-elf-readelf.exe    BK3268_RW_Full_Func_designkit.elf -a > 0_section_information.txt
#ba-elf-nm -r          BK3268_RW_Full_Func_designkit.elf    > 1_reverse_sort_symbol.txt
#ba-elf-nm --size-sort BK3268_RW_Full_Func_designkit.elf    > 2_symbol_by_size.txt
#ba-elf-nm -p          BK3268_RW_Full_Func_designkit.elf    > 3_no_sort_symbol.txt
#ba-elf-nm -n          BK3268_RW_Full_Func_designkit.elf    > 4_numeric_sort_symbol.txt
#ba-elf-nm -l          BK3268_RW_Full_Func_designkit.elf    > 5_symbol_and_line_number.txt
#ba-elf-nm -g          BK3268_RW_Full_Func_designkit.elf    > 6_external_symbol.txt
#ba-elf-nm -a          BK3268_RW_Full_Func_designkit.elf    > 7_debug_symbol.txt
#ba-elf-nm -u          BK3268_RW_Full_Func_designkit.elf    > 8_undefined_symbol.txt
#ba-elf-nm -S          BK3268_RW_Full_Func_designkit.elf    > 9_print_size_defined_symbol.txt

ba-elf-objcopy -O binary BK3268_RW_Full_Func_designkit.elf  BK3268_DM_RW_MCU.bin

./bk3268_mcu_dsp_bin_merge 0xB6980 BK3268_DM_RW_MCU.bin tl420_code_data.img BK3268_MCU_WITH_DSP.bin

echo ./encrypt BK3268_DM_RW_DSP.bin 00000000
