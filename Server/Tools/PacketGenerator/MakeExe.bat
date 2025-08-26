@echo off
cd /d %~dp0

python PacketGenerator.py --path ..\..\Common\protoc-21.12-win64\bin\Protocol.proto --output ClientPacketHandler --recv S_ --send C_
copy "ClientPacketHandler.h" "..\..\Client\GameCoding\ClientPacketHandler.h"

python PacketGenerator.py --path ..\..\Common\protoc-21.12-win64\bin\Protocol.proto --output ServerPacketHandler --recv C_ --send S_
copy "ServerPacketHandler.h" "..\..\Server\ServerPacketHandler.h"

echo.
echo
pause
