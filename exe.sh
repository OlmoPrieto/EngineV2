GREEN='\033[0;32m'
NC='\033[0m'

clear
make config=release

DATE=$date
echo -e "\n${GREEN} -- Starting execution $(DATE) --${NC}"

./Engine/bin/Engine