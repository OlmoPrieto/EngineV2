#./clean.sh
export ARCHFLAGS='-arch i386 -arch x86_64'
./genie --file=project_settings.lua --os=macosx --platform=x64 gmake