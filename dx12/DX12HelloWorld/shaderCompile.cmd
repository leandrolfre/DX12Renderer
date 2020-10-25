@echo off
for %%f in (%~dp0Shaders\*_SD.hlsl) do (
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc.exe" "%~dp0Shaders\%%~nf.hlsl" /I "inc" /Od /Zi /T vs_5_1 /E "VS" /Fo "%~dp0Shaders\%%~nf_vs.cso" /Fc "%~dp0Shaders\%%~nf_vs.asm"
	"C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc.exe" "%~dp0Shaders\%%~nf.hlsl" /I "inc" /Od /Zi /T ps_5_1 /E "PS" /Fo "%~dp0Shaders\%%~nf_ps.cso" /Fc "%~dp0Shaders\%%~nf_ps.asm"
)

for %%f in (%~dp0Shaders\*_CS.hlsl) do (
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc.exe" "%~dp0Shaders\%%~nf.hlsl" /I "inc" /Od /Zi /T cs_5_1 /E "%%~nf" /Fo "%~dp0Shaders\%%~nf_cs.cso" /Fc "%~dp0Shaders\%%~nf_cs.asm"
)