@echo off
:: 烧录（自动复位）一键脚本
python "D:\ClaudeGlobalConfig\skills\flash-keil\scripts\keil_flasher.py" --flash --project "MDK-ARM/scope-siggen.uvprojx" --uv4 "D:\k5\UV4\UV4.exe"
pause
