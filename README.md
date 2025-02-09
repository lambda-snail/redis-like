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

Which is very similar to the 'real' redis server installed via `apt-get`:

```shell
SET: 50377.83 requests per second, p50=0.495 msec                   
GET: 47460.84 requests per second, p50=0.519 msec
```

## TODO

- Add support for more commands (finish the challenge)
- Heavily refactor the RESP parser
- Remove the ugly string copying and hard-coded RESP responses that occur in several places
- Add support for varying request sizes in the buffer pool

## Disclaimer

Naming is hard. Despite the name of this project, I am not in any way affiliated with Redis. If you are affiliated with Redis and you think the name of this project
needs to change, please contact me and I will make the change.

# Requirements

The code has been verified to work with clang-19 on Ubuntu (24.04). On Windows there are compilation errors that are similar
to [this](https://developercommunity.visualstudio.com/t/C20-modules--boost::asio-still-being-/10038468) report. The ticket is
still "under consideration" after almost three years, so it's anyone's guess when it an be resolved. If you know a workaround
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
