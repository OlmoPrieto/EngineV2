GREEN='\033[0;32m'
NC='\033[0m'

clear
make config=release

echo -e "\n${GREEN} -- Starting execution --${NC}"

./Engine/bin/Engine