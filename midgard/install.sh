make
echo | gyc -E -Wp,-v - &> files
Y_INCLUDE_PATH=$(grep -A1 "here:" files | tail -n -1)/ymir/
echo $Y_INCLUDE_PATH
cp -r core/ $Y_INCLUDE_PATH
cp -r std/ $Y_INCLUDE_PATH
rm files
make install
