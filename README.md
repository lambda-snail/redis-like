# About

This is my solution to the [Redis Server](https://codingchallenges.fyi/challenges/challenge-redis) coding challenge by
John Crickett. 

It is writen in C++23 using Clang and `libstdc++` on Ubuntu. Due to some issues with Asio, I have not yet been able to make it
compile on Windows, unfortunately. This is also the first time I build something more than a test program that
uses modules.

## Disclaimer

Naming is hard. Despite the name of this project, I am not in any way affiliated with Redis. If you are affiliated with Redis and you think the name of this project
needs to change, please contact me and I will make the change.

# Dependencies

This project stands on the shoulders of the following giants:

- [Asio](https://think-async.com/Asio): The stand-alone version of the popular networking library
- [CLI11](https://github.com/CLIUtils/CLI11): A command line parsing library
