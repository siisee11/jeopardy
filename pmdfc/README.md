# PMDFC
Persistent Memory Distributed File Cache

## Commands

```modinfo ****.ko``` to see module information

```insmod ****.ko``` or ```make load``` to insert module.

```rmmod ****.ko``` or ```make unload``` to remove module.

## Tries and Errors

Client send --> Server recv successes only once.

## Reference

[In-kernel networking using tcp ip](https://github.com/abysamross/simple-linux-kernel-tcp-client-server)
[Server side codes](https://github.com/byeongkeonLee/PM_disaggregated_serverside)
