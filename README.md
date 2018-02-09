# radar-tool

A compatible radar tool to generate key-pairs and signing transactions.

## Table of contents

* [Dependencies](#dependencies)
  * [Radard submodule](#radard-submodule)
  * [Other dependencies](#other-dependencies)
* [Build and run](#build-and-run)
* [How to use](#How-to-use)


## Dependencies

### Radard submodule

radar-tool includes a git submodule to include the radard
source code, which is not cloned by default. To get the
radard source, either clone this repository using
```
$ git clone --recursive <location>
```
or after cloning, run the following commands
```
$ git submodule init
$ git submodule update
```

Note: even though the entire radard source tree is included
in the submodule, only a subset of it is used by the library.

### Other dependencies

* C++14 or greater
* [Boost](http://www.boost.org/)
* [OpenSSL](https://www.openssl.org/)

## Build and run

For linux and other unix-like OSes, run the following commands:

```
$ cd radar-tool
$ mkdir -p build/gcc.debug
$ cd build/gcc.debug
$ cmake ../.. -Dtarget=gcc.debug
$ cmake --build .
$ ./radar-tool
```

For 64-bit Windows, open a MSBuild Command Prompt for Visual Studio
and run the following commands:

```
> cd radar-tool
> mkdir build
> cd build
> cmake -G"Visual Studio 14 2015 Win64" ..
> cmake --build .
> .\Debug\radar-tool.exe
```

32-bit Windows builds are not officially supported. 

## How to use

Generate key pairs

```
> .\radar-tool.exe key_gen
{
   "private" : "sh7xxvydX5eSXVXgrqvVpw5Smtwqi",
   "public" : "rPKjLLTGEx9hYZLyEjU2LjpdLsXDCXQpBi",
   "status" : "success"
}
```

Generate public key from your private key 

```
> .\radar-tool.exe key_conv sh7xxvydX5eSXVXgrqvVpw5Smtwqi
{
   "public" : "rPKjLLTGEx9hYZLyEjU2LjpdLsXDCXQpBi",
   "status" : "success"
}
```

Check a public key is illegal or not.

```
> .\radar-tool.exe key_chk rPKjLLTGEx9hYZLyEjU2LjpdLsXDCXQpBi
{
   "result" : "true",
   "status" : "success"
}
```

Get signature of your transaction

```
> .\radar-tool.exe tx_sign sh7xxvydX5eSXVXgrqvVpw5Smtwqi "{\"Account\":\"rPKjLLTGEx9hYZLyEjU2LjpdLsXDCXQpBi\",\"Amount\":{\"currency\":\"VBC\",\"issuer\":\"rrrrrrrrrrrrrrrrrrrrVFngv46\",\"value\":\"1000\"},\"Destination\":\"rJUG2KtMLn3rCYb1986svLraAeDfcLk1xb\",\"Fee\":\"1000\",\"Flags\":2147483648,\"Sequence\":18,\"TransactionType\":\"Payment\"}"
{
   "result" : "true",
   "status" : "success"
}
```