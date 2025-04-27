# About

This is my solution to the [Redis Server](https://codingchallenges.fyi/challenges/challenge-redis) coding challenge by John Crickett. 

It is writen in C++23 using Clang and `libstdc++` on Ubuntu. Due to some issues when using Asio with modules on msvc, I have not yet been able to make it
compile on Windows, unfortunately. This is also the first time I build something more than a test program that uses modules.

## State

The challenge is almost done, except for the extra commands in the second to last step. The code is a mess, but it runs well:

The following command

```shell
redis-benchmark -q -t SET,GET -k 1 -d 512 -n 100000 -c 50 -r 1000000
```

This server produces the following benchmark results:

```shell
SET: 49726.51 requests per second, p50=0.503 msec                   
GET: 46533.27 requests per second, p50=0.535 msec
```

Which is very similar to the 'real' redis server installed via `apt-get` on my computer:

```shell
SET: 50377.83 requests per second, p50=0.495 msec                   
GET: 47460.84 requests per second, p50=0.519 msec
```

## TODO

Things that could (should) be improved if this was a real production application:

- Heavily refactor the RESP parser - currently it attempts to rely on the buffer allocated by the connection handler to avoid 
  any allocations for as long as possible. Just before storing data in the database, it is "materialized" and storage for the
  entry is allocated. This works, but led to code duplication that should be refactored.

- Add support for varying request sizes in the buffer pool - for the challenge requests are assumed to be 1 kiB or less. In a
  real redis application requests can of course be much larger than that. The connection would need to check if there is still
  data in the buffer after reading a request, and if so allocate a larger buffer. Another approach would be to parse the RESP
  commands using a parser that can work incrementally.

- Currently, all data is stored as string in the database - not sure if this is a good idea or not. 

## Disclaimer

Naming is hard. Despite the name of this project, I am not in any way affiliated with Redis. If you are affiliated with Redis and you think the name of this project
needs to change, please contact me and I will make the change.

# Requirements

The code has been verified to work with clang-19 on Ubuntu (24.04). On Windows there are compilation errors that are similar
to [this](https://developercommunity.visualstudio.com/t/C20-modules--boost::asio-still-being-/10038468) report. The ticket is
still "under consideration" after almost three years, so it's anyone's guess when it can be resolved. If you know a workaround
to this on Windows, PRs are always welcome :)

# Running

```shell
  ./redis-server
```

To see available arguments, run

```shell
  ./redis-server -h
```

# Dependencies

This project stands on the shoulders of the following giants:

- [Asio](https://think-async.com/Asio): The stand-alone version of the popular networking library
- [CLI11](https://github.com/CLIUtils/CLI11): A command line parsing library