################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/glps.c \
../rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/glps_task.c 

OBJS += \
./rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/glps.o \
./rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/glps_task.o 

C_DEPS += \
./rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/glps.d \
./rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/glps_task.d 


# Each subdirectory must supply rules for building sources it contributes
rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/%.o: ../rw_ble/sdk/ble_stack/ble/profiles/glp/glps/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: BA ELF GNU C compiler'
	ba-elf-gcc -DBA22_DEee -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\bluetooth\protocol\avctp" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\bluetooth\core" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\bluetooth\protocol\avdtp" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\libs\FatLib" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\include\os" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\libs\Mp3Lib" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\libs\AEC" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\bluetooth" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\bluetooth\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\config" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\jos" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\pkg\sbc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\jos\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\beken_app" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\beken_driver" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\beken_no_os" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\common" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\common\bluetooth" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\include\bluetooth" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\bt\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\hc\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\lc\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\lc\dl\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\lc\uslc\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\lc\lslc\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\lmp\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\sys\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\tc\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\transport\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\core\hw\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\hal\hw\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\hal\hw\radio\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\hal\beken\hw\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\controller\hal\beken\sys\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\include" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\include\class" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\cd" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\drivers\comm" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\drivers\msd" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\examples" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\functions\trans_fn" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\systems" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\systems\none" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\systems\none\afs" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\host\port\usb\src\test" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\project\app" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\project\config" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\ke\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\ke\src" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\nvds\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\h4tl\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\rwip\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\dbg\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\common\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\rf\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\rf\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\nvds\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\core_modules\ecc_p256\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\arch" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\arch\compiler" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\arch\boot" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\arch\ll" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\driver\reg" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\driver\uart" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\build\ble-full\reg\fw" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\plactform\src\rom\hci" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ahi\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\ll\src\rwble" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\ll\src\em" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\ll\src\lld" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gatt\attc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\FFF0\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\find" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\find\findt\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\find\findl\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\dis\diss\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\dis\diss\src" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\prox\proxr\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\hrp" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\hrp\hrps\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\ll\src\llc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\ll\src\llm" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\inc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gap" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gap\gapm" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gap\gapc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gap\smpm" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gap\smpc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gatt" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gatt\attm" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gatt\attc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gatt\atts" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gatt\gattm" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\gatt\gattc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\l2c\l2cm" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\hl\src\l2c\l2cc" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\FFF0\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\FFE0\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\bas\bass\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\bas\bass\src" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\hogp" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\hogp\hogpd\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\FCC0\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\FCC0\util" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\hogp\hogpd\src" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ble\profiles\ota\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\ea\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\em\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\sdk\ble_stack\hci\api" -I"D:\Beken\BK3268\BK3268_Printer_SDK_V1.0.1\SDK\BK3268_RW_Full_Func_designkit\BK3268_RW_Full_Func_designkit\rw_ble\libs" -O1 -G 4 -flto -g3 -Wall -c -fmessage-length=0  -mno-hard-float -ffast-math -march=ba2 -mle -mabi=3 -mtj-rodata -msingle-float -mdsp -mmac -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


