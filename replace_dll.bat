cd "C:\Games\Steam\steamapps\common\Sven Co-op\svencoop\addons\metamod\dlls"

if exist Mute_old.dll (
    del Mute_old.dll
)
if exist Mute.dll (
    rename Mute.dll Mute_old.dll 
)

exit /b 0