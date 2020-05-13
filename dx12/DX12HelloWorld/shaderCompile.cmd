@echo off
for %%f in (%~dp0*.hlsl) do (
    "C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc.exe" "%~dp0%%~nf.hlsl" /Od /Zi /T vs_5_0 /E "VS" /Fo "%~dp0%%~nf_vs.cso" /Fc "%~dp0%%~nf_vs.asm"
	"C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\fxc.exe" "%~dp0%%~nf.hlsl" /Od /Zi /T ps_5_0 /E "PS" /Fo "%~dp0%%~nf_ps.cso" /Fc "%~dp0%%~nf_ps.asm"
)
