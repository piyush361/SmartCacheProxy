# Smart Cache Proxy

**Smart Cache Proxy (SCP)** is a caching proxy server designed to efficiently manage requests and improve performance by storing and serving frequently accessed data.

---

## Project Structure

- swp.c # Main proxy server code
- swp # Compiled binary (optional)
- proxy_parse.h (http parse library)
- proxy_parse.c (http parse functions)
- usertests/ # Client test programs
  - test1 # Non-persistent test
  - test2 # Non-persistent test
  - test3 # Persistent test
- Cache.c # Cache functions
- stats.c # Logger functions
- Pool.c # Connection Pool functions
- stats.h  , Cache.h and Pool.h # type definitions


---

## Compilation

You can compile the main proxy server using `gcc`:

```bash
gcc swp.c -o swp

```
A file called proxy_stats.txt will be created and will be updated continuosly for proxy stats after executing the binary.




