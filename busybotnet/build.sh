#kek

echo 'welcome to BusyBotNet beta installer thing'
echo 'this is intended to be run on ubuntu but can be done manually pretty much anywhere'
echo 'this tool currently assumes you hae a working development environment consisting of gcc, make, cmake, libpcap, zlib, openssl, and whatever else it needs. all it does it streamline the process of building the appropriate libssh archive library and then buildig bbn. in the uture it will also compile libpcap, openssl, and zlib, along with install all development tools required for building.'
echo 'got that? enter to continue or control c to quit'
read OK
sudo apt install build-essential libssl-dev cmake flex bison gcc-4.9-multilib-arm-linux-gnueabi libc6-dev-i386 libc6-dev-armhf-cross -y
sudo apt purge libssh-dev libpcap-dev -y
cd libpcap
./configure
make 
sudo make install
cd ..
cd libssh-0.7.4
rm -rf build
mkdir build
cd build
cmake ..
make
sudo make install
cd ../..
make clean
make defconfig
make
echo 'lol u did it'
file busybox
cp -v busybox binaries/busybox_x64

#
#cd libpcap
#CFLAGS='-m32' LDFLAGS='-m32' ./configure
#CFLAGS='-m32' LDFLAGS='-m32' make
#sudo make install
#cd ..
#cd libssh-0.7.4
#rm -rf build
##mkdir build
#cd build
#CFLAGS='-m32' LDFLAGS='-m32' cmake ..
##CFLAGS='-m32' LDFLAGS='-m32' make
###sudo make install
#cd ../..
#make clean
#CFLAGS='-m32' LDFLAGS='-m32' make defconfig
#CFLAGS='-m32' LDFLAGS='-m32' make
#echo 'lol u did it'
#file busybox
#cp -v busybox binaries/busybox_x32


