# Mirai Botnet Client, Echo Loader and CNC source code

This is a fork of jgamblin/Mirai-Source-Code. The intent is to create a clean build environment of Mirai for analysis and sandboxing.

Things have changed from the original:
- Merged Felicitychou's additions
- setup Vagrant file
- Removed obfuscation of table.c, so no need to run "enc" tool anymore. Added de-obfuscator to /tools/ dir.
- modified some of the original shell scripts to install more cross compiler packages and remove build errors
- modified build.sh to download go packages


steps to setup build environment
 - `git pull`
 - `vagrant up`
 - `vagrant ssh`
 - `cd /vagrant/mirai`
 - `./build.sh`

Steps to create database:
 - `cat Configure_CNC_Database.txt | mysql -u root --password=password`

Start the CnC
- make a prompt file in ./release
- `cd ./release`
- `sudo ./cnc`
- `telnet localhost`

After building the binaries, you'll want to deploy the built bot's to another machine for communication. I suggest the following steps:
- Create bare linux vm (ubuntu server works)
- join that vm to virtual box network mirai_net (see Vagrantfile )
- drop compiled  x86 binary (rename to the magic word or it won't run right)
- Spoof DNS. The easy version is to go `sudo python /vagrant/tools/fakedns.py` after a vagrant up (again see source and Vagrantfile)
- Make sure your gateways are dead ended, dont do NAT or packet forwarding or any nonesense....


# Original README.md

This is the source code released from [here](http://hackforums.net/showthread.php?tid=5420472) as discussed in this [Brian Krebs Post](https://krebsonsecurity.com/2016/10/source-code-for-iot-botnet-mirai-released/).

I found 

mirai.src.zip from [VT](https://www.virustotal.com/en/file/68d01cd712da9c5f889ce774ae7ad41cd6fbc13c42864aa593b60c1f6a7cef63/analysis/)

loader.src.zip from [VT](https://www.virustotal.com/en/file/fffad2fbd1fa187a748f6d2009b942d4935878d2c062895cde53e71d125b735e/analysis/)

dlr.src.zip from [VT](https://www.virustotal.com/en/file/519d4e3f9bc80893838f94fd0365d587469f9468b4fa2ff0fb0c8f7e8fb99429/analysis/)

Maybe they are original files.



Configuring_CNC_Database.txt from [pastebin.com/86d0iL9g](http://pastebin.com/86d0iL9g)

Setting_Up_Cross_Compilers.sh from [pastebin.com/1rRCc3aD](http://pastebin.com/1rRCc3aD)

Felicitychou
